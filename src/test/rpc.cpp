#include <fstream>
#include <gtest/gtest.h>
#include <libcatena/rpc.h>
#include <libcatena/utility.h>
#include "test/defs.h"

TEST(CatenaRPC, TestChainfile){
	Catena::RPCService rpc(20202, TEST_X509_CHAIN);
	EXPECT_EQ(rpc.Port(), 20202);
	// FIXME check for expected length of chain
}

TEST(CatenaRPC, BadRPCPort){
	EXPECT_THROW(Catena::RPCService(-1, TEST_X509_CHAIN), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService(65536, TEST_X509_CHAIN), Catena::NetworkException);
}

TEST(CatenaRPC, BadPeerfile){
	Catena::RPCService rpc(20202, TEST_X509_CHAIN);
	// Throw it a nonexistant file
	EXPECT_THROW(rpc.AddPeers(""), std::ifstream::failure);
	// Throw it a directory
	EXPECT_THROW(rpc.AddPeers("/"), std::ifstream::failure);
	// Throw it some crap
	EXPECT_THROW(rpc.AddPeers(MOCKLEDGER), Catena::ConvertInputException);
}
