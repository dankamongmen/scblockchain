#ifndef CATENA_LIBCATENA_TLS
#define CATENA_LIBCATENA_TLS

#include <openssl/ssl.h>
#include <libcatena/exceptions.h>

namespace Catena {

// A TLSName, (hopefully) unique within the network, is the Issuer CN plus the
// Subject CN of the node's certificate.
using TLSName = std::pair<std::string, std::string>;

inline std::ostream& StrTLSName(std::ostream& stream, const TLSName& name){
	stream << name.first << "→" << name.second;
	return stream;
}

// RAII wrappers for OpenSSL objects e.g. SSL_CTX, SSL
class SSLRAII {
public:

SSLRAII() = delete;

// Usually called ala SSLRAII(SSL_new(sslctx))
SSLRAII(SSL* ssl) :
  ssl(ssl) {
	if(ssl == nullptr){
		throw NetworkException("couldn't get TLS conn");
	}
}

SSLRAII(const SSLRAII& s) {
	SSL_up_ref(s.ssl);
	ssl = s.ssl;
}

SSLRAII& operator=(const SSLRAII& other) {
	if(this != &other){
		SSL_free(ssl);
		if( (ssl = other.ssl) ){
			SSL_up_ref(ssl);
		}
	}
	return *this;
}

SSL* get() {
	return ssl;
}

void SetFD(int sd) {
	if(1 != SSL_set_fd(ssl, sd)){
		throw NetworkException("couldn't bind SSL sd");
	}
}

~SSLRAII() {
	SSL_free(ssl);
}

private:
SSL* ssl;
};

class SSLCtxRAII {
public:

SSLCtxRAII() = delete;

// Usually called ala SSLCtxRAII(SSL_CTX_new(TLS_method()))
SSLCtxRAII(SSL_CTX* ctx) :
  sslctx(ctx) {
	if(sslctx == nullptr){
		throw NetworkException("couldn't get TLS context");
	}
}

SSLCtxRAII(const SSLCtxRAII& ctx) {
	SSL_CTX_up_ref(ctx.sslctx);
	sslctx = ctx.sslctx;
}

SSLCtxRAII& operator=(const SSLCtxRAII& other) {
	if(this != &other){
		SSL_CTX_free(sslctx);
		if( (sslctx = other.sslctx) ){
			SSL_CTX_up_ref(sslctx);
		}
	}
	return *this;
}

SSL_CTX* get() {
	return sslctx;
}

~SSLCtxRAII() {
	SSL_CTX_free(sslctx);
}

SSL* NewSSL() const {
	SSL* ret = SSL_new(sslctx);
	if(ret == nullptr){
		throw NetworkException("couldn't get TLS context");
	}
	return ret;
}

SSLRAII NewSSLRAII() const {
	return SSLRAII(SSL_new(sslctx));
}

private:
SSL_CTX* sslctx;
};

}

#endif
