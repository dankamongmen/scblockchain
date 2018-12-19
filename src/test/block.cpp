#include <fstream>
#include <gtest/gtest.h>
#include "libcatena/truststore.h"
#include "libcatena/builtin.h"
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, BlocksGenesisBlock){
	Catena::TrustStore tstore;
	Catena::BuiltinKeys bkeys;
        bkeys.AddToTrustStore(tstore);
	Catena::Blocks cbs;
	ASSERT_FALSE(cbs.LoadFile(GENESISBLOCK_EXTERNAL, tstore));
	EXPECT_EQ(1, cbs.getBlockCount());
}

TEST(CatenaBlocks, BlocksInvalidFile){
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	EXPECT_THROW(cbs.LoadFile("", tstore), std::ifstream::failure);
}

TEST(CatenaBlocks, EmptyBlock){
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	// Should pass on 0 bytes
	EXPECT_FALSE(cbs.LoadData("", 0, tstore));
}

// Chunks too small to be a valid block
TEST(CatenaBlocks, BlocksInvalidShort){
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	char block[Catena::Block::BLOCKHEADERLEN];
	// Should fail on fewer bytes than the minimum
	EXPECT_TRUE(cbs.LoadData(block, sizeof(block) - 1, tstore));
}

// A chunk large enough to be a valid block, but containing all 0s
TEST(CatenaBlocks, BlocksInvalidZeroes){
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	char block[Catena::Block::BLOCKHEADERLEN];
	memset(block, 0, sizeof(block));
	EXPECT_TRUE(cbs.LoadData(block, sizeof(block), tstore));
}

// Generate a simple block, and read it back
TEST(CatenaBlocks, BlockGenerated){
	Catena::TrustStore tstore;
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const unsigned char[]> block;
	size_t size;
	Catena::Block b;
	std::tie(block, size) = b.serializeBlock(prevhash);
	ASSERT_NE(nullptr, block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.LoadData(block.get(), size, tstore));
}

// Generate a block with some NoOps, and read it back
TEST(CatenaBlocks, BlockGeneratedNoOps){
	Catena::TrustStore tstore;
	for(auto i = 0 ; i < 4096 ; i += 16){
		unsigned char prevhash[HASHLEN] = {0};
		Catena::Block b;
		for(auto j = 0 ; j < i + 1 ; ++j){
			b.AddTransaction(std::make_unique<Catena::NoOpTX>());
		}
		std::unique_ptr<const unsigned char[]> block;
		size_t size;
		std::tie(block, size) = b.serializeBlock(prevhash);
		ASSERT_NE(nullptr, block);
		ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
		Catena::Blocks cbs;
		EXPECT_FALSE(cbs.LoadData(block.get(), size, tstore));
	}
}

// Generate a simple block with invalid prev, and read it back
TEST(CatenaBlocks, BlockGeneratedBadprev){
	Catena::TrustStore tstore;
	unsigned char prevhash[HASHLEN] = {1};
	std::unique_ptr<const unsigned char[]> block;
	size_t size;
	Catena::Block b;
	std::tie(block, size) = b.serializeBlock(prevhash);
	ASSERT_NE(nullptr, block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_TRUE(cbs.LoadData(block.get(), size, tstore));
}

// Generate two blocks, and read them back
TEST(CatenaBlocks, ChainGenerated){
	Catena::TrustStore tstore;
	unsigned char prevhash[HASHLEN] = {0};
	std::unique_ptr<const unsigned char[]> b1, b2;
	size_t s1, s2;
	Catena::Block blk1, blk2;
	std::tie(b1, s1) = blk1.serializeBlock(prevhash);
	ASSERT_NE(nullptr, b1);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s1);
	std::tie(b2, s2) = blk2.serializeBlock(prevhash);
	ASSERT_NE(nullptr, b2);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s2);
	char block[s1 + s2];
	memcpy(block, b1.get(), s1);
	memcpy(block + s1, b2.get(), s2);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.LoadData(block, s1 + s2, tstore));
	EXPECT_EQ(2, cbs.getBlockCount());
}
