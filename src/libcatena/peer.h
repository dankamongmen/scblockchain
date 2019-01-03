#ifndef CATENA_LIBCATENA_PEER
#define CATENA_LIBCATENA_PEER

namespace Catena {

// A Peer represents another Catena network node. A given Peer might either be
// inactive (no connection), pending (connection attempted but not yet
// established), or active (connection established).

class Peer {
public:
Peer() = delete;
Peer(const std::string& addr, int defaultport);
virtual ~Peer() = default;
};

}

#endif
