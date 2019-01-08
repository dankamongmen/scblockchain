#include <fstream>
#include <gtest/gtest.h>
#include <libcatena/rpc.h>
#include <libcatena/utility.h>
#include "test/defs.h"

TEST(CatenaRPC, TestChainfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, 20202, TEST_X509_CHAIN);
	EXPECT_EQ(rpc.Port(), 20202);
	// FIXME check for expected length of chain
}

TEST(CatenaRPC, BadChainfile){
	Catena::Chain chain;
	EXPECT_THROW(Catena::RPCService rpc(chain, 20202, ""), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService rpc(chain, 20202, PUBLICKEY), Catena::NetworkException);
}

TEST(CatenaRPC, BadRPCPort){
	Catena::Chain chain;
	EXPECT_THROW(Catena::RPCService(chain, -1, TEST_X509_CHAIN), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService(chain, 65536, TEST_X509_CHAIN), Catena::NetworkException);
}

TEST(CatenaRPC, Peerfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, 40404, TEST_X509_CHAIN);
	rpc.AddPeers(RPC_TEST_PEERS);
	int active, defined, max;
	rpc.PeerCount(&defined, &active, &max);
	EXPECT_EQ(4, defined);
	EXPECT_EQ(0, active);
	EXPECT_EQ(Catena::MaxActiveRPCPeers, max);
}

TEST(CatenaRPC, BadPeerfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, 40404, TEST_X509_CHAIN);
	// Throw it a nonexistant file
	EXPECT_THROW(rpc.AddPeers(""), std::ifstream::failure);
	EXPECT_THROW(rpc.AddPeers("ghrampogfkjl"), std::ifstream::failure);
	// Throw it a directory
	EXPECT_THROW(rpc.AddPeers("/"), Catena::ConvertInputException);
	// Throw it some crap
	EXPECT_THROW(rpc.AddPeers(MOCKLEDGER), Catena::ConvertInputException);
}
