#ifndef CATENA_LIBCATENA_BUILTIN
#define CATENA_LIBCATENA_BUILTIN

#include <vector>
#include <libcatena/truststore.h>
#include <libcatena/keypair.h>
#include <libcatena/hash.h>

namespace Catena {

class BuiltinKeys {
public:
BuiltinKeys();

// Verify the signature on the specified data, using builtin index idx
bool Verify(size_t idx, const unsigned char* in, size_t inlen,
		const unsigned char* sig, size_t siglen){
	if(idx >= keys.size()){
		throw SigningException("invalid builtin key index");
	}
	return keys[idx].Verify(in, inlen, sig, siglen);
}

size_t Count(){
	return keys.size();
}

void AddToTrustStore(TrustStore& tstore){
	TXSpec txs;
	txs.first.fill(0xffu);
	for(size_t i = 0 ; i < keys.size() ; ++i){
		txs.second = i;
		tstore.AddKey(&keys[i], txs);
	}
}

private:
std::vector<Keypair> keys;
};

}

#endif
