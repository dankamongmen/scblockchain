#ifndef CATENA_LIBCATENA_TLS
#define CATENA_LIBCATENA_TLS

#include <openssl/ssl.h>

// RAII wrappers for OpenSSL objects e.g. SSL_CTX

namespace Catena {

class NetworkException : public std::runtime_error {
public:
NetworkException() : std::runtime_error("network error"){}
NetworkException(const std::string& s) : std::runtime_error(s){}
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

private:
SSL_CTX* sslctx;
};

}

#endif
