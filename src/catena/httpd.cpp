#include <microhttpd.h>
#include <libcatena/chain.h>
#include "catena/httpd.h"

namespace CatenaAgent {

int HTTPDServer::Handler(void* cls, struct MHD_Connection* conn, const char* url,
				const char* method, const char* version,
				const char* upload_data, size_t* upload_len,
				void** conn_cls){
	(void)cls;
	(void)conn;
	(void)url;
	(void)method;
	(void)version;
	(void)upload_data;
	(void)upload_len;
	(void)conn_cls;
	return MHD_HTTP_OK;
}

HTTPDServer::HTTPDServer(Catena::Chain& chain, unsigned port) :
  chain(chain) {
	unsigned flags = 0;
	mhd = MHD_start_daemon(flags, port, NULL, NULL, Handler, NULL,
				MHD_OPTION_END);
	if(mhd == nullptr){
		throw HTTPException();
	}
}

}
