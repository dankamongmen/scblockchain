#include <cstring>
#include <fstream>
#include <iostream>
#include "libcatena/block.h"

#define BLOCKHEADERLEN 96

unsigned CatenaBlocks::getBlockCount(){
	return blockcount;
}

int CatenaBlocks::verifyData(const char *data, unsigned len){
	int blocknum = 0;
	while(len){
		// FIXME free extracted blocks on any error (unique_ptr?)
		if(len < BLOCKHEADERLEN){
			std::cerr << "needed " << BLOCKHEADERLEN <<
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
		if(chdr.version != 0){
			std::cerr << "expected version 0, got " << chdr.version << std::endl;
			return -1;
		}
		for(int i = 0 ; i < 3 ; ++i){
			chdr.totlen <<= 8;
			chdr.totlen += *data++;
		}
		if(chdr.totlen < BLOCKHEADERLEN || chdr.totlen > len){
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

void CatenaBlocks::loadFile(const std::string& fname){
	std::ifstream f;
	blockcount = 0; // FIXME discard existing blocks
	char *memblock;
	std::streampos size;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(fname, std::ios::in | std::ios::binary | std::ios::ate);
	size = f.tellg();
	memblock = new char[size];
	f.seekg(0, std::ios::beg);
	f.read(memblock, size);
	int blocknum = verifyData(memblock, size);
	if(blocknum <= 0){
		return;
	}
	blockcount = blocknum;
}
