#include <gtest/gtest.h>
#include <libcatena/chain.h>
#include "test/defs.h"

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain(GENESISBLOCK_EXTERNAL);
}

TEST(CatenaChain, ChainGenesisMock){
	Catena::Chain(MOCKLEDGER);
}
