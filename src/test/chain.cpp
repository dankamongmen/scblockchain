#include <gtest/gtest.h>
#include <libcatena/chain.h>

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain("genesisblock");
}

TEST(CatenaChain, ChainGenesisMock){
	Catena::Chain("test/genesisblock-test");
}
