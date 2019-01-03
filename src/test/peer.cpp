#include <gtest/gtest.h>
#include <libcatena/peer.h>
#include <libcatena/utility.h>

TEST(CatenaPeer, Constructor){
	Catena::Peer peer("127.0.0.1", 40404);
	EXPECT_EQ(peer.Port(), 40404);
	Catena::Peer p1("127.0.0.1:80", 40404);
	EXPECT_EQ(p1.Port(), 80);
}

TEST(CatenaPeer, BadSyntax){
	EXPECT_THROW(Catena::Peer("", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.808.0.1", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("  127.0.0.1", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("m127.0.0.1", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1m", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1 ", 40404), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPort){
	EXPECT_THROW(Catena::Peer("127.0.0.1", 65536), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1", -1), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPortSyntax){
	EXPECT_THROW(Catena::Peer("127.0.0.1:", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:-1", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:65536", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:40404 ", 40404), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1: 40404", 40404), Catena::ConvertInputException);
}
