#include <gtest/gtest.h>
#include <libcatena/chain.h>
#include <libcatena/sig.h>
#include "test/defs.h"

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain chain(GENESISBLOCK_EXTERNAL);
	EXPECT_EQ(2, chain.PubkeyCount());
	EXPECT_EQ(1, chain.GetBlockCount());
	EXPECT_EQ(1, chain.TXCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
}

TEST(CatenaChain, ChainGenesisMock){
	Catena::Chain chain(MOCKLEDGER);
	EXPECT_EQ(MOCKLEDGER_PUBKEYS, chain.PubkeyCount());
	EXPECT_EQ(MOCKLEDGER_BLOCKS, chain.GetBlockCount());
	EXPECT_EQ(MOCKLEDGER_TXS, chain.TXCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
	EXPECT_EQ(chain.LookupRequestCount(),
		chain.LookupRequestCount(true) + chain.LookupRequestCount(false));
	EXPECT_GE(chain.LookupRequestCount(true), 0);
	EXPECT_GE(chain.LookupRequestCount(false), 0);
}

TEST(CatenaChain, ChainAddConsortiumMember){
	Catena::Chain chain(MOCKLEDGER);
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	chain.AddPrivateKey(cm1, kp);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	nlohmann::json j = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), j);
	// FIXME do commit, check result
}

// FIXME add tests which reject

// FIXME expand to cover other Add*() functionality

// FIXME add rpc test (chain.EnableRPC())
