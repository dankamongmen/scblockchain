#ifndef CATENA_LIBCATENA_MEMBER
#define CATENA_LIBCATENA_MEMBER

#include <nlohmann/json_fwd.hpp>
#include <libcatena/tx.h>

namespace Catena {

class ConsortiumMemberTX : public Transaction {
public:
ConsortiumMemberTX() = default;
ConsortiumMemberTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
void Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore, LedgerMap& lmap) override;
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

const char*
GetJSONPayload() const {
	return reinterpret_cast<const char*>(payload.get()) + 2 + keylen;
}

size_t GetJSONPayloadLength() const {
	return payloadlen - keylen - 2;
}

};

}

#endif
