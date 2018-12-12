#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

namespace Catena {

class Transaction {
public:
Transaction() = default;
bool extract(const unsigned char* data, unsigned len);

private:
bool extractConsortiumMember(const unsigned char* data, unsigned len);
};

}

#endif
