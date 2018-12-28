#include <cstring>
#include <gtest/gtest.h>
#include <libcatena/utility.h>

static const struct {
	unsigned long hbo;
	unsigned char conv[9];
	size_t bytes;
} tests[] = {
	{ 0, "\x00", 1, },
	{ 1, "\x01", 1 },
	{ 1, "\x00\x01", 2 },
	{ 1, "\x00\x00\x00\x00\x00\x00\x00\x01", 8, },
	{ 128, "\x00\x00\x00\x00\x00\x00\x00\x80", 8, },
	{ 255, "\x00\x00\x00\x00\x00\x00\x00\xff", 8, },
	{ 256, "\x01\x00", 2, },
	{ 256, "\x00\x00\x00\x00\x00\x00\x01\x00", 8, },
	{ 1420, "\x05\x8c", 2, },
	{ 1420, "\x00\x05\x8c", 3, },
	{ 1420, "\x00\x00\x05\x8c", 4, },
	{ 65536, "\x01\x00\x00", 3, },
	{ 65536, "\x00\x01\x00\x00", 4, },
	{ 0, "", 0, },
};

TEST(CatenaUtility, ulong_to_nbo){
	for(auto t = tests ; t->bytes ; ++t){
		unsigned char conv[sizeof(t->conv)];
		memset(conv, 0xff, sizeof(conv));
		Catena::ulong_to_nbo(t->hbo, conv, t->bytes);
		EXPECT_EQ(0, memcmp(conv, t->conv, t->bytes));
	}
}

TEST(CatenaUtility, nbo_to_ulong){
	for(auto t = tests ; t->bytes ; ++t){
		EXPECT_EQ(t->hbo, Catena::nbo_to_ulong(t->conv, t->bytes));
	}
}

// Try to read a directory and device as a binary file (expect an exception)
TEST(CatenaUtility, ReadBinaryFileIrregulars){
	size_t len;
	EXPECT_THROW(Catena::ReadBinaryFile("/", &len), std::ifstream::failure);
	EXPECT_THROW(Catena::ReadBinaryFile("/dev/null", &len), std::ifstream::failure);
}

TEST(CatenaUtility, SplitInputEmpty){
	const char* tests[] = {
		"", " ", "  ", "\t", " \t ", "\n\n\t  \v\f "
	};
	for(const auto& t : tests){
		auto tokes = Catena::SplitInput(t);
		EXPECT_EQ(0, tokes.size());
	}
}

TEST(CatenaUtility, SplitInputMonad){
	const char* tests[] = { // each ought just be 'a'
		"a", " a ", " \\a ", "\ta ", "'a'", " 'a' ", "'a' ",
	};
	for(const auto& t : tests){
		const auto tokes = Catena::SplitInput(t);
		EXPECT_EQ(1, tokes.size());
		EXPECT_EQ("a", tokes.back());
	}
}

TEST(CatenaUtility, SplitInputDyad){
	const char* tests[] = { // ought get {"aa", "bb"}
		"aa bb", " aa\tbb ", " \\aa b\\b ", "'aa' 'bb'", " 'aa' 'b''b'"
	};
	for(const auto& t : tests){
		const auto tokes = Catena::SplitInput(t);
		EXPECT_EQ(2, tokes.size());
		EXPECT_EQ("aa", tokes[0]);
		EXPECT_EQ("bb", tokes[1]);
	}
}

TEST(CatenaUtility, SplitEscapedQuote){
	const char* tests[] = { // ought get "'a'a'"
		"'\\'a\\'a\\''", "\\'a\\'a\\'"
	};
	for(const auto& t : tests){
		const auto tokes = Catena::SplitInput(t);
		EXPECT_EQ(1, tokes.size());
		EXPECT_EQ("'a'a'", tokes[0]);
	}
}

TEST(CatenaUtility, SplitUnterminatedQuote){
	const char* tests[] = { // all ought fail
		"'", "'akjajhla", "'\\'", "\\''", "'a", "'' '"
	};
	for(const auto& t : tests){
		const auto tokes = Catena::SplitInput(t);
		EXPECT_EQ(0, tokes.size());
	}
}
