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

class PolledListenFD : public PolledFD {
public:
PolledListenFD(int family) :
  PolledFD(socket(family, SOCK_STREAM | SOCK_CLOEXEC, 0)) {
	int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
		close(sd);
		throw NetworkException("couldn't set SO_REUSEADDR");
	}
}

bool Callback(RPCService& rpc) override {
	rpc.Accept(sd);
	return false;
}
};

// A TLS-wrapped, established connection we originated to a Peer
class PolledConnFD : public PolledFD {
public:
PolledConnFD(BIO* bio, std::shared_ptr<Peer> p) :
  PolledFD(BIO_get_fd(bio, NULL)),
  bio(bio),
  peer(p) {
	if(bio == nullptr){
		throw NetworkException("tried to poll on null bio");
	}
}

virtual ~PolledConnFD() {
  BIO_free_all(bio);
  peer->Disconnect();
  // FIXME isn't the sd also going to get close()d in the base destructor?
}

bool Callback(RPCService& rpc) override {
  (void)rpc;
	char buf[BUFSIZ]; // FIXME ugh
	auto r = BIO_read(bio, buf, sizeof(buf));
	if(r > 0){
		std::cout << "read us some crap " << r << std::endl;
	}else{
    SSL* s;
    BIO_get_ssl(bio, &s);
		auto ra = SSL_get_error(s, r);
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
BIO* bio;
std::shared_ptr<Peer> peer;
};

class PolledTLSFD : public PolledFD {
public:
PolledTLSFD(int sd, SSL* ssl) :
  PolledFD(sd),
  ssl(ssl),
  accepting(true) {
	if(ssl == nullptr){
		throw NetworkException("tried to poll on null tls");
	}
	SSL_set_fd(ssl, sd); // FIXME can fail
}

virtual ~PolledTLSFD() {
	SSL_free(ssl);
}

// FIXME get rid of this, have Accept() use Callback()
SSL* HackSSL() const {
	return ssl;
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
				rpc.EpollMod(sd, ev);
			}else if(oerr == SSL_ERROR_WANT_WRITE){
				struct epoll_event ev = {
					.events = EPOLLOUT | EPOLLRDHUP,
					.data = { .ptr = &*this, },
				};
				rpc.EpollMod(sd, ev);
			}else{
				std::cerr << "error accepting TLS" << std::endl;
				return true;
			}
		}else{
			struct epoll_event ev = {
				.events = EPOLLIN | EPOLLRDHUP,
				.data = { .ptr = &*this, },
			};
			accepting = false;
			rpc.EpollMod(sd, ev);
		}
		return false;
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
SSL* ssl;
bool accepting;
};

// Relies on constructor to purge any added elements if constructor is thrown
void RPCService::OpenListeners() {
	auto lsd = std::make_unique<PolledListenFD>(AF_INET);
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
    throw NetworkException("couldn't set up listener queues");
  }
  struct epoll_event ev = {
    .events = EPOLLIN,
    .data = { .ptr = lsd.get(), },
  };
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)){
    throw NetworkException("couldn't epoll on sd4");
  }
  epolls.emplace(fd, std::move(lsd));
  lsd = std::make_unique<PolledListenFD>(AF_INET6);
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
    throw NetworkException("couldn't set up listener queues");
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

// FIXME need arrange this all as non-blocking
int RPCService::Accept(int sd) {
	struct sockaddr ss;
	socklen_t slen = sizeof(ss);
	int ret = accept4(sd, &ss, &slen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if(ret < 0){
		throw NetworkException(std::string("error accept()ing socket: ") + strerror(errno));
	}
  std::unique_ptr<PolledTLSFD> sfd;
	try{
		char paddr[INET6_ADDRSTRLEN];
		char pport[6];
		if(getnameinfo(&ss, slen, paddr, sizeof(paddr), pport, sizeof(pport),
				NI_NUMERICHOST | NI_NUMERICSERV)){
			throw NetworkException("error naming socket");
		}
		std::cout << "accepted on " << sd << " from " << paddr << ":" << pport << std::endl;
		sfd = std::make_unique<PolledTLSFD>(ret, sslctx.NewSSL());
	}catch(...){
		close(ret);
		throw;
	}
  // FIXME rewrite all the following as a call to sfd->Callback
  auto ra = SSL_accept(sfd->HackSSL());
  if(1 != ra){
    auto oerr = SSL_get_error(sfd->HackSSL(), ra);
    if(oerr == SSL_ERROR_WANT_READ){
      // FIXME this isn't picking up a remote shutdown
      struct epoll_event ev = {
        .events = EPOLLIN | EPOLLRDHUP,
        .data = { .ptr = sfd.get(), },
      };
      if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
        throw NetworkException("couldn't epoll-r on new sd");
      }
    }else if(oerr == SSL_ERROR_WANT_WRITE){
      struct epoll_event ev = {
        .events = EPOLLOUT | EPOLLRDHUP,
        .data = { .ptr = sfd.get(), },
      };
      if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
        throw NetworkException("couldn't epoll-w on new sd");
      }
    }else{
      throw NetworkException("error accepting TLS");
    }
  }else{
    // FIXME this isn't picking up a remote shutdown
    // FIXME will be seen as accept()ing socket
    struct epoll_event ev = {
      .events = EPOLLIN | EPOLLRDHUP,
      .data = { .ptr = sfd.get(), },
    };
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
      throw NetworkException("couldn't epoll-r on new sd");
    }
  }
  epolls.emplace(ret, std::move(sfd));
  return ret;
}

// Crap that we have to go checking a locked strucutre each epoll(), especially
// when those epoll()s are on a period. FIXME kill this off, rigourize.
void RPCService::HandleCompletedConns() {
	const auto& conns = connqueue.get()->GetCompletedPeers();
	for(const auto& c : conns){
    try{
      BIO* b = c.second->get();
      if(c.first->Name() == rpcName){ // connected to ourselves
        c.first->Disconnect();
        BIO_free_all(b);
      }else{
        int fd = BIO_get_fd(b, NULL); // FIXME can fail
        auto mapins = epolls.emplace(fd, std::make_unique<PolledConnFD>(b, c.first));
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
      std::cerr << e.what() << " connecting to " << c.first->Address() << std::endl;
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
					if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL)){
						std::cerr << "error removing epoll on " << fd << std::endl;
					}
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

int RPCService::EpollMod(int fd, struct epoll_event& ev) {
	return epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

}
