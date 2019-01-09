#ifndef CATENA_LIBCATENA_PEER
#define CATENA_LIBCATENA_PEER

#include <future>
#include <openssl/bio.h>
#include <libcatena/tls.h>

namespace Catena {

// A Peer represents another Catena network node. A given Peer might either be
// inactive (no connection), pending (connection attempted but not yet
// established), or active (connection established).

// For returning (copied) details about a Peer beyond libcatena
struct PeerInfo {
std::string address;
int port;
time_t lasttime;
};

class Peer {
public:
Peer() = delete;
Peer(const std::string& addr, int defaultport, std::shared_ptr<SSLCtxRAII> sctx);
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

// FIXME needs lock against Connect() for at least "lasttime" purposes
PeerInfo Info() const {
	PeerInfo ret{address, port, lasttime};
	return ret;
}

private:
std::shared_ptr<SSLCtxRAII> sslctx;
std::string address;
int port;
time_t lasttime; // last time this was used, successfully or otherwise

BIO* TLSConnect(int sd);
};

}

#endif
