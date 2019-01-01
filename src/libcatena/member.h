#ifndef CATENA_LIBCATENA_MEMBER
#define CATENA_LIBCATENA_MEMBER

#include <nlohmann/json.hpp>
#include <libcatena/tx.h>

namespace Catena {

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
