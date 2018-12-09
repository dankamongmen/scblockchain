#include <fstream>
#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, CatenaBlocksGenesisBlock){
	CatenaBlocks cbs;
	ASSERT_TRUE(cbs.loadFile(GENESISBLOCK_EXTERNAL));
	EXPECT_LT(0, cbs.getBlockCount());
}

TEST(CatenaBlocks, CatenaBlocksInvalidFile){
	CatenaBlocks cbs;
	EXPECT_THROW(cbs.loadFile(""), std::ifstream::failure);
}

// Chunks too small to be a valid block
TEST(CatenaBlocks, CatenaBlocksInvalidShort){
	CatenaBlocks cbs;
	// Should fail on 0 bytes
	EXPECT_FALSE(cbs.loadData("", 0));
	char block[CatenaBlock::BLOCKHEADERLEN];
	// Should fail on fewer bytes than the minimum
	EXPECT_FALSE(cbs.loadData(block, sizeof(block) - 1));
}

// A chunk large enough to be a valid block, but containing all 0s
TEST(CatenaBlocks, CatenaBlocksInvalidZeroes){
	CatenaBlocks cbs;
	char block[CatenaBlock::BLOCKHEADERLEN];
	memset(block, 0, sizeof(block));
	EXPECT_FALSE(cbs.loadData(block, sizeof(block)));
}

// Generate a simple block, and read it back
TEST(CatenaBlocks, CatenaBlockGenerated){
	std::unique_ptr<const char[]> block;
	unsigned size;
	std::tie(block, size) = CatenaBlock::serializeBlock();
	ASSERT_TRUE(0 != block);
	ASSERT_LE(CatenaBlock::BLOCKHEADERLEN, size);
	CatenaBlocks cbs;
	EXPECT_TRUE(cbs.loadData(block.get(), size));
}
