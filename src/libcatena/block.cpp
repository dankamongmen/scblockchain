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
			unsigned len, LedgerMap* lmap, TrustStore* tstore){
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
			if(tx->Validate(*tstore, *lmap)){
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
		unsigned len, const CatenaHash& prevhash, uint64_t prevutc){
	if(len < Block::BLOCKHEADERLEN){
		std::cerr << "needed " << Block::BLOCKHEADERLEN <<
			" bytes, had only " << len << std::endl;
		return true;
	}
	memcpy(chdr->hash.data(), data, chdr->hash.size());
	data += chdr->hash.size();
	unsigned const char* hashstart = data;
	memcpy(chdr->prev.data(), data, chdr->prev.size());
	if(chdr->prev != prevhash){
		std::cerr << "invalid prev hash (wanted " << prevhash << ")" << std::endl;
		return true;
	}
	data += chdr->prev.size();
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
	if(hash != chdr->hash){
		std::cerr << "invalid block hash (wanted " << hash << ")" << std::endl;
		return true;
	}
	return false;
}

// Verify new blocks relative to the loaded blocks (i.e., do not replay already-
// verified blocks). If any block fails verification, the Blocks structure is
// unchanged, and -1 is returned.
int Blocks::VerifyData(const unsigned char *data, unsigned len, LedgerMap& lmap,
			TrustStore& tstore){
	unsigned offset = 0;
	uint64_t prevutc = 0;
	auto origblockcount = GetBlockCount();
	int blocknum = origblockcount;
	if(blocknum){
		prevutc = GetLastUTC();
		offset = offsets.back() + headers.back().totlen;
	}
	std::vector<unsigned> new_offsets;
	std::vector<BlockHeader> new_headers;
	CatenaHash prevhash;
	GetLastHash(prevhash);
	TrustStore new_tstore = tstore; // FIXME expensive copies here :(
	auto new_lmap = lmap;
	while(len){
		Block block;
		BlockHeader chdr;
		chdr.txidx = blocknum;
		if(Block::ExtractHeader(&chdr, data, len, prevhash, prevutc)){
			return -1;
		}
		data += Block::BLOCKHEADERLEN;
		prevhash = chdr.hash;
		prevutc = chdr.utc;
		if(block.ExtractBody(&chdr, data, chdr.totlen - Block::BLOCKHEADERLEN,
					&new_lmap, &new_tstore)){
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
	tstore = new_tstore; // FIXME another set of expensive copies (swap? move?)
	lmap = new_lmap;
	return blocknum - origblockcount;
}

bool Blocks::LoadData(const void* data, unsigned len, LedgerMap& lmap, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	auto blocknum = VerifyData(static_cast<const unsigned char*>(data),
					len, lmap, tstore);
	if(blocknum < 0){
		return true;
	}
	return false;
}

bool Blocks::LoadFile(const std::string& fname, LedgerMap& lmap, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	filename = "";
	size_t size;
	// Returns nullptr on zero-byte file, but LoadData handles that fine
	const auto& memblock = ReadBinaryFile(fname, &size);
	bool ret;
	if(!(ret = LoadData(memblock.get(), size, lmap, tstore))){
		filename = fname;
	}
	return ret;
}

bool Blocks::AppendBlock(const unsigned char* block, size_t blen, LedgerMap& lmap, TrustStore& tstore){
	std::cout << "Validating " << blen << " byte block\n";
	if(VerifyData(block, blen, lmap, tstore) <= 0){
		return true;
	}
	if(filename != ""){
		std::cout << "Appending " << blen << " byte block\n";
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

void Blocks::GetLastHash(CatenaHash& hash) const {
	if(headers.empty()){ // will be genesis block
		memset(hash.data(), 0xff, HASHLEN);
	}else{
		hash = headers.back().hash;
	}
}

// Returns nullptr on a failure to lex the block or verify its hash
std::vector<std::unique_ptr<Transaction>>
Block::Inspect(const unsigned char* b, const BlockHeader* chdr){
	CatenaHash hash;
	catenaHash(b + HASHLEN, chdr->totlen - HASHLEN, hash);
	if(hash != chdr->hash){
		throw BlockValidationException("bad hash on inspection");
	}
	if(ExtractBody(chdr, b + BLOCKHEADERLEN, chdr->totlen - BLOCKHEADERLEN, nullptr, nullptr)){
		throw BlockValidationException();
	}
	return std::move(transactions);
}

std::vector<BlockDetail> Blocks::Inspect(int start, int end) const {
	std::vector<BlockDetail> ret;
	if(start < 0){
		return ret;
	}
	if(end < 0 || (size_t)end + 1 > headers.size()){
		end = headers.size();
	}else{
		++end;
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
Block::SerializeBlock(CatenaHash& prevhash) const {
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
	memcpy(targ, prevhash.data(), prevhash.size()); // copy in previous hash
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
	memcpy(prevhash.data(), block, HASHLEN);
	return std::make_pair(std::move(ret), len);
}

void Block::AddTransaction(std::unique_ptr<Transaction> tx){
	transactions.push_back(std::move(tx));
}

// Toss any transactions, resetting the block
void Block::Flush(){
	transactions.clear();
}

}
