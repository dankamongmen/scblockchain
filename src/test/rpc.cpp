#include <fstream>
#include <gtest/gtest.h>
#include <libcatena/rpc.h>
#include <libcatena/chain.h>
#include <libcatena/utility.h>
#include "test/defs.h"

TEST(CatenaRPC, TestChainfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, 20202, TEST_X509_CHAIN, TEST_NODEKEY);
	EXPECT_EQ(rpc.Port(), 20202);
	// FIXME check for expected length of chain
}

TEST(CatenaRPC, BadChainfile){
	Catena::Chain chain;
	EXPECT_THROW(Catena::RPCService rpc(chain, 20202, "", TEST_NODEKEY), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService rpc(chain, 20202, PUBLICKEY, TEST_NODEKEY), Catena::NetworkException);
}

TEST(CatenaRPC, BadRPCPort){
	Catena::Chain chain;
	EXPECT_THROW(Catena::RPCService(chain, -1, TEST_X509_CHAIN, TEST_NODEKEY), Catena::NetworkException);
	EXPECT_THROW(Catena::RPCService(chain, 65536, TEST_X509_CHAIN, TEST_NODEKEY), Catena::NetworkException);
}

TEST(CatenaRPC, Peerfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, Catena::DefaultRPCPort, TEST_X509_CHAIN, TEST_NODEKEY);
	rpc.AddPeers(RPC_TEST_PEERS);
	int active, defined, max;
	rpc.PeerCount(&defined, &active, &max);
	EXPECT_EQ(4, defined);
	EXPECT_EQ(0, active);
	EXPECT_EQ(Catena::MaxActiveRPCPeers, max);
}

// Duplicates should be filtered, so adding the peerfile twice ought only show
// the original number of peers.
TEST(CatenaRPC, DoubleAddPeerfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, Catena::DefaultRPCPort, TEST_X509_CHAIN, TEST_NODEKEY);
	rpc.AddPeers(RPC_TEST_PEERS);
	int active, defined, max;
	rpc.PeerCount(&defined, &active, &max);
	EXPECT_EQ(4, defined);
	EXPECT_EQ(0, active);
	EXPECT_EQ(Catena::MaxActiveRPCPeers, max);
	rpc.AddPeers(RPC_TEST_PEERS);
	rpc.PeerCount(&defined, &active, &max);
	EXPECT_EQ(4, defined);
}

TEST(CatenaRPC, BadPeerfile){
	Catena::Chain chain;
	Catena::RPCService rpc(chain, Catena::DefaultRPCPort, TEST_X509_CHAIN, TEST_NODEKEY);
	// Throw it a nonexistant file
	EXPECT_THROW(rpc.AddPeers(""), std::ifstream::failure);
	EXPECT_THROW(rpc.AddPeers("ghrampogfkjl"), std::ifstream::failure);
	// Throw it a directory
	EXPECT_THROW(rpc.AddPeers("/"), Catena::ConvertInputException);
	// Throw it some crap
	EXPECT_THROW(rpc.AddPeers(MOCKLEDGER), Catena::ConvertInputException);
}
