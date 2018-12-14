#include <gtest/gtest.h>
#include <libcatena/chain.h>

TEST(CatenaChain, ChainGenesisBlock){
	Catena::Chain("test/genesisblock");
}
