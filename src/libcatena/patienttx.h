#ifndef CATENA_LIBCATENA_PATIENTTX
#define CATENA_LIBCATENA_PATIENTTX

#include <memory>
#include <libcatena/tx.h>

namespace Catena {

class PatientTX : public Transaction {
public:
PatientTX() = default;
PatientTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore, LedgerMap& lmap) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

private:
unsigned char signature[SIGLEN];
CatenaHash signerhash;
uint32_t signeridx;
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload; // key + ciphertext
size_t keylen; // length of public key within payload
size_t payloadlen; // total length of signed payload

const unsigned char*
GetPubKey() const {
	return payload.get() + 2;
}

const unsigned char*
GetPayload() const {
	return payload.get() + 2 + keylen;
}

size_t GetPayloadLength() const {
	return payloadlen - keylen - 2;
}

};

class PatientStatusDelegationTX : public Transaction {
public:
PatientStatusDelegationTX() = default;
PatientStatusDelegationTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore, LedgerMap& lmap) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

private:
unsigned char signature[SIGLEN];
CatenaHash signerhash;
uint32_t signeridx;
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload; // key + ciphertext
uint32_t cmidx; // ConsortiumMember idx, from payload
int statustype; // extracted from the payload
size_t payloadlen; // total length of signed payload

const unsigned char*
GetJSONPayload() const {
	return payload.get() + 4 + signerhash.size() + 4;
}

size_t GetJSONPayloadLength() const {
	return payloadlen - signerhash.size() - 4 - 4;
}

};

}

#endif
