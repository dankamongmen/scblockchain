#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <cstring>
#include <libcatena/truststore.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

class Transaction {
public:
Transaction() = delete;
Transaction(const unsigned char* hash, unsigned idx){
	txidx = idx;
	memcpy(blockhash, hash, sizeof(blockhash));
}

virtual ~Transaction() = default;
virtual bool extract(const unsigned char* data, unsigned len) = 0;
virtual bool validate(TrustStore& tstore) = 0;

static std::unique_ptr<Transaction>
lexTX(const unsigned char* data, unsigned len,
	const unsigned char* hash, unsigned idx);

private:
bool extractConsortiumMember(const unsigned char* data, unsigned len);

protected:
// FIXME shouldn't need to keep these, but don't want to explicitly pass them
// into validate(). wrap them up in a lambda?
unsigned txidx; // transaction index within block
unsigned char blockhash[HASHLEN]; // containing block hash
};

class NoOpTX : public Transaction {
public:
NoOpTX(const unsigned char* hash, unsigned idx) : Transaction(hash, idx) {}
bool extract(const unsigned char* data, unsigned len) override;
bool validate(TrustStore& tstore __attribute__ ((unused))){
	return false;
}
};

class ConsortiumMemberTX : public Transaction {
public:
ConsortiumMemberTX(const unsigned char* hash, unsigned idx) : Transaction(hash, idx) {}
bool extract(const unsigned char* data, unsigned len) override;
bool validate(TrustStore& tstore) override;

private:
unsigned char signature[SIGLEN];
std::array<unsigned char, HASHLEN> signerhash;
uint32_t signidx; // transaction idx or internal signing idx
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload;
size_t payloadlen;
};

}

#endif
