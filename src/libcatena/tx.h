#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>
#include <libcatena/hash.h>
#include <libcatena/sig.h>

namespace Catena {

class Transaction {
public:
Transaction() = default;
virtual ~Transaction() = default;
static std::unique_ptr<Transaction> lexTX(const unsigned char* data, unsigned len);
virtual bool extract(const unsigned char* data, unsigned len) = 0;

private:
bool extractConsortiumMember(const unsigned char* data, unsigned len);
};

class NoOpTX : public Transaction {
public:
bool extract(const unsigned char* data, unsigned len) override;
};

class ConsortiumMemberTX : public Transaction {
public:
bool extract(const unsigned char* data, unsigned len) override;
private:
unsigned char signature[SIGLEN];
unsigned char signerhash[HASHLEN]; // only used for LedgerSigned
uint32_t signidx; // transaction idx or internal signing idx
uint16_t sigtype; // FIXME convert to named enum
};

}

#endif
