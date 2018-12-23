#include <iostream>
#include <microhttpd.h>
#include <libcatena/chain.h>
#include "catena/httpd.h"

namespace CatenaAgent {

int HTTPDServer::Handler(void* cls, struct MHD_Connection* conn, const char* url,
				const char* method, const char* version,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	(void)cls;
	(void)url;
	(void)method;
	(void)version;
	(void)upload_data;
	(void)upload_len;
	(void)conn_cls;
	char buf[] = "<!DOCTYPE html><html lang=\"en\"><head><title>catena</title></head><body>heya</body></html>";
	auto resp = MHD_create_response_from_buffer(strlen(buf), buf, MHD_RESPMEM_MUST_COPY);
	if(resp == nullptr){
		std::cerr << "couldn't create HTTP response" << std::endl;
		// FIXME try to queue and send error
		return MHD_NO;
	}
	if(MHD_NO == MHD_add_response_header(resp, MHD_HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8")){
		MHD_destroy_response(resp); // FIXME again, try to send error?
		return MHD_NO;
	}
	auto ret = MHD_queue_response(conn, MHD_HTTP_OK, resp);
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
	mhd = MHD_start_daemon(flags, port, NULL, NULL, Handler, NULL,
				MHD_OPTION_END);
	if(mhd == nullptr){
		throw HTTPException();
	}
}

}
