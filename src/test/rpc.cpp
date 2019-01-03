#include <gtest/gtest.h>
#include <libcatena/rpc.h>

TEST(CatenaRPC, BadRPCPort){
	auto chainfile = "";
	EXPECT_THROW(Catena::RPCService(-1, chainfile), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService(65536, chainfile), Catena::NetworkException);
}
