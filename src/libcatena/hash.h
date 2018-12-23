#ifndef CATENA_LIBCATENA_HASH
#define CATENA_LIBCATENA_HASH

#include <ostream>

namespace Catena {

#define HASHLEN 32 // length of hash outputs in bytes

void catenaHash(const void* in, unsigned len, std::array<unsigned char, HASHLEN>& hash);
std::ostream& hashOStream(std::ostream& s, const std::array<unsigned char, HASHLEN>& hash);

// Naked interface to hashing functions. hash must be at least HASHLEN bytes.
void catenaHash(const void* in, unsigned len, void* hash);
std::ostream& hashOStream(std::ostream& s, const void* hash);

}

#endif
