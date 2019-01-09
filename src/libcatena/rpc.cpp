#include <sstream>
#include <fstream>
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
	sd6 = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if(sd6 < 0){
		throw NetworkException("couldn't get TCP/IPv6 socket");
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
	close(sd4); // FIXME should only be done after Epoller is stopped?
	close(sd6);
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
