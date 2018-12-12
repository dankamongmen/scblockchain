#include <gtest/gtest.h>
#include "libcatena/sig.h"

#define PUBLICKEY "test/ecdsa.pub"

TEST(CatenaSigs, LoadPubkeyFile){
	Catena::Keypair(PUBLICKEY);
}
