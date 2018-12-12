#ifndef CATENA_LIBCATENA_BUILTIN
#define CATENA_LIBCATENA_BUILTIN

#include <vector>
#include "libcatena/sig.h"

namespace Catena {

class BuiltinKeys {
public:
BuiltinKeys();
private:
std::vector<Keypair> keys;
};

}

#endif
