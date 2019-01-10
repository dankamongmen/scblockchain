#ifndef CATENA_CATENA_HTTPD
#define CATENA_CATENA_HTTPD

#include <microhttpd.h>
#include <libcatena/chain.h>
#include <nlohmann/json_fwd.hpp>

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

nlohmann::json InspectJSON(int start, int end) const;

std::ostream& HTMLSysinfo(std::ostream& ss) const;
std::ostream& HTMLChaininfo(std::ostream& ss) const;
std::ostream& HTMLMembers(std::ostream& ss) const;
std::ostream& BlockHTML(std::ostream& ss, const Catena::CatenaHash& hash,
				bool printbytes) const;
std::ostream& JSONtoHTML(std::ostream& ss, const nlohmann::json& json) const;

template <typename Iterator> std::ostream&
TXListHTML(std::ostream& ss, const Iterator begin, const Iterator end) const;

// GET handlers
struct MHD_Response* Summary(struct MHD_Connection*) const;
struct MHD_Response* Favicon(struct MHD_Connection*) const;
struct MHD_Response* Show(struct MHD_Connection*) const;
struct MHD_Response* TStore(struct MHD_Connection*) const;
struct MHD_Response* Inspect(struct MHD_Connection*) const;
struct MHD_Response* UstatusHTML(struct MHD_Connection* conn) const;
struct MHD_Response* UstatusJSON(struct MHD_Connection* conn) const;
struct MHD_Response* ShowMemberHTML(struct MHD_Connection* conn) const;
struct MHD_Response* ShowBlockHTML(struct MHD_Connection* conn) const;

static int Handler(void* cls, struct MHD_Connection* conn, const char* url,
	const char* method, const char* version, const char* upload_data,
	size_t* upload_len, void** conn_cls);

// POST handlers
int ExternalLookupTXReq(struct PostState* ps, const char* upload) const;
int LookupAuthReqTXReq(struct PostState* ps, const char* upload) const;
int LookupAuthTXReq(struct PostState* ps, const char* upload) const;
int UserTXReq(struct PostState* ps, const char* upload) const;
int UserDelegationTXReq(struct PostState* ps, const char* upload) const;
int UserStatusTXReq(struct PostState* ps, const char* upload) const;
int MemberTXReq(struct PostState* ps, const char* upload) const;

int HandlePost(struct MHD_Connection* conn, const char* url,
		const char* upload_data, size_t* upload_len,
		void** conn_cls);

};

}

#endif
