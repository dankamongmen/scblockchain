#ifndef CATENA_LIBCATENA_TRUSTSTORE
#define CATENA_LIBCATENA_TRUSTSTORE

#include <cstring>
#include <unordered_map>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

using KeyLookup = std::pair<const std::array<unsigned char, HASHLEN>, unsigned>;

struct keylookup_hash {
	template <class T1, class T2>
	size_t operator()(const std::pair<T1, T2>& k) const {
		size_t sha;
		// FIXME assert sizeof(sha) < KEYLEN
		// FIXME use back part of sha, since leading bits might be 0s
		memcpy(&sha, k.first.data(), sizeof(sha));
		return sha;
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
		return true;
	}
}

friend std::ostream& operator<<(std::ostream& s, const TrustStore& ts);

private:
std::unordered_map<KeyLookup, Keypair, keylookup_hash> keys;
};

}

#endif
