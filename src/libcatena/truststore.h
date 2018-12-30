#ifndef CATENA_LIBCATENA_TRUSTSTORE
#define CATENA_LIBCATENA_TRUSTSTORE

#include <memory>
#include <cstring>
#include <unordered_map>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

using SymmetricKey = std::array<unsigned char, 32>; // 256-bit AES key

class SigningException : public std::runtime_error {
public:
SigningException() : std::runtime_error("error signing"){}
SigningException(const std::string& s) : std::runtime_error(s){}
};

using KeyLookup = std::pair<std::array<unsigned char, HASHLEN>, unsigned>;

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
TrustStore() : signingkey(nullptr) {}
virtual ~TrustStore() = default;
TrustStore(const TrustStore& ts);
TrustStore& operator=(const TrustStore& ts);

// Add the keypair (usually just public key), using the specified hash and
// index as its source (this is how it will be referenced in the ledger).
void addKey(const Keypair* kp, const KeyLookup& kidx);

bool Verify(const KeyLookup& kidx, const unsigned char* in, size_t inlen,
		const unsigned char* sig, size_t siglen){
	try{
		return keys.at(kidx).Verify(in, inlen, sig, siglen);
	}catch(const std::out_of_range& oor){ // no such key
		return true;
	}
}

const KeyLookup& GetLookup(const Keypair& kp);

std::pair<std::unique_ptr<unsigned char[]>, size_t>
Sign(const unsigned char* in, size_t inlen, KeyLookup* signer) const;

// Returned ciphertext includes 128 bits of random AES IV, so size >= 16
std::pair<std::unique_ptr<unsigned char>, size_t>
Encrypt(const void* in, size_t len, const SymmetricKey& key) const;

// Only the plaintext is returned. AES IV and ciphertext are provided.
std::pair<std::unique_ptr<unsigned char>, size_t>
Decrypt(const void* in, size_t len, const SymmetricKey& key) const;

friend std::ostream& operator<<(std::ostream& s, const TrustStore& ts);

private:
std::unordered_map<KeyLookup, Keypair, keylookup_hash> keys;
std::unique_ptr<KeyLookup> signingkey;
};

}

#endif
