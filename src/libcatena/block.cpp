#include <memory>
#include <cstring>
#include "libcatena/utility.h"
#include "libcatena/chain.h"
#include "libcatena/block.h"
#include "libcatena/hash.h"
#include "libcatena/tx.h"

namespace Catena {

const int Block::BLOCKVERSION;
const int Block::BLOCKHEADERLEN;

bool Block::ExtractBody(const BlockHeader* chdr, const unsigned char* data,
			unsigned len, TrustStore* tstore){
	if(len / 4 < chdr->txcount){
		std::cerr << "no room for " << chdr->txcount << "-offset table in " << len << " bytes" << std::endl;
		return true;
	}
	uint32_t offsets[chdr->txcount];
	for(unsigned i = 0 ; i < chdr->txcount ; ++i){
		uint32_t offset = nbo_to_ulong(data, sizeof(offset));
		data += sizeof(offset);
		if(offset >= len){
			std::cerr << "no room for offset " << offset << std::endl;
			return true;
		}
		offsets[i] = offset;
	}
	len -= chdr->txcount * 4;
	for(unsigned i = 0 ; i < chdr->txcount ; ++i){
		// extract from offset[i]
		unsigned txlen;
		if(i < chdr->txcount - 1){
			txlen = offsets[i + 1] - offsets[i];
		}else{
			txlen = len;
		}
		std::unique_ptr<Transaction> tx(Transaction::lexTX(data, txlen, chdr->hash, i));
		if(tx == nullptr){
			return true;
		}
		if(tstore){
			if(tx->Validate(*tstore)){
				return true;
			}
		}
		transactions.push_back(std::move(tx));
		data += txlen;
		len -= txlen;
	}
	return false;
}

bool Block::ExtractHeader(BlockHeader* chdr, const unsigned char* data,
		unsigned len, const std::array<unsigned char, HASHLEN>& prevhash,
		uint64_t prevutc){
	if(len < Block::BLOCKHEADERLEN){
		std::cerr << "needed " << Block::BLOCKHEADERLEN <<
			" bytes, had only " << len << std::endl;
		return true;
	}
	std::memcpy(chdr->hash, data, sizeof(chdr->hash));
	data += sizeof(chdr->hash);
	unsigned const char* hashstart = data;
	std::memcpy(chdr->prev, data, sizeof(chdr->prev));
	if(memcmp(chdr->prev, prevhash.data(), prevhash.size())){
		std::cerr << "invalid prev hash (wanted ";
		hashOStream(std::cerr, prevhash.data()) << ")" << std::endl;
		return true;
	}
	data += sizeof(chdr->prev);
	chdr->version = nbo_to_ulong(data, 2);
	data += 2; // 16-bit version field
	if(chdr->version != Block::BLOCKVERSION){
		std::cerr << "expected version " << Block::BLOCKVERSION << ", got " << chdr->version << std::endl;
		return true;
	}
	chdr->totlen = nbo_to_ulong(data, 3);
	data += 3; // 24-bit totlen field
	if(chdr->totlen < Block::BLOCKHEADERLEN || chdr->totlen > len){
		std::cerr << "invalid totlen " << chdr->totlen << std::endl;
		return true;
	}
	chdr->txcount = nbo_to_ulong(data, 3);
	data += 3; // 24-bit txcount field
	chdr->utc = nbo_to_ulong(data, 5);
	data += 5; // 40-bit UTC field
	// FIXME reject 0 UTC?
	if(chdr->utc < prevutc){ // allow non-strictly-increasing timestamps?
		std::cerr << "utc " << chdr->utc << " was less than " << prevutc << std::endl;
		return true;
	}
	for(int i = 0 ; i < 19 ; ++i){ // 19 reserved bytes
		if(*data){
			std::cerr << "non-zero reserved byte" << std::endl;
			return false;
		}
		++data;
	}
	CatenaHash hash;
	catenaHash(hashstart, chdr->totlen - HASHLEN, hash);
	if(memcmp(hash.data(), chdr->hash, HASHLEN)){
		std::cerr << "invalid block hash (wanted " << hash << ")" << std::endl;
		return true;
	}
	return false;
}

// Verify new blocks relative to the loaded blocks (i.e., do not replay already-
// verified blocks). If any block fails verification, the Blocks structure is
// unchanged, and -1 is returned.
int Blocks::VerifyData(const unsigned char *data, unsigned len, TrustStore& tstore){
	uint64_t prevutc = 0;
	unsigned offset = 0;
	if(offsets.size()){
		offset = offsets.back() + headers.back().totlen;
	}
	int blocknum = 0;
	std::vector<unsigned> new_offsets;
	std::vector<BlockHeader> new_headers;
	std::array<unsigned char, HASHLEN> prevhash;
	GetLastHash(prevhash);
	TrustStore new_tstore = tstore; // FIXME expensive copy here :(
	while(len){
		Block block;
		BlockHeader chdr;
		if(Block::ExtractHeader(&chdr, data, len, prevhash, prevutc)){
			headers.clear();
			offsets.clear();
			return -1;
		}
		data += Block::BLOCKHEADERLEN;
		memcpy(prevhash.data(), chdr.hash, prevhash.size());
		prevutc = chdr.utc;
		if(block.ExtractBody(&chdr, data, chdr.totlen - Block::BLOCKHEADERLEN, &new_tstore)){
			headers.clear();
			offsets.clear();
			return -1;
		}
		data += chdr.totlen - Block::BLOCKHEADERLEN;
		len -= chdr.totlen;
		new_offsets.push_back(offset);
		new_headers.push_back(chdr);
		offset += chdr.totlen;
		++blocknum;
	}
	headers.insert(headers.end(), new_headers.begin(), new_headers.end());
	offsets.insert(offsets.end(), new_offsets.begin(), new_offsets.end());
	tstore = new_tstore; // FIXME another expensive copy
	return blocknum;
}

bool Blocks::LoadData(const void* data, unsigned len, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	auto blocknum = VerifyData(static_cast<const unsigned char*>(data),
					len, tstore);
	if(blocknum < 0){
		return true;
	}
	return false;
}

bool Blocks::LoadFile(const std::string& fname, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	filename = "";
	size_t size;
	// Returns nullptr on zero-byte file, but LoadData handles that fine
	const auto& memblock = ReadBinaryFile(fname, &size);
	bool ret;
	if(!(ret = LoadData(memblock.get(), size, tstore))){
		filename = fname;
	}
	return ret;
}

bool Blocks::AppendBlock(const unsigned char* block, size_t blen, TrustStore& tstore){
	std::cout << "Appending " << blen << " byte block\n";
	if(VerifyData(block, blen, tstore) <= 0){
		return true;
	}
	if(filename != ""){
		// FIXME if we have an error writing out, do we need to remove
		// the new data from internal data structures from VerifyData?
		// Safest thing might be to copy+append first, then verify, then
		// atomically move appended file over old...
		std::ofstream ofs;
		ofs.open(filename, std::ios::out | std::ios::binary | std::ios_base::app);
		ofs.write(reinterpret_cast<const char*>(block), blen);
		if(ofs.rdstate()){
			std::cerr << "error updating file " << filename << std::endl;
			return true;
		}
		std::cout << "Wrote " << blen << " bytes to " << filename << std::endl;
	}else{
		std::cout << "Ledger is not file-backed, not writing data\n";
	}
	return false;
}

void Blocks::GetLastHash(std::array<unsigned char, HASHLEN>& hash) const {
	if(headers.empty()){ // will be genesis block
		memset(hash.data(), 0xff, HASHLEN);
	}else{
		memcpy(hash.data(), headers.back().hash, HASHLEN);
	}
}

// Returns nullptr on a failure to lex the block or verify its hash
std::vector<std::unique_ptr<Transaction>>
Block::Inspect(const unsigned char* b, const BlockHeader* chdr){
	CatenaHash hash;
	catenaHash(b + HASHLEN, chdr->totlen - HASHLEN, hash);
	if(memcmp(hash.data(), chdr->hash, HASHLEN)){
		throw BlockValidationException("bad hash on inspection");
	}
	if(ExtractBody(chdr, b + BLOCKHEADERLEN, chdr->totlen - BLOCKHEADERLEN, nullptr)){
		throw BlockValidationException();
	}
	return std::move(transactions);
}

std::vector<BlockDetail> Blocks::Inspect(int start, int end) const {
	std::vector<BlockDetail> ret;
	if(start < 0 || end < 0){
		return ret;
	}
	if((size_t)end > headers.size()){
		end = headers.size();
	}
	if(filename == ""){ // FIXME need keep copy of internal buffer
		throw BlockValidationException();
	}
	int idx = start;
	while(idx < end){
		size_t blen = headers[idx].totlen;
		auto mblock = ReadBinaryBlob(filename, offsets[idx], blen);
		if(mblock == nullptr){
			throw BlockValidationException();
		}
		Block b; // FIXME embed these into Blocks
		auto trans = b.Inspect(mblock.get(), &headers[idx]);
		ret.emplace_back(headers[idx], offsets[idx], std::move(trans));
		++idx;
	}
	return ret;
}

std::pair<std::unique_ptr<const unsigned char[]>, size_t>
Block::SerializeBlock(unsigned char* prevhash) const {
	std::vector<std::pair<std::unique_ptr<unsigned char[]>, size_t>> txserials;
	std::vector<size_t> offsets;
	size_t txoffset = 0;
	size_t len = 0;
	for(const auto& txp : transactions){
		const auto& tx = txp.get();
		txserials.push_back(tx->Serialize());
		offsets.push_back(txoffset);
		txoffset += txserials.back().second;
		len += txserials.back().second + 4; // 4 for offset table entry
	}
	len += BLOCKHEADERLEN;
	auto block = new unsigned char[len]();
	std::unique_ptr<const unsigned char[]> ret(block);
	unsigned char *targ = block + HASHLEN; // leave hash aside for now
	memcpy(targ, prevhash, HASHLEN); // copy in previous hash
	targ += HASHLEN;
	targ = ulong_to_nbo(BLOCKVERSION, targ, 2);
	targ = ulong_to_nbo(len, targ, 3);
	targ = ulong_to_nbo(transactions.size(), targ, 3);
	time_t now = time(NULL); // FIXME throw exception on result < 0?
	targ = ulong_to_nbo(now, targ, 5); // 40 bits for UTC
	memset(targ, 0x00, 19); // reserved bytes
	targ += 19;
	auto offtable = targ;
	auto txtable = offtable + 4 * txserials.size();
	for(auto i = 0u ; i < txserials.size() ; ++i){
		const auto& txp = txserials[i];
		offtable = ulong_to_nbo(offsets[i], offtable, 4);
		memcpy(txtable, txp.first.get(), txp.second);
		txtable += txp.second;
	}
	catenaHash(block + HASHLEN, len - HASHLEN, block);
	memcpy(prevhash, block, HASHLEN);
	return std::make_pair(std::move(ret), len);
}

void Block::AddTransaction(std::unique_ptr<Transaction> tx){
	transactions.push_back(std::move(tx));
}

// Toss any transactions, resetting the block
void Block::Flush(){
	transactions.clear();
}

template <typename Iterator> std::ostream&
DumpTransactions(std::ostream& s, const Iterator begin, const Iterator end){
	// FIXME reset stream after using setfill/setw
	int i = 0;
	while(begin + i != end){
		s << std::setfill('0') << std::setw(5) << i <<
			" " << begin[i].get() << "\n";
		++i;
	}
	return s;
}

std::ostream& operator<<(std::ostream& stream, const Block& b){
	return DumpTransactions(stream, b.transactions.begin(), b.transactions.end());
}

std::ostream& operator<<(std::ostream& stream, const BlockHeader& bh){
	stream << "hash: ";
	hashOStream(stream, bh.hash);
	stream << "\nprev: ";
	hashOStream(stream, bh.prev);
	stream << "\nver " << bh.version <<
		" transactions: " << bh.txcount <<
		" " << "bytes: " << bh.totlen << " ";
	char buf[80];
	time_t btime = bh.utc;
	if(ctime_r(&btime, buf)){
		stream << buf; // has its own newline
	}else{
		stream << "\n";
	}
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const BlockDetail& b){
	stream << b.bhdr;
	DumpTransactions(stream, b.transactions.begin(), b.transactions.end());
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Blocks& blocks){
	for(auto& h : blocks.headers){
		stream << h;
	}
	return stream;
}

}
