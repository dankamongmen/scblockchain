#ifndef CATENA_LIBCATENA_BUILTIN
#define CATENA_LIBCATENA_BUILTIN

#include <vector>
#include <iostream>
#include "libcatena/sig.h"

namespace Catena {

class BuiltinKeys {
public:
BuiltinKeys();
// Verify the signature on the specified data, using builtin index idx
bool Verify(size_t idx, const unsigned char* in, size_t inlen,
		const unsigned char* sig, size_t siglen){
	if(idx >= keys.size()){
		std::cerr << "invalid builtin key index: " << idx << std::endl;
		return true;
	}
	return keys[idx].Verify(in, inlen, sig, siglen);
}

size_t Count(){
	return keys.size();
}
private:
std::vector<Keypair> keys;
};

}

#endif
