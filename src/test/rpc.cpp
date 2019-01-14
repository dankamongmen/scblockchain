#include <fstream>
#include <gtest/gtest.h>
#include <libcatena/rpc.h>
#include <libcatena/chain.h>
#include <libcatena/utility.h>
#include "test/defs.h"

TEST(CatenaRPC, TestChainfile){
  const Catena::RPCServiceOptions opts = {
    .port = 20202,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
	EXPECT_EQ(rpc.Port(), 20202);
	// FIXME check for expected length of chain
}

TEST(CatenaRPC, TestNoListeners){
  const Catena::RPCServiceOptions opts = {
    .port = 0,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
	EXPECT_EQ(rpc.Port(), 0);
}

TEST(CatenaRPC, BadChainfile){
	Catena::Chain chain;
  Catena::RPCServiceOptions opts;
  opts.port = 20202;
  opts.keyfile = TEST_NODEKEY;
  opts.chainfile = "";
	EXPECT_THROW(Catena::RPCService rpc(chain, opts), Catena::NetworkException);
  opts.chainfile = PUBLICKEY;
	EXPECT_THROW(Catena::RPCService rpc(chain, opts), Catena::NetworkException);
}

TEST(CatenaRPC, BadRPCPort){
	Catena::Chain chain;
  Catena::RPCServiceOptions opts;
  opts.keyfile = TEST_NODEKEY;
  opts.chainfile = TEST_X509_CHAIN;
  opts.port = -1;
	EXPECT_THROW(Catena::RPCService(chain, opts), Catena::NetworkException);
  opts.port = 65536;
	EXPECT_THROW(Catena::RPCService(chain, opts), Catena::NetworkException);
}

TEST(CatenaRPC, Peerfile){
	Catena::Chain chain;
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::RPCService rpc(chain, opts);
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
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::RPCService rpc(chain, opts);
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
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::RPCService rpc(chain, opts);
	// Throw it a nonexistant file
	EXPECT_THROW(rpc.AddPeers(""), std::ifstream::failure);
	EXPECT_THROW(rpc.AddPeers("ghrampogfkjl"), std::ifstream::failure);
	// Throw it a directory
	EXPECT_THROW(rpc.AddPeers("/"), Catena::ConvertInputException);
	// Throw it some crap
	EXPECT_THROW(rpc.AddPeers(MOCKLEDGER), Catena::ConvertInputException);
}
