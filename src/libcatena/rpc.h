#ifndef CATENA_LIBCATENA_RPC
#define CATENA_LIBCATENA_RPC

#include <string>
#include <vector>
#include <stdexcept>
#include <openssl/ssl.h>
#include <libcatena/peer.h>

namespace Catena {

class NetworkException : public std::runtime_error {
public:
NetworkException() : std::runtime_error("network error"){}
NetworkException(const std::string& s) : std::runtime_error(s){}
};

class RPCService {
public:
RPCService() = delete;
// chainfile must specify a valid X.509 certificate chain. Peers must present a
// cert trusted by this chain.
// FIXME probably need to accept our own cert+key files, ugh. might be able to
//   have our own cert as the last in the cachainfile?
RPCService(int port, const std::string& chainfile);
virtual ~RPCService() = default;

// peerfile must contain one peer per line, specified as an IPv4 or IPv6
// address and optional ":port" suffix. If a port is not specified for a peer,
// the RPC service port is assumed. Blank lines and comment lines beginning
// with '#' are ignored. If parsing succeeds, any new entries are added to the
// peer set.
void AddPeers(const std::string& peerfile);

int Port() const {
	return port;
}

static constexpr int MaxActivePeers = 8;

void PeerCount(int* defined, int* active, int* maxactive) {
	*defined = peers.size();
	*active = 0; // FIXME
	*maxactive = MaxActivePeers;
}

private:
int port;
std::vector<Peer> peers;
SSL_CTX* sslctx;
};

}

#endif
