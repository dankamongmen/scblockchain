#include <iostream>
#include <libcatena/truststore.h>
#include <libcatena/utility.h>

namespace Catena {

std::ostream& operator<<(std::ostream& s, const TrustStore& ts){
	for(const auto& k : ts.keys){
		const KeyLookup& kl = k.first;
		const Keypair& kp = k.second;
		if(kp.HasPrivateKey()){
			s << "(*) ";
		}
		HexOutput(s, kl.first) << ":" << kl.second << "\n";
		s << kp;
	}
	return s;
}

const KeyLookup& TrustStore::GetLookup(const Keypair& kp){
	for(const auto& k : keys){
		if(k.second == kp){
			return k.first;
		}
	}
	throw std::out_of_range("no such public key");
}

void TrustStore::addKey(const Keypair* kp, const KeyLookup& kidx){
	auto it = keys.find(kidx);
	if(it != keys.end()){
		if(kp->HasPrivateKey() && !signingkey){
			signingkey = std::make_unique<KeyLookup>(kidx);
		}
		it->second = *kp;
	}else{
		keys.insert({kidx, *kp});
	}
}

// FIXME should probably throw exceptions on errors
std::pair<std::unique_ptr<unsigned char[]>, size_t>
TrustStore::Sign(const unsigned char* in, size_t inlen, KeyLookup* signer) const {
	if(signingkey == nullptr){
		return std::make_pair(nullptr, 0);
	}
	const auto& it = keys.find(*signingkey.get());
	if(it == keys.end()){
		return std::make_pair(nullptr, 0);
	}
	*signer = it->first;
	return it->second.Sign(in, inlen);
}

}
