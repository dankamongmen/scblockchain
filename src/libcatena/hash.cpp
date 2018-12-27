#include <stdexcept>
#include <openssl/evp.h>
#include <libcatena/utility.h>
#include <libcatena/hash.h>

namespace Catena {

// FIXME can we extract these context creations?
void catenaHash(const void* in, unsigned len, void* hash){
	auto mdctx = EVP_MD_CTX_create(); // FIXME clean up on all paths
	if(!mdctx){
		throw std::runtime_error("couldn't get EVP context");
	}
	if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)){
		throw std::runtime_error("couldn't get SHA256 context");
	}
	if(1 != EVP_DigestUpdate(mdctx, in, len)){
		throw std::runtime_error("error running SHA256");
	}
	if(1 != EVP_DigestFinal_ex(mdctx, (unsigned char*)hash, NULL)){
		throw std::runtime_error("error finalizing SHA256");
	}
	EVP_MD_CTX_destroy(mdctx);
}

void catenaHash(const void* in, unsigned len, CatenaHash& hash){
	catenaHash(in, len, hash.data());
}

std::ostream& hashOStream(std::ostream& s, const void* hash){
	HexOutput(s, reinterpret_cast<const unsigned char*>(hash), HASHLEN);
	return s;
}

std::ostream& operator<<(std::ostream& s, const CatenaHash& hash){
	HexOutput(s, hash.data(), hash.size());
	return s;
}

}
