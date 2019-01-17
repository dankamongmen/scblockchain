#ifndef CATENA_LIBCATENA_PATIENTSTATUSTX
#define CATENA_LIBCATENA_PATIENTSTATUSTX

#include <libcatena/tx.h>

namespace Catena {

class UserStatusTX : public Transaction {
public:
UserStatusTX() = default;
UserStatusTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
void Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore, LedgerMap& lmap) override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
nlohmann::json JSONify() const override;

private:
unsigned char signature[SIGLEN];
CatenaHash signerhash;
uint32_t signeridx;
uint32_t usdidx;
size_t siglen; // length of signature, up to SIGLEN
std::unique_ptr<unsigned char[]> payload;
size_t payloadlen; // total length of signed payload

const unsigned char*
GetJSONPayload() const {
	return payload.get() + signerhash.size() + 4;
}

size_t GetJSONPayloadLength() const {
	return payloadlen - signerhash.size() - 4;
}

};

}

#endif
