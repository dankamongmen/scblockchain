#ifndef CATENA_LIBCATENA_CHAIN
#define CATENA_LIBCATENA_CHAIN

#include <libcatena/truststore.h>

namespace Catena {

// The blockchain (one or more CatenaBlocks on disk) as indexed in memory
class Chain {
public:
Chain() = default;
Chain(const BuiltinKeys& bkeys);
private:
TrustStore tstore;
};

}

#endif
