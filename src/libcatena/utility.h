#ifndef CATENA_LIBCATENA_UTILITY
#define CATENA_LIBCATENA_UTILITY

#include <fstream>
#include <iomanip>
#include <ostream>
#include <iostream>

namespace Catena {

// Simple extractor of network byte order unsigned integers, safe for all
// alignment restrictions.
// FIXME rewrite as template<bytes> atop std::array
inline unsigned long nbo_to_ulong(const unsigned char* data, int bytes){
	unsigned long ret = 0;
	while(bytes-- > 0){
		ret *= 0x100;
		ret += *data++;
	}
	return ret;
}

// Write an unsigned integer into the specified bytes using network byte order,
// zeroing out any unused leading bytes. Returns the memory following the
// written bytes.
// FIXME rewrite as template<bytes> atop std::array
inline unsigned char* ulong_to_nbo(unsigned long hbo, unsigned char* data, int bytes){
	int offset = bytes;
	while(offset-- > 0){
		data[offset] = hbo % 0x100;
		hbo >>= 8;
	}
	return data + bytes;
}

inline std::ostream& HexOutput(std::ostream& s, const unsigned char* data, size_t len){
	std::ios state(NULL);
	state.copyfmt(s);
	s << std::hex;
	for(size_t i = 0 ; i < len ; ++i){
		s << std::setfill('0') << std::setw(2) << (int)data[i];
	}
	s.copyfmt(state);
	return s;
}

template<size_t SIZE>
std::ostream& HexOutput(std::ostream& s, const std::array<unsigned char, SIZE>& data){
	std::ios state(NULL);
	state.copyfmt(s);
	s << std::hex;
	for(auto& e : data){
		s << std::setfill('0') << std::setw(2) << (int)e;
	}
	s.copyfmt(state);
	return s;
}

inline std::unique_ptr<char[]> ReadBinaryFile(const std::string& fname, size_t *len){
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(fname, std::ios::in | std::ios::binary | std::ios::ate);
	*len = f.tellg();
	std::unique_ptr<char[]> memblock(new char[*len]);
	f.seekg(0, std::ios::beg);
	f.read(memblock.get(), *len);
	return memblock;
}

}

#endif
