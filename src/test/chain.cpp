#include <gtest/gtest.h>
#include <libcatena/chain.h>

#define MOCKBLOCK "test/genesisblock-test"

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain("genesisblock");
}

TEST(CatenaChain, ChainGenesisMock){
	Catena::Chain(MOCKBLOCK);
}
