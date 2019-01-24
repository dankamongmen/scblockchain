#ifndef CATENA_LIBCATENA_PROTO
#define CATENA_LIBCATENA_PROTO

#include <vector>

namespace Catena {

template <typename T, typename F>
std::vector<unsigned char> PrepCall(unsigned method,
    const std::vector<unsigned char>& payload);

}

#endif
