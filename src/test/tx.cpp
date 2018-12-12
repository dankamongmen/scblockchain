#include <gtest/gtest.h>
#include "libcatena/tx.h"

TEST(CatenaTransactions, CatenaEmptyTX){
	Catena::Transaction tx;
	EXPECT_TRUE(tx.extract({}, 0));
	EXPECT_TRUE(tx.extract({0}, 1));
}

static inline const unsigned char *uccast(const char* s){
	return reinterpret_cast<const unsigned char*>(s);
}

TEST(CatenaTransactions, CatenaNoOp){
	Catena::Transaction tx;
	EXPECT_FALSE(tx.extract(uccast("\x00\x00\x00\x00"), 4));
}

TEST(CatenaTransactions, CatenaNoOpInvalid){
	Catena::Transaction tx;
	EXPECT_TRUE(tx.extract(uccast("\x00\x00\x00"), 3)); // too short
	EXPECT_TRUE(tx.extract(uccast("\x00\x00\x00\x00\x00"), 5)); // too long
}
