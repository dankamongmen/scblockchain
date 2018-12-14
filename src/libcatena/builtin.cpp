#include "libcatena/builtin.h"

namespace Catena {

// Headway key #1
static const unsigned char key0[] =
"-----BEGIN PUBLIC KEY-----\n"
"MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////\n"
"/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"
"AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m\n"
"fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0\n"
"SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB\n"
"AgEBA0IABMHATB6cBQYel6ln6rvD+3nQQkv+pCcy5FaZbtgbNzp85uIJlBOdgRqU\n"
"c8QhbDlfR85mT+KKpKH6TqQkI990Wm8=\n"
"-----END PUBLIC KEY-----\n";

BuiltinKeys::BuiltinKeys(){
	keys.emplace_back(key0, sizeof(key0) - 1);
}

}
