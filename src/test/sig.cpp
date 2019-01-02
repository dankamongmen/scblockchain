#include <cstring>
#include <gtest/gtest.h>
#include "libcatena/truststore.h"
#include "libcatena/utility.h"
#include "libcatena/sig.h"
#include "test/defs.h"

TEST(CatenaSigs, LoadECMaterial){
	Catena::Keypair(ECDSAKEY);
}

TEST(CatenaSigs, LoadMemoryInvalid){
	EXPECT_THROW(Catena::Keypair(reinterpret_cast<const unsigned char*>(""), 0), Catena::KeypairException);
	EXPECT_THROW(Catena::Keypair(reinterpret_cast<const unsigned char*>(""), 1), Catena::KeypairException);
}

TEST(CatenaSigs, LoadECMaterialInvalid){ // public key ought not be accepted
	EXPECT_THROW(Catena::Keypair(PUBLICKEY), Catena::KeypairException);
}

TEST(CatenaSigs, ECSignNoPrivateKey){
	Catena::Keypair kv(reinterpret_cast<const unsigned char*>(PUBLICKEY), strlen(PUBLICKEY));
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
	Catena::Keypair kv(ECDSAKEY);
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
	Catena::Keypair kv(ECDSAKEY);
	Catena::Keypair peer(reinterpret_cast<const unsigned char*>(ELOOK_TEST_PUBKEY), strlen(ELOOK_TEST_PUBKEY));
	auto key1 = kv.DeriveSymmetricKey(peer);
	auto key2 = peer.DeriveSymmetricKey(kv);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}

TEST(CatenaSigs, ECDSADeriveKeySingleKey){
	// Shouldn't work with two equal keyspecs, neither with a private key...
	Catena::Keypair kv(reinterpret_cast<const unsigned char*>(PUBLICKEY), strlen(PUBLICKEY));
	Catena::Keypair peer(reinterpret_cast<const unsigned char*>(PUBLICKEY), strlen(PUBLICKEY));
	EXPECT_THROW(peer.DeriveSymmetricKey(kv), Catena::SigningException);
	// But it should work fine once we have a private key
	Catena::Keypair kv2(ECDSAKEY);
	auto key1 = kv2.DeriveSymmetricKey(peer);
	auto key2 = peer.DeriveSymmetricKey(kv2);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}

TEST(CatenaSigs, ECDSADeriveKeyNoPrivates){
	// Shouldn't work with different keyspecs, neither with a private key...
	Catena::Keypair kv(reinterpret_cast<const unsigned char*>(PUBLICKEY), strlen(PUBLICKEY));
	Catena::Keypair peer(reinterpret_cast<const unsigned char*>(ELOOK_TEST_PUBKEY), strlen(ELOOK_TEST_PUBKEY));
	EXPECT_THROW(kv.DeriveSymmetricKey(peer), Catena::SigningException);
	EXPECT_THROW(peer.DeriveSymmetricKey(kv), Catena::SigningException);
}

TEST(CatenaSigs, ECDSADeriveKeyBothPrivates){
	Catena::Keypair kv(ECDSAKEY);
	Catena::Keypair peer(ELOOK_TEST_PRIVKEY);
	auto key1 = kv.DeriveSymmetricKey(peer);
	auto key2 = peer.DeriveSymmetricKey(kv);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}

TEST(CatenaSigs, MergeKeys){
	Catena::Keypair kv(ECDSAKEY);
	Catena::Keypair kv2(reinterpret_cast<const unsigned char*>(PUBLICKEY), strlen(PUBLICKEY));
	auto kv3 = kv2; // exercise copy assignment operator
	EXPECT_TRUE(kv.HasPrivateKey());
	ASSERT_FALSE(kv2.HasPrivateKey());
	ASSERT_FALSE(kv3.HasPrivateKey());
	kv2.Merge(kv); // merge into pubkey-only
	EXPECT_TRUE(kv.HasPrivateKey());
	EXPECT_TRUE(kv2.HasPrivateKey());
	EXPECT_FALSE(kv3.HasPrivateKey());
	kv2.Merge(kv3); // merge pubkey-only into both
	EXPECT_TRUE(kv.HasPrivateKey());
	EXPECT_TRUE(kv2.HasPrivateKey());
	EXPECT_FALSE(kv3.HasPrivateKey());
}
