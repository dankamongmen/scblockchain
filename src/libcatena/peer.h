#ifndef CATENA_LIBCATENA_PEER
#define CATENA_LIBCATENA_PEER

#include <future>

namespace Catena {

// A Peer represents another Catena network node. A given Peer might either be
// inactive (no connection), pending (connection attempted but not yet
// established), or active (connection established).

class Peer {
public:
Peer() = delete;
Peer(const std::string& addr, int defaultport);
virtual ~Peer() = default;

int Port() const {
	return port;
}

std::string Address() const {
	return address;
}

int Connect();

// FIXME we'll almost certainly need to take some shared_ptr around a condition
// variable, then pass a lambda which calls Connect and then signals the
// condition variable...
std::future<int> ConnectAsync() {
	return std::async(std::launch::async, &Peer::Connect, this);
}

private:
std::string address;
int port;
};

}

#endif
