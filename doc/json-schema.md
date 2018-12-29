# Catena JSON schemata

# InspectResult

Returned by the `/inspect` endpoint. Array of BlockDetails structures.

## BlockDetails

Map of strings to T:
* `version`: Integer corresponding to block spec version
* `transactions`: Array of Transaction structures
* `hash`: String containing hex encoding of 256-bit block hash (64 chars)
* `prev`: String containing hex encoding of 256-bit block hash (64 chars)
* `utc`: Integer block creation UTC timestamp
* `bytes`: Integer size of the block in bytes

`prev` is included so that the genesis block can always be distinguished by its
previous hash of all ones, and so that the previous hash is available when only
a partial range of blocks are being detailed.

## Transaction

Map of strings to T:
* `type`: Integer transaction type
* optional `sigbytes`: Integer size of the signature in bytes
* optional `signerhash`: String containing hex encoding of 256-bit signer block
* optional `signeridx`: Integer corresponding to signer TX index within block
* optional `pubkey`: String containing PEM-encoded public key
* optional `payload`: JSON payload, dependent on transaction type

# ConsortiumMemberTXRequest

# ExternalLookupTXRequest

# TXRequestResponse

Returned by all POST endpoints. Use the HTTP code to determine whether the
operation was a success or not; this payload is free-form.

* optional `failreason`: String with freeform cause of failure
