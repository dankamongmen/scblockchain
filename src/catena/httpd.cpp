#include <sstream>
#include <iostream>
#include <unistd.h>
#include <microhttpd.h>
#include <libcatena/utility.h>
#include <libcatena/chain.h>
#include <libcatena/hash.h>
#include "catena/favicon.h"
#include "catena/version.h"
#include "catena/httpd.h"

namespace CatenaAgent {

HTTPDServer::~HTTPDServer(){
	if(mhd){
		MHD_stop_daemon(mhd);
	}
}

constexpr char htmlhdr[] =
 "<!DOCTYPE html>"
 "<html lang=\"en\"><link rel=\"stylesheet\" type=\"text/css\" href=\"//fonts.googleapis.com/css?family=Open+Sans\" />"
 "<head>"
 "<meta name=\"google\" content=\"notranslate\">"
 "<title>catena node</title>"
 "<style>"
 "body { font-family: \"Open Sans\"; margin: 5px ; padding: 5px; background: whitesmoke; }"
 "table { table-layout: fixed; border-collapse; collapse: border-spacing: 0; width: 90%; }"
 "td { width: 10%; background: #dfdfdf; border: 1px solid transparent; transition: all 0.3s; }"
 "td+td { width: auto; }"
 "tr:nth-child(even) td { background: #f1f1f1; }"
 "tr:nth-child(odd) td { background: #fefefe; }"
 "tr td:hover { background: #00dddd; }"
 "</style>"
 "</head>";

std::stringstream& HTTPDServer::HTMLSysinfo(std::stringstream& ss) const {
	ss << "<h3>system</h3><table>";
	ss << "<tr><td>cxx</td><td>" << Catena::GetCompilerID() << "</td></tr>";
	ss << "<tr><td>libc</td><td>" << Catena::GetLibcID() << "</td></tr>";
	ss << "<tr><td>json</td><td>JSON for Modern C++ " <<
		NLOHMANN_JSON_VERSION_MAJOR << "." <<
		NLOHMANN_JSON_VERSION_MINOR << "." <<
		NLOHMANN_JSON_VERSION_PATCH << "</td>";
	ss << "<tr><td>crypto</td><td>" <<
		SSLeay_version(SSLEAY_VERSION) << "</td></tr>";
	ss << "</table>";
	return ss;
}

std::stringstream& HTTPDServer::HTMLChaininfo(std::stringstream& ss) const {
	ss << "<h3>chain</h3><table>";
	ss << "<tr><td>private key</td><td>";
	try{
		auto kl = chain.PrivateKeyTXSpec();
		ss << Catena::hashOString(kl.first) << "." << kl.second << "</td></tr>";
	}catch(Catena::SigningException& e){
		ss << "n/a</td></tr>";
	}
	ss << "<tr><td>chain bytes</td><td>" << chain.Size() << "</td></tr>";
	ss << "<tr><td>blocks</td><td>" << chain.GetBlockCount() << "</td></tr>";
	ss << "<tr><td>transactions</td><td>" << chain.TXCount() << "</td></tr>";
	ss << "<tr><td>outstanding TXs</td><td>" << chain.OutstandingTXCount() << "</td></tr>";
	ss << "<tr><td>lookup requests</td><td>" << chain.LookupRequestCount() << "</td></tr>";
	ss << "<tr><td>lookup authorizations</td><td>" << chain.LookupRequestCount(true) << "</td></tr>";
	ss << "<tr><td>external IDs</td><td>" << chain.ExternalLookupCount() << "</td></tr>";
	char timebuf[80];
	auto lastutc = chain.MostRecentBlock();
	if(lastutc == -1){
		strcpy(timebuf, "n/a");
	}else{
		ctime_r(&lastutc, timebuf);
	}
	ss << "<tr><td>last block time</td><td>" << timebuf << "</td></tr>";
	ss << "<tr><td>public keys</td><td>" << chain.PubkeyCount() << "</td></tr>";
	ss << "<tr><td>status delegations</td><td>" << chain.StatusDelegationCount() << "</td></tr>";
	ss << "</table>";
	return ss;
}

struct MHD_Response*
HTTPDServer::Favicon(struct MHD_Connection* conn __attribute__ ((unused))) const {
	auto resp = MHD_create_response_from_buffer(sizeof(FaviconBMP),
			(char*)FaviconBMP, MHD_RESPMEM_PERSISTENT);
        if(MHD_NO == MHD_add_response_header(resp, "Content-Type", "image/vnd.microsoft.icon")){
		MHD_destroy_response(resp);
		return nullptr;
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::Summary(struct MHD_Connection* conn __attribute__ ((unused))) const {
	char hname[128];
	gethostname(hname, sizeof(hname));
	hname[sizeof(hname) - 1] = '\0'; // gethostname() doesn't truncate
	std::stringstream ss;
	ss << htmlhdr;
	ss << "<body><h2>catena v" << VERSION << " on " << hname << "</h2>";
	HTMLSysinfo(ss);
	HTMLChaininfo(ss);
	ss << "</body>";
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::Show(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	ss << chain;
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct MHD_Response*
HTTPDServer::TStore(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	chain.DumpTrustStore(ss);
	std::string s = ss.str();
	auto resp = MHD_create_response_from_buffer(s.size(),
			const_cast<char*>(s.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

nlohmann::json HTTPDServer::InspectJSON(int start, int end) const {
	auto blks = chain.Inspect(start, end);
	std::vector<nlohmann::json> jblks;
	for(const auto& b : blks){
		std::vector<nlohmann::json> jtxs;
		for(const auto& tx : b.transactions){
			jtxs.emplace_back(tx.get()->JSONify());
		}
		nlohmann::json jblk;
		jblk["transactions"] = jtxs;
		jblk["version"] = b.bhdr.version;
		jblk["utc"] = b.bhdr.utc;
		jblk["bytes"] = b.bhdr.totlen;
		jblk["hash"] = Catena::hashOString(b.bhdr.hash);
		jblk["prev"] = Catena::hashOString(b.bhdr.prev);
		nlohmann::json(b.bhdr.totlen);
		jblks.emplace_back(jblk);
	}
	nlohmann::json ret(jblks);
	return ret;
}

struct MHD_Response*
HTTPDServer::Pstatus(struct MHD_Connection* conn) const {
	(void)conn;
	return nullptr; // FIXME
}

struct MHD_Response*
HTTPDServer::Inspect(struct MHD_Connection* conn) const {
	auto sstart = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "begin");
	auto sstop = MHD_lookup_connection_value(conn, MHD_GET_ARGUMENT_KIND, "end");
	long start = 0;
	if(sstart){
		char* e = const_cast<char*>(sstart);
		start = strtol(sstart, &e, 10);
		if(start < 0 || start == LONG_MAX || *e){
			return nullptr; // FIXME return 400 with explanation
		}
	}
	long end = -1;
	if(sstop){
		char* e = const_cast<char*>(sstart);
		end = strtol(sstop, &e, 10);
		if(end < -1 || end == LONG_MAX || *e){ // allow explicit -1
			return nullptr;
		}
	}
	auto res = InspectJSON(start, end).dump();
	auto resp = MHD_create_response_from_buffer(res.size(), const_cast<char*>(res.c_str()), MHD_RESPMEM_MUST_COPY);
	if(resp){
		if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json")){
			MHD_destroy_response(resp);
			return nullptr;
		}
	}
	return resp;
}

struct PostState {
	std::string response;
};

int HTTPDServer::ExternalLookupTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)uplen;
	nlohmann::json json;
	try{
		json = nlohmann::json::parse(upload);
		auto pubkey = json.find("pubkey");
		auto lookuptype = json.find("lookuptype");
		auto payload = json.find("payload");
		if(pubkey == json.end() || lookuptype == json.end() || payload == json.end()){
			std::cerr << "missing necessary elements from ExternalLookupTXRequest" << std::endl;
			return MHD_NO;
		}
		if(!(*pubkey).is_string()){
			std::cerr << "pubkey was not a string" << std::endl;
			return MHD_NO;
		}
		if(!(*lookuptype).is_number()){
			std::cerr << "lookuptype was not a number" << std::endl;
			return MHD_NO;
		}
		if(!(*payload).is_string()){
			std::cerr << "payload was not a string" << std::endl;
			return MHD_NO;
		}
		auto kstr = (*pubkey).get<std::string>();
		auto pstr = (*payload).get<std::string>();
		auto ltype = (*lookuptype).get<int>();
		try{
			chain.AddExternalLookup(reinterpret_cast<const unsigned char*>(kstr.c_str()),
					kstr.size(), pstr, ltype);
		}catch(Catena::SigningException& e){
			std::cerr << "couldn't sign transaction (" << e.what() << ")" << std::endl;
			return MHD_NO; // FIXME return error response
		}
	}catch(nlohmann::detail::parse_error& e){
		std::cerr << "error extracting JSON from " << upload << std::endl;
		return MHD_NO;
	}
	ps->response = "{}";
	return MHD_YES;
}

int HTTPDServer::LookupAuthTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::LookupAuthReqTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::MemberTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::PatientTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::PatientDelegationTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::PatientStatusTXReq(struct PostState* ps, const char* upload, size_t uplen) const {
	(void)ps;
	(void)upload;
	(void)uplen;
	return MHD_NO;
}

int HTTPDServer::HandlePost(struct MHD_Connection* conn, const char* url,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	const struct {
		const char *uri;
		int (HTTPDServer::*fxn)(struct PostState*, const char*, size_t) const;
	} cmds[] = {
		{ "/member", &HTTPDServer::MemberTXReq, },
		{ "/exlookup", &HTTPDServer::ExternalLookupTXReq, },
		{ "/lauthreq", &HTTPDServer::LookupAuthReqTXReq, },
		{ "/lauth", &HTTPDServer::LookupAuthTXReq, },
		{ "/patient", &HTTPDServer::PatientTXReq, },
		{ "/delpstatus", &HTTPDServer::PatientDelegationTXReq, },
		{ "/pstatus", &HTTPDServer::PatientStatusTXReq, },
		{ nullptr, nullptr },
	},* cmd;
	int retcode = MHD_HTTP_OK; // FIXME callbacks need be able to set retcode
	struct MHD_Response* resp = nullptr;
	for(cmd = cmds ; cmd->uri ; ++cmd){
		if(strcmp(cmd->uri, url) == 0){
			break;
		}
	}
	if(!cmd->uri){
		retcode = MHD_HTTP_NOT_FOUND;
		char buf[1] = {0};
		std::cerr << "no POST service provided at " << url << ", returning 404" << std::endl;
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		auto ret = MHD_queue_response(conn, retcode, resp);
		MHD_destroy_response(resp);
		return ret;
	}
	auto mpp = static_cast<struct PostState*>(*conn_cls);
	if(mpp == nullptr){
		*conn_cls = new PostState;
		return MHD_YES;
	}
	if(upload_len && *upload_len){
		auto ret = (this->*(cmd->fxn))(mpp, upload_data, *upload_len);
		if(ret == MHD_NO){
			std::cerr << "error handling " << url << std::endl;
		}
		*upload_len = 0;
		return ret;
	}
	resp = MHD_create_response_from_buffer(mpp->response.size(),
			const_cast<char*>(mpp->response.c_str()),
			MHD_RESPMEM_MUST_COPY);
	if(resp == nullptr){
		return MHD_NO;
	}
	auto ret = MHD_queue_response(conn, retcode, resp);
	MHD_destroy_response(resp);
	return ret;
}

// FIXME all the places where we just plainly return MHD_NO, we ought try
//  queueing an actual MHD_HTTP_INTERNAL_SERVER_ERROR.
// cls is this pointer
int HTTPDServer::Handler(void* cls, struct MHD_Connection* conn, const char* url,
				const char* method, const char* version,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	HTTPDServer* this_ = reinterpret_cast<HTTPDServer*>(cls);
	(void)version;
	(void)conn_cls;
	if(strcmp(method, MHD_HTTP_METHOD_POST) == 0){
		return this_->HandlePost(conn, url, upload_data, upload_len, conn_cls);
	}
	if(strcmp(method, MHD_HTTP_METHOD_GET)){
		return MHD_NO;
	}
	int retcode = MHD_HTTP_OK; // FIXME callbacks need be able to set retcode
	const struct {
		const char *uri;
		struct MHD_Response* (HTTPDServer::*fxn)(struct MHD_Connection*) const;
	} cmds[] = {
		{ "/", &HTTPDServer::Summary, },
		{ "/favicon.ico", &HTTPDServer::Favicon, },
		{ "/show", &HTTPDServer::Show, },
		{ "/tstore", &HTTPDServer::TStore, },
		{ "/inspect", &HTTPDServer::Inspect, },
		{ "/pstatus", &HTTPDServer::Pstatus, },
		{ nullptr, nullptr },
	},* cmd;
	struct MHD_Response* resp = nullptr;
	for(cmd = cmds ; cmd->uri ; ++cmd){
		if(strcmp(cmd->uri, url) == 0){
			resp = (*reinterpret_cast<HTTPDServer*>(cls).*(cmd->fxn))(conn);
			break;
		}
	}
	if(!cmd->uri){
		retcode = MHD_HTTP_NOT_FOUND;
		char buf[1] = {0};
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		std::cerr << "no GET service provided at " << url << ", returning 404" << std::endl;
	}
	if(resp == nullptr){
		std::cerr << "couldn't create HTTP response, sending error" << std::endl;
		char buf[1] = {0};
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		if(resp == nullptr){
			return MHD_NO;
		}
		retcode = MHD_HTTP_INTERNAL_SERVER_ERROR;
	}
	auto ret = MHD_queue_response(conn, retcode, resp);
	MHD_destroy_response(resp);
	if(ret == MHD_NO){
		std::cerr << "couldn't queue HTTP response" << std::endl;
		return MHD_NO;
	}
	return ret;
}

HTTPDServer::HTTPDServer(Catena::Chain& chain, unsigned port) :
  chain(chain) {
	const unsigned flags = MHD_USE_SELECT_INTERNALLY | MHD_USE_EPOLL_LINUX_ONLY;
	mhd = MHD_start_daemon(flags, port, NULL, NULL, Handler, this,
				MHD_OPTION_END);
	if(mhd == nullptr){
		throw HTTPException();
	}
}

}
