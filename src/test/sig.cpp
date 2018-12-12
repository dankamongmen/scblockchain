#include <cstring>
#include <gtest/gtest.h>
#include "libcatena/sig.h"

#define PUBLICKEY "test/ecdsa.pub"
#define ECDSAKEY "test/ecdsa.pem"

TEST(CatenaSigs, LoadPubkeyFile){
	Catena::Keypair(PUBLICKEY);
}

TEST(CatenaSigs, LoadPubkeyFileInvalid){
	EXPECT_THROW(Catena::Keypair(ECDSAKEY), std::runtime_error);
}

TEST(CatenaSigs, LoadECMaterialInvalid){
	EXPECT_THROW(Catena::Keypair(PUBLICKEY, PUBLICKEY), std::runtime_error);
}

TEST(CatenaSigs, LoadECMaterial){
	Catena::Keypair(PUBLICKEY, ECDSAKEY);
}

TEST(CatenaSigs, ECSign){
	static const struct {
		const char *data;
	} tests[] = {
		{
			.data = "",
		}, {
			.data = "The quick brown fox jumps over the lazy dog",
		}, {
			.data = "Test vector from febooti.com",
		}, {
			.data = NULL,
		}
	}, *t;

	Catena::Keypair kv(PUBLICKEY, ECDSAKEY);
	for(t = tests ; t->data ; ++t){
		unsigned char sig[SIGLEN];
		memset(sig, 0, sizeof(sig));
		size_t siglen = kv.Sign(reinterpret_cast<const unsigned char*>(t->data),
				strlen(t->data), sig, sizeof(sig));
		ASSERT_LT(0, siglen);
		EXPECT_FALSE(kv.Verify(reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, siglen));
	}
}
