#include <libcatena/rpc.h>

namespace Catena {

RPCService::RPCService(int port, const std::string& chainfile, const char* peerfile) :
  port(port) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
	}
	// FIXME do crap
	(void)chainfile;
	(void)peerfile;
}

}
