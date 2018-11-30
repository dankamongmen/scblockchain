#include <fstream>
#include <iostream>
#include "libcatena/block.h"

void CatenaBlocks::verifyData(const char *data, unsigned len){
	if(data){
		std::cout << "data length: " << len << std::endl;
	}
}

void CatenaBlocks::loadFile(const std::string& fname){
	std::ifstream f;
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
	verifyData(memblock, size);
}
