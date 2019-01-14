#include <gtest/gtest.h>
#include <libcatena/rpc.h>
#include <libcatena/peer.h>
#include <libcatena/utility.h>

static std::shared_ptr<Catena::SSLCtxRAII> GetSSLCTX() {
	return std::make_shared<Catena::SSLCtxRAII>(Catena::SSLCtxRAII(SSL_CTX_new(TLS_method())));
}

TEST(CatenaPeer, Constructor){
	auto sctx = GetSSLCTX();
	Catena::Peer peer("127.0.0.1", Catena::DefaultRPCPort, sctx, true);
	EXPECT_EQ(peer.Port(), Catena::DefaultRPCPort);
	Catena::Peer p1("127.0.0.1:80", Catena::DefaultRPCPort, sctx, true);
	EXPECT_EQ(p1.Port(), 80);
}

TEST(CatenaPeer, BadSyntax){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.808.0.1", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("  127.0.0.1", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("m127.0.0.1", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1m", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1 ", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPort){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("127.0.0.1", 65536, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1", -1, sctx, true), Catena::ConvertInputException);
}

TEST(CatenaPeer, PeerBadPortSyntax){
	auto sctx = GetSSLCTX();
	EXPECT_THROW(Catena::Peer("127.0.0.1:", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:-1", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:65536", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1:Catena::DefaultRPCPort ", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
	EXPECT_THROW(Catena::Peer("127.0.0.1: Catena::DefaultRPCPort", Catena::DefaultRPCPort, sctx, true), Catena::ConvertInputException);
}
