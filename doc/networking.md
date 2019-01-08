# Catena P2P networking

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
