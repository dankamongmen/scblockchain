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
unsigned transaction type.

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
