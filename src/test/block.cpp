#include <fstream>
#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, BlocksGenesisBlock){
	Catena::Blocks cbs;
	ASSERT_TRUE(cbs.loadFile(GENESISBLOCK_EXTERNAL));
	EXPECT_EQ(1, cbs.getBlockCount());
}

TEST(CatenaBlocks, BlocksInvalidFile){
	Catena::Blocks cbs;
	EXPECT_THROW(cbs.loadFile(""), std::ifstream::failure);
}

// Chunks too small to be a valid block
TEST(CatenaBlocks, BlocksInvalidShort){
	Catena::Blocks cbs;
	// Should fail on 0 bytes
	EXPECT_FALSE(cbs.loadData("", 0));
	char block[Catena::Block::BLOCKHEADERLEN];
	// Should fail on fewer bytes than the minimum
	EXPECT_FALSE(cbs.loadData(block, sizeof(block) - 1));
}

// A chunk large enough to be a valid block, but containing all 0s
TEST(CatenaBlocks, BlocksInvalidZeroes){
	Catena::Blocks cbs;
	char block[Catena::Block::BLOCKHEADERLEN];
	memset(block, 0, sizeof(block));
	EXPECT_FALSE(cbs.loadData(block, sizeof(block)));
}

// Generate a simple block, and read it back
TEST(CatenaBlocks, BlockGenerated){
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const char[]> block;
	unsigned size;
	std::tie(block, size) = Catena::Block::serializeBlock(prevhash);
	ASSERT_TRUE(0 != block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_TRUE(cbs.loadData(block.get(), size));
}

// Generate a simple block with invalid prev, and read it back
TEST(CatenaBlocks, BlockGeneratedBadprev){
	unsigned char prevhash[HASHLEN] = {1};
	std::unique_ptr<const char[]> block;
	unsigned size;
	std::tie(block, size) = Catena::Block::serializeBlock(prevhash);
	ASSERT_TRUE(0 != block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.loadData(block.get(), size));
}

// Generate two blocks, and read them back
TEST(CatenaBlocks, ChainGenerated){
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const char[]> b1, b2;
	unsigned s1, s2;
	std::tie(b1, s1) = Catena::Block::serializeBlock(prevhash);
	ASSERT_TRUE(0 != b1);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s1);
	std::tie(b2, s2) = Catena::Block::serializeBlock(prevhash);
	ASSERT_TRUE(0 != b2);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s2);
	char block[s1 + s2];
	memcpy(block, b1.get(), s1);
	memcpy(block + s1, b2.get(), s2);
	Catena::Blocks cbs;
	EXPECT_TRUE(cbs.loadData(block, s1 + s2));
	EXPECT_EQ(2, cbs.getBlockCount());
}
