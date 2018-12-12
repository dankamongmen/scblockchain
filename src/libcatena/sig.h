#ifndef CATENA_LIBCATENA_SIG
#define CATENA_LIBCATENA_SIG

// We use DER-encoded ECDSA secp256k1 curve for signatures

#define SIGLEN 72 // length of signature outputs in bytes

#include <openssl/evp.h>

namespace Catena {

class Keypair {
public:
Keypair(const char* pubfile, const char* privfile = 0);
~Keypair();
size_t Sign(const unsigned char* in, size_t inlen, unsigned char* out, size_t outlen);
bool Verify(const unsigned char* in, size_t inlen, const unsigned char* sig, size_t siglen);

private:
EVP_PKEY* pubkey;
EVP_PKEY* privkey;
};

EVP_PKEY* loadPubkey(const char* fname);

}

#endif
