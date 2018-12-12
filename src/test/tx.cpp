#include <gtest/gtest.h>
#include "libcatena/tx.h"

TEST(CatenaTransactions, EmptyTX){
	Catena::Transaction tx;
	EXPECT_TRUE(tx.extract({}, 0));
	EXPECT_TRUE(tx.extract({0}, 1));
}

static inline const unsigned char *uccast(const char* s){
	return reinterpret_cast<const unsigned char*>(s);
}

TEST(CatenaTransactions, NoOp){
	Catena::Transaction tx;
	EXPECT_FALSE(tx.extract(uccast("\x00\x00\x00\x00"), 4));
}

TEST(CatenaTransactions, NoOpInvalid){
	Catena::Transaction tx;
	EXPECT_TRUE(tx.extract(uccast("\x00\x00\x00"), 3)); // too short
	EXPECT_TRUE(tx.extract(uccast("\x00\x00\x00\x00\x00"), 5)); // too long
}
