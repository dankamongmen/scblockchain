@0xc10fa0ff2d1acc7b;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("Catena::Proto");

# Intended for use with Cap'n Proto's RPC protocol (rpc.capnp)

# Method identifiers (capnp::rpc::Call::methodId)
const methodAdvertiseNode  :UInt16 = 1; # uses AdvertiseNode, no return
const methodDiscoverNodes  :UInt16 = 2; # uses void, returns methodAdvertiseNodes
const methodAdvertiseNodes :UInt16 = 3; # uses AdvertiseNodes, no return

struct TLSName {
  subjectCN @0 :Text;
  issuerCN @1 :Text;
}

struct AdvertiseNode {
  name @0 :TLSName;
  ads @1 :List(Text);
}

struct AdvertiseNodes {
  nodes @0 :List(AdvertiseNode);
}
