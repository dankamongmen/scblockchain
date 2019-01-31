#include <queue>
#include <netdb.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <proto/catena.capnp.h>
#include <libcatena/utility.h>
#include <libcatena/proto.h>
#include <libcatena/rpc.h>

namespace Catena {

// RPC messages are prefaced with a 32-bit NBO length
constexpr auto MSGLEN_PREFACE_BYTES = 4;
// ...though, for now, we only allow messages up to 16MB
constexpr auto MSGLEN_MAX = 1u << 24;

// Each epoll()ed file descriptor has an associated free pointer. That pointer
// should yield up a PolledFD derivative.
class PolledFD {
public:
PolledFD(int sd) :
  sd(sd) {
	  if(sd < 0){
		  throw NetworkException("tried to poll on negative fd");
	  }
}

// If Callback() returns true, the PolledFD will be deleted in the epoll loop.
virtual bool Callback(RPCService& rpc) = 0;

virtual bool IsConnection() const = 0;
virtual bool IsOutgoing() const = 0;
virtual std::string IPName() const = 0;
virtual TLSName Name() const = 0;

virtual void EnqueueCall(std::vector<unsigned char>&& call) {
  (void)call;
  throw NetworkException("can't call on generic polled sd");
}


virtual ~PolledFD() {
	if(close(sd)){
		std::cerr << "error closing epoll()ed sd " << sd << std::endl;
	}
}

int FD() const {
	return sd;
}

protected:
int sd;
};

class PolledTLSFD : public PolledFD {
public:

// accepting form, SSL is anchor object
PolledTLSFD(int sd, SSL* ssl, const TLSName& name) :
  PolledTLSFD(sd, name, ssl, nullptr, true) {
	if(ssl == nullptr){
		throw NetworkException("tried to poll on null tls");
	}
	SSL_set_fd(ssl, sd); // FIXME can fail
}

// connected form, associated with Peer, BIO is anchor object
PolledTLSFD(int sd, BIO* bio, const std::shared_ptr<Peer>& p, const TLSName& name) :
  PolledTLSFD(sd, name, nullptr, bio, false) {
	if(bio == nullptr || 1 != BIO_get_ssl(bio, &ssl)){
		throw NetworkException("tried to poll on null tls");
  }
  peer = p;
}

virtual ~PolledTLSFD() {
  if(bio == nullptr){
	  SSL_free(ssl);
  }else{
    BIO_free_all(bio); // SSL is derived object from bio
    // FIXME isn't the sd also going to get close()d in the base destructor?
  }
  if(peer){
    peer->Disconnect();
  }
}

bool IsConnection() const override { return true; }

bool IsOutgoing() const override { return bio ? true : false; }

std::string IPName() const override {
  return ipname;
}

TLSName Name() const override {
  return name;
}

void SetName(const TLSName& tname) {
  name = tname;
}

// Always ensure that we're checking for POLLOUT after use!
void EnqueueCall(std::vector<unsigned char>&& call) override {
  writeq.emplace(call);
}

bool Callback(RPCService& rpc) override {
  struct epoll_event ev = {
    .events = EPOLLRDHUP | EPOLLIN,
    .data = { .ptr = &*this, },
  };
	if(accepting){
		auto ra = SSL_accept(ssl);
		if(ra == 0){
			std::cerr << "error accepting TLS" << std::endl;
			return true;
		}else if(ra < 0){
			auto oerr = SSL_get_error(ssl, ra);
			// FIXME handle partial SSL_accept()
			if(oerr == SSL_ERROR_WANT_READ){
				rpc.EpollMod(sd, &ev);
			}else if(oerr == SSL_ERROR_WANT_WRITE){
        ev.events |= EPOLLOUT;
        ev.events &= ~EPOLLIN;
				rpc.EpollMod(sd, &ev);
			}else{
				std::cerr << "error accepting TLS" << std::endl;
				return true;
			}
      return false;
		}
    SetName(SSLPeerName(ssl));
    accepting = false;
    ev.events |= EPOLLOUT,
    rpc.EpollMod(sd, &ev);
	}
  // post-handshake, rw path
  // write state machine: for now, just send, and expect to be able to FIXME
  int wrote = 0;
  while(writeq.size()){
    auto wq = writeq.front();
    unsigned char buf[MSGLEN_PREFACE_BYTES];
    ulong_to_nbo(wq.size(), buf, sizeof(buf));
    size_t wb;
    if(1 != SSL_write_ex(ssl, buf, sizeof(buf), &wb)){
      throw NetworkException("couldn't write message length");
    }
    if(1 != SSL_write_ex(ssl, wq.data(), wq.size(), &wb)){
      throw NetworkException("couldn't write message");
    }
    writeq.pop();
    ++wrote;
  }
  if(wrote){
    ev.events &= ~EPOLLOUT;
    rpc.EpollMod(sd, &ev);
  }
  // read state machine: we always have some amount we want to read. if we are
  // starting a new sequence, we want to read a 32-bit (4 byte) NBO length. if
  // we have read that length, we want to read whatever it specified. thus, we
  // have three variables at work:
  //
  // how much we have read, haveRead,
  // how much we want to read in this codon, wantRead (wantRead > haveRead)
  // what state we're in (reading length, reading message)
  //
  // we try to read wantRead - haveRead into buffer + haveRead. if we get the
  // full amount, if we're reading length, prep for reading message. if we're
  // reading message, parse and dispatch, and prep for reading length.
  auto r = SSL_read(ssl, readbuf.data() + haveRead, wantRead - haveRead);
	if(r > 0){
    if(haveRead + r == wantRead){
      if(readingMsg == false){ // we were reading the length
        wantRead = nbo_to_ulong(readbuf.data(), MSGLEN_PREFACE_BYTES);
        if(wantRead > MSGLEN_MAX){
          throw NetworkException("message too large");
        }
        readbuf.reserve(wantRead);
      }else{
        std::cout << "received " << wantRead << "-byte rpc on " << sd << std::endl;
        try{
          Dispatch(rpc, readbuf.data(), wantRead);
        }catch(::kj::Exception &e){
          throw NetworkException(e.getDescription());
        }
        wantRead = MSGLEN_PREFACE_BYTES;
      }
      haveRead = 0;
      readingMsg = !readingMsg;
    }else{
      haveRead += r;
    }
	}else{
		auto ra = SSL_get_error(ssl, r);
		if(ra == SSL_ERROR_WANT_WRITE){
			// FIXME else check for SSL_ERROR_WANT_WRITE
		}else if(ra != SSL_ERROR_WANT_READ){
			std::cerr << "lost ssl connection with error " << ra << std::endl;
			return true;
		}
	}
	return false;
}

private:
SSL* ssl; // always set
BIO* bio; // if set, owns ->ssl, which otherwise is its own anchor object
std::shared_ptr<Peer> peer;
bool accepting;
std::string ipname;
TLSName name;
unsigned wantRead;
unsigned haveRead;
bool readingMsg;
std::vector<unsigned char> readbuf;
std::queue<std::vector<unsigned char>> writeq;

// meant to be called by the two more specific constructors, don't use directly
PolledTLSFD(int sd, const TLSName& name, SSL* ssl, BIO* bio, bool accepting) :
  PolledFD(sd),
  ssl(ssl),
  bio(bio),
  accepting(accepting),
  name(name),
  wantRead(MSGLEN_PREFACE_BYTES),
  haveRead(0),
  readingMsg(false) {
  readbuf.reserve(wantRead);
  NameFDPeer();
}

void Dispatch(RPCService& rpc, const unsigned char* buf, size_t len) {
  // FIXME alignment requirements!?!
  const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(buf),
        reinterpret_cast<const capnp::word*>(buf + len));
  capnp::FlatArrayMessageReader node(view);
  auto nodeAd = node.getRoot<capnp::rpc::Call>();
  switch(nodeAd.getMethodId()){
    case Proto::METHOD_ADVERTISE_NODE:{
      auto pload = nodeAd.getParams();
      if(!pload.hasContent()){
        throw NetworkException("NodeAdvertise was missing payload");
      }
      auto r = pload.getContent().getAs<Proto::AdvertiseNode>();
      rpc.HandleAdvertiseNode(r);
      break;
    }case Proto::METHOD_DISCOVER_NODES:{
      auto cb = [&rpc](Proto::AdvertiseNodes::Builder& builder) -> void {
        rpc.NodesAdvertisementFill(builder);
      };
      EnqueueCall(PrepCall<Proto::AdvertiseNodes, decltype(cb)>(Proto::METHOD_ADVERTISE_NODES, cb));
      struct epoll_event ev = {
        .events = EPOLLRDHUP | EPOLLIN | EPOLLOUT,
        .data = { .ptr = &*this, },
      };
      rpc.EpollMod(sd, &ev);
      break;
    }case Proto::METHOD_ADVERTISE_NODES:{
      auto pload = nodeAd.getParams();
      if(!pload.hasContent()){
        throw NetworkException("NodeAdvertises was missing payload");
      }
      auto r = pload.getContent().getAs<Proto::AdvertiseNodes>();
      rpc.HandleAdvertiseNodes(r);
      break;
    }default:
      throw NetworkException("unknown rpc");
  }
}

void NameFDPeer() {
	struct sockaddr ss;
	socklen_t slen = sizeof(ss);
  char paddr[INET6_ADDRSTRLEN];
  char pport[7];
  pport[0] = ':';
  if(getpeername(sd, &ss, &slen)){
    throw NetworkException("error getting socket peer");
  }
  if(getnameinfo(&ss, slen, paddr, sizeof(paddr), pport + 1, sizeof(pport) - 1,
      NI_NUMERICHOST | NI_NUMERICSERV)){
    throw NetworkException("error naming socket");
  }
  ipname = std::string(paddr) + pport;
}

};

class PolledListenFD : public PolledFD {
public:
PolledListenFD(int family, const SSLCtxRAII& sslctx) :
  PolledFD(socket(family, SOCK_STREAM | SOCK_CLOEXEC, 0)),
  sslctx(sslctx) {
	int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
		close(sd);
		throw NetworkException("couldn't set SO_REUSEADDR");
	}
}

bool IsConnection() const override { return false; }

bool IsOutgoing() const override { return false; }

std::string IPName() const override {
  return "fixme[l3+l4]"; // FIXME
}

TLSName Name() const override {
  return std::make_pair("fixmeICN", "fixmeSCN"); // FIXME, use local?
}

bool Callback(RPCService& rpc) override {
	Accept(rpc);
	return false;
}
private:
SSLCtxRAII sslctx;

void Accept(RPCService& rpc) { // FIXME need arrange this all as non-blocking
	struct sockaddr ss;
	socklen_t slen = sizeof(ss);
	int ret = accept4(sd, &ss, &slen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if(ret < 0){
		throw NetworkException(std::string("error accept()ing socket: ") + strerror(errno));
	}
  std::unique_ptr<PolledTLSFD> sfd;
	try{
		sfd = std::make_unique<PolledTLSFD>(ret, sslctx.NewSSL(), TLSName());
		std::cout << "accepted " << ret << " on " << sd << " from " << sfd->IPName() << std::endl;
	}catch(...){
		close(ret);
		throw;
	}
  // from here on, ret is associated with sfd, and will be closed on exit
  struct epoll_event ev = {
    .events = EPOLLIN | EPOLLRDHUP,
    .data = { .ptr = sfd.get(), },
  };
  rpc.EpollAdd(ret, &ev, std::move(sfd));
}

};

// Relies on constructor to purge any added elements if constructor is thrown
void RPCService::OpenListeners() {
	auto lsd = std::make_unique<PolledListenFD>(AF_INET, sslctx);
  int fd = lsd->FD();
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY;
  if(bind(fd, (const struct sockaddr*)&sin, sizeof(sin))){
    throw NetworkException("couldn't bind IPv4 listener");
  }
  if(listen(fd, SOMAXCONN)){
    throw NetworkException("couldn't listen for IPv4");
  }
  struct epoll_event ev = {
    .events = EPOLLIN,
    .data = { .ptr = lsd.get(), },
  };
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)){
    throw NetworkException("couldn't epoll on sd4");
  }
  epolls.emplace(fd, std::move(lsd));
  lsd = std::make_unique<PolledListenFD>(AF_INET6, sslctx);
  fd = lsd->FD();
  struct sockaddr_in6 sin6;
  memset(&sin6, 0, sizeof(sin6));
  sin6.sin6_family = AF_INET6;
  sin6.sin6_port = htons(port);
  memcpy(&sin6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
  int v6only = 1;
  if(setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only))){
    throw NetworkException("no pure IPv6 listeners");
  }
  if(bind(fd, (const struct sockaddr*)&sin6, sizeof(sin6))){
    throw NetworkException("couldn't bind IPv6 listener");
  }
  if(listen(fd, SOMAXCONN)){
    throw NetworkException("couldn't listen for IPv6");
  }
  ev.data.ptr = lsd.get();
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)){
    throw NetworkException("couldn't epoll on sd6");
  }
  epolls.emplace(fd, std::move(lsd));
}

void RPCService::PrepSSLCTX(SSL_CTX* ctx, const char* chainfile, const char* keyfile) {
	if(1 != SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION)){
		throw NetworkException("couldn't force TLSv1.3+");
	}
	if(1 != SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM)){
		throw NetworkException("couldn't load private key");
	}
	if(1 != SSL_CTX_use_certificate_chain_file(ctx, chainfile)){
		throw NetworkException("couldn't load certificate chain");
	}
	if(1 != SSL_CTX_load_verify_locations(ctx, chainfile, NULL)){
		throw NetworkException("couldn't load verification chain");
	}
	if(1 != SSL_CTX_check_private_key(ctx)){
		throw NetworkException("key didn't match cert");
	}
}

RPCService::RPCService(Chain& ledger, const RPCServiceOptions& opts) :
  port(opts.port),
  ledger(ledger),
  sslctx(SSLCtxRAII(SSL_CTX_new(TLS_method()))),
  cancelled(false),
  clictx(std::make_shared<SSLCtxRAII>(SSLCtxRAII(SSL_CTX_new(TLS_method())))),
  connqueue(std::make_shared<PeerQueue>()),
  advertised(opts.addresses) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
  }
	PrepSSLCTX(sslctx.get(), opts.chainfile.c_str(), opts.keyfile.c_str());
	auto x509 = SSL_CTX_get0_certificate(sslctx.get()); // view, don't free
	name = X509NetworkName(x509);
	PrepSSLCTX(clictx.get()->get(), opts.chainfile.c_str(), opts.keyfile.c_str());
	SSL_CTX_set_verify(sslctx.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	epollfd = epoll_create1(EPOLL_CLOEXEC);
	if(epollfd < 0){
		throw NetworkException("couldn't get an epoll");
	}
	try{
    if(port != 0){
      OpenListeners();
    }
	  epoller = std::thread(&RPCService::Epoller, this);
	}catch(...){
	  close(epollfd);
		throw;
	}
}

// FIXME probably need rule of 5
RPCService::~RPCService() {
	cancelled.store(true);
	epoller.join();
	if(close(epollfd)){
		std::cerr << "warning: error closing epoll fd\n";
	}
}

// Crap that we have to go checking a locked strucutre each epoll(), especially
// when those epoll()s are on a period. FIXME kill this off, rigourize.
void RPCService::HandleCompletedConns() {
	const auto& conns = connqueue.get()->GetCompletedPeers();
	for(const auto& c : conns){
    try{
      ConnFuture cf = c.second->get();
      if(cf.name == name){ // connected to ourselves
        c.first->Disconnect();
        BIO_free_all(cf.bio);
      }else{
        int fd = BIO_get_fd(cf.bio, NULL); // FIXME can fail
        auto mapins = epolls.emplace(fd, std::make_unique<PolledTLSFD>(fd, cf.bio, c.first, cf.name));
        struct epoll_event ev = {
          .events = EPOLLIN | EPOLLRDHUP | EPOLLOUT,
          .data = { .ptr = (*mapins.first).second.get(), },
        };
        auto cb = [this](Proto::AdvertiseNode::Builder& builder) -> void {
          NodeAdvertisementFill(builder);
        };
        (*mapins.first).second->EnqueueCall(PrepCall<Proto::AdvertiseNode, decltype(cb)>(Proto::METHOD_ADVERTISE_NODE, cb));
        (*mapins.first).second->EnqueueCall(PrepCall(Proto::METHOD_DISCOVER_NODES));
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)){
          epolls.erase(mapins.first);
          throw NetworkException("couldn't epoll-r on new sd");
        }
      }
    }catch(NetworkException& e){
      std::cerr << e.what() << " connecting to " << c.first->Address()
        << ":" << c.first->Port() << std::endl;
    }
	}
}

// FIXME for now, we just iterate over the peer list checking for any needing a
// connection. we ought convert it into a list sorted by conntime for o(1).
void RPCService::LaunchNewConns() {
  // We'll establish a connection to anyone that hasn't been touched since...
  time_t threshold = time(nullptr) - RetryConnSeconds;
  // FIXME check to see if we have available connection spaces, bail if not
  std::lock_guard<std::mutex> guard(lock);
  for(auto& p : peers){
    // FIXME need a tristate here, since we're not Connected() while connect(2)ing..
    if(!p->Connected()){
      if(p->LastTime() < threshold){
			  Peer::ConnectAsync(p, connqueue);
      }
    }
  }
}

void RPCService::Epoller() {
	(void)ledger;
	while(1) {
		if(cancelled.load()){
			return;
		}
		HandleCompletedConns();
    LaunchNewConns();
		struct epoll_event ev; // FIXME accept multiple events
		auto eret = epoll_wait(epollfd, &ev, 1, 100); // FIXME kill timeout
		if(eret < 0 && errno != EINTR){
			throw NetworkException(std::string("epoll_wait() error: ") + strerror(errno));
		}
		if(eret){
			PolledFD* pfd = static_cast<PolledFD*>(ev.data.ptr);
      int fd = pfd->FD();
			try{
				if(pfd->Callback(*this)){
          EpollDel(fd);
				}
			}catch(NetworkException& e){
				std::cerr << "error handling epoll result: " << e.what() << std::endl;
        EpollDel(fd);
			}
		}
	}
}

void RPCService::AddPeers(const std::string& peerfile) {
	std::ifstream in(peerfile);
	if(!in.is_open()){
		throw std::ifstream::failure("couldn't open " + peerfile);
	}
	std::vector<std::shared_ptr<Peer>> ret;
	std::string line;
	while(std::getline(in, line)){
		if(line.length() == 0 || line[0] == '#'){
			continue;
		}
		ret.emplace_back(std::make_shared<Peer>(line, DefaultRPCPort, clictx, true));
	}
	if(!in.eof()){
		throw ConvertInputException("couldn't extract lines from file");
	}
  AddPeerList(ret);
}

void RPCService::AddPeerList(std::vector<std::shared_ptr<Peer>>& pl) {
	for(auto const& r : pl){
		bool dup = false;
		for(auto const& p : peers){
			// FIXME what about if we discovered the peer, but then
			// have it added? need mark it as configured
			if(r.get()->Port() == p.get()->Port() &&
					r.get()->Address() == p.get()->Address()){
				dup = true;
				break;
			}
		}
		if(!dup){
      lock.lock();
			peers.emplace_back(r);
      lock.unlock();
		}
	}
}

void RPCService::EpollAdd(int fd, struct epoll_event* ev, std::unique_ptr<PolledFD> pfd){
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev)){
    throw NetworkException("epoll rejected new sd " + std::to_string(fd));
  }
  auto mapins = epolls.emplace(fd, std::move(pfd));
  const auto pfdp = (*mapins.first).second.get();
  try{
    if(pfdp->Callback(*this)){
      EpollDel(fd);
    }
  }catch(NetworkException& e){
    EpollDel(fd);
    throw e;
  }
}

int RPCService::EpollMod(int fd, struct epoll_event* ev) {
	return epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, ev);
}

void RPCService::EpollDel(int fd) {
  if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL)){
    std::cerr << "error removing epoll on " << fd << std::endl;
  }
  epolls.erase(fd);
}

int RPCService::ActiveConnCount() const {
  std::lock_guard<std::mutex> guard(lock);
  int total = 0;
  for(const auto& e : epolls){ // FIXME rewrite as std::accumulate?
    if(e.second->IsConnection()){
      ++total;
    }
  }
  return total;
}

std::vector<ConnInfo> RPCService::Conns() const {
  std::lock_guard<std::mutex> guard(lock);
	std::vector<ConnInfo> ret;
	for(const auto& e : epolls){
    if(e.second->IsConnection()){
      auto out = e.second->IsOutgoing();
		  ret.emplace_back(e.second->IPName(), e.second->Name(), out);
    }
	}
	return ret;
}

// Iterate over the connections, and enqueue the transaction to each one.
void RPCService::BroadcastTX(const Transaction& tx) {
  auto serialtx = tx.Serialize();
  std::lock_guard<std::mutex> guard(lock);
  for(auto& e : epolls){
    if(e.second->IsConnection()){
      auto cb = [&serialtx](Proto::BroadcastTX::Builder& builder) -> void {
        builder.setTx(kj::arrayPtr(serialtx.first.get(), serialtx.second));
      };
      e.second->EnqueueCall(PrepCall<Proto::BroadcastTX, decltype(cb)>(Proto::METHOD_BROADCAST_T_X, cb));
      struct epoll_event ev = {
        .events = EPOLLRDHUP | EPOLLIN | EPOLLOUT,
        .data = { .ptr = e.second.get(), },
      };
      EpollMod(e.second->FD(), &ev);
    }
  }
}

void RPCService::NodesAdvertisementFill(Proto::AdvertiseNodes::Builder& builder) const {
  auto peers = Peers(); // locks and unlocks, we use returned copy unlocked
  auto lnodes = builder.initNodes(peers.size());
  for(auto i = 0u ; i < peers.size() ; ++i){
    auto ads = lnodes[i].initAds(1);
    ads.set(0, peers[i].address + ":" + std::to_string(peers[i].port)); // FIXME
  }
}

void RPCService::NodeAdvertisementFill(Proto::AdvertiseNode::Builder& builder) const {
  auto bname = builder.initName();
  bname.setIssuerCN(name.first);
  bname.setSubjectCN(name.second);
  auto adcount = advertised.size();
  auto ads = builder.initAds(adcount);
  for(auto it = 0u ; it < adcount ; ++it){
    ads.set(it, advertised[it]);
  }
}

std::vector<unsigned char> RPCService::NodeAdvertisement() const {
  std::vector<unsigned char> ret;
  auto adcount = advertised.size();
  if(adcount){
    capnp::MallocMessageBuilder builder;
    auto nodeAd = builder.initRoot<Proto::AdvertiseNode>();
    NodeAdvertisementFill(nodeAd);
    auto words = capnp::messageToFlatArray(builder);
    auto bytes = words.asBytes();
    ret.reserve(bytes.size());
    ret.insert(ret.end(), bytes.begin(), bytes.end());
  }
  return ret;
}

void RPCService::HandleAdvertiseNode(const Proto::AdvertiseNode::Reader& reader) {
  if(!reader.hasAds()){
    throw NetworkException("NodeAdvertise was missing ads");
  }
  std::vector<std::shared_ptr<Peer>> ret;
  for(const auto& a : reader.getAds()){
    // FIXME coalesce each getAds into a single Peer
		ret.emplace_back(std::make_shared<Peer>(a.cStr(), DefaultRPCPort, clictx, false));
  }
  AddPeerList(ret);
}

void RPCService::HandleAdvertiseNodes(const Proto::AdvertiseNodes::Reader& reader) {
  if(!reader.hasNodes()){
    throw NetworkException("NodeAdvertises was missing nodes");
  }
  for(auto nreader : reader.getNodes()){
    HandleAdvertiseNode(nreader);
  }
}

}
