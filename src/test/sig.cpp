#include <gtest/gtest.h>
#include "libcatena/sig.h"

#define PUBLICKEY "test/ecdsa.pub"
#define ECDSAKEY "test/ecdsa.pem"

TEST(CatenaSigs, LoadPubkeyFile){
	Catena::Keypair(PUBLICKEY);
}

TEST(CatenaSigs, LoadECMaterialInvalid){
	EXPECT_THROW(Catena::Keypair(PUBLICKEY, PUBLICKEY), std::runtime_error);
}

TEST(CatenaSigs, LoadECMaterial){
	Catena::Keypair(PUBLICKEY, ECDSAKEY);
}
