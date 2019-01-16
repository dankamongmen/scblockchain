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
constexpr int DefaultRPCPort = 40404;

class Chain;
class PolledListenFD;

struct RPCServiceOptions {
  int port; // port on which to listen, may be 0 for no listening service
  std::string chainfile; // chain of PEM-encoded certs, from node to root CA
  std::string keyfile; // PEM-encoded key for node cert (first in chainfile)
  std::vector<std::string> addresses; // addresses to advertise, may be empty
};

class RPCService {
public:
RPCService() = delete;
// chainfile and keyfile in opts must be valid files
RPCService(Chain& ledger, const RPCServiceOptions& opts);

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

std::vector<std::string> Advertisement() const {
  return advertised;
}

void PeerCount(int* defined, int* act, int* maxactive) {
	*defined = peers.size();
	*act = active.size();
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
std::vector<std::shared_ptr<Peer>> active;
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
std::vector<std::string> advertised;

void Epoller();
int EpollListeners();
void OpenListeners();
void PrepSSLCTX(SSL_CTX* ctx, const char* chainfile, const char* keyfile);
void HandleCompletedConns();
};

}

#endif
