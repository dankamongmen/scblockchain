#include <gtest/gtest.h>
#include <libcatena/chain.h>
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
	EXPECT_EQ(4, chain.PubkeyCount());
	EXPECT_EQ(MOCKLEDGER_BLOCKS, chain.GetBlockCount());
	EXPECT_EQ(MOCKLEDGER_TXS, chain.TXCount());
	EXPECT_EQ(0, chain.OutstandingTXCount());
}
