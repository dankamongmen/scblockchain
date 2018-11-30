#include <fstream>
#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, CatenaBlocksGenesisBlock){
	CatenaBlocks cbs;
	cbs.loadFile(GENESISBLOCK_EXTERNAL);
	ASSERT_LT(0, cbs.getBlockCount());
}

TEST(CatenaBlocks, CatenaBlocksInvalidFile){
	CatenaBlocks cbs;
	EXPECT_THROW(cbs.loadFile(""), std::ifstream::failure);
}
