#include <cstring>
#include <gtest/gtest.h>
#include "libcatena/truststore.h"
#include "libcatena/utility.h"
#include "libcatena/sig.h"
#include "test/defs.h"

TEST(CatenaSigs, LoadPubkeyFile){
	Catena::Keypair(PUBLICKEY);
}

TEST(CatenaSigs, LoadECMaterial){
	Catena::Keypair(PUBLICKEY, ECDSAKEY);
}

TEST(CatenaSigs, LoadMemoryInvalid){
	EXPECT_THROW(Catena::Keypair(reinterpret_cast<const unsigned char*>(""), 0), Catena::KeypairException);
	EXPECT_THROW(Catena::Keypair(reinterpret_cast<const unsigned char*>(""), 1), Catena::KeypairException);
}

TEST(CatenaSigs, LoadPubkeyFileInvalid){
	EXPECT_THROW(Catena::Keypair(ECDSAKEY), Catena::KeypairException);
}

TEST(CatenaSigs, LoadECMaterialInvalid){
	EXPECT_THROW(Catena::Keypair(PUBLICKEY, PUBLICKEY), Catena::KeypairException);
	EXPECT_THROW(Catena::Keypair(ECDSAKEY, ECDSAKEY), Catena::KeypairException);
}

TEST(CatenaSigs, ECSignNoPrivateKey){
	Catena::Keypair kv(PUBLICKEY);
	unsigned char sig[SIGLEN] = {0};
	EXPECT_THROW(kv.Sign(reinterpret_cast<const unsigned char*>(""),
				0, sig, sizeof(sig)), Catena::SigningException);
	EXPECT_THROW(kv.Sign(reinterpret_cast<const unsigned char*>(""), 0),
				Catena::SigningException);
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
		++*sig;
		EXPECT_TRUE(kv.Verify(reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, siglen));
	}
}

// FIXME add some external test vectors to ensure interoperability

TEST(CatenaSigs, ECDSADeriveKey){
	Catena::Keypair kv(PUBLICKEY, ECDSAKEY);
	Catena::Keypair peer(ELOOK_TEST_PUBKEY);
	auto key1 = kv.DeriveSymmetricKey(peer);
	auto key2 = peer.DeriveSymmetricKey(kv);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}

TEST(CatenaSigs, ECDSADeriveKeyNoPrivates){
	Catena::Keypair kv(PUBLICKEY);
	Catena::Keypair peer(ELOOK_TEST_PUBKEY);
	EXPECT_THROW(kv.DeriveSymmetricKey(peer), Catena::SigningException);
	EXPECT_THROW(peer.DeriveSymmetricKey(kv), Catena::SigningException);
}

TEST(CatenaSigs, ECDSADeriveKeyBothPrivates){
	Catena::Keypair kv(PUBLICKEY, ECDSAKEY);
	Catena::Keypair peer(ELOOK_TEST_PUBKEY, ELOOK_TEST_PRIVKEY);
	auto key1 = kv.DeriveSymmetricKey(peer);
	auto key2 = peer.DeriveSymmetricKey(kv);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}
