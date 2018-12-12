#include <gtest/gtest.h>
#include "libcatena/tx.h"

TEST(CatenaTransactions, EmptyTX){
	EXPECT_EQ(Catena::Transaction::lexTX({}, 0), nullptr);
	EXPECT_EQ(Catena::Transaction::lexTX({0}, 1), nullptr);
}

static inline const unsigned char *uccast(const char* s){
	return reinterpret_cast<const unsigned char*>(s);
}

TEST(CatenaTransactions, NoOp){
	EXPECT_NE(Catena::Transaction::lexTX(uccast("\x00\x00\x00\x00"), 4), nullptr);
}

TEST(CatenaTransactions, NoOpInvalid){
	// too short
	EXPECT_EQ(Catena::Transaction::lexTX(uccast("\x00\x00\x00"), 3), nullptr);
	// too long
	EXPECT_EQ(Catena::Transaction::lexTX(uccast("\x00\x00\x00\x00\x00"), 5), nullptr);
	// bad noop type
	EXPECT_EQ(Catena::Transaction::lexTX(uccast("\x00\x00\x00\x01"), 4), nullptr);
}
