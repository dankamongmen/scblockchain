#include <gtest/gtest.h>
#include "libcatena/builtin.h"

TEST(CatenaBuiltin, LoadKeys){
	Catena::BuiltinKeys bkeys;
}

TEST(CatenaBuiltin, ECSigVerify){
	static const struct {
		const char *data;
		const unsigned char sig[SIGLEN];
	} tests[] = {
		{
			.data = "",
			.sig = {
  0x30, 0x46, 0x02, 0x21, 0x00, 0xdb, 0x1d, 0x46, 0x88, 0xa4, 0x80, 0x7f,
  0xc4, 0x76, 0xe8, 0xe7, 0x5c, 0x87, 0x6a, 0x9e, 0x95, 0x70, 0xae, 0x42,
  0xf5, 0x1f, 0xa1, 0x11, 0x8f, 0xc9, 0x82, 0xed, 0x30, 0x0c, 0xf8, 0x1b,
  0xa7, 0x02, 0x21, 0x00, 0xa0, 0x8a, 0xd7, 0xbf, 0x3c, 0x44, 0xf0, 0xe5,
  0xee, 0x4c, 0xcc, 0x3c, 0x89, 0x0d, 0xed, 0x4a, 0x70, 0x5b, 0xb9, 0xb5,
  0xf2, 0xbb, 0x04, 0xad, 0x17, 0x78, 0xf1, 0x04, 0xe0, 0xce, 0x9e, 0xad,
			},
		}, {
			.data = NULL,
			.sig = { 0 },
		}
	}, *t;
	Catena::BuiltinKeys bkeys;
	unsigned char sig[SIGLEN];
	for(t = tests ; t->data ; ++t){
		memcpy(sig, t->sig, sizeof(sig));
		EXPECT_FALSE(bkeys.Verify(0, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
		EXPECT_THROW(bkeys.Verify(1, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)), Catena::SigningException);
		++*sig;
		EXPECT_TRUE(bkeys.Verify(0, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
		--*sig;
		EXPECT_FALSE(bkeys.Verify(0, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
	}
}
