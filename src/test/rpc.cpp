#include <gtest/gtest.h>
#include <libcatena/rpc.h>

TEST(CatenaRPC, BadRPCPort){
	auto chainfile = "";
	const char* peerfile = nullptr;
	EXPECT_THROW(Catena::RPCService(-1, chainfile, peerfile), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService(65536, chainfile, peerfile), Catena::NetworkException);
}
