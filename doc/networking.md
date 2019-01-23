# Catena P2P networking

Catena will not set up a P2P network unless the `-r` option is given to specify
a TCP port on which to listen. The standard Catena RPC port is 40404. When
configured to use P2P RPCs, Catena will accept connections on this port, and
attempt to establish connections to peers.

Catena P2P uses TLSv1.3, with certificate validation on both sides of each
connection. All nodes in a Catena network must be configured with a certificate
and key signed by some CA signed by the distributed network CA (Catena *does
not* use the host's default CA store). A node's "name" for the purposes of
Catena P2P networking is the pair of X509v3 Common Names from its Issuer and
Subject fields, and each node's name should be different.

Thus, a network of:
* Node 1, IssuerCN: TestCorp CA, SubjectCN: testnode1
* Node 2, IssuerCN: TestCorp CA, SubjectCN: testnode2
* Node 3, IssuerCN: TestCorp2 CA, SubjectCN: testnode1

is valid, but a network of:
* Node 1, IssuerCN: TestCorp CA, SubjectCN: TestCorp
* Node 2, IssuerCN: TestCorp CA, SubjectCN: TestCorp

is questionable, since a node will not maintain more than one connection to a
given *name*, even if the addresses are different.

Use of `-r` requires `-C` and `-v`, specifying a certificate chain and private
key respectively. The chain file ought consist of the node's certificate,
followed by the signing certificates, up to the network-wide root. On most
deployments, this implies a total of 4 PEM certificates (see
[below](#Public-key-infrastructure)).

A node is uniquely identified by its name. Names are strongly decoupled from
addresses and hostnames: a node connecting to an address, and finding an
unexpected name there, must not consider this an error. The address from which
a node receives a connection must not be interpreted as a connectable address
for the peer. Peer rejection must not be based off whether the peer address has
been advertised. This allows for maximum flexibility in node addressing and
advertising.

## Peer configuration and discovery

Catena nodes are advertised as collections of endpoints. Endpoints are IPv4
addresses, IPv6 addresses, or DNS names which ought resolve to collections of
IP addresses, and a port. When connecting to an advertised node, the order in
which endpoints will be tried is undefined, but all endpoints will be tried
unless one succeeds. Whether they will be tried serially or in parallel is
undefined. It is expected that all the endpoints in an advertisement reach the
same node. Such a collection corresponds to the NodeAdvertisement protobuf.

When interacting with Catena, an advertisement is a comma-delimited set of
one or more endpoints. If an address/name is followed by a colon and port
number, that port number is used. The default port is otherwise used. Note that
ports are per-endpoint.

Catena may be provided a file using the `-P` option. This file should contain
an advertisement on each line. Peers thus provided are considered "configured
peers", and always kept in the peer list (as opposed to "discovered peers",
which can fall out).

If the node wishes to be discoverable, it must advertise addresses by which it
can be reached. This advertisment is provided with the `-A` option. Multiple
`-A` arguments will be grouped into a single advertisement. If `-A` is not
provided with `-r`, the node can still be connected to, but will not be
advertised.

When responding to PeerDiscovery RPCs, the node can include an advertisement
for itself. Likewise, the node can send a NodeAdvertisement one-way RPC to
advertise itself to a connected peer. Peers may do whatever they like with an
advertisement. It is considered bad form to send the same NodeAdvertisement
more than once on a connection.

NodeAdvertisements are currently timestamped with the node startup time, and it
is thought that they might one day be named and signed.

### P2P connections

Each node has an artificial maximum number K of established connections. In a
network of N nodes, it is expected that K &lt; N. We'd like to maintain the
following properties:

1. The network is connected, i.e., there are no isolated subnetworks.
2. The pathlength between any two nodes is minimized, which seems related to
minimizing clustering coefficient across the graph.
3. No two nodes have more than one active connection between them, even if one
or both nodes advertise multiple addresses.
4. Nodes ought be able to move among addresses, even ones recently occupied by
other nodes, without artificial latency.
5. Connections ought not "flap" without true benefits to the network.

If the network is disconnected, partitioned subnetworks will build different
ledgers. When the subnetworks reconnect, only one ledger can persist, and any
transactions on discarded ledgers are effectively lost. Minimized pathlengths
imply minimized steps for a broadcasted transaction to reach the entire
network (at the possible expense of some wasted bandwidth). It does not make
sense for two nodes to maintain more than one connection.

It is essential that the network must not evolve to a state where two
subnetworks are statically saturated (i.e. each node has K connections, all
within the subnetwork). Such subnetworks will never reconnect. This requirement
drives the peer connection selection algorithm. Nodes ought prefer connections
that lead to lower max pathlengths. Doing so perfectly requires perfect
knowledge of the network, so this decision is made in the context of connected
peers only.

Two connections between nodes N1 and N2 could arise several ways:

1. N1 and N2 could truly simultaneously establish connections to one another.
2. N1, already connected to N2, could establish a new connection to an
 advertised address, and discover N2 on the other side.
3. N2, already having accepted a conn from N1, could establish a new connection
 to an advertised address, and discover N1 on the other side.
4. N1, already connected to N2, restarts silently. It connects to N2, which
 thinks it already has a connection from N1.
5. N2, already having accepted a conn from N1, restarts silently. It connects
 to N1, which thinks it already has a connection to N2.

Cases 1, 4, and 5 imply that we cannot simply "choose the older (or newer)
connection". If we choose the older connection, one side might have an older
connection which no longer exists on the other side. If we choose the newer
connection, two peers can "flap", establishing and tearing down connections to
one another (remember that a node doesn't know what node it's connecting to,
only some advertised addresses). In the event of simultaneous connections,
there might not even be an "older" connection, and the nodes are not guaranteed
to see the same ordering in any case.

There is a natural symmetry break provided by TLS: the ServerCertificate message
is sent and verified prior to the ClientCertificate message. Upon receiving the
peer cert, a node discovers that peer's name, and can make meaningful decisions.
After verifying the ServerCertificate, N1 (having connected to N2) should:

1. If name(N2) == name(N1), disconnect
2. If N1 already has a connection to name(N2), the new connection is
undesirable, *unless* N2 has forgotten the connection. N1 should disconnect the
new session immediately, and ping N2 on the old connection, triggering a short
timeout. If N2 is indeed a dead connection, it will quickly expire, and a new
connection can arise to N2. It is true (see below) that N2 would be likely to
disconnect the newer connection itself assuming that the old connection is
indeed valid, but in the meantime, what should N1 do with two open connections?
3. If N1 already has a connection from name(N2), only one of the two connections
is desirable, but the two sides need agree on which one. With different clocks,
connection age is not a metric. Instead, we derive the desired direction between
two peers:
    * Hash name(N1) and name(N2), generating B-bit hashes H(N1), H(N2)
    * Add H(N1) and H(N2) modulo 2ⁿ and take the result's parity
    * If even, the name having the lesser hash is deemed the client
    * If odd, the name having the lesser hash is deemed the server
If N1 is decided to be the client by this procedure, it ought disconnect the
existing connection from name(N2), and continue with its new connection. It can
expect the server to make the same decision (i.e. if N2 receives N1's
ClientCertificate on the new connection before receiving the shutdown of the
old connection, it will perform its own shutdown of the old connection, and let
the new one through). If the new connection is torn down, N1 ought ping N2 on
the old connection, triggering a short timeout (for the same reason as in 2
above).
We do the parity operation rather than e.g. simple lexicographic comparison to
break a complete ordering on names, which would otherwise bias the network (over
time, the lexicographically extreme nodes would be all-client or all-server). We
compare hashes for the same reason. We hash because otherwise structure in name
distribution would be reflected in our partial ordering. All three are necessary
to deliver a deterministic relation without global bias.
4. The new connection should be considered established.

At this point, N1 sends a ClientCertificate message to N2 (having accepted the
connection from N1). N2 should:

1. If name(N1) == name(N2), disconnect (ought already have happened)
2. If N2 already has a connection from name(N1), the new connection is
undesirable, *unless* N1 has forgotten the old connection. As in 2 above, N2
ought disconnect the new session, and ping N1 on the old one. If we could
assume that N1 wouldn't send its ClientCertificate early, and would only do so
after having verified that it has no existing connection to N2, N2 could
prefer the newer connection, and kill the old. We have chosen not to exploit
this assumption, as TLS does not seem to require it.
3. If N2 already has a connection to name(N1), the situation is exactly the
same as 3 above. Hash the names to break symmetry, establish a bias, disconnect
the undesirable connection, and ping on the old one if appropriate.

## Public key infrastructure

Catena nodes employ a 4-level PKI. At the top is the self-signed, long-lived
HCN root CA, which is kept offline by Headway. It is used only to sign
intermediate CA certificates. This certificate forms the trust anchor, and is
distributed with Catena releases.

At the second level are intermediate HCN CA certificates. These are signed by
the HCN root CA. At any given time, it is expected that either one or two
intermediate CAs are valid. A new intermediate CA is created well prior to
the active intermediate CA's expiry, and distributed with new Catena releases.
They are kept offline by Headway, but brought out of cold storage more often
than the root key.

At the third level are consortium member CA certificates. These are signed by
one of the active intermediate HCN CA certificates, and their lifetime is tied
to that of the signing certificate. These CAs are owned by the consortium
members. They are not distributed with the Catena releases. Clients connecting
to P2P peers ought transmit the consortium member CA cert in thier ClientHello.

At the fourth level are node certificates, one per node. These are signed by
their operating consortium member CA, and their lifetime is tied to that of
the signing certificate. These certs are owned by the consortium members. They
are not distributed with the Catena releases.

Catena requires a certificate chain file to start RPC service. This chain file
must begin with the node's certificate, and include each subsequent certificate,
terminating with the root. When a node connects to a peer, it must offer its
own certificate, and its signing consortium member's certificate. For
compatibility with all network nodes, it ought also offer the second-level
intermediate CA certificate, but this is not enforced. The root CA certificate
need never be offered, as all peers are guaranteed to have it. Catena clients
*must not* trust the host system's CA store for consortium mode operation,
instead accepting only certificates terminating in the HCN root CA.

## RPC protocol

Each message is a 64-bit NBO length in bytes, followed by a Cap'n Proto
rpc::Call message of that size. See src/proto/rpc.capnp for details.
