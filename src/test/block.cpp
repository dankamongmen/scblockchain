#include <fstream>
#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, CatenaBlocksGenesisBlock){
	CatenaBlocks cbs;
	ASSERT_TRUE(cbs.loadFile(GENESISBLOCK_EXTERNAL));
	EXPECT_EQ(1, cbs.getBlockCount());
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
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const char[]> block;
	unsigned size;
	std::tie(block, size) = CatenaBlock::serializeBlock(prevhash);
	ASSERT_TRUE(0 != block);
	ASSERT_LE(CatenaBlock::BLOCKHEADERLEN, size);
	CatenaBlocks cbs;
	EXPECT_TRUE(cbs.loadData(block.get(), size));
}

// Generate two blocks, and read them back
TEST(CatenaBlocks, CatenaChainGenerated){
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const char[]> b1, b2;
	unsigned s1, s2;
	std::tie(b1, s1) = CatenaBlock::serializeBlock(prevhash);
	ASSERT_TRUE(0 != b1);
	ASSERT_LE(CatenaBlock::BLOCKHEADERLEN, s1);
	std::tie(b2, s2) = CatenaBlock::serializeBlock(prevhash);
	ASSERT_TRUE(0 != b2);
	ASSERT_LE(CatenaBlock::BLOCKHEADERLEN, s2);
	char block[s1 + s2];
	memcpy(block, b1.get(), s1);
	memcpy(block + s1, b2.get(), s2);
	CatenaBlocks cbs;
	EXPECT_TRUE(cbs.loadData(block, s1 + s2));
	EXPECT_EQ(2, cbs.getBlockCount());
}
