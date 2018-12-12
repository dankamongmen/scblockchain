#ifndef CATENA_LIBCATENA_SIG
#define CATENA_LIBCATENA_SIG

// We use DER-encoded ECDSA secp256k1 curve for signatures

#define SIGLEN 72 // length of signature outputs in bytes

#include <openssl/evp.h>

namespace Catena {

class Keypair {
public:
Keypair(const char* pubfile);
~Keypair();

private:
EVP_PKEY* pubkey;
};

EVP_PKEY* loadPubkey(const char* fname);

}

#endif
