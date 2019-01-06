#include <sstream>
#include <fstream>
#include <libcatena/utility.h>
#include <libcatena/rpc.h>

namespace Catena {

RPCService::RPCService(Chain& chain, int port, const std::string& chainfile) :
  port(port),
  ledger(chain) {
	if(port < 0 || port > 65535){
		throw NetworkException("invalid port " + std::to_string(port));
	}
	(void)chainfile; // FIXME do crap
}

void RPCService::AddPeers(const std::string& peerfile) {
	std::ifstream in(peerfile);
	if(!in.is_open()){
		throw std::ifstream::failure("couldn't open " + peerfile);
	}
	std::vector<Peer> ret;
	std::string line;
	while(std::getline(in, line)){
		if(line.length() == 0 || line[0] == '#'){
			continue;
		}
		ret.emplace_back(line, port);
	}
	if(!in.eof()){
		throw ConvertInputException("couldn't extract lines from file");
	}
	// FIXME filter out duplicates?
	peers.insert(peers.end(), ret.begin(), ret.end());
}

}
