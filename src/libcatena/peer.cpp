#include <libcatena/utility.h>
#include <libcatena/peer.h>

namespace Catena {

Peer::Peer(const std::string& addr, int defaultport) { 
	// If there's a colon, the remainder must be a valid port. If there is
	// no colon, assume the entirety to be the address.
	auto colon = addr.find(':');
	if(colon == std::string::npos){
		port = defaultport;
		if(port < 0 || port > 65535){
			throw ConvertInputException("bad port: " + std::to_string(port));
		}
		address = addr;
	}else{
		address = std::string(addr, colon);
		port = StrToLong(addr.substr(colon + 1, addr.length() - colon),
					0, 65535);
	}
}

}
