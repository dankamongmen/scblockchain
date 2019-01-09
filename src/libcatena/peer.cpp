#include <cctype>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libcatena/utility.h>
#include <libcatena/peer.h>
#include <libcatena/rpc.h>
#include <libcatena/dns.h>

#include <iostream>

namespace Catena {

Peer::Peer(const std::string& addr, int defaultport, std::shared_ptr<SSLCtxRAII> sctx) :
  sslctx(sctx),
  lasttime(time(NULL)) {
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
		port = StrToLong(addr.substr(colon + 1, addr.length() - colon),
					0, 65535);
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

static int
tls_cert_verify(int preverify_ok, X509_STORE_CTX* x509_ctx){
	if(!preverify_ok){
		int e = X509_STORE_CTX_get_error(x509_ctx);
		std::cerr << "cert verification error: " << X509_verify_cert_error_string(e) << std::endl;
	}
	X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
	if(cert){
		X509_NAME* certsub = X509_get_subject_name(cert);
		char *cn = X509_NAME_oneline(certsub, nullptr, 0);
		if(cn){
			std::cout << "server cert: " << cn << std::endl;
			free(cn);
		}
	}
	return preverify_ok;
}

BIO* Peer::TLSConnect(int sd) {
	auto ret = BIO_new_ssl_connect(sslctx.get()->get());
	if(ret == nullptr){
		throw NetworkException("coudln't get TLS connect BIO");
	}
	SSL* s;
	BIO_get_ssl(ret, &s);
	SSL_set_fd(s, sd);
	SSL_set_verify(s, SSL_VERIFY_PEER, tls_cert_verify);
	if(1 != SSL_connect(s)){
		std::cout << "ssl connect failure" << std::endl;
		BIO_free_all(ret);
		return nullptr;
	}
	if(X509_V_OK != SSL_get_verify_result(s)){
		std::cout << "ssl verify failure" << std::endl;
		BIO_free_all(ret);
		return nullptr;
	}
	return ret;
}

int Peer::Connect() {
	lasttime = time(NULL);
	struct addrinfo hints{};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	AddrInfo ai(address.c_str(), NULL, &hints); // blocking call, can throw
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
			std::cerr << "connect failed on sd " << fd << "\n";
			close(fd);
			continue;
		}
		std::cout << "connected " << fd << " to " << address << "\n";
		lasttime = time(NULL);
		try{
			TLSConnect(fd);
		}catch(...){
			close(fd);
			throw;
		}
		// FIXME add SOCK_NONBLOCK
		return fd;
	}while( (info = info->ai_next) );
	lasttime = time(NULL);
	throw NetworkException("couldn't connect to " + address);
}

}
