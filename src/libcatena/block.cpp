#include <memory>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include "libcatena/block.h"
#include "libcatena/hash.h"
#include "libcatena/tx.h"

namespace Catena {

const int Block::BLOCKVERSION;
const int Block::BLOCKHEADERLEN;

bool Block::extractBody(BlockHeader* chdr, const unsigned char* data, unsigned len){
	if(len / 4 < chdr->txcount){
		std::cerr << "no room for " << chdr->txcount << "-offset table in " << len << " bytes" << std::endl;
		return true;
	}
	uint32_t offsets[chdr->txcount];
	for(unsigned i = 0 ; i < chdr->txcount ; ++i){
		uint32_t offset = 0;
		for(int byte = 0 ; byte < 4 ; ++byte){
			offset <<= 8;
			offset += *data++;
		}
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
		std::unique_ptr<Transaction> tx(Transaction::lexTX(data, txlen));
		if(tx == nullptr){
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
	chdr->version = *data++ * 0x100; // 16-bit version field
	chdr->version += *data++;
	if(chdr->version != Block::BLOCKVERSION){
		std::cerr << "expected version " << Block::BLOCKVERSION << ", got " << chdr->version << std::endl;
		return true;
	}
	chdr->totlen = 0;
	for(int i = 0 ; i < 3 ; ++i){ // 24-bit totlen field
		chdr->totlen <<= 8;
		chdr->totlen += *data++;
	}
	if(chdr->totlen < Block::BLOCKHEADERLEN || chdr->totlen > len){
		std::cerr << "invalid totlen " << chdr->totlen << std::endl;
		return true;
	}
	chdr->txcount = 0;
	for(int i = 0 ; i < 3 ; ++i){
		chdr->txcount <<= 8;
		chdr->txcount += *data++;
	}
	chdr->utc = 0; // 40-bit UTC field
	for(int i = 0 ; i < 5 ; ++i){
		chdr->utc <<= 8;
		chdr->utc += *data++;
	}
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

int Blocks::verifyData(const unsigned char *data, unsigned len){
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
		if(block.extractBody(&chdr, data, chdr.totlen - Block::BLOCKHEADERLEN)){
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

bool Blocks::loadData(const void* data, unsigned len){
	offsets.clear();
	headers.clear();
	auto blocknum = verifyData(static_cast<const unsigned char*>(data), len);
	if(blocknum <= 0){
		return true;
	}
	return false;
}

bool Blocks::loadFile(const std::string& fname){
	offsets.clear();
	headers.clear();
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(fname, std::ios::in | std::ios::binary | std::ios::ate);
	auto size = f.tellg();
	std::unique_ptr<char[]> memblock(new char[size]);
	f.seekg(0, std::ios::beg);
	f.read(memblock.get(), size);
	return loadData(memblock.get(), size);
}

std::pair<std::unique_ptr<const char[]>, unsigned>
Block::serializeBlock(unsigned char* prevhash){
	auto block = new char[BLOCKHEADERLEN]();
	unsigned len = BLOCKHEADERLEN;
	char *targ = block + HASHLEN;
	memcpy(targ, prevhash, HASHLEN);
	targ += HASHLEN;
	*targ++ = BLOCKVERSION / 0x100;
	*targ++ = BLOCKVERSION % 0x100;
	*targ++ = BLOCKHEADERLEN / 0x10000;
	*targ++ = BLOCKHEADERLEN / 0x100;
	*targ++ = BLOCKHEADERLEN % 0x100;
	targ += 3; // txcount
	time_t now = time(NULL);
	++targ; // first byte of utc
	*targ++ = (now & 0xff000000) >> 24u;
	*targ++ = (now & 0x00ff0000) >> 16u;
	*targ++ = (now & 0x0000ff00) >> 8u;
	*targ++ = (now & 0x000000ff);
	catenaHash(block + HASHLEN, BLOCKHEADERLEN - HASHLEN, block);
	memcpy(prevhash, block, HASHLEN);
	std::unique_ptr<const char[]> ret(block);
	return std::make_pair(std::move(ret), len);
}

}
