#include <gtest/gtest.h>
#include <libcatena/externallookuptx.h>
#include <libcatena/keypair.h>
#include <libcatena/chain.h>
#include "test/defs.h"

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain chain(GENESISBLOCK_EXTERNAL);
	EXPECT_EQ(2, chain.PubkeyCount());
	EXPECT_EQ(1, chain.GetBlockCount());
	EXPECT_EQ(1, chain.TXCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
  EXPECT_EQ(0, chain.RPCPort());
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

TEST(CatenaChain, AddConsortiumMemberKeySupplied){
  size_t len;
  auto res = Catena::ReadBinaryFile(ECDSAKEY, &len);
  ASSERT_NE(res.get(), nullptr);
	Catena::Chain chain(MOCKLEDGER);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	nlohmann::json j = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	EXPECT_EQ(0, chain.OutstandingTXCount());
  chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					      pem.length(), j, res.get(), len);
	EXPECT_EQ(1, chain.OutstandingTXCount());
}

TEST(CatenaChain, AddConsortiumMemberNoKey){ // try it without a privkey loaded
	Catena::Chain chain("", 0);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	nlohmann::json j = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
  EXPECT_THROW(chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					      pem.length(), j),
      Catena::SigningException);
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
  std::string extid; // FIXME appears to accept bogus cm1!
  EXPECT_THROW(chain.AddExternalLookup(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
                pem.length(), extid, static_cast<Catena::ExtIDTypes>(1)),
	  Catena::TransactionException);
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
  symkey.fill(0xff);
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
  symkey.fill(0xff);
  EXPECT_THROW(chain.AddLookupAuth(larspec, uspec, symkey), Catena::InvalidTXSpecException);
}

TEST(CatenaChain, AddUser){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto cmj = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), cmj);
	chain.CommitOutstanding();
	Catena::TXSpec cm2(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(cm2, newkp);
	auto j = nlohmann::json::parse("{ \"name\": \"test user, only a test\" }");
  Catena::SymmetricKey symkey;
  symkey.fill(0xff);
	chain.AddUser(cm2, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), symkey, j);
	chain.CommitOutstanding();
	EXPECT_EQ(2, chain.TXCount());
	EXPECT_EQ(2, chain.GetBlockCount());
}

TEST(CatenaChain, AddUserNoCMRKey){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto cmj = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), cmj);
	chain.CommitOutstanding();
	Catena::TXSpec cm2(chain.MostRecentBlockHash(), 0);
	auto j = nlohmann::json::parse("{ \"name\": \"test user, only a test\" }");
  Catena::SymmetricKey symkey;
  symkey.fill(0xff);
	EXPECT_THROW(chain.AddUser(cm2, reinterpret_cast<const unsigned char*>(pem.c_str()),
					      pem.length(), symkey, j),
      Catena::SigningException);
}

TEST(CatenaChain, AddUserBadCMR){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto uj = nlohmann::json::parse("{ \"name\": \"test user, only a test\" }");
  Catena::SymmetricKey symkey;
  symkey.fill(0xff);
	chain.AddUser(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					      pem.length(), symkey, uj);
  // FIXME shouldn't this happen above?
  EXPECT_THROW(chain.CommitOutstanding(), Catena::InvalidTXSpecException);
}

TEST(CatenaChain, AddUserStatusDelegation){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto cmj = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), cmj);
	chain.CommitOutstanding();
	Catena::TXSpec cm2(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(cm2, newkp);
	auto j = nlohmann::json::parse("{ \"name\": \"test user, only a test\" }");
  Catena::SymmetricKey symkey;
  symkey.fill(0xff);
	Catena::Keypair unewkp;
	unewkp.Generate();
	auto upem = unewkp.PubkeyPEM();
	chain.AddUser(cm2, reinterpret_cast<const unsigned char*>(upem.c_str()),
					upem.length(), symkey, j);
	chain.CommitOutstanding();
  Catena::TXSpec uspec(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(uspec, unewkp);
	auto psdj = nlohmann::json::parse("{ \"Reason\": \"i ❤ delegations\" }");
  chain.AddUserStatusDelegation(cm2, uspec, 0, psdj);
	chain.CommitOutstanding();
	EXPECT_EQ(3, chain.TXCount());
	EXPECT_EQ(3, chain.GetBlockCount());
}

TEST(CatenaChain, AddUserStatus){
	Catena::Keypair kp(ECDSAKEY);
	Catena::TXSpec cm1(CM1_TEST_TX);
	Catena::Keypair newkp;
	newkp.Generate();
	auto pem = newkp.PubkeyPEM();
	ASSERT_LT(0, pem.length());
	Catena::Chain chain("", 0);
	chain.AddPrivateKey(cm1, kp);
	auto cmj = nlohmann::json::parse("{ \"Entity\": \"Test entity\" }");
	chain.AddConsortiumMember(cm1, reinterpret_cast<const unsigned char*>(pem.c_str()),
					pem.length(), cmj);
	chain.CommitOutstanding();
	Catena::TXSpec cm2(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(cm2, newkp);
	auto j = nlohmann::json::parse("{ \"name\": \"test user, only a test\" }");
  Catena::SymmetricKey symkey;
  symkey.fill(0xff);
	Catena::Keypair unewkp;
	unewkp.Generate();
	auto upem = unewkp.PubkeyPEM();
	chain.AddUser(cm2, reinterpret_cast<const unsigned char*>(upem.c_str()),
					upem.length(), symkey, j);
	chain.CommitOutstanding();
  Catena::TXSpec uspec(chain.MostRecentBlockHash(), 0);
	chain.AddPrivateKey(uspec, unewkp);
	auto psdj = nlohmann::json::parse("{ \"Reason\": \"i ❤ delegations\" }");
  chain.AddUserStatusDelegation(cm2, uspec, 0, psdj);
	chain.CommitOutstanding();
	auto usj = nlohmann::json::parse("{ \"greencoins\": \"1729\" }");
  Catena::TXSpec usdspec(chain.MostRecentBlockHash(), 0);
  chain.AddUserStatus(usdspec, usj);
  chain.CommitOutstanding();
	EXPECT_EQ(4, chain.TXCount());
	EXPECT_EQ(4, chain.GetBlockCount());
}

TEST(CatenaChain, AddUserStatusBadUSD){
	Catena::Chain chain("", 0);
  auto usj = nlohmann::json::parse("{ \"greencoins\": \"1729\" }");
  Catena::TXSpec usdspec(chain.MostRecentBlockHash(), 0);
  EXPECT_THROW(chain.AddUserStatus(usdspec, usj), Catena::InvalidTXSpecException);
}

TEST(CatenaChain, EnableRPC){
	Catena::Chain chain("", 0);
  Catena::RPCServiceOptions opts;
  opts.port = Catena::DefaultRPCPort;
  opts.keyfile = TEST_NODEKEY;
  opts.chainfile = TEST_X509_CHAIN;
  chain.EnableRPC(opts);
  EXPECT_EQ(Catena::DefaultRPCPort, chain.RPCPort());
}

TEST(CatenaChain, EnableRPCTwice){
	Catena::Chain chain("", 0);
  Catena::RPCServiceOptions opts;
  opts.port = Catena::DefaultRPCPort;
  opts.keyfile = TEST_NODEKEY;
  opts.chainfile = TEST_X509_CHAIN;
  chain.EnableRPC(opts);
  EXPECT_THROW(chain.EnableRPC(opts), Catena::NetworkException);
}
