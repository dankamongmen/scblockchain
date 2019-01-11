#ifndef CATENA_LIBCATENA_PEER
#define CATENA_LIBCATENA_PEER

#include <future>
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

void AddPeer(const std::shared_ptr<Peer>& p) {
	std::lock_guard<std::mutex> lock(pmutex);
	peers.push_back(p);
}

std::vector<std::shared_ptr<Peer>> Peers() {
	std::lock_guard<std::mutex> lock(pmutex);
	return std::move(peers);
}

private:
std::vector<std::shared_ptr<Peer>> peers;
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

int Connect();

static void ConnectAsync(const std::shared_ptr<Peer>& p,
			std::shared_ptr<PeerQueue> pq) {
	// FIXME probably should be a future that gets put on PeerQueue?
	std::thread t([](auto p, auto pq){
			try{
				p.get()->Connect();
				pq.get()->AddPeer(p);
			}catch(...){ // FIXME at most, catch Catena exceptions
				// FIXME smells
			}
			}, p, pq);
	t.detach();
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
