#ifndef CATENA_LIBCATENA_SIG
#define CATENA_LIBCATENA_SIG

// We use DER-encoded ECDSA secp256k1 curve for signatures

#define SIGLEN 72 // maximum length of signature outputs in bytes

#include <memory>
#include <openssl/evp.h>

namespace Catena {

using SymmetricKey = std::array<unsigned char, 32>; // 256-bit AES key

class KeypairException : public std::runtime_error {
public:
KeypairException() : std::runtime_error("keypair error"){}
KeypairException(const std::string& s) : std::runtime_error(s){}
};

class Keypair {
public:
Keypair() = delete;
Keypair(const std::string& privfile);
// Instantiate a verification-only keypair from memory
Keypair(const unsigned char* pubblob, size_t len);

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

~Keypair();

size_t Sign(const unsigned char* in, size_t inlen,
		unsigned char* out, size_t outlen) const;

std::pair<std::unique_ptr<unsigned char[]>, size_t>
Sign(const unsigned char* in, size_t inlen) const;

bool Verify(const unsigned char* in, size_t inlen, const unsigned char* sig, size_t siglen);

// Require that both have the pubkey set, and that it is equal. Only if both
// have a privkey must it be equal (i.e., two Keypairs with matching public
// key, where only one has a private key defined, are considered equal).
/*
inline bool operator==(const Keypair& rhs) const {
	if(EVP_PKEY_cmp(pubkey, rhs.pubkey) != 1){
		return 0;
	}
	if(privkey && rhs.privkey){
		return EVP_PKEY_cmp(privkey, rhs.privkey);
	}
	return 1;
}
*/

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

private:
EVP_PKEY* pubkey;
EVP_PKEY* privkey;
};

EVP_PKEY* loadPubkey(const char* fname);

}

#endif
