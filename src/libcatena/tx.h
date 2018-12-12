#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

class CatenaTX {
public:
CatenaTX() = default;
bool extract(const unsigned char* data, unsigned len);

private:
bool extractConsortiumMember(const unsigned char* data, unsigned len);
};

#endif
