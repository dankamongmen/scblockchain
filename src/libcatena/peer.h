#ifndef CATENA_LIBCATENA_PEER
#define CATENA_LIBCATENA_PEER

#include <list>
#include <future>
#include <chrono>
#include <algorithm>
#include <openssl/bio.h>
#include <libcatena/tls.h>

namespace Catena {

// A Peer represents another Catena network node. A given Peer might either be
// inactive (no connection), pending (connection attempted but not yet
// established), or active (connection established).
class Peer;

// For returning (copied) details about a Peer beyond libcatena
struct PeerInfo {
std::string address;
int port;
time_t lasttime;
std::string subject;
std::string issuer;
bool configured;
};

// Necessary state to accept and begin using newly-connected Peers. Received
// under a shared_ptr, since the RPCService that initiated the ConnectAsync
// might have disappeared while we were connecting.
class PeerQueue {
public:

void AddPeer(const std::shared_ptr<Peer>& p, std::unique_ptr<std::future<BIO*>>&& bio) {
	std::lock_guard<std::mutex> lock(pmutex);
	peers.emplace_back(p, std::move(bio));
}

std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<BIO*>>>>
GetCompletedPeers() {
  std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<BIO*>>>> ret;
	std::lock_guard<std::mutex> lock(pmutex);
  ret.splice(ret.end(),
             peers,
             std::remove_if(peers.begin(), peers.end(),
                            [](auto& p) -> bool {
                              return std::future_status::ready == p.second->wait_for(std::chrono::seconds(0));
                            }),
             peers.end());
  return ret;
}

private:
std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<BIO*>>>> peers;
std::mutex pmutex;
};

class Peer {
public:
Peer() = delete;
// Set configured if the entry was provided to us via configuration (as
// opposed to discovery), and should thus never be removed.
Peer(const std::string& addr, int defaultport, std::shared_ptr<SSLCtxRAII> sctx,
		bool configured);
virtual ~Peer() = default;

int Port() const {
	return port;
}

std::string Address() const {
	return address;
}

BIO* Connect();

// FIXME pq shouldn't need be shared anymore, might need share p with lambda though
static void
ConnectAsync(std::shared_ptr<Peer> p, std::shared_ptr<PeerQueue> pq) {
	auto fut = std::make_unique<std::future<BIO*>>
    (std::async(std::launch::async,
                [](const auto p, const auto pq) -> BIO* {
                     (void)pq;
                     return p.get()->Connect();
                   }, p, pq));
  pq->AddPeer(p, std::move(fut));
}

// FIXME needs lock against Connect() for at least "lasttime" purposes
PeerInfo Info() const {
	PeerInfo ret{address, port, lasttime, lastSubjectCN, lastIssuerCN,
			configured};
	return ret;
}

std::pair<std::string, std::string> Name() const {
	return std::make_pair(lastIssuerCN, lastSubjectCN);
}

private:
std::shared_ptr<SSLCtxRAII> sslctx;
std::string address;
int port;
time_t lasttime; // last time this was used, successfully or otherwise
std::string lastSubjectCN; // subject CN from last TLS handshake
std::string lastIssuerCN; // issuer CN from last TLS handshake
bool configured; // were we provided during initial configuration?

BIO* TLSConnect(int sd);
};

}

#endif
