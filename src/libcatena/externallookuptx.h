#ifndef CATENA_LIBCATENA_EXTERNALLOOKUPTX
#define CATENA_LIBCATENA_EXTERNALLOOKUPTX

#include <libcatena/tx.h>

namespace Catena {

class ExternalLookupTX : public Transaction {
public:
ExternalLookupTX() = default;
ExternalLookupTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

private:
unsigned char signature[SIGLEN];
CatenaHash signerhash;
uint32_t signeridx;
uint16_t lookuptype;
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload;
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

}

#endif