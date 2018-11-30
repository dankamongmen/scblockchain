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
				"bytes, had only " << len << std::endl;
			return -1;
		}
		CatenaBlockHeader chdr;
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
		std::memcpy(&chdr.totlen, data, 3);
		if(chdr.totlen < BLOCKHEADERLEN){
			std::cerr << "invalid totlen " << chdr.totlen << std::endl;
			return -1;
		}
		data += 3;
		// FIXME copy remaining fields
		len -= chdr.totlen;
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
	try{
		f.open(fname, std::ios::in | std::ios::binary | std::ios::ate);
		size = f.tellg();
    		memblock = new char[size];
		f.seekg(0, std::ios::beg);
		f.read(memblock, size);
	}catch(std::ifstream::failure& e){
		std::cerr << "error reading file " << fname << std::endl;
		throw;
	}
	int blocknum = verifyData(memblock, size);
	if(blocknum <= 0){
		return;
	}
	blockcount = blocknum;
}
