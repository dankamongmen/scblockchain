#include <netdb.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <libcatena/utility.h>
#include <libcatena/rpc.h>

#include <iostream>

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

~PolledTLSFD() {
	SSL_free(ssl);
}

// FIXME get rid of this, have Accept() use Callback()
SSL* HackSSL() const {
	return ssl;
}

// FIXME it isn't always an accept! could be established...
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
		std::cout << "read us some crap " << r << "\n";
	}else{
		auto ra = SSL_get_error(ssl, r);
		if(ra == SSL_ERROR_WANT_WRITE){
			// FIXME else check for SSL_ERROR_WANT_WRITE
		}else if(ra != SSL_ERROR_WANT_READ){
			std::cout << "lost ssl connection with error " << ra << std::endl;
			return true;
		}
	}
	return false;
}

private:
SSL* ssl;
bool accepting;
};

void RPCService::OpenListeners() {
	lsd4 = new PolledListenFD(AF_INET);
	try{
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = INADDR_ANY;
		if(bind(lsd4->FD(), (const struct sockaddr*)&sin, sizeof(sin))){
			throw NetworkException("couldn't bind IPv4 listener");
		}
		lsd6 = new PolledListenFD(AF_INET6);
		try{
			struct sockaddr_in6 sin6;
			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			sin6.sin6_port = htons(port);
			memcpy(&sin6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
			int v6only = 1;
			if(setsockopt(lsd6->FD(), IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only))){
				throw NetworkException("no pure IPv6 listeners");
			}
			if(bind(lsd6->FD(), (const struct sockaddr*)&sin6, sizeof(sin6))){
				throw NetworkException("couldn't bind IPv6 listener");
			}
			if(listen(lsd4->FD(), SOMAXCONN) || listen(lsd6->FD(), SOMAXCONN)){
				throw NetworkException("couldn't set up listener queues");
			}
		}catch(...){
			delete lsd6;
			throw;
		}
	}catch(...){
		delete lsd4;
		throw;
	}
}

int RPCService::EpollListeners() {
	int ret = epoll_create1(EPOLL_CLOEXEC);
	if(ret < 0){
		throw NetworkException("couldn't get an epoll");
	}
	try{
		struct epoll_event ev = {
			.events = EPOLLIN,
			.data = { .ptr = lsd4, },
		};
		if(epoll_ctl(ret, EPOLL_CTL_ADD, lsd4->FD(), &ev)){
			throw NetworkException("couldn't epoll on sd4");
		}
		ev.data.ptr = lsd6;
		if(epoll_ctl(ret, EPOLL_CTL_ADD, lsd6->FD(), &ev)){
			throw NetworkException("couldn't epoll on sd6");
		}
	}catch(...){
		close(ret);
		throw;
	}
	return ret;
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

RPCService::RPCService(Chain& ledger, int port, const std::string& chainfile,
			const std::string& keyfile) :
  port(port),
  ledger(ledger),
  sslctx(SSLCtxRAII(SSL_CTX_new(TLS_method()))),
  cancelled(false),
  clictx(std::make_shared<SSLCtxRAII>(SSLCtxRAII(SSL_CTX_new(TLS_method())))),
  connqueue(std::make_shared<PeerQueue>()) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
	}
  if(port == 0){
    port = DefaultRPCPort;
  }
	PrepSSLCTX(sslctx.get(), chainfile.c_str(), keyfile.c_str());
	auto x509 = SSL_CTX_get0_certificate(sslctx.get()); // view, don't free
	rpcName = X509NetworkName(x509);
	PrepSSLCTX(clictx.get()->get(), chainfile.c_str(), keyfile.c_str());
	SSL_CTX_set_verify(sslctx.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, tls_cert_verify);
	OpenListeners();
	try{
		epollfd = EpollListeners();
		try{
			epoller = std::thread(&RPCService::Epoller, this);
		}catch(...){
			close(epollfd);
			throw;
		}
	}catch(...){
		delete lsd4;
		delete lsd6;
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
	delete lsd4;
	delete lsd6;
}

// FIXME need arrange this all as non-blocking
int RPCService::Accept(int sd) {
	struct sockaddr ss;
	socklen_t slen = sizeof(ss);
	int ret = accept4(sd, &ss, &slen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if(ret < 0){
		throw NetworkException("error accept()ing socket");
	}
	try{
		char paddr[INET6_ADDRSTRLEN];
		char pport[6];
		if(getnameinfo(&ss, slen, paddr, sizeof(paddr), pport, sizeof(pport),
				NI_NUMERICHOST | NI_NUMERICSERV)){
			throw NetworkException("error naming socket");
		}
		std::cout << "accepted on " << sd << " from " << paddr << ":" << pport << std::endl;
		// FIXME don't want SSLRAII, as we hand this off, but also don't
		// want to leak SSL* on exceptions...also, we need this cleaned
		// up on RPCService destroy even if we never see another event...
		auto sfd = new PolledTLSFD(ret, sslctx.NewSSL());
		// FIXME rewrite all the following as a call to sfd->Callback
		auto ra = SSL_accept(sfd->HackSSL());
		if(1 != ra){
			auto oerr = SSL_get_error(sfd->HackSSL(), ra);
			if(oerr == SSL_ERROR_WANT_READ){
				// FIXME this isn't picking up a remote shutdown
				struct epoll_event ev = {
					.events = EPOLLIN | EPOLLRDHUP,
					.data = { .ptr = sfd, },
				};
				if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
					delete sfd;
					throw NetworkException("couldn't epoll-r on new sd");
				}
			}else if(oerr == SSL_ERROR_WANT_WRITE){
				struct epoll_event ev = {
					.events = EPOLLOUT | EPOLLRDHUP,
					.data = { .ptr = sfd, },
				};
				if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
					delete sfd;
					throw NetworkException("couldn't epoll-w on new sd");
				}
			}else{
				delete sfd;
				throw NetworkException("error accepting TLS");
			}
		}else{
			// FIXME this isn't picking up a remote shutdown
			// FIXME will be seen as accept()ing socket
			struct epoll_event ev = {
				.events = EPOLLIN | EPOLLRDHUP,
				.data = { .ptr = sfd, },
			};
			if(epoll_ctl(epollfd, EPOLL_CTL_ADD, ret, &ev)){
				delete sfd;
				throw NetworkException("couldn't epoll-r on new sd");
			}
		}
		return ret;
	}catch(...){
		close(ret);
		throw;
	}
}

// Crap that we have to go checking a locked strucutre each epoll(), especially
// when those epoll()s are on a period. FIXME kill this off, rigourize.
void RPCService::HandleCompletedConns() {
	auto conns = connqueue.get()->Peers();
	for(auto& c : conns){
		if(c.first->Name() == rpcName){
			BIO_free_all(c.second);
			// FIXME update peer with zee infos
		}else{
			// FIXME add sd from BIO to epoll set
			active.emplace_back(c.first);
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
		struct epoll_event ev; // FIXME accept multiple events
		auto eret = epoll_wait(epollfd, &ev, 1, 100); // FIXME kill timeout
		if(eret < 0 && errno != EINTR){
			throw NetworkException(std::string("epoll_wait() error: ") + strerror(errno));
		}
		if(eret){
			PolledFD* pfd = static_cast<PolledFD*>(ev.data.ptr);
			try{
				if(pfd->Callback(*this)){
					if(epoll_ctl(epollfd, EPOLL_CTL_DEL, pfd->FD(), NULL)){
						std::cerr << "error removing epoll on " << pfd->FD() << std::endl;
					}
					delete pfd;
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
			peers.emplace_back(r);
			Peer::ConnectAsync(r, connqueue);
		}
	}
}

int RPCService::EpollMod(int fd, struct epoll_event& ev) {
	return epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

}
