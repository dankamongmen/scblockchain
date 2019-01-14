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
same node.

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
