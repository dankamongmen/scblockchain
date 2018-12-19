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
