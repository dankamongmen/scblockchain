#ifndef CATENA_LIBCATENA_DNS
#define CATENA_LIBCATENA_DNS

// RAII wrapper for getaddrinfo() results
#include <netdb.h>
#include <sys/types.h>
#include <libcatena/rpc.h>

namespace Catena {

class AddrInfo {
public:
AddrInfo(const char* node, const char* service, const struct addrinfo* hints) {
	struct addrinfo* res;
	int e;
	if( (e = getaddrinfo(node, service, hints, &res)) ){
		throw NetworkException(gai_strerror(e));
	}
	info = res;
}

~AddrInfo() {
	freeaddrinfo(info);
}

AddrInfo(const AddrInfo&) = delete;

AddrInfo& operator=(const AddrInfo&) = delete;

const struct addrinfo* AddrList() const {
	return info;
}

private:
struct addrinfo *info; // linked list of results
};

}

#endif
