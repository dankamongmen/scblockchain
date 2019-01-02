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

nlohmann::json InspectJSON(int start, int end) const;

std::stringstream& HTMLSysinfo(std::stringstream& ss) const;
std::stringstream& HTMLChaininfo(std::stringstream& ss) const;
std::stringstream& HTMLMembers(std::stringstream& ss) const;
std::stringstream& JSONtoHTML(std::stringstream& ss, const nlohmann::json& json) const;

// GET handlers
struct MHD_Response* Summary(struct MHD_Connection*) const;
struct MHD_Response* Favicon(struct MHD_Connection*) const;
struct MHD_Response* Show(struct MHD_Connection*) const;
struct MHD_Response* TStore(struct MHD_Connection*) const;
struct MHD_Response* Inspect(struct MHD_Connection*) const;
struct MHD_Response* PstatusHTML(struct MHD_Connection* conn) const;
struct MHD_Response* PstatusJSON(struct MHD_Connection* conn) const;
struct MHD_Response* ShowMemberHTML(struct MHD_Connection* conn) const;

static int Handler(void* cls, struct MHD_Connection* conn, const char* url,
	const char* method, const char* version, const char* upload_data,
	size_t* upload_len, void** conn_cls);

// POST handlers
int ExternalLookupTXReq(struct PostState* ps, const char* upload) const;
int LookupAuthReqTXReq(struct PostState* ps, const char* upload) const;
int LookupAuthTXReq(struct PostState* ps, const char* upload) const;
int PatientTXReq(struct PostState* ps, const char* upload) const;
int PatientDelegationTXReq(struct PostState* ps, const char* upload) const;
int PatientStatusTXReq(struct PostState* ps, const char* upload) const;
int MemberTXReq(struct PostState* ps, const char* upload) const;

int HandlePost(struct MHD_Connection* conn, const char* url,
		const char* upload_data, size_t* upload_len,
		void** conn_cls);

};

}

#endif
