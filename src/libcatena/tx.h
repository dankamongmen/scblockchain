#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <cstring>
#include <json.hpp>
#include <libcatena/truststore.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

class Transaction {
public:
Transaction() = default;
Transaction(const CatenaHash& hash, unsigned idx) :
	txidx(idx),
	blockhash(hash) {}

virtual ~Transaction() = default;
virtual bool Extract(const unsigned char* data, unsigned len) = 0;
virtual bool Validate(TrustStore& tstore) = 0;
virtual nlohmann::json JSONify() const = 0;

// Send oneself to an ostream
virtual std::ostream& TXOStream(std::ostream& s) const = 0;

// Serialize oneself
virtual std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const = 0;

friend std::ostream& operator<<(std::ostream& s, Transaction* t){
	return t->TXOStream(s);
}

static std::unique_ptr<Transaction>
lexTX(const unsigned char* data, unsigned len,
	const CatenaHash& blkhash, unsigned txidx);

protected:
// FIXME shouldn't need to keep these, but don't want to explicitly pass them
// into validate(). wrap them up in a lambda?
unsigned txidx; // transaction index within block
CatenaHash blockhash; // containing block hash
};

class NoOpTX : public Transaction {
public:
NoOpTX() = default;
NoOpTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore __attribute__ ((unused))) override {
	return false;
}
nlohmann::json JSONify() const override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
};

class ConsortiumMemberTX : public Transaction {
public:
ConsortiumMemberTX() = default;
ConsortiumMemberTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

private:
unsigned char signature[SIGLEN];

// specifier of who signed this tx
CatenaHash signerhash;
uint32_t signeridx; // must be exactly 32 bits for serialization
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload;
size_t keylen; // length of public key
size_t payloadlen; // total length of signed payload

const unsigned char*
GetPubKey() const {
	return payload.get() + 2;
}

const unsigned char*
GetJSONPayload() const {
	return payload.get() + 2 + keylen;
}

size_t GetJSONPayloadLength() const {
	return payloadlen - keylen - 2;
}

};

}

#endif
