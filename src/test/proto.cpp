#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <proto/catena.capnp.h>
#include <test/defs.h>

static capnp::MallocMessageBuilder&
AssembleAdvertiseNode(capnp::MallocMessageBuilder& message) {
  auto nodeAd = message.initRoot<Catena::Proto::AdvertiseNode>();
  auto name = nodeAd.initName();
  name.setSubjectCN(TEST_SUBJECT_CN);
  name.setIssuerCN(TEST_ISSUER_CN);
  auto ads = nodeAd.initAds(1);
  ads.set(0, "127.0.0.1:40404");
  return message;
}

TEST(CatenaProto, AdvertiseNodeFd){
  capnp::MallocMessageBuilder message;
  AssembleAdvertiseNode(message);
  char tmpname[128] = "catenatest.XXXXXX";
  int fd = mkstemp(tmpname);
  ASSERT_GT(fd, 0);
  unlink(tmpname);
  capnp::writeMessageToFd(fd, message);
  ASSERT_EQ(0, lseek(fd, 0, SEEK_SET));
  auto rmessage = capnp::StreamFdMessageReader(fd);
  auto rnodeAd = rmessage.getRoot<Catena::Proto::AdvertiseNode>();
  EXPECT_EQ(1, rnodeAd.getAds().size());
  const auto rnodeName = rnodeAd.getName();
  EXPECT_STREQ(TEST_SUBJECT_CN, rnodeName.getSubjectCN().cStr());
  EXPECT_STREQ(TEST_ISSUER_CN, rnodeName.getIssuerCN().cStr());
  close(fd);
}

TEST(CatenaProto, AdvertiseNodeFlatArray){
  capnp::MallocMessageBuilder message;
  AssembleAdvertiseNode(message);
  kj::Array<capnp::word> words = capnp::messageToFlatArray(message);
  capnp::FlatArrayMessageReader reader(words);
  auto rnodeAd = reader.getRoot<Catena::Proto::AdvertiseNode>();
  EXPECT_EQ(1, rnodeAd.getAds().size());
}
