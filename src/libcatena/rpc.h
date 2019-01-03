#ifndef CATENA_LIBCATENA_RPC
#define CATENA_LIBCATENA_RPC

#include <string>
#include <vector>
#include <stdexcept>
#include <libcatena/peer.h>

namespace Catena {

class NetworkException : public std::runtime_error {
public:
NetworkException() : std::runtime_error("network error"){}
NetworkException(const std::string& s) : std::runtime_error(s){}
};

class RPCService {
public:
RPCService() = delete;
RPCService(int port, const std::string& chainfile, const char* peerfile);
virtual ~RPCService() = default;

private:
unsigned port;
std::vector<Peer> peers;
};

}

#endif
