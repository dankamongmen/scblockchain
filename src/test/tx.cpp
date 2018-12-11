#include <gtest/gtest.h>
#include "libcatena/tx.h"

TEST(CatenaTransactions, CatenaEmptyTX){
	CatenaTX tx;
	EXPECT_TRUE(tx.extract("", 0));
	EXPECT_TRUE(tx.extract("\x00", 1));
}

TEST(CatenaTransactions, CatenaNoOp){
	CatenaTX tx;
	EXPECT_FALSE(tx.extract("\x00\x00", 2));
}

TEST(CatenaTransactions, CatenaNoOpInvalid){
	CatenaTX tx;
	EXPECT_TRUE(tx.extract("\x00\x00\x00", 3)); // too long
}
