#include <sstream>
#include <iostream>
#include <microhttpd.h>
#include <libcatena/chain.h>
#include "catena/httpd.h"

namespace CatenaAgent {

struct MHD_Response* HTTPDServer::Show(){
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

struct MHD_Response* HTTPDServer::TStore(){
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

// FIXME all the places where we just plainly return MHD_NO, we ought try
//  queueing an actual MHD_HTTP_INTERNAL_SERVER_ERROR.
// cls is this pointer
int HTTPDServer::Handler(void* cls, struct MHD_Connection* conn, const char* url,
				const char* method, const char* version,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	(void)cls;
	(void)version;
	(void)upload_data;
	(void)upload_len;
	(void)conn_cls;
	if(strcmp(method, MHD_HTTP_METHOD_GET)){
		return MHD_NO;
	}
	int retcode = MHD_HTTP_OK; // FIXME callbacks need be able to set retcode
	const struct {
		const char *uri;
		struct MHD_Response* (HTTPDServer::*fxn)();
	} cmds[] = {
		{ "/show", &HTTPDServer::Show, },
		{ "/tstore", &HTTPDServer::TStore, },
		{ nullptr, nullptr },
	},* cmd;
	struct MHD_Response* resp = nullptr;
	for(cmd = cmds ; cmd->uri ; ++cmd){
		if(strcmp(cmd->uri, url) == 0){
			resp = (*reinterpret_cast<HTTPDServer*>(cls).*(cmd->fxn))();
			break;
		}
	}
	if(!cmd->uri){
		retcode = MHD_HTTP_NOT_FOUND;
		char buf[1] = {0};
		resp = MHD_create_response_from_buffer(0, buf, MHD_RESPMEM_MUST_COPY);
		std::cerr << "no service provided at " << url << ", returning 404" << std::endl;
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
