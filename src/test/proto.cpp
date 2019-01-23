#include <gtest/gtest.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <proto/catena.capnp.h>

TEST(CatenaProto, AdvertiseNode){
  ::capnp::MallocMessageBuilder message;
  auto nodeAd = message.initRoot<Catena::Proto::AdvertiseNode>();
  auto ads = nodeAd.initAds(1);
  ads.set(0, "127.0.0.1:40404");
  char tmpname[128] = "catenatest.XXXXXX";
  int fd = mkstemp(tmpname);
  ASSERT_GT(fd, 0);
  unlink(tmpname);
  capnp::writeMessageToFd(fd, message);
  ASSERT_EQ(0, lseek(fd, 0, SEEK_SET));
  auto rmessage = capnp::StreamFdMessageReader(fd);
  auto rnodeAd = rmessage.getRoot<Catena::Proto::AdvertiseNode>();
  EXPECT_EQ(1, rnodeAd.getAds().size());
  close(fd);
}
