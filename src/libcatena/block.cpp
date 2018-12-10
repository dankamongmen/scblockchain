#include <memory>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include "libcatena/block.h"
#include "libcatena/hash.h"

const int CatenaBlock::BLOCKVERSION;
const int CatenaBlock::BLOCKHEADERLEN;

bool CatenaBlock::extractHeader(CatenaBlockHeader* chdr, const char* data,
		unsigned len, const unsigned char* prevhash, uint64_t prevutc){
	if(len < CatenaBlock::BLOCKHEADERLEN){
		std::cerr << "needed " << CatenaBlock::BLOCKHEADERLEN <<
			" bytes, had only " << len << std::endl;
		return false;
	}
	std::memcpy(chdr->hash, data, sizeof(chdr->hash));
	data += sizeof(chdr->hash);
	const char* hashstart = data;
	std::memcpy(chdr->prev, data, sizeof(chdr->prev));
	if(memcmp(chdr->prev, prevhash, sizeof(chdr->prev))){
		std::cerr << "invalid prev hash (wanted ";
		// FIXME factor this out? reused elsewhere...
		std::ios state(NULL);
		state.copyfmt(std::cerr);
		std::cerr << std::hex;
		for(int i = 0 ; i < HASHLEN ; ++i){
			std::cerr << std::setfill('0') << std::setw(2) << (int)prevhash[i];
		}
		std::cerr.copyfmt(state);
		std::cerr << ")" << std::endl;
		return false;
	}
	data += sizeof(chdr->prev);
	chdr->version = *data++ * 0x100; // 16-bit version field
	chdr->version += *data++;
	if(chdr->version != CatenaBlock::BLOCKVERSION){
		std::cerr << "expected version " << CatenaBlock::BLOCKVERSION << ", got " << chdr->version << std::endl;
		return false;
	}
	chdr->totlen = 0;
	for(int i = 0 ; i < 3 ; ++i){ // 24-bit totlen field
		chdr->totlen <<= 8;
		chdr->totlen += *data++;
	}
	if(chdr->totlen < CatenaBlock::BLOCKHEADERLEN || chdr->totlen > len){
		std::cerr << "invalid totlen " << chdr->totlen << std::endl;
		return false;
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
	if(chdr->utc < prevutc){ // allow non-strictly-increasing timestamps?
		std::cerr << "utc " << chdr->utc << " was less than " << prevutc << std::endl;
		return false;
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
		std::ios state(NULL);
		state.copyfmt(std::cerr);
		std::cerr << std::hex;
		for(int i = 0 ; i < HASHLEN ; ++i){
			std::cerr << std::setfill('0') << std::setw(2) << (int)hash[i];
		}
		std::cerr.copyfmt(state);
		std::cerr << ")" << std::endl;
		return false;
	}
	return true;
}

int CatenaBlocks::verifyData(const char *data, unsigned len){
	unsigned char prevhash[HASHLEN] = {0};
	uint64_t prevutc = 0;
	unsigned totlen = 0;
	int blocknum = 0;
	offsets.clear();
	headers.clear();
	while(len){
		CatenaBlockHeader chdr;
		if(!CatenaBlock::extractHeader(&chdr, data, len, prevhash, prevutc)){
			headers.clear();
			offsets.clear();
			return -1;
		}
		data += CatenaBlock::BLOCKHEADERLEN;
		memcpy(prevhash, chdr.hash, sizeof(prevhash));
		prevutc = chdr.utc;
		// FIXME extract data section, stash block
		len -= chdr.totlen;
		offsets.push_back(totlen);
		headers.push_back(chdr);
		totlen += chdr.totlen;
		++blocknum;
	}
	return blocknum;
}

bool CatenaBlocks::loadData(const char *data, unsigned len){
	auto blocknum = verifyData(data, len);
	if(blocknum <= 0){
		return false;
	}
	return true;
}

bool CatenaBlocks::loadFile(const std::string& fname){
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
CatenaBlock::serializeBlock(unsigned char* prevhash){
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
	time_t now = time(NULL);
	++targ;
	*targ++ = (now & 0xff000000) >> 24u;
	*targ++ = (now & 0x00ff0000) >> 16u;
	*targ++ = (now & 0x0000ff00) >> 8u;
	*targ++ = (now & 0x000000ff);
	catenaHash(block + HASHLEN, BLOCKHEADERLEN - HASHLEN, block);
	memcpy(prevhash, block, HASHLEN);
	std::unique_ptr<const char[]> ret(block);
	return std::make_pair(std::move(ret), len);
}
