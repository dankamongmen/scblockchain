#include <gtest/gtest.h>
#include <libcatena/ledgermap.h>

// Transactions for the same block/idx ought hash equally
TEST(CatenaLedgerMap, TXSpecHashReflexivity){
	Catena::CatenaHash ch;
	ch.fill(0);
	Catena::TXSpec tx1{ch, 0xff}, tx2{ch, 0xff};
	auto h1 = std::hash<Catena::TXSpec>{}(tx1);
	auto h2 = std::hash<Catena::TXSpec>{}(tx2);
	EXPECT_EQ(h1, h2);
}

// Multiple transactions from the same block ought hash to different values
TEST(CatenaLedgerMap, TXSpecHashIDX){
	Catena::CatenaHash ch;
	ch.fill(0);
	Catena::TXSpec tx1(ch, 0), tx2{ch, 1};
	auto h1 = std::hash<Catena::TXSpec>{}(tx1);
	auto h2 = std::hash<Catena::TXSpec>{}(tx2);
	EXPECT_NE(h1, h2);
}

// Transactions from blocks with different LSBs ought hash to different values
TEST(CatenaLedgerMap, TXSpecHashBlock){
	Catena::CatenaHash ch;
	ch.fill(0);
	Catena::TXSpec tx1{ch, 0};
	ch.fill(0xff);
	Catena::TXSpec tx2{ch, 0};
	auto h1 = std::hash<Catena::TXSpec>{}(tx1);
	auto h2 = std::hash<Catena::TXSpec>{}(tx2);
	EXPECT_NE(h1, h2);
}
