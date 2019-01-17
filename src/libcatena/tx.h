#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <cstring>
#include <nlohmann/json.hpp>
#include <libcatena/truststore.h>
#include <libcatena/ledgermap.h>
#include <libcatena/utility.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

enum class TXTypes {
	ConsortiumMember = 0x0001,
	ExternalLookup = 0x0002,
	User = 0x0003,
	UserStatus = 0x0004,
	LookupAuthReq = 0x0005,
	LookupAuth = 0x0006,
	UserStatusDelegation = 0x0007,
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
virtual bool Validate(TrustStore& tstore, LedgerMap& lmap) = 0;
virtual nlohmann::json JSONify() const = 0;

// Send oneself to an ostream
virtual std::ostream& TXOStream(std::ostream& s) const = 0;

// Serialize oneself
virtual std::pair<std::unique_ptr<unsigned char[]>, size_t> Serialize() const = 0;

friend std::ostream& operator<<(std::ostream& s, const Transaction* t){
	return t->TXOStream(s);
}

static std::unique_ptr<Transaction>
LexTX(const unsigned char* data, unsigned len,
	const CatenaHash& blkhash, unsigned txidx);

protected:
// FIXME shouldn't need to keep these, but don't want to explicitly pass them
// into validate(). wrap them up in a lambda?
unsigned txidx; // transaction index within block
CatenaHash blockhash; // containing block hash
};

}

#endif
