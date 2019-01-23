@0xc10fa0ff2d1acc7b;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("Catena::Proto");

# Intended for use with Cap'n Proto's RPC protocol (rpc.capnp)

# Method identifiers (capnp::rpc::Call::methodId)
const methodAdvertiseNode :UInt16 = 1; # uses AdvertiseNode, no return

struct AdvertiseNode {
  ads @0 :List(Text);  
}
