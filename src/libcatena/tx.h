#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

#include <memory>

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
};

}

#endif
