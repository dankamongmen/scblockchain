#include <memory>
#include <cstring>
#include "libcatena/utility.h"
#include "libcatena/block.h"
#include "libcatena/hash.h"
#include "libcatena/tx.h"

namespace Catena {

const int Block::BLOCKVERSION;
const int Block::BLOCKHEADERLEN;

bool Block::extractBody(BlockHeader* chdr, const unsigned char* data,
			unsigned len, TrustStore& tstore){
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
		if(tx->validate(tstore)){
			return true;
		}
		transactions.push_back(std::move(tx));
		data += txlen;
		len -= txlen;
	}
	return false;
}

bool Block::extractHeader(BlockHeader* chdr, const unsigned char* data,
		unsigned len, const unsigned char* prevhash, uint64_t prevutc){
	if(len < Block::BLOCKHEADERLEN){
		std::cerr << "needed " << Block::BLOCKHEADERLEN <<
			" bytes, had only " << len << std::endl;
		return true;
	}
	std::memcpy(chdr->hash, data, sizeof(chdr->hash));
	data += sizeof(chdr->hash);
	unsigned const char* hashstart = data;
	std::memcpy(chdr->prev, data, sizeof(chdr->prev));
	if(memcmp(chdr->prev, prevhash, sizeof(chdr->prev))){
		std::cerr << "invalid prev hash (wanted ";
		hashOStream(std::cerr, prevhash) << ")" << std::endl;
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
	unsigned char hash[HASHLEN];
	catenaHash(hashstart, chdr->totlen - HASHLEN, hash);
	if(memcmp(hash, chdr->hash, HASHLEN)){
		std::cerr << "invalid block hash (wanted ";
		hashOStream(std::cerr, hash) << ")" << std::endl;
		return true;
	}
	return false;
}

int Blocks::verifyData(const unsigned char *data, unsigned len, TrustStore& tstore){
	unsigned char prevhash[HASHLEN] = {0};
	uint64_t prevutc = 0;
	unsigned totlen = 0;
	int blocknum = 0;
	std::vector<unsigned> new_offsets;
	std::vector<BlockHeader> new_headers;
	while(len){
		Block block;
		BlockHeader chdr;
		if(Block::extractHeader(&chdr, data, len, prevhash, prevutc)){
			headers.clear();
			offsets.clear();
			return -1;
		}
		data += Block::BLOCKHEADERLEN;
		memcpy(prevhash, chdr.hash, sizeof(prevhash));
		prevutc = chdr.utc;
		if(block.extractBody(&chdr, data, chdr.totlen - Block::BLOCKHEADERLEN, tstore)){
			headers.clear();
			offsets.clear();
			return -1;
		}
		len -= chdr.totlen;
		new_offsets.push_back(totlen);
		new_headers.push_back(chdr);
		totlen += chdr.totlen;
		++blocknum;
	}
	headers.swap(new_headers);
	offsets.swap(new_offsets);
	return blocknum;
}

bool Blocks::LoadData(const void* data, unsigned len, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	auto blocknum = verifyData(static_cast<const unsigned char*>(data),
					len, tstore);
	if(blocknum < 0){
		return true;
	}
	return false;
}

bool Blocks::LoadFile(const std::string& fname, TrustStore& tstore){
	offsets.clear();
	headers.clear();
	size_t size;
	auto memblock = ReadBinaryFile(fname, &size);
	return LoadData(memblock.get(), size, tstore);
}

void Blocks::GetLastHash(unsigned char* hash) const {
	if(headers.empty()){ // will be genesis block
		memset(hash, 0xff, HASHLEN);
	}else{
		memcpy(hash, headers.back().hash, HASHLEN);
	}
}

std::ostream& operator<<(std::ostream& stream, const Blocks& blocks){
	for(auto& h : blocks.headers){
		stream << "hash: ";
		hashOStream(stream, h.hash);
		stream << "\nprev: ";
		hashOStream(stream, h.prev);
		stream << "\ntransactions: " << h.txcount <<
			" " << "bytes: " << h.totlen << "\n";
	}
	return stream;
}

std::pair<std::unique_ptr<const unsigned char[]>, size_t>
Block::serializeBlock(unsigned char* prevhash){
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

std::ostream& operator<<(std::ostream& stream, const Block& b){
	// FIXME reset stream after using setfill/setw
	for(size_t i = 0 ; i < b.transactions.size() ; ++i){
		stream << std::setfill('0') << std::setw(5) << i <<
			" " << b.transactions[i].get() << "\n";
	}
	return stream;
}

}
