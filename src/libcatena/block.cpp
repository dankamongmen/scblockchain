#include <memory>
#include <cstring>
#include <fstream>
#include <iostream>
#include "libcatena/block.h"

const int CatenaBlock::BLOCKVERSION;
const int CatenaBlock::BLOCKHEADERLEN;

// NOT the on-disk packed format
struct CatenaBlockHeader {
	char hash[HASHLEN];
	char prev[HASHLEN];
	unsigned version;
	unsigned totlen;
	unsigned txcount;
	uint64_t utc;
};

unsigned CatenaBlocks::getBlockCount(){
	return blockcount;
}

int CatenaBlocks::verifyData(const char *data, unsigned len){
	int blocknum = 0;
	while(len){
		// FIXME free extracted blocks on any error (unique_ptr?)
		if(len < CatenaBlock::BLOCKHEADERLEN){
			std::cerr << "needed " << CatenaBlock::BLOCKHEADERLEN <<
				" bytes, had only " << len << std::endl;
			return -1;
		}
		CatenaBlockHeader chdr;
		memset(&chdr, 0, sizeof(chdr));
		// FIXME could probably do this faster
		std::memcpy(chdr.hash, data, sizeof(chdr.hash));
		data += sizeof(chdr.hash);
		std::memcpy(chdr.prev, data, sizeof(chdr.prev));
		data += sizeof(chdr.prev);
		// 16-bit version field
		std::memcpy(&chdr.version, data, 2); // FIXME unsafe
		data += 2;
		if(chdr.version != CatenaBlock::BLOCKVERSION){
			std::cerr << "expected version " << CatenaBlock::BLOCKVERSION << ", got " << chdr.version << std::endl;
			return -1;
		}
		for(int i = 0 ; i < 3 ; ++i){
			chdr.totlen <<= 8;
			chdr.totlen += *data++;
		}
		if(chdr.totlen < CatenaBlock::BLOCKHEADERLEN || chdr.totlen > len){
			std::cerr << "invalid totlen " << chdr.totlen << std::endl;
			return -1;
		}
		for(int i = 0 ; i < 3 ; ++i){
			chdr.txcount <<= 8;
			chdr.txcount += *data++;
		}
		for(int i = 0 ; i < 40 ; ++i){
			chdr.utc <<= 8;
			chdr.utc += *data++;
		}
		// FIXME ensure reserved bytes are zeroes
		// FIXME extract data section, stash block
		len -= chdr.totlen;
		++blocknum;
	}
	blockcount = blocknum;
	return blockcount;
}

bool CatenaBlocks::loadData(const char *data, unsigned len){
	blockcount = 0; // FIXME discard existing blocks
	auto blocknum = verifyData(data, len);
	if(blocknum <= 0){
		return false;
	}
	blockcount = blocknum;
	return true;
}

bool CatenaBlocks::loadFile(const std::string& fname){
	blockcount = 0; // FIXME discard existing blocks
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(fname, std::ios::in | std::ios::binary | std::ios::ate);
	auto size = f.tellg();
	std::unique_ptr<char[]> memblock(new char[size]);
	f.seekg(0, std::ios::beg);
	f.read(memblock.get(), size);
	return loadData(memblock.get(), size);
}

std::unique_ptr<const char[]> CatenaBlock::serializeBlock(unsigned &len){
	auto block = new char[BLOCKHEADERLEN]();
	len = BLOCKHEADERLEN;
	char *targ = block + HASHLEN * 2;
	*targ++ = BLOCKVERSION / 0x100;
	*targ++ = BLOCKVERSION % 0x100;
	*targ++ = BLOCKHEADERLEN / 0x10000;
	*targ++ = BLOCKHEADERLEN / 0x100;
	*targ++ = BLOCKHEADERLEN % 0x100;
	std::unique_ptr<const char[]> ret(block);
	return ret;
}
