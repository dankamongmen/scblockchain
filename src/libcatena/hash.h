#ifndef CATENA_LIBCATENA_HASH
#define CATENA_LIBCATENA_HASH

#include <ostream>

namespace Catena {

#define HASHLEN 32 // length of hash outputs in bytes

using CatenaHash = std::array<unsigned char, HASHLEN>;

void catenaHash(const void* in, unsigned len, CatenaHash& hash);
// This might be a bit overbroad, and capture things unexpectedly...? find out
std::ostream& operator<<(std::ostream& s, const CatenaHash& hash);
std::string hashOString(const CatenaHash& hash);

// Naked interface to hashing functions. hash must be at least HASHLEN bytes.
void catenaHash(const void* in, unsigned len, void* hash);
std::ostream& hashOStream(std::ostream& s, const void* hash);

}

#endif
