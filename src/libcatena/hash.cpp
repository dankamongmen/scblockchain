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

std::string hashOString(const CatenaHash& hash){
	std::stringstream ss;
	ss << hash;
	return ss.str();
}

std::ostream& hashOStream(std::ostream& s, const void* hash){
	HexOutput(s, reinterpret_cast<const unsigned char*>(hash), HASHLEN);
	return s;
}

CatenaHash StrToCatenaHash(const std::string& s) {
	CatenaHash ret;
	// 2 chars for each hash byte
	if(s.size() < 2 * ret.size()){
		throw ConvertInputException("too small for hash: " + s);
	}
	for(size_t i = 0 ; i < ret.size() ; ++i){
		char c1 = s[i * 2];
		char c2 = s[i * 2 + 1];
		ret[i] = ASCHexToVal(c1) * 16 + ASCHexToVal(c2);
	}
	return ret;
}

TXSpec TXSpec::StrToTXSpec(const std::string& s) {
	TXSpec ret;
	ret.first = StrToCatenaHash(s);
	if(s[2 * ret.first.size()] != '.'){
		throw ConvertInputException("txspec expected '.': " + s);
	}
	ret.second = StrToLong(s.substr(2 * ret.first.size() + 1), 0, 0xffffffff);
	return ret;
}

}
