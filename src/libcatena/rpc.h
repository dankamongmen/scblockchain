#ifndef CATENA_LIBCATENA_RPC
#define CATENA_LIBCATENA_RPC

// RPC service for a Catena node. We currently create the Chain object, and then
// hand it to RPCService, but someday soon Chain will be originating events, and
// might want to know about said service...perhaps it ought be part of Chain?

#include <string>
#include <vector>
#include <stdexcept>
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <libcatena/peer.h>
#include <libcatena/tls.h>

namespace Catena {

constexpr int MaxActiveRPCPeers = 8;

class Chain;
class PolledListenFD;

class RPCService {
public:
RPCService() = delete;
// chainfile must specify a valid X.509 certificate chain. Peers must present a
// cert trusted by this chain.
// FIXME probably need to accept our own cert files, ugh. might be able to
//   have our own cert as the last in the cachainfile?
RPCService(Chain& ledger, int port, const std::string& chainfile,
		const std::string& keyfile);

~RPCService();

std::pair<std::string, std::string> Name() const {
	return rpcName;
}

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

std::vector<PeerInfo> Peers() const {
	std::vector<PeerInfo> ret;
	for(auto p : peers){
		ret.push_back(p->Info()); // FIXME ideally construct in place
	}
	return ret;
}

// Callback for listen()ing sockets. Initiates SSL_accept().
int Accept(int sd);
// Should only be called from within an epoll loop callback
int EpollMod(int sd, struct epoll_event& ev);

private:
int port;
Chain& ledger;
std::vector<std::shared_ptr<Peer>> peers;
SSLCtxRAII sslctx;
PolledListenFD* lsd4; // IPv4 and IPv6 listening sockets
PolledListenFD* lsd6; // don't use RAII since they're registered with epoll
int epollfd; // epoll descriptor
std::thread epoller; // sits on epoll() with listen()ing socket and peers
std::atomic<bool> cancelled; // lame signal to Epoller
std::shared_ptr<SSLCtxRAII> clictx; // shared with Peers
std::string issuerCN; // taken from our X509 certificate
std::string subjectCN;
std::shared_ptr<PeerQueue> connqueue;
std::pair<std::string, std::string> rpcName;

void Epoller();
int EpollListeners();
void OpenListeners();
void PrepSSLCTX(SSL_CTX* ctx, const char* chainfile, const char* keyfile);
void HandleCompletedConns();
};

}

#endif
