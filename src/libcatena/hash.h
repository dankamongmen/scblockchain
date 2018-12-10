#ifndef CATENA_LIBCATENA_HASH
#define CATENA_LIBCATENA_HASH

#include <ostream>

#define HASHLEN 32 // length of hash outputs in bytes

// Interface to hashing functions. hash must be at least HASHLEN bytes.
void catenaHash(const void* in, unsigned len, void* hash);
std::ostream& hashOStream(std::ostream& s, const void* hash);

#endif
