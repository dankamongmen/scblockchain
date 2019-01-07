#ifndef CATENA_LIBCATENA_RPC
#define CATENA_LIBCATENA_RPC

// RPC service for a Catena node. We currently create the Chain object, and then
// hand it to RPCService, but someday soon Chain will be originating events, and
// might want to know about said service...perhaps it ought be part of Chain?

#include <string>
#include <vector>
#include <stdexcept>
#include <openssl/ssl.h>
#include <libcatena/chain.h>
#include <libcatena/peer.h>
#include <libcatena/tls.h>

namespace Catena {

constexpr int MaxActiveRPCPeers = 8;

class RPCService {
public:
RPCService() = delete;
// chainfile must specify a valid X.509 certificate chain. Peers must present a
// cert trusted by this chain.
// FIXME probably need to accept our own cert+key files, ugh. might be able to
//   have our own cert as the last in the cachainfile?
RPCService(Chain& chain, int port, const std::string& chainfile);

// peerfile must contain one peer per line, specified as an IPv4 or IPv6
// address and optional ":port" suffix. If a port is not specified for a peer,
// the RPC service port is assumed. Blank lines and comment lines beginning
// with '#' are ignored. If parsing succeeds, any new entries are added to the
// peer set.
void AddPeers(const std::string& peerfile);

int Port() const {
	return port;
}

void PeerCount(int* defined, int* active, int* maxactive) {
	*defined = peers.size();
	*active = 0; // FIXME
	*maxactive = MaxActiveRPCPeers;
}

private:
int port;
Chain& ledger;
std::vector<Peer> peers;
SSLCtxRAII sslctx;
};

}

#endif
