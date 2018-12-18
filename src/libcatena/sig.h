#ifndef CATENA_LIBCATENA_SIG
#define CATENA_LIBCATENA_SIG

// We use DER-encoded ECDSA secp256k1 curve for signatures

#define SIGLEN 72 // maximum length of signature outputs in bytes

#include <openssl/evp.h>

namespace Catena {

class Keypair {
public:
Keypair() = delete;
Keypair(const char* pubfile, const char* privfile = 0);
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

~Keypair();
size_t Sign(const unsigned char* in, size_t inlen, unsigned char* out, size_t outlen);
bool Verify(const unsigned char* in, size_t inlen, const unsigned char* sig, size_t siglen);

inline bool operator==(const Keypair& rhs) const {
	if(EVP_PKEY_cmp(pubkey, rhs.pubkey) != 1){
		return 0;
	}
	if(privkey && rhs.privkey){
		return EVP_PKEY_cmp(privkey, rhs.privkey);
	}
	return 1;
}

bool HasPrivateKey() const {
	return privkey != nullptr;
}

private:
EVP_PKEY* pubkey;
EVP_PKEY* privkey;
};

EVP_PKEY* loadPubkey(const char* fname);

}

#endif
