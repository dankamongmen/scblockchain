#include <cstring>
#include <sstream>
#include <fstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <libcatena/utility.h>
#include <libcatena/rpc.h>

#include <iostream>
#include <unistd.h>

namespace Catena {

void RPCService::OpenListeners() {
	sd4 = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(sd4 < 0){
		throw NetworkException("couldn't get TCP/IPv4 socket");
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
	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = htons(port);
	memcpy(&sin6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
	int v6only = 1;
	if(setsockopt(sd6, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only))){
		close(sd6);
		close(sd4);
		throw NetworkException("no pure IPv6 listeners");
	}
	if(bind(sd6, (const struct sockaddr*)&sin6, sizeof(sin6))){
		close(sd6);
		close(sd4);
		throw NetworkException("couldn't bind IPv6 listener");
	}
	if(listen(sd4, SOMAXCONN) || listen(sd6, SOMAXCONN)){
		close(sd6);
		close(sd4);
		throw NetworkException("couldn't set up listener queues");
	}
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
	epoller = std::thread(&RPCService::Epoller, this);
}

// FIXME probably need rule of 5
RPCService::~RPCService() {
	cancelled = true; // FIXME needs be at least an atomic
	epoller.join();
	if(close(sd4)){
		std::cerr << "warning: error closing ipv4 listening socket\n";
	}
	if(close(sd6)){
		std::cerr << "warning: error closing ipv6 listening socket\n";
	}
}

void RPCService::Epoller() {
	while(1) {
		// FIXME
		if(cancelled){
			std::cout << "not epollin' no mo'\n";
			return;
		}
		std::cout << "epollin'\n";
		sleep(1);
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
