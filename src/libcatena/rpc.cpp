#include <netdb.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <libcatena/utility.h>
#include <libcatena/rpc.h>

namespace Catena {

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
virtual std::string IPName() const = 0;
virtual TLSName Name() const = 0;

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
  PolledFD(sd),
  ssl(ssl),
  bio(nullptr),
  accepting(true),
  name(name) {
	if(ssl == nullptr){
		throw NetworkException("tried to poll on null tls");
	}
	SSL_set_fd(ssl, sd); // FIXME can fail
  NameFDPeer();
}

// connected form, associated with Peer, BIO is anchor object
PolledTLSFD(int sd, BIO* bio, std::shared_ptr<Peer> p, const TLSName& name) :
  PolledFD(sd),
  ssl(nullptr),
  bio(bio),
  peer(p),
  accepting(false),
  name(name) {
	if(bio == nullptr || 1 != BIO_get_ssl(bio, &ssl)){
		throw NetworkException("tried to poll on null tls");
  }
  NameFDPeer();
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

bool IsConnection() const { return true; }

std::string IPName() const {
  return ipname;
}

TLSName Name() const {
  return name;
}

void SetName(const TLSName& tname) {
  name = tname;
}

bool Callback(RPCService& rpc) override {
	if(accepting){
		auto ra = SSL_accept(ssl);
		if(ra == 0){
			std::cerr << "error accepting TLS" << std::endl;
			return true;
		}else if(ra < 0){
			auto oerr = SSL_get_error(ssl, ra);
			// FIXME handle partial SSL_accept()
			if(oerr == SSL_ERROR_WANT_READ){
				struct epoll_event ev = {
					.events = EPOLLIN | EPOLLRDHUP,
					.data = { .ptr = &*this, },
				};
				rpc.EpollMod(sd, &ev);
			}else if(oerr == SSL_ERROR_WANT_WRITE){
				struct epoll_event ev = {
					.events = EPOLLOUT | EPOLLRDHUP,
					.data = { .ptr = &*this, },
				};
				rpc.EpollMod(sd, &ev);
			}else{
				std::cerr << "error accepting TLS" << std::endl;
				return true;
			}
      return false;
		}
    struct epoll_event ev = { // successful negotiation
      .events = EPOLLIN | EPOLLRDHUP,
      .data = { .ptr = &*this, },
    };
    SetName(SSLPeerName(ssl));
    accepting = false;
    rpc.EpollMod(sd, &ev);
	}
	char buf[BUFSIZ]; // FIXME ugh
	auto r = SSL_read(ssl, buf, sizeof(buf));
	if(r > 0){
		std::cout << "read us some crap " << r << std::endl;
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

bool IsConnection() const { return false; }

std::string IPName() const {
  return "fixme[l3+l4]"; // FIXME
}

TLSName Name() const {
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
	rpcName = X509NetworkName(x509);
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
      if(cf.name == rpcName){ // connected to ourselves
        c.first->Disconnect();
        BIO_free_all(cf.bio);
      }else{
        int fd = BIO_get_fd(cf.bio, NULL); // FIXME can fail
        auto mapins = epolls.emplace(fd, std::make_unique<PolledTLSFD>(fd, cf.bio, c.first, cf.name));
        struct epoll_event ev = {
          .events = EPOLLIN | EPOLLRDHUP,
          .data = { .ptr = (*mapins.first).second.get(), },
        };
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
			try{
				if(pfd->Callback(*this)){
          int fd = pfd->FD();
          EpollDel(fd);
          epolls.erase(fd);
				}
			}catch(NetworkException& e){
				std::cerr << "error handling epoll result: " << e.what() << std::endl;
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
	for(auto const& r : ret){
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
		  ret.emplace_back(e.second->IPName(), e.second->Name());
    }
	}
	return ret;
}

}
