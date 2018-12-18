#ifndef CATENA_LIBCATENA_UTILITY
#define CATENA_LIBCATENA_UTILITY

#include <iomanip>
#include <ostream>

namespace Catena {

inline unsigned long nbo_to_ulong(const unsigned char* data, int bytes){
	unsigned long ret = 0;
	while(bytes-- > 0){
		ret *= 0x100;
		ret += *data++;
	}
	return ret;
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

}

#endif
