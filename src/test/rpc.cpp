#include <fstream>
#include <gtest/gtest.h>
#include <capnp/message.h>
#include <libcatena/rpc.h>
#include <capnp/serialize.h>
#include <libcatena/chain.h>
#include <libcatena/utility.h>
#include <proto/catena.capnp.h>
#include "test/defs.h"

TEST(CatenaRPC, TestChainfile){
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
	EXPECT_EQ(rpc.Port(), 40404);
  EXPECT_EQ(rpc.Advertisement().size(), 0);
	// FIXME check for expected length of chain
}

TEST(CatenaRPC, TestPortOptions){
  const Catena::RPCServiceOptions opts = {
    .port = 20202,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
	EXPECT_EQ(rpc.Port(), 20202);
}

TEST(CatenaRPC, TestAdvertisementOptions){
  const std::vector<std::string> addrs = {
    "127.0.0.1:40404", "127.0.0.1", "localhost",
  };
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = addrs,
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
  EXPECT_EQ(rpc.Advertisement(), addrs);
}

TEST(CatenaRPC, TestNullAdvertisementProto){
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = {},
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
  auto navec = rpc.NodeAdvertisement();
  // FIXME alignment requirements!
  const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(navec))),
        reinterpret_cast<const capnp::word*>(&(*std::end(navec))));
  capnp::FlatArrayMessageReader node(view);
  EXPECT_THROW(node.getRoot<Catena::Proto::AdvertiseNode>(), ::kj::Exception);
}

TEST(CatenaRPC, TestAdvertisementProto){
  const std::vector<std::string> addrs = {
    "127.0.0.1:40404", "127.0.0.1", "localhost",
  };
  const Catena::RPCServiceOptions opts = {
    .port = Catena::DefaultRPCPort,
    .chainfile = TEST_X509_CHAIN,
    .keyfile = TEST_NODEKEY,
    .addresses = addrs,
  };
	Catena::Chain chain;
	Catena::RPCService rpc(chain, opts);
  auto navec = rpc.NodeAdvertisement();
  // FIXME alignment requirements!
  const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(navec))),
        reinterpret_cast<const capnp::word*>(&(*std::end(navec))));
  capnp::FlatArrayMessageReader node(view);
  auto nodeAd = node.getRoot<Catena::Proto::AdvertiseNode>();
  EXPECT_EQ(addrs.size(), nodeAd.getAds().size());
  auto rname = nodeAd.getName();
  EXPECT_STREQ(rname.getSubjectCN().cStr(), TEST_SUBJECT_CN);
  EXPECT_STREQ(rname.getIssuerCN().cStr(), TEST_ISSUER_CN);
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
	int defined, max;
	rpc.PeerCount(&defined, &max);
	EXPECT_EQ(TEST_CONFIGURED_PEERS, defined);
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
	int defined, max;
	rpc.PeerCount(&defined, &max);
	EXPECT_EQ(TEST_CONFIGURED_PEERS, defined);
	EXPECT_EQ(Catena::MaxActiveRPCPeers, max);
	rpc.AddPeers(RPC_TEST_PEERS);
	rpc.PeerCount(&defined, &max);
	EXPECT_EQ(TEST_CONFIGURED_PEERS, defined);
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
