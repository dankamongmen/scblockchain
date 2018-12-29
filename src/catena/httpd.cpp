#include <sstream>
#include <iostream>
#include <microhttpd.h>
#include <libcatena/utility.h>
#include <libcatena/chain.h>
#include <libcatena/hash.h>
#include "catena/httpd.h"

namespace CatenaAgent {

HTTPDServer::~HTTPDServer(){
	if(mhd){
		MHD_stop_daemon(mhd);
	}
}

constexpr char htmlhdr[] =
 "<!DOCTYPE html>"
 "<html><link rel=\"stylesheet\" type=\"text/css\" href=\"//fonts.googleapis.com/css?family=Open+Sans\" />"
 "<head><title>catena node</title>"
 "<style>body { font-family: \"Open Sans\"; margin: 5px ; padding: 5px; background: whitesmoke; }"
 "table { table-layout: fixed; border-collapse; collapse: border-spacing: 0; width: 90%; }"
 "td { width: 10%; background: #dfdfdf; border: 1px solid transparent; transition: all 0.3s; }"
 "td+td { width: auto; }"
 "tr:nth-child(even) td { background: #f1f1f1; }"
 "tr:nth-child(odd) td { background: #fefefe; }"
 ".hrule { margin: 1em 0 1em 0; width: 50%; height: 1px; box-shadow: 0 0 5px 1px #ff8300; }"
 "tr td:hover { background: #00dddd; }"
 "</style></head>";

std::stringstream& HTTPDServer::HTMLSysinfo(std::stringstream& ss) const {
	ss << "<table>";
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

struct MHD_Response*
HTTPDServer::Summary(struct MHD_Connection* conn __attribute__ ((unused))) const {
	std::stringstream ss;
	ss << htmlhdr;
	ss << "<body><h1>catena node</h1>";
	HTMLSysinfo(ss);
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
		nlohmann::json(b.bhdr.totlen);
		jblks.emplace_back(jblk);
	}
	nlohmann::json ret(jblks);
	return ret;
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

int HTTPDServer::ExternalLookupReq(void* coninfo_cls,
		enum MHD_ValueKind kind, const char* key, const char *filename,
                const char* content_type, const char* transfer_encoding,
                const char* value, uint64_t off, size_t size){
	(void)size;
	(void)off;
	(void)value;
	(void)transfer_encoding;
	(void)content_type;
	(void)filename;
	(void)key;
	(void)kind;
	(void)coninfo_cls;
	return MHD_YES;
}

int HTTPDServer::HandlePost(struct MHD_Connection* conn, const char* url,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	const struct {
		const char *uri;
		int (*fxn)(void*, enum MHD_ValueKind, const char*,
                                const char*, const char*,
                                const char*, const char*,
                                uint64_t, size_t);
	} cmds[] = {
		{ "/exlookup", ExternalLookupReq, },
		{ nullptr, nullptr },
	},* cmd;
	std::cerr << "GOT US THE POST " << (upload_len ? *upload_len : 0) << "b\n";
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
	auto mpp = static_cast<struct MHD_PostProcessor*>(*conn_cls);
	if(mpp == nullptr){
		mpp = MHD_create_post_processor(conn, BUFSIZ, cmd->fxn, nullptr);
		*conn_cls = mpp;
		return MHD_YES;
	}
	if(upload_len && *upload_len){
		auto ret = MHD_post_process(mpp, upload_data, *upload_len);
		*upload_len = 0;
		return ret;
	}
	MHD_destroy_post_processor(mpp);
	char buf[1] = ""; // FIXME get resptext through mpp
	resp = MHD_create_response_from_buffer(1, buf, MHD_RESPMEM_MUST_COPY);
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
		{ "/show", &HTTPDServer::Show, },
		{ "/tstore", &HTTPDServer::TStore, },
		{ "/inspect", &HTTPDServer::Inspect, },
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
