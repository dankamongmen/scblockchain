#ifndef CATENA_LIBCATENA_PROTO
#define CATENA_LIBCATENA_PROTO

#include <vector>
#include <capnp/message.h>
#include <capnp/rpc.capnp.h>
#include <capnp/serialize.h>
#include <proto/catena.capnp.h>
#include <libcatena/proto.h>

namespace Catena {

template <typename T, typename F>
std::vector<unsigned char> PrepCall(unsigned method, F fxn) {
  capnp::MallocMessageBuilder builder;
  auto call = builder.initRoot<capnp::rpc::Call>();
  call.setMethodId(method);
  auto pload = call.initParams();
  auto anyp = pload.initContent();
  typename T::Builder targbuilder = anyp.initAs<T>();
  fxn(targbuilder);
  auto words = capnp::messageToFlatArray(builder);
  auto bytes = words.asBytes();
  std::vector<unsigned char> ret;
  ret.reserve(bytes.size());
  ret.insert(ret.end(), bytes.begin(), bytes.end());
  return ret;
}

}

#endif
