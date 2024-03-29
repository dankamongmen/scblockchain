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
