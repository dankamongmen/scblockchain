#include <gtest/gtest.h>
#include <libcatena/peer.h>
#include <libcatena/utility.h>

static std::shared_ptr<Catena::SSLCtxRAII> GetSSLCTX() {
	return std::make_shared<Catena::SSLCtxRAII>(Catena::SSLCtxRAII(SSL_CTX_new(TLS_method())));
}

TEST(CatenaPeer, Constructor){
	auto sctx = GetSSLCTX();
	Catena::Peer peer("127.0.0.1", 40404, sctx);
	EXPECT_EQ(peer.Port(), 40404);
	Catena::Peer p1("127.0.0.1:80", 40404, sctx);
	EXPECT_EQ(p1.Port(), 80);
}

TEST(CatenaPeer, BadSyntax){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.808.0.1", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("  127.0.0.1", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("m127.0.0.1", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1m", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1 ", 40404, sctx), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPort){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("127.0.0.1", 65536, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1", -1, sctx), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPortSyntax){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("127.0.0.1:", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:-1", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:65536", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:40404 ", 40404, sctx), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1: 40404", 40404, sctx), Catena::ConvertInputException);
}
