#include <gtest/gtest.h>
#include <libcatena/peer.h>
#include <libcatena/utility.h>

TEST(CatenaPeer, PeerBadPort){
	EXPECT_THROW(Catena::Peer("127.0.0.1", 65536), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1", -1), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPortSyntax){
	EXPECT_THROW(Catena::Peer("127.0.0.1:", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:-1", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:65536", 40404), Catena::ConvertInputException);
}
