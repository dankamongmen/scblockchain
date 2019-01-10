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
  lasttime(time(NULL)),
  configured(configured) {
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

static std::string X509CN(X509_NAME* xname) {
	int lastpos = -1;
	lastpos = X509_NAME_get_index_by_NID(xname, NID_commonName, lastpos);
	if(lastpos == -1){
		throw NetworkException("no subject common name in cert");
	}
	X509_NAME_ENTRY* e = X509_NAME_get_entry(xname, lastpos);
	auto asn1 = X509_NAME_ENTRY_get_data(e);
	auto str = ASN1_STRING_get0_data(asn1);
	std::string ret(reinterpret_cast<const char*>(str));
	return ret;
}

static std::string X509SubjectCN(const X509* cert) {
	auto xname = X509_get_subject_name(cert);
	return X509CN(xname);
}

static std::string X509IssuerCN(const X509* cert) {
	auto xname = X509_get_issuer_name(cert);
	return X509CN(xname);
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
		return nullptr; // FIXME throw
	}
	if(X509_V_OK != SSL_get_verify_result(s)){
		std::cout << "ssl verify failure" << std::endl;
		BIO_free_all(ret);
		return nullptr; // FIXME throw
	}
	auto x509 = SSL_get_peer_certificate(s);
	if(x509 == nullptr){
		std::cout << "error getting peer cert" << std::endl;
		BIO_free_all(ret);
		return nullptr; // FIXME throw
	}
	try{
		auto subcn = X509SubjectCN(x509);
		auto isscn = X509IssuerCN(x509);
		lastSubjectCN = subcn;
		lastIssuerCN = isscn;
	}catch(...){
		X509_free(x509);
		BIO_free_all(ret);
		throw;
	}
	X509_free(x509);
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
