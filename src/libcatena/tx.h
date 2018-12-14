#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <libcatena/truststore.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

class Transaction {
public:
Transaction() = default;
virtual ~Transaction() = default;
static std::unique_ptr<Transaction> lexTX(const unsigned char* data, unsigned len);
virtual bool extract(const unsigned char* data, unsigned len) = 0;
virtual bool validate(TrustStore& tstore, const unsigned char* data, size_t len) = 0;

private:
bool extractConsortiumMember(const unsigned char* data, unsigned len);
};

class NoOpTX : public Transaction {
public:
bool extract(const unsigned char* data, unsigned len) override;
bool validate(TrustStore& tstore __attribute__ ((unused)),
		const unsigned char* data __attribute__ ((unused)),
		size_t len __attribute__ ((unused))){
	return false;
}
};

class ConsortiumMemberTX : public Transaction {
public:
bool extract(const unsigned char* data, unsigned len) override;
bool validate(TrustStore& tstore, const unsigned char* data, size_t len){
	return tstore.Verify({signerhash, signidx}, data, len, signature, siglen);
}

private:
unsigned char signature[SIGLEN];
std::array<unsigned char, HASHLEN> signerhash;
uint32_t signidx; // transaction idx or internal signing idx
size_t siglen; // length of signature, up to SIGLEN
};

}

#endif
