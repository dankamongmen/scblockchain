# Catena on-disk block format

A file may have any non-zero number of blocks, so long as those blocks are:

1. Ordered oldest-to-newest,
2. Consecutive (no missing blocks, except at the ends), and
3. Well-formed.

If any of these three restrictions is violated, the entire file is considered
invalid. A full chain may be composed of one or more files. Using multiple
files may improve parallelism during startup processing. There is no padding
between blocks, and there ought be no extraneous data following the last block
(i.e., the entirety of the file must parse as valid blocks).

To be a valid blockchain:

1. One and only one block must be a genesis block, and
2. All other blocks must reference one and only one existing previous block.

All multibyte integer sequences are written in big-endian format.

## Version 0 block format

A block is formed of a header, immediately followed by a data section. The
total size of the header plus the data section may not exceed 16MB-1 (this
value is derived from the 24-bit *totlen* field of the header).

### Version 0 block header

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                              hash                             +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                            hashptr                            +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            version            |             totlen            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |                    txcount                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                              utc                              |
+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |                                               |
+-+-+-+-+-+-+-+-+                                               +
|                                                               |
+                                                               +
|                                                               |
+                            reserved                           +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

The *hash* field is a hash of the entire block, except the *hash* field itself.
The *hashptr* field identifies the previous block by its *hash* field. For the
genesis block, *hashptr* is all zeroes. *version* is currently 0. *totlen* is
the length of the entire block, including the header, in bytes. *txcount* is
the number of transactions in the data section. *utc* is the 40-bit UTC
publication time of the block. *reserved* ought be set to all zeroes.

### Version 0 block data

A table of 32-bit offsets, one per transaction (*txcount* from the header), is
followed by transactions of arbitrary length, each prefixed with a 16-bit
unsigned transaction type. We do not attempt to structure these transactions
with an eye towards alignment, but instead compactness. Lengths are not stored
within the transactions themselves, since they can be computed from the offset
table (transactions ought not have trailing padding).

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Offset_1                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Offset_2                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                              ...                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Offset_n                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

## Transactions

### NewVersion

Transaction type 0x0000, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the signing party, followed by the
signature, followed by a NewVersion JSON payload. Only builtin keys can sign
this message.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0000)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                           signer hash                         +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          signer index                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ...NewVersion payload...                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

The version and utc announced in the transaction payload ought not be less than
or equal to any other published NewVersion transaction, to protect against
replay.

### Consortium Member

Transaction type 0x0001, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the signing party, followed by the
signature. ECDSA SHA256 signatures are 70, 71, or 72 bytes. A signer hash
of all 1s indicates a builtin key. The signature is taken over the DSA public
key (which can later be referred to using the transaction hash+index), and
a freeform JSON-encoded payload.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0001)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                           signer hash                         +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          signer index                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|                ...2 bytes for DSA key length...               |
|                      ...DSA public key...                     |
|                  ...json consortium payload...                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

The JSON payload is free-form. An example payload might be:
```
{
	"Entity": "Headway",
	"WWW": "https://headwayplatform.com/",
	"Contact": "nick@headwayplatform.com",
	"Location": "Atlanta, GA"
}
```

### External Lookup

Transaction type 0x0002, followed by a 16-bit lookup type, followed by the
256-bit hash and 32-bit index of the registering consortium, followed by the
16-bit signature length, followed by the signature, 16-bit public key length,
followed by the public key, followed by the external identifier, of form
determined by the lookup type (the external identifier should only contain
printable characters). Lookup types include:

* 0x0000 Sharecare ID (RFC 4122 UUID)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0002)         |           lookup type         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                    ConsortiumMember hash                      +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ConsortiumMember index                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               ...2 bytes for signature length...              |
|              ...ECDSA signature (70--72 bytes)...             |
|                ...2 bytes for DSA key length...               |
|                      ...DSA public key...                     |
|                   ...external identifier...                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### User

Transaction type 0x0003, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the registering consortium, followed by the
signature, followed by the 16-bit public key length, followed by the public key,
followed by the 128-bit IV, followed by the encrypted payload. The encrypted
payload together with the IV decrypt to a JSON-encoded payload.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0005)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                    ConsortiumMember hash                      +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ConsortiumMember index                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|                ...2 bytes for DSA key length...               |
|                      ...DSA public key...                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                             AES IV                            +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ...encrypted payload...                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### UserStatus

Transaction type 0x0004, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the publishing consortium, followed by the
signature, followed by the 256-bit hash and 32-bit index of the referenced
UserStatusDelegation transaction, followed by the JSON-encoded payload.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0004)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                    ConsortiumMember hash                      +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ConsortiumMember index                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|           ...32 bytes for UserStatusDelegation hash...        |
|           ...4 bytes for UserStatusDelegation txidx...        |
|                    ...json status payload...                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### LookupAuthReq

Transaction type 0x0005, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the requesting consortium, followed by the
signature, followed by the 256-bit hash and 32-bit index of the referenced
ExternalLookup transaction, followed by the JSON-encoded payload.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0005)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                    ConsortiumMember hash                      +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    ConsortiumMember index                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|             ...32 bytes for ExternalLookup hash...            |
|             ...4 bytes for ExternalLookup txidx...            |
|                   ...json request payload...                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### LookupAuth

Transaction type 0x0006, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the LookupAuthReq being authorized,
followed by the signature, followed by the encrypted payload. The encrypted
payload is decrypted via a symmetric key derived from the ConsortiumMember
and ExternalLookup transactions referenced by the signing ExternalLookup
transaction.

The encrypted payload decodes to the 256-bit hash and 32-bit index of the
User transaction being looked up, a 16-bit symmetric key type, and possibly
a symmetric key (usable to decrypt the User transaction's payload). The
current key types are:

* 0x0000 - No symmetric key provided, User TXSpec lookup only
* 0x0001 - 256-bit (32 byte) AES key

Though this transaction bears a LookupAuthReq TXSpec, it is signed by the key
associated with the ExternalLookup TXSpec in that LookupAuthReq. A
LookupAuthReq transaction does not itself publish a public key.


```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0006)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                       LookupAuthReq hash                      +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      LookupAuthReq index                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|                    ...encrypted payload...                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### UserStatusDelegation

Transaction type 0x0007, followed by a 16-bit signature length, followed by
the 256-bit hash and 32-bit index of the delegating user, followed by the
signature, followed by the 256-bit hash and 32-bit index of the authorized
ConsortiumMember transaction, followed by a 32-bit status type, followed by a
subtype-dependent JSON-encoded payload.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         type (0x0007)         |            siglength          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                           User hash                           +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+                                                               +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          User index                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|              ...ECDSA signature (70--72 bytes)...             |
|            ...32 bytes for ConsortiumMember hash...           |
|            ...4 bytes for ConsortiumMember txidx...           |
|                 ...4 bytes for status type...                 |
|                      ...json payload...                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
