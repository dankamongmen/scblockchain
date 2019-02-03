#ifndef CATENA_LIBCATENA_TRUSTSTORE
#define CATENA_LIBCATENA_TRUSTSTORE

#include <memory>
#include <cstring>
#include <unordered_map>
#include <libcatena/ledgermap.h>
#include <libcatena/keypair.h>
#include <libcatena/hash.h>

namespace Catena {

using KeyLookup = TXSpec;

class TrustStore {
public:
TrustStore() = default;
TrustStore(const TrustStore& ts) : keys(ts.keys) {}
virtual ~TrustStore() = default;

// Add the keypair (usually just public key), using the specified hash and
// index as its source (this is how it will be referenced in the ledger).
void AddKey(const Keypair* kp, const KeyLookup& kidx);

bool Verify(const KeyLookup& kidx, const unsigned char* in, size_t inlen,
		const unsigned char* sig, size_t siglen){
	try{
		return keys.at(kidx).Verify(in, inlen, sig, siglen);
	}catch(const std::out_of_range& oor){ // no such key
		return true;
	}
}

int PubkeyCount() const {
	return keys.size();
}

// Sign the provided blob with the specified key. The private component of the
// specified key must have already been loaded into the truststore.
std::pair<std::unique_ptr<unsigned char[]>, size_t>
Sign(const unsigned char* in, size_t inlen, const KeyLookup& signer) const;

// Sign the provided blob with the provided private key. The public component
// of the specified key must have already been loaded into the truststore, and
// match the provided private key.
std::pair<std::unique_ptr<unsigned char[]>, size_t>
Sign(const unsigned char* in, size_t inlen, const KeyLookup& signer,
      const void* pkey, size_t plen) const;

// Returned ciphertext includes 128 bits of random AES IV, so size >= 16
// Throws EncryptException on error.
std::pair<std::unique_ptr<unsigned char[]>, size_t>
Encrypt(const void* in, size_t len, const SymmetricKey& key) const;

// Only the plaintext is returned. AES IV and ciphertext are provided.
// Throws DecryptException on error.
std::pair<std::unique_ptr<unsigned char[]>, size_t>
Decrypt(const void* in, size_t len, const SymmetricKey& key) const;

// At least one KeyLookup must map to a Keypair with a private key, and both
// KeyLookups must map to valid public keys.
SymmetricKey DeriveSymmetricKey(const KeyLookup& k1, const KeyLookup& k2) const;

// Both KeyLookups must map to valid public keys, and k2 must match the
// provided private key.
SymmetricKey DeriveSymmetricKey(const KeyLookup& k1, const KeyLookup& k2,
    const void* pkey, size_t plen) const;

friend std::ostream& operator<<(std::ostream& s, const TrustStore& ts);

private:
std::unordered_map<KeyLookup, Keypair> keys;
};

}

#endif
