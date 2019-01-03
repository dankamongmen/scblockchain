#include <sstream>
#include <fstream>
#include <libcatena/rpc.h>

namespace Catena {

RPCService::RPCService(int port, const std::string& chainfile) :
  port(port) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
	}
	(void)chainfile; // FIXME do crap
}

void RPCService::AddPeers(const std::string& peerfile) {
	std::ifstream in(peerfile); // FIXME zee error cheques?
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	std::vector<Peer> ret;
	std::string line;
	while(std::getline(in, line)){
		if(line.length() == 0 || line[0] == '#'){
			continue;
		}
		ret.emplace_back(line, port);
	}
	// FIXME more error cheques
	// FIXME add ret to peer set
}

}
