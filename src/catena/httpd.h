#ifndef CATENA_CATENA_HTTPD
#define CATENA_CATENA_HTTPD

#include <microhttpd.h>
#include <libcatena/chain.h>

namespace CatenaAgent {

class HTTPException : public std::runtime_error {
public:
HTTPException() : std::runtime_error("error providing HTTP service"){}
};

class HTTPDServer {
public:
HTTPDServer() = delete;
HTTPDServer(Catena::Chain& chain, unsigned port);

virtual ~HTTPDServer() = default;

private:
MHD_Daemon* mhd; // has no free function
Catena::Chain& chain;

static int Handler(void* cls, struct MHD_Connection* conn, const char* url,
	const char* method, const char* version, const char* upload_data,
	size_t* upload_len, void** conn_cls);

};

}

#endif
