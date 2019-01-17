#include <gtest/gtest.h>
#include <libcatena/externallookuptx.h>
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

TEST(CatenaChain, AddConsortiumMember){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto origsize = chain.Size();
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(0, chain.GetBlockCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
	nlohmann::json j = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), j);
	EXPECT_EQ(1, chain.OutstandingTXCount());
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(origsize, chain.Size());
	chain.CommitOutstanding();
	EXPECT_LT(origsize, chain.Size());
	EXPECT_EQ(1, chain.TXCount());
	EXPECT_EQ(1, chain.GetBlockCount());
}

TEST(CatenaChain, AddExternalLookup){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto origsize = chain.Size();
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(0, chain.GetBlockCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
  std::string extid = "50a0a990-1a25-11e9-9131-8b159f637c76";
	chain.AddExternalLookup(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), extid, Catena::ExtIDTypes::SharecareID);
	EXPECT_EQ(1, chain.OutstandingTXCount());
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(origsize, chain.Size());
	chain.CommitOutstanding();
	EXPECT_LT(origsize, chain.Size());
	EXPECT_EQ(1, chain.TXCount());
	EXPECT_EQ(1, chain.GetBlockCount());
}

TEST(CatenaChain, AddExternalLookupBadType){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
  std::string extid;
  EXPECT_THROW(chain.AddExternalLookup(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
                pem.length(), extid, static_cast<Catena::ExtIDTypes>(1)),
	  Catena::BlockValidationException);
}

TEST(CatenaChain, AddLookupAuthReq){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
  std::string extid;
  Catena::TXSpec el1;
	auto j = nlohmann::json::parse("{ \"reason\": \"gotta have that SCID\" }");
	auto origsize = chain.Size();
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(0, chain.GetBlockCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
  chain.AddLookupAuthReq(cm1, el1, j); // FIXME accepts a bogus ELspec!
	EXPECT_EQ(1, chain.OutstandingTXCount());
	EXPECT_EQ(0, chain.TXCount());
	EXPECT_EQ(origsize, chain.Size());
	chain.CommitOutstanding();
	EXPECT_LT(origsize, chain.Size());
	EXPECT_EQ(1, chain.TXCount());
	EXPECT_EQ(1, chain.GetBlockCount());
}

TEST(CatenaChain, AddLookupAuth){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto j = nlohmann::json::parse("{ \"reason\": \"gotta have that SCID\" }");
  std::string extid = "50a0a990-1a25-11e9-9131-8b159f637c76";
	chain.AddExternalLookup(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), extid, Catena::ExtIDTypes::SharecareID);
	chain.CommitOutstanding();
  Catena::TXSpec el1(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(el1, newkp);
  chain.AddLookupAuthReq(cm1, el1, j);
	chain.CommitOutstanding();
  Catena::TXSpec larspec(chain.MostRecentBlockHash(), 0);
  Catena::TXSpec uspec;
  Catena::SymmetricKey symkey;
  chain.AddLookupAuth(larspec, uspec, symkey);
	chain.CommitOutstanding();
	EXPECT_EQ(3, chain.TXCount());
	EXPECT_EQ(3, chain.GetBlockCount());
}

TEST(CatenaChain, AddLookupAuthBadLAR){
	Catena::Chain chain("", 0);
  Catena::TXSpec larspec(chain.MostRecentBlockHash(), 0);
  Catena::TXSpec uspec;
  Catena::SymmetricKey symkey;
  EXPECT_THROW(chain.AddLookupAuth(larspec, uspec, symkey), Catena::InvalidTXSpecException);
}

// FIXME add rpc test (chain.EnableRPC())
