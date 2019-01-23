#ifndef CATENA_LIBCATENA_RPC
#define CATENA_LIBCATENA_RPC

// RPC service for a Catena node. We currently create the Chain object, and then
// hand it to RPCService, but someday soon Chain will be originating events, and
// might want to know about said service...perhaps it ought be part of Chain?

#include <string>
#include <vector>
#include <numeric>
#include <stdexcept>
#include <sys/epoll.h>
#include <unordered_map>
#include <openssl/ssl.h>
#include <libcatena/peer.h>
#include <libcatena/tls.h>

namespace Catena {

constexpr int MaxActiveRPCPeers = 8;
constexpr int DefaultRPCPort = 40404;
constexpr int RetryConnSeconds = 300;

class Chain;
class PolledFD;

// For returning (copied) details about connections beyond libcatena
struct ConnInfo {
ConnInfo(std::string&& ipname, const TLSName& name, bool outgoing) :
  ipname(ipname),
  name(name),
  outgoing(outgoing) {}

std::string ipname; // IPv[46] address plus port
TLSName name;
bool outgoing;
};

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

int ActiveConnCount() const;

void PeerCount(int* defined, int* maxactive) const {
  std::lock_guard<std::mutex> guard(lock);
	*defined = peers.size();
	*maxactive = MaxActiveRPCPeers;
}

std::vector<PeerInfo> Peers() const {
  std::lock_guard<std::mutex> guard(lock);
	std::vector<PeerInfo> ret;
	for(auto p : peers){
		ret.push_back(p->Info()); // FIXME ideally construct in place
	}
	return ret;
}

std::vector<ConnInfo> Conns() const;

// Should only be called from within an epoll loop callback
int EpollMod(int sd, struct epoll_event* ev);
void EpollAdd(int fd, struct epoll_event* ev, std::unique_ptr<PolledFD> pfd);
void EpollDel(int fd);

private:
int port;
Chain& ledger;
std::vector<std::shared_ptr<Peer>> peers;
// active connection state, keyed by file descriptor
std::unordered_map<int, std::unique_ptr<PolledFD>> epolls;
SSLCtxRAII sslctx;
int epollfd; // epoll descriptor
std::thread epoller; // sits on epoll() with listen()ing socket and peers
std::atomic<bool> cancelled; // lame signal to Epoller
std::shared_ptr<SSLCtxRAII> clictx; // shared with Peers
TLSName name;
std::shared_ptr<PeerQueue> connqueue;
std::pair<std::string, std::string> rpcName;
std::vector<std::string> advertised;
mutable std::mutex lock;

void Epoller(); // launched as epoller thread, joined in destructor
void OpenListeners();
void PrepSSLCTX(SSL_CTX* ctx, const char* chainfile, const char* keyfile);
void HandleCompletedConns();
void LaunchNewConns();
};

}

#endif
