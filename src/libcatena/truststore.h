#ifndef CATENA_LIBCATENA_TRUSTSTORE
#define CATENA_LIBCATENA_TRUSTSTORE

#include <cstring>
#include <unordered_map>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

using KeyLookup = std::pair<const unsigned char*, unsigned>;

struct keylookup_hash {
	template <class T1, class T2>
	size_t operator()(const std::pair<T1, T2>& k) const {
		size_t sha;
		// FIXME assert sizeof(sha) < KEYLEN
		// FIXME use back part of sha, since leading bits might be 0s
		memcpy(&sha, k.first, sizeof(sha));
		return sha;
	}
};

struct keylookup_equal {
	bool operator()(const KeyLookup& k1, const KeyLookup& k2) const {
		return !memcmp(k1.first, k2.first, HASHLEN);
	}
};

class TrustStore {
public:
TrustStore() = default;
virtual ~TrustStore() = default;

// Add the keypair (usually just public key), using the specified hash and
// index as its source (this is how it will be referenced in the ledger).
void addKey(const Keypair* kp, const KeyLookup& kidx){
	keys.insert({kidx, *kp});
}

bool Verify(const KeyLookup& kidx, const unsigned char* in, size_t inlen,
		const unsigned char* sig, size_t siglen){
	try{
		return keys.at(kidx).Verify(in, inlen, sig, siglen);
	}catch(const std::out_of_range& oor){ // no such key
std::cerr << "whoa there, no such key little buddy!" << std::endl;
		return true;
	}
}

private:
std::unordered_map<KeyLookup, Keypair, keylookup_hash, keylookup_equal> keys;
};

}

#endif
