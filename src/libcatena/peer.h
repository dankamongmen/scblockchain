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
bool configured;
bool connected;
};

// Information related to established connection, passed via std::future
struct ConnFuture {
  BIO* bio;
  TLSName name;
};

// Necessary state to transfer newly-connected Peers from their AsyncConnect
// threads to RPCService. Received under a shared_ptr, since the RPCService that
// initiated the ConnectAsync might have disappeared while we were connecting.
class PeerQueue {
public:
void AddPeer(const std::shared_ptr<Peer>& p, std::unique_ptr<std::future<ConnFuture>> bio) {
	std::lock_guard<std::mutex> lock(pmutex);
	peers.emplace_back(p, std::move(bio));
}

std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<ConnFuture>>>>
GetCompletedPeers() {
  std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<ConnFuture>>>> ret;
	std::lock_guard<std::mutex> lock(pmutex);
  auto it = peers.begin();
  while(it != peers.end()){
    if(std::future_status::ready == it->second->wait_for(std::chrono::seconds(0))){
      auto newit = ++it;
      --it; // seriously?
      // FIXME can we not splice to ret.end()?
      ret.splice(ret.begin(), peers, it);
      it = newit;
    }else{
      ++it;
    }
  }
  return ret;
}

private:
std::list<std::pair<std::shared_ptr<Peer>, std::unique_ptr<std::future<ConnFuture>>>> peers;
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

ConnFuture Connect();

// FIXME pq shouldn't need be shared anymore, might need share p with lambda though
static void
ConnectAsync(std::shared_ptr<Peer> p, std::shared_ptr<PeerQueue> pq) {
  std::promise<ConnFuture> prom;
  auto fut = std::make_unique<std::future<ConnFuture>>(prom.get_future());
  std::thread t([](const auto p, auto pq, auto prom) {
                     (void)pq;
                     try{
                       auto b = p.get()->Connect();
                       prom.set_value(b);
                     }catch(...){
                       try{
                         prom.set_exception(std::current_exception());
                       }catch(...){}
                     }
                   }, p, pq, std::move(prom));
  t.detach();
  pq->AddPeer(p, std::move(fut));
}

// FIXME needs lock against Connect() for at least "lasttime" purposes
PeerInfo Info() const {
	PeerInfo ret{address, port, lasttime, configured, connected};
	return ret;
}

time_t LastTime() const {
  return lasttime;
}

bool Connected() const {
  return connected;
}

void Disconnect() {
  if(connected){
    connected = false;
    lasttime = time(nullptr);
  }
}

private:
std::shared_ptr<SSLCtxRAII> sslctx;
std::string address;
int port;
time_t lasttime; // last time this was used, successfully or otherwise
bool configured; // were we provided during initial configuration?
bool connected; // are we actively connected?

ConnFuture TLSConnect(int sd);

void MarkConnected() {
  if(!connected){
    lasttime = time(nullptr);
    connected = true;
  }
}

};

}

#endif
