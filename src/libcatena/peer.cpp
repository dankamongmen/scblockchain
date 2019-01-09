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

Peer::Peer(const std::string& addr, int defaultport) :
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

int Peer::Connect() {
	lasttime = time(NULL);
	struct addrinfo hints{};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	AddrInfo ai(address.c_str(), NULL, &hints); // blocking call, can throw
	const struct addrinfo* info = ai.AddrList();
	do{
		// only set SOCK_NONBLOCK after we've connect()ed
		int fd = socket(info->ai_family, SOCK_STREAM | SOCK_CLOEXEC, info->ai_protocol);
std::cout << "socket family " << info->ai_family << " sd " << fd << "\n";
		if(fd < 0){
			continue;
		}
		if(connect(fd, info->ai_addr, info->ai_addrlen)){ // blocks
std::cerr << "connect failed on sd " << fd << "\n";
			close(fd);
			continue;
		}
		// FIXME overlay TLS
		// FIXME add SOCK_NONBLOCK
		lasttime = time(NULL);
		return fd;
	}while( (info = info->ai_next) );
	lasttime = time(NULL);
	throw NetworkException("couldn't connect to " + address);
}

}
