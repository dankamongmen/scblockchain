#include <gtest/gtest.h>
#include <libcatena/hash.h>
#include <libcatena/tx.h>

TEST(CatenaTransactions, EmptyTX){
	unsigned char hash[HASHLEN];
	memset(hash, 0, sizeof(hash));
	EXPECT_EQ(Catena::Transaction::lexTX({}, 0, hash, 0), nullptr);
	EXPECT_EQ(Catena::Transaction::lexTX({0}, 1, hash, 0), nullptr);
}

static inline const unsigned char *uccast(const char* s){
	return reinterpret_cast<const unsigned char*>(s);
}

TEST(CatenaTransactions, NoOp){
	unsigned char hash[HASHLEN];
	memset(hash, 0, sizeof(hash));
	EXPECT_NE(Catena::Transaction::lexTX(uccast("\x00\x00\x00\x00"), 4, hash, 0), nullptr);
}
