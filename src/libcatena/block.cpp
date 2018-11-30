#include <fstream>
#include <iostream>
#include "libcatena/block.h"

void CatenaBlock::loadFile(const std::string& fname){
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try{
		f.open(fname, std::ios::in | std::ios::binary);
	}catch(std::ifstream::failure& e){
		std::cerr << "error opening file " << fname << std::endl;
		throw;
	}
	f.close();
}
