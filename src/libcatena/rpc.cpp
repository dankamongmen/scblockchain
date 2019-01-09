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

void RPCService::OpenListeners() {
	sd4 = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(sd4 < 0){
		throw NetworkException("couldn't get TCP/IPv4 socket");
	}
	int reuse = 1;
	if(setsockopt(sd4, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
		close(sd4);
		throw NetworkException("couldn't set IPv4 SO_REUSEADDR");
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd4, (const struct sockaddr*)&sin, sizeof(sin))){
		close(sd4);
		throw NetworkException("couldn't bind IPv4 listener");
	}
	sd6 = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(sd6 < 0){
		close(sd4);
		throw NetworkException("couldn't get TCP/IPv6 socket");
	}
	try{
		if(setsockopt(sd6, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
			throw NetworkException("couldn't set IPv4 SO_REUSEADDR");
		}
		struct sockaddr_in6 sin6;
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(port);
		memcpy(&sin6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
		int v6only = 1;
		if(setsockopt(sd6, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only))){
			throw NetworkException("no pure IPv6 listeners");
		}
		if(bind(sd6, (const struct sockaddr*)&sin6, sizeof(sin6))){
			throw NetworkException("couldn't bind IPv6 listener");
		}
		if(listen(sd4, SOMAXCONN) || listen(sd6, SOMAXCONN)){
			throw NetworkException("couldn't set up listener queues");
		}
	}catch(...){
		close(sd6);
		close(sd4);
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
			.data = { .fd = sd4, },
		};
		if(epoll_ctl(ret, EPOLL_CTL_ADD, sd4, &ev)){
			throw NetworkException("couldn't epoll on sd4");
		}
		ev.data.fd = sd6;
		if(epoll_ctl(ret, EPOLL_CTL_ADD, sd6, &ev)){
			throw NetworkException("couldn't epoll on sd6");
		}
	}catch(...){
		close(ret);
		throw;
	}
	return ret;
}

RPCService::RPCService(Chain& ledger, int port, const std::string& chainfile) :
  port(port),
  ledger(ledger),
  sslctx(SSLCtxRAII(SSL_CTX_new(TLS_method()))),
  cancelled(false) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
	}
	if(1 != SSL_CTX_set_min_proto_version(sslctx.get(), TLS1_3_VERSION)){
		throw NetworkException("couldn't force TLSv1.3+");
	}
	if(1 != SSL_CTX_use_certificate_chain_file(sslctx.get(), chainfile.c_str())){
		throw NetworkException("couldn't load certificate chain");
	}
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
		close(sd4);
		close(sd6);
		throw;
	}
}

// FIXME probably need rule of 5
RPCService::~RPCService() {
	cancelled.store(true);
	epoller.join();
	if(close(sd4)){
		std::cerr << "warning: error closing ipv4 listening socket\n";
	}
	if(close(sd6)){
		std::cerr << "warning: error closing ipv6 listening socket\n";
	}
	if(close(epollfd)){
		std::cerr << "warning: error closing epoll fd\n";
	}
}

// FIXME need arrange this all as non-blocking
int RPCService::Accept(int sd) {
	struct sockaddr ss;
	socklen_t slen = sizeof(ss);
	int ret = accept(sd, &ss, &slen);
	if(ret < 0){
		throw NetworkException("error accept()ing socket");
	}
	char paddr[INET6_ADDRSTRLEN];
	char pport[6];
	if(getnameinfo(&ss, slen, paddr, sizeof(paddr), pport, sizeof(pport),
			NI_NUMERICHOST | NI_NUMERICSERV)){
		throw NetworkException("error naming socket");
	}
	std::cout << "accepted on " << sd << " from " << paddr << ":" << pport << std::endl;
	// FIXME do TLS wrap
	return ret;
}

void RPCService::Epoller() {
	while(1) {
		if(cancelled.load()){
			return;
		}
		struct epoll_event ev; // FIXME accept multiple events
		auto eret = epoll_wait(epollfd, &ev, 1, 100); // FIXME kill timeout
		if(eret < 0 && errno != EINTR){
			throw NetworkException(std::string("epoll_wait() error: ") + strerror(errno));
		}
		if(eret){
			if(ev.data.fd == sd4){
				try{
					auto sd = Accept(sd4);
					close(sd); // FIXME
				}catch(NetworkException& e){
					std::cerr << "couldn't accept on " << sd4 << ": " << e.what() << std::endl;
				}
			}else if(ev.data.fd == sd6){
				try{
					auto sd = Accept(sd4);
					close(sd); // FIXME
				}catch(NetworkException& e){
					std::cerr << "couldn't accept on " << sd4 << ": " << e.what() << std::endl;
				}
			}else{
				continue; // FIXME handle new ones
			}
		}
	}
}

void RPCService::AddPeers(const std::string& peerfile) {
	std::ifstream in(peerfile);
	if(!in.is_open()){
		throw std::ifstream::failure("couldn't open " + peerfile);
	}
	std::vector<Peer> ret;
	std::string line;
	while(std::getline(in, line)){
		if(line.length() == 0 || line[0] == '#'){
			continue;
		}
		ret.emplace_back(line, port);
	}
	if(!in.eof()){
		throw ConvertInputException("couldn't extract lines from file");
	}
	for(auto const& r : ret){
		// FIXME filter out duplicates?
		peers.insert(peers.end(), r);
		peers.back().ConnectAsync();
	}
}

}
