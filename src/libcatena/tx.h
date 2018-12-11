#ifndef CATENA_LIBCATENA_TX
#define CATENA_LIBCATENA_TX

class CatenaTX {
public:
CatenaTX() = default;
bool extract(const char* data, unsigned len);
};

#endif
