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
Transaction() = default;
Transaction(const unsigned char* hash, unsigned idx){
	txidx = idx;
	memcpy(blockhash, hash, sizeof(blockhash));
}

virtual ~Transaction() = default;
virtual bool Extract(const unsigned char* data, unsigned len) = 0;
virtual bool Validate(TrustStore& tstore) = 0;

// Send oneself to an ostream
virtual std::ostream& TXOStream(std::ostream& s) const = 0;

// Serialize oneself
virtual std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const = 0;

friend std::ostream& operator<<(std::ostream& s, Transaction* t){
	return t->TXOStream(s);
}

static std::unique_ptr<Transaction>
lexTX(const unsigned char* data, unsigned len,
	const unsigned char* hash, unsigned idx);

protected:
// FIXME shouldn't need to keep these, but don't want to explicitly pass them
// into validate(). wrap them up in a lambda?
unsigned txidx; // transaction index within block
unsigned char blockhash[HASHLEN]; // containing block hash
};

class NoOpTX : public Transaction {
public:
NoOpTX() = default;
NoOpTX(const unsigned char* hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore __attribute__ ((unused))) override {
	return false;
}
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
};

class ConsortiumMemberTX : public Transaction {
public:
ConsortiumMemberTX() = default;
ConsortiumMemberTX(const unsigned char* hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;

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
