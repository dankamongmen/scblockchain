#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <cstring>
#include <nlohmann/json.hpp>
#include <libcatena/truststore.h>
#include <libcatena/patientmap.h>
#include <libcatena/utility.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

enum class TXTypes {
	NoOp = 0x0000,
	ConsortiumMember = 0x0001,
	ExternalLookup = 0x0002,
	Patient = 0x0003,
	PatientStatus = 0x0004,
	LookupAuthReq = 0x0005,
	LookupAuth = 0x0006,
	PatientStatusDelegation = 0x0007,
};

// TXType is always serialized as a 16-bit unsigned integer
inline unsigned char* TXType_to_nbo(TXTypes tt, unsigned char* data){
	return ulong_to_nbo(static_cast<unsigned long>(tt), data, 2);
}

class Transaction {
public:
Transaction() = default;
Transaction(const CatenaHash& hash, unsigned idx) :
	txidx(idx),
	blockhash(hash) {}

virtual ~Transaction() = default;
virtual bool Extract(const unsigned char* data, unsigned len) = 0;
virtual bool Validate(TrustStore& tstore, PatientMap& pmap) = 0;
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

// Canonical format is hex representation of block hash, followed by period,
// followed by transaction index with optional leading zeroes. No other content
// is allowed. Throws ConvertInputException on any lexing error.
static TXSpec StrToTXSpec(const std::string& s);

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
bool Validate(TrustStore& tstore __attribute__ ((unused)),
		PatientMap& map __attribute__ ((unused))) override {
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
bool Validate(TrustStore& tstore, PatientMap& pmap) override;
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
