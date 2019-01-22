#include <cctype>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <libcatena/utility.h>
#include <libcatena/peer.h>
#include <libcatena/rpc.h>
#include <libcatena/dns.h>

namespace Catena {

Peer::Peer(const std::string& addr, int defaultport, std::shared_ptr<SSLCtxRAII> sctx,
		bool configured) :
  sslctx(sctx),
  lasttime(-1), // trigger a connect immediately
  configured(configured),
  connected(false) {
	// If there's a colon, the remainder must be a valid port. If there is
	// no colon, assume the entirety to be the address.
	auto colon = addr.find(':');
	if(colon == std::string::npos){
		port = defaultport;
		if(port < 0 || port > 65535){
			throw ConvertInputException("bad port: " + std::to_string(port));
		}
		address = addr;
	}else if(colon == 0){ // can't start with colon
		throw ConvertInputException("bad address: " + addr);
	}else{
		address = std::string(addr, 0, colon);
		// StrToLong() will reject any trailing crap, but admits
		// leading whitespace / sign. enforce a number
		if(!isdigit(addr[colon + 1])){
			throw ConvertInputException("bad port: " + addr);
		}
		port = StrToLong(addr.substr(colon + 1, addr.length() - colon), 0, 65535);
	}
	// getaddrinfo() will happily process a name with trailing whitespace
	// (and even crap after said whitespace); purge
	if(address.find_first_of(" \n\t\v\f") != std::string::npos){
		throw ConvertInputException("bad address: " + address);
	}
	struct addrinfo hints{};
	struct addrinfo *res;
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(address.c_str(), NULL, &hints, &res)){
		throw ConvertInputException("bad address: " + address);
	}
	freeaddrinfo(res);
}

ConnFuture Peer::TLSConnect(int sd) {
	auto bio = BIO_new_ssl_connect(sslctx.get()->get());
	if(bio == nullptr){
		throw NetworkException("coudln't get TLS connect BIO");
	}
	SSL* s;
	BIO_get_ssl(bio, &s);
	SSL_set_fd(s, sd);
	SSL_set_verify(s, SSL_VERIFY_PEER, tls_cert_verify);
	if(1 != SSL_connect(s)){
		BIO_free_all(bio);
    throw NetworkException("ssl connect failure");
	}
	if(X509_V_OK != SSL_get_verify_result(s)){
		BIO_free_all(bio);
    throw NetworkException("ssl verify failure");
	}
  ConnFuture cf;
	auto x509 = SSL_get_peer_certificate(s);
	if(x509 == nullptr){
		BIO_free_all(bio);
    throw NetworkException("error getting peer cert");
	}
	try{
		cf.name = X509NetworkName(x509);
	}catch(...){
		X509_free(x509);
		BIO_free_all(bio);
		throw;
	}
	X509_free(x509);
  cf.bio = bio;
	return cf;
}

ConnFuture Peer::Connect() {
	lasttime = time(nullptr);
	struct addrinfo hints{};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	AddrInfo ai(address.c_str(), nullptr, &hints); // blocking call, can throw
	const struct addrinfo* info = ai.AddrList();
	do{
		if(info->ai_family == AF_INET){
			((struct sockaddr_in*)info->ai_addr)->sin_port = htons(port);
		}else if(info->ai_family == AF_INET6){
			((struct sockaddr_in6*)info->ai_addr)->sin6_port = htons(port);
		}else{
			continue;
		}
		// only set SOCK_NONBLOCK after we've connect()ed
		int fd = socket(info->ai_family, SOCK_STREAM | SOCK_CLOEXEC, info->ai_protocol);
		if(fd < 0){
			continue;
		}
		if(connect(fd, info->ai_addr, info->ai_addrlen)){ // blocks
			close(fd);
			lasttime = time(NULL);
			continue;
		}
		std::cout << "connected " << fd << " to " << address << ":" << port << std::endl;
		lasttime = time(NULL);
		try{
			auto ret = TLSConnect(fd); // FIXME RAII that fucker
			FDSetNonblocking(fd);
      MarkConnected();
			return ret;
		}catch(...){
			close(fd);
			throw;
		}
	}while( (info = info->ai_next) );
	lasttime = time(NULL);
	throw NetworkException("no one listening");
}

}
