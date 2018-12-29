#ifndef CATENA_CATENA_HTTPD
#define CATENA_CATENA_HTTPD

#include <microhttpd.h>
#include <nlohmann/json.hpp>
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

HTTPDServer(const HTTPDServer&) = delete;
HTTPDServer& operator=(HTTPDServer const&) = delete;
virtual ~HTTPDServer();

private:
MHD_Daemon* mhd; // has no free function
Catena::Chain& chain;

struct MHD_Response* Summary(struct MHD_Connection*) const;
struct MHD_Response* Show(struct MHD_Connection*) const;
struct MHD_Response* TStore(struct MHD_Connection*) const;
struct MHD_Response* Inspect(struct MHD_Connection*) const;
nlohmann::json InspectJSON(int start, int end) const;
std::stringstream& HTMLSysinfo(std::stringstream& ss) const;

static int Handler(void* cls, struct MHD_Connection* conn, const char* url,
	const char* method, const char* version, const char* upload_data,
	size_t* upload_len, void** conn_cls);

// POST handlers
static int ExternalLookupReq(void* coninfo_cls, enum MHD_ValueKind kind,
		const char* key, const char *filename,
                const char* content_type, const char* transfer_encoding,
                const char* value, uint64_t off, size_t size);

int HandlePost(struct MHD_Connection* conn, const char* url,
		const char* upload_data, size_t* upload_len,
		void** conn_cls);

};

}

#endif
