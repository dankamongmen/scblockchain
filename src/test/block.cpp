#include <fstream>
#include <gtest/gtest.h>
#include <libcatena/block.h>
#include <libcatena/builtin.h>
#include <libcatena/truststore.h>
#include "test/defs.h"

TEST(CatenaBlocks, BlocksGenesisBlock){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::BuiltinKeys bkeys;
        bkeys.AddToTrustStore(tstore);
	Catena::Blocks cbs;
	ASSERT_FALSE(cbs.LoadFile(GENESISBLOCK_EXTERNAL, lmap, tstore));
	EXPECT_EQ(1, cbs.GetBlockCount());
	EXPECT_EQ(1, cbs.TXCount());
}

TEST(CatenaBlocks, BlocksMockLedger){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::BuiltinKeys bkeys;
        bkeys.AddToTrustStore(tstore);
	Catena::Blocks cbs;
	ASSERT_FALSE(cbs.LoadFile(MOCKLEDGER, lmap, tstore));
	EXPECT_EQ(MOCKLEDGER_BLOCKS, cbs.GetBlockCount());
	EXPECT_EQ(MOCKLEDGER_TXS, cbs.TXCount());
}

TEST(CatenaBlocks, BlocksInvalidFile){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	EXPECT_THROW(cbs.LoadFile("", lmap, tstore), std::ifstream::failure);
}

TEST(CatenaBlocks, EmptyBlock){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	// Should pass on 0 bytes
	EXPECT_FALSE(cbs.LoadData("", 0, lmap, tstore));
}

// Chunks too small to be a valid block
TEST(CatenaBlocks, BlocksInvalidShort){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	char block[Catena::Block::BLOCKHEADERLEN];
	// Should fail on fewer bytes than the minimum
	EXPECT_THROW(cbs.LoadData(block, sizeof(block) - 1, lmap, tstore),
			Catena::BlockHeaderException);
}

// A chunk large enough to be a valid block, but containing all 0s
TEST(CatenaBlocks, BlocksInvalidZeroes){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::Blocks cbs;
	char block[Catena::Block::BLOCKHEADERLEN];
	memset(block, 0, sizeof(block));
	EXPECT_THROW(cbs.LoadData(block, sizeof(block), lmap, tstore),
			Catena::BlockHeaderException);
}

// Generate a simple block, and read it back
TEST(CatenaBlocks, BlockGenerated){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::CatenaHash prevhash;
	memset(prevhash.data(), 0xff, prevhash.size());
	std::unique_ptr<const unsigned char[]> block;
	size_t size;
	Catena::Block b;
	std::tie(block, size) = b.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.LoadData(block.get(), size, lmap, tstore));
}

// Generate a block with some NoOps, and read it back
TEST(CatenaBlocks, BlockGeneratedNoOps){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	for(auto i = 0 ; i < 4096 ; i += 16){
		Catena::CatenaHash prevhash;
		memset(prevhash.data(), 0xff, prevhash.size());
		Catena::Block b;
		for(auto j = 0 ; j < i + 1 ; ++j){
			b.AddTransaction(std::make_unique<Catena::NoOpTX>());
		}
		EXPECT_EQ(i + 1, b.TransactionCount());
		std::unique_ptr<const unsigned char[]> block;
		size_t size;
		std::tie(block, size) = b.SerializeBlock(prevhash);
		ASSERT_NE(nullptr, block);
		ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
		Catena::Blocks cbs;
		EXPECT_FALSE(cbs.LoadData(block.get(), size, lmap, tstore));
	}
}

// Generate a simple block with invalid prev, and read it back
TEST(CatenaBlocks, BlockGeneratedBadprev){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::CatenaHash prevhash;
	prevhash.fill(1);
	prevhash[0] = 0;
	std::unique_ptr<const unsigned char[]> block;
	size_t size;
	Catena::Block b;
	std::tie(block, size) = b.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, block);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, size);
	Catena::Blocks cbs;
	EXPECT_THROW(cbs.LoadData(block.get(), size, lmap, tstore),
			Catena::BlockHeaderException);
}

// Generate two simple blocks, concatenate them, and read them back
TEST(CatenaBlocks, ChainGenerated){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::CatenaHash prevhash;
	memset(prevhash.data(), 0xff, prevhash.size());
	std::unique_ptr<const unsigned char[]> b1, b2;
	size_t s1, s2;
	Catena::Block blk1, blk2;
	std::tie(b1, s1) = blk1.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, b1);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s1);
	std::tie(b2, s2) = blk2.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, b2);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s2);
	char block[s1 + s2];
	memcpy(block, b1.get(), s1);
	memcpy(block + s1, b2.get(), s2);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.LoadData(block, s1 + s2, lmap, tstore));
	EXPECT_EQ(2, cbs.GetBlockCount());
}

// Generate two simple blocks, and append the second using AppendData
TEST(CatenaBlocks, BlockAppendBlock){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::CatenaHash prevhash;
	memset(prevhash.data(), 0xff, prevhash.size());
	std::unique_ptr<const unsigned char[]> b1, b2;
	size_t s1, s2;
	Catena::Block blk1, blk2;
	std::tie(b1, s1) = blk1.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, b1);
	Catena::Blocks cbs;
	EXPECT_FALSE(cbs.LoadData(b1.get(), s1, lmap, tstore));
	EXPECT_EQ(1, cbs.GetBlockCount());
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s1);
	std::tie(b2, s2) = blk2.SerializeBlock(prevhash);
	ASSERT_NE(nullptr, b2);
	ASSERT_LE(Catena::Block::BLOCKHEADERLEN, s2);
	EXPECT_FALSE(cbs.AppendBlock(b2.get(), s2, lmap, tstore));
	EXPECT_EQ(2, cbs.GetBlockCount());
	EXPECT_EQ(0, cbs.TXCount());
}

TEST(CatenaBlocks, BlockInspectMockLedger){
	Catena::LedgerMap lmap;
	Catena::TrustStore tstore;
	Catena::BuiltinKeys bkeys;
        bkeys.AddToTrustStore(tstore);
	Catena::Blocks cbs;
	ASSERT_FALSE(cbs.LoadFile(MOCKLEDGER, lmap, tstore));
	EXPECT_EQ(MOCKLEDGER_BLOCKS, cbs.GetBlockCount());
	auto i = cbs.Inspect(0, cbs.GetBlockCount());
	ASSERT_EQ(cbs.GetBlockCount(), i.size());
	EXPECT_EQ(1, i[0].transactions.size());
	EXPECT_EQ(1, i[1].transactions.size());
}
