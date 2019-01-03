#include <gtest/gtest.h>
#include <openssl/rand.h>
#include <libcatena/truststore.h>
#include <libcatena/utility.h>
#include <libcatena/builtin.h>
#include <libcatena/hash.h>
#include <libcatena/sig.h>
#include "test/defs.h"

TEST(CatenaTrustStore, BuiltinKeys){
	Catena::BuiltinKeys bkeys;
	Catena::TrustStore tstore;
	bkeys.AddToTrustStore(tstore);
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
	unsigned char sig[SIGLEN];
	Catena::CatenaHash hash;
	hash.fill(0xffu);
	for(t = tests ; t->data ; ++t){
		memcpy(sig, t->sig, sizeof(sig));
		EXPECT_FALSE(tstore.Verify({hash, 0}, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
		// non-existant key
		EXPECT_TRUE(tstore.Verify({hash, 1}, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
		++*sig;
		EXPECT_TRUE(tstore.Verify({hash, 0}, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));
		--*sig;
		EXPECT_FALSE(tstore.Verify({hash, 0}, reinterpret_cast<const unsigned char *>(t->data),
				strlen(t->data), sig, sizeof(sig)));

	}
}

TEST(CatenaTrustStore, CopyConstructorEmpty){
	Catena::TrustStore tstore, tstore1;
	tstore1 = tstore;
	Catena::TrustStore tstore2 = tstore;
	EXPECT_EQ(tstore.PubkeyCount(), tstore2.PubkeyCount());
}

TEST(CatenaTrustStore, CopyConstructorBuiltins){
	Catena::TrustStore tstore, tstore1;
	Catena::BuiltinKeys bkeys;
	bkeys.AddToTrustStore(tstore);
	tstore1 = tstore;
	Catena::TrustStore tstore2 = tstore;
	EXPECT_EQ(tstore.PubkeyCount(), tstore2.PubkeyCount());
}

TEST(CatenaTrustStore, SignNoKeys){
	Catena::TrustStore tstore;
	unsigned char buf[] = {0};
	Catena::KeyLookup kl;
	EXPECT_THROW(tstore.Sign(buf, sizeof(buf), kl), Catena::SigningException);
}

TEST(CatenaTrustStore, SignOnlyBuiltinKeys){
	Catena::TrustStore tstore;
	Catena::BuiltinKeys bkeys;
	bkeys.AddToTrustStore(tstore);
	unsigned char buf[] = {0};
	Catena::KeyLookup kl;
	EXPECT_THROW(tstore.Sign(buf, sizeof(buf), kl), Catena::SigningException);
}

TEST(CatenaTrustStore, SymmetricFail){
	Catena::TrustStore tstore;
	Catena::SymmetricKey key;
	EXPECT_THROW(tstore.Encrypt(nullptr, 0, key), Catena::EncryptException);
	EXPECT_THROW(tstore.Encrypt(nullptr, 256, key), Catena::EncryptException);
}

static const char* tests[] = {
	"", "h", "hello", "thisisfifteenab", "thisissixteenabc",
	"0123456789012345678901234567890", // blocksize - 1 raw, bsize with NUL
	"0123456789012345678901234567891", // bsize raw, bsize + 1 with NUL
	"this one is almost certainly longer than the block size, i think",
	NULL

};

TEST(CatenaTrustStore, SignTestCMKey){
	Catena::TrustStore tstore;
	Catena::Keypair kp(ECDSAKEY);
	Catena::KeyLookup kl;
	RAND_bytes(kl.first.data(), kl.first.size());
	kl.second = 5;
	tstore.AddKey(&kp, kl);
	for(auto t = tests ; *t ; ++t){
		auto sig = tstore.Sign(reinterpret_cast<const unsigned char*>(*t),
					strlen(*t), kl);
		EXPECT_FALSE(tstore.Verify(kl, reinterpret_cast<const unsigned char*>(*t),
					strlen(*t), sig.first.get(), sig.second));
	}
}

TEST(CatenaTrustStore, SymmetricEncryptDecryptString){
	Catena::TrustStore tstore;
	Catena::SymmetricKey key;
	ASSERT_EQ(1, RAND_bytes(key.data(), key.size()));
	for(auto t = tests ; *t ; ++t){
		auto ret = tstore.Encrypt(*t, strlen(*t) + 1, key);
		EXPECT_GE(ret.second, 16); // should have at least an IV
		auto dec = tstore.Decrypt(ret.first.get(), ret.second, key);
		ASSERT_EQ(strlen(*t) + 1, dec.second);
		EXPECT_STREQ(*t, reinterpret_cast<char*>(dec.first.get()));
	}
}

TEST(CatenaTrustStore, SymmetricEncryptDecryptUnterminated){
	Catena::TrustStore tstore;
	Catena::SymmetricKey key;
	ASSERT_EQ(1, RAND_bytes(key.data(), key.size()));
	for(auto t = tests ; *t ; ++t){
		// Encrypt without the NUL terminator, and do a memcmp
		auto ret = tstore.Encrypt(*t, strlen(*t), key);
		EXPECT_GE(ret.second, 16); // should have at least an IV
		auto dec = tstore.Decrypt(ret.first.get(), ret.second, key);
		ASSERT_EQ(strlen(*t), dec.second);
		EXPECT_EQ(0, memcmp(dec.first.get(), *t, dec.second));
		// Secondly, encrypt without the NUL terminator, and do a memcmp
	}
}

// FIXME add standard test vectors to ensure interoperability

TEST(CatenaTrustStore, KeyDerivation){
	Catena::TrustStore tstore;
	Catena::Keypair kv(ECDSAKEY);
	Catena::Keypair peer(reinterpret_cast<const unsigned char*>(ELOOK_TEST_PUBKEY), strlen(ELOOK_TEST_PUBKEY));
	Catena::KeyLookup kl1, kl2;
	RAND_bytes(kl1.first.data(), kl1.first.size());
	RAND_bytes(kl2.first.data(), kl2.first.size());
	kl1.second = kl2.second = 0;
	tstore.AddKey(&kv, kl1);
	tstore.AddKey(&peer, kl2);
	auto key1 = tstore.DeriveSymmetricKey(kl1, kl2);
	auto key2 = tstore.DeriveSymmetricKey(kl2, kl1);
	EXPECT_EQ(key1, key2); // Both sides ought derive the same key
}

// FIXME test KDF against builtin key
