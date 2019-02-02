#ifndef CATENA_LIBCATENA_SIG
#define CATENA_LIBCATENA_SIG

// We use DER-encoded ECDSA secp256k1 curve for signatures

#define SIGLEN 72 // maximum length of signature outputs in bytes

#include <memory>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <libcatena/exceptions.h>

namespace Catena {

using SymmetricKey = std::array<unsigned char, 32>; // 256-bit AES key

class Keypair {
public:
Keypair() :
  pubkey(nullptr),
  privkey(nullptr) {}

Keypair(const std::string& privfile);

// Instantiate a verification-only keypair from memory. For a private keypair
// constructed from memory, use class static PrivateKeypair().
Keypair(const unsigned char* pubblob, size_t len);

// Create a keypair from a private key blob
static Keypair PrivateKeypair(const void* in, size_t len);

Keypair(const Keypair& kp) :
  pubkey(kp.pubkey),
  privkey(kp.privkey){
	if(privkey){
		EVP_PKEY_up_ref(privkey);
	}
	EVP_PKEY_up_ref(pubkey);
}

Keypair& operator=(Keypair kp){
	std::swap(privkey, kp.privkey);
	std::swap(pubkey, kp.pubkey);
	return *this;
}

inline bool operator==(const Keypair& x) const {
  if(pubkey && x.pubkey){
    return EVP_PKEY_cmp(pubkey, x.pubkey) == 1;
  }else if(pubkey || x.pubkey){
    return false;
  }
  return true;
}

~Keypair();

size_t Sign(const unsigned char* in, size_t inlen,
		unsigned char* out, size_t outlen) const;

std::pair<std::unique_ptr<unsigned char[]>, size_t>
Sign(const unsigned char* in, size_t inlen) const;

bool Verify(const unsigned char* in, size_t inlen, const unsigned char* sig, size_t siglen);

bool HasPrivateKey() const {
	return privkey != nullptr;
}

void Merge(const Keypair& pair) {
	if(pair.pubkey && pubkey){
		if(EVP_PKEY_cmp(pair.pubkey, pubkey) != 1){
			throw KeypairException("can't merge different pubkeys");
		}
	}else if(pair.pubkey){
		pubkey = pair.pubkey;
		EVP_PKEY_up_ref(pubkey);
	}
	if(pair.privkey && privkey){
		if(EVP_PKEY_cmp(pair.privkey, privkey) != 1){
			throw KeypairException("can't merge different privkeys");
		}
	}else if(pair.privkey){
		privkey = pair.privkey;
		EVP_PKEY_up_ref(privkey);
	}
}

// Derive a symmetric key from this keypair together with peer. At least one
// must have a private key loaded.
SymmetricKey DeriveSymmetricKey(const Keypair& peer) const;

static std::ostream& PrintPublicKey(std::ostream& s, const EVP_PKEY* evp);

friend std::ostream& operator<<(std::ostream& s, const Keypair& kp){
	return Keypair::PrintPublicKey(s, kp.pubkey);
}

static SymmetricKey CreateSymmetricKey() {
	SymmetricKey ret;
	if(1 != RAND_bytes(ret.data(), ret.size())){
		throw KeypairException("couldn't generate random AES key");
	}
	return ret;
}

// Generate an ECDSA keypair using secp256k1
void Generate();

// Get the PEM-encoded public key
std::string PubkeyPEM() const;

private:
EVP_PKEY* pubkey;
EVP_PKEY* privkey;
};

}

#endif
