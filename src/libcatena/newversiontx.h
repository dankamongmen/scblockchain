#ifndef CATENA_LIBCATENA_NEWVERSIONTX
#define CATENA_LIBCATENA_NEWVERSIONTX

#include <libcatena/tx.h>

namespace Catena {

class NewVersionTX : public Transaction {
public:
NewVersionTX() = default;
NewVersionTX(const CatenaHash& hash, unsigned idx) : Transaction(hash, idx) {}
bool Extract(const unsigned char* data, unsigned len) override;
bool Validate(TrustStore& tstore __attribute__ ((unused)),
		LedgerMap& map __attribute__ ((unused))) override {
	return false;
}
nlohmann::json JSONify() const override;
std::ostream& TXOStream(std::ostream& s) const override;
std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const override;
};

}

#endif
