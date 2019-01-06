#include <cstring>
#include <gtest/gtest.h>
#include <libcatena/utility.h>
#include <libcatena/hash.h>
#include <libcatena/tx.h>

TEST(CatenaTransactions, EmptyTX){
	Catena::CatenaHash hash;
	unsigned char buf[1] = {0};
	EXPECT_THROW(Catena::Transaction::LexTX(buf, 0, hash, 0), Catena::TransactionException);
	EXPECT_THROW(Catena::Transaction::LexTX(buf, 1, hash, 0), Catena::TransactionException);
}

static inline const unsigned char *uccast(const char* s){
	return reinterpret_cast<const unsigned char*>(s);
}

TEST(CatenaTransactions, NoOp){
	Catena::CatenaHash hash;
	EXPECT_NE(Catena::Transaction::LexTX(uccast("\x00\x00\x00\x00"), 4, hash, 0), nullptr);
}

TEST(CatenaTransactions, NoOpSerialize){
	const unsigned char expected[] = { 0x0, 0x0 };
	Catena::NoOpTX tx;
	auto r = tx.Serialize();
	ASSERT_EQ(r.second, sizeof(expected));
	EXPECT_EQ(0, memcmp(r.first.get(), expected, sizeof(expected)));
}

TEST(CatenaTransactions, StrToTXSpec){
	auto tx = Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.0");
	EXPECT_EQ(0, tx.second);
	tx = Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.1");
	EXPECT_EQ(1, tx.second);
	tx = Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.00011");
	EXPECT_EQ(11, tx.second);
	tx = Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.4294967295");
	EXPECT_EQ(4294967295, tx.second);

}

TEST(CatenaTransactions, StrToTXSpecBad){
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("0.1"), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff."), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.-1"), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.1 "), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec(" ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.1"), Catena::ConvertInputException);
	EXPECT_THROW(Catena::TXSpec::StrToTXSpec("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.4294967296"), Catena::ConvertInputException);
}
