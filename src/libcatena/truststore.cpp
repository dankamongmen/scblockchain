#include <iostream>
#include <libcatena/truststore.h>
#include <libcatena/utility.h>

namespace Catena {

std::ostream& operator<<(std::ostream& s, const TrustStore& ts){
	for(const auto& k : ts.keys){
		const KeyLookup& kl = k.first;
		if(k.second.HasPrivateKey()){
			s << "(*) ";
		}
		s << "hash: ";
		HexOutput(s, kl.first) << ":" << kl.second << "\n";
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
		it->second = *kp;
	}else{
		keys.insert({kidx, *kp});
	}
}

}
