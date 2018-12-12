#include <gtest/gtest.h>
#include <libcatena/hash.h>

TEST(CatenaHash, SHA256Vectors){
	static const struct {
		const char *data;
		const char *hash;
	} tests[] = {
		{
			.data = "",
			.hash = "\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55",
		}, {
			.data = "The quick brown fox jumps over the lazy dog",
			.hash = "\xd7\xa8\xfb\xb3\x07\xd7\x80\x94\x69\xca\x9a\xbc\xb0\x08\x2e\x4f\x8d\x56\x51\xe4\x6d\x3c\xdb\x76\x2d\x02\xd0\xbf\x37\xc9\xe5\x92",
		}, {
			.data = "Test vector from febooti.com",
			.hash = "\x07\x7b\x18\xfe\x29\x03\x6a\xda\x48\x90\xbd\xec\x19\x21\x86\xe1\x06\x78\x59\x7a\x67\x88\x02\x90\x52\x1d\xf7\x0d\xf4\xba\xc9\xab",
		}, {
			.data = NULL,
			.hash = NULL,
		}
	}, *t;
	for(t = tests ; t->data ; ++t){
		unsigned char h[HASHLEN];
		Catena::catenaHash(t->data, strlen(t->data), h);
		EXPECT_EQ(0, memcmp(h, t->hash, sizeof(h)));
	}
}

TEST(CatenaHash, SHA256Serialize){
	static const struct {
		const char *hash;
		const char *str;
	} tests[] = {
		{
			.hash = "\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55",
			.str = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
		}, {
			.hash = "\xd7\xa8\xfb\xb3\x07\xd7\x80\x94\x69\xca\x9a\xbc\xb0\x08\x2e\x4f\x8d\x56\x51\xe4\x6d\x3c\xdb\x76\x2d\x02\xd0\xbf\x37\xc9\xe5\x92",
			.str = "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
		}, {
			.hash = "\x07\x7b\x18\xfe\x29\x03\x6a\xda\x48\x90\xbd\xec\x19\x21\x86\xe1\x06\x78\x59\x7a\x67\x88\x02\x90\x52\x1d\xf7\x0d\xf4\xba\xc9\xab",
			.str = "077b18fe29036ada4890bdec192186e10678597a67880290521df70df4bac9ab",
		}, {
			.hash = NULL,
			.str = NULL,
		}
	}, *t;
	for(t = tests ; t->hash ; ++t){
		std::stringstream ss;
		Catena::hashOStream(ss, t->hash);
		EXPECT_STREQ(ss.str().c_str(), t->str);
	}
}
