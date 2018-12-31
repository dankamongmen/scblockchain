#ifndef CATENA_LIBCATENA_LOOKUPAUTHREQTX
#define CATENA_LIBCATENA_LOOKUPAUTHREQTX

#include <libcatena/tx.h>

namespace Catena {

class LookupAuthReqTX : public Transaction {
public:
LookupAuthReqTX() = default;
LookupAuthReqTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
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
uint32_t subjectidx; // subject idx of request (ExternalLookupTX), from payload
size_t payloadlen; // total length of signed payload

const unsigned char*
GetJSONPayload() const {
	return payload.get() + 32 + 4;
}

size_t GetJSONPayloadLength() const {
	return payloadlen - 32 - 4;
}

};

class LookupAuthTX : public Transaction {
public:
LookupAuthTX() = default;
LookupAuthTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

enum class Keytype {
	None,
	AES256,
};

private:
unsigned char signature[SIGLEN];

// specifier of who signed this tx
CatenaHash signerhash;
uint32_t signeridx; // must be exactly 32 bits for serialization
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload;
uint32_t subjectidx; // subject idx of request (LookupAuthReqTX), from payload
size_t payloadlen; // total length of signed payload

};

}

#endif
