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
* `type`: String containing human-readble transaction type
* optional `sigbytes`: Integer size of the signature in bytes
* optional `signerhash`: String containing hex encoding of 256-bit signer block
* optional `signeridx`: Integer corresponding to signer TX index within block
* optional `pubkey`: String containing PEM-encoded public key
* optional `payload`: payload, dependent on transaction type and subtype
* optional `subtype`: Integer corresponding to transaction subtype
    * ExternalLookup: lookup type

# NewConsortiumMemberTX

Accepted by the `/member` endpoint. Map of strings to T:
* `pubkey`: String containing PEM-encoded public key
* `payload`: JSON payload containing freeform details related to this entity

# NewExternalLookupTX

Accepted by the `/exlookup` endpoint. Map of strings to T:
* `pubkey`: String containing PEM-encoded public key
* `lookuptype`: Integer, lookup type
* `payload`: String payload, dependent on lookuptype

# NewLookupAuthorizationRequestTX

Map of strings to T:
* `exthash`: String containing hex encoding of 256-bit block hash (64 chars)
* `exttxidx`: Integer containing ExternalLookup transaction ID
* `reqhash`: String containing hex encoding of 256-bit block hash (64 chars)
* `reqtxidx`: Integer containing ConsortiumMember transaction ID
* `payload`: JSON payload containing freeform details related to this request

# NewLookupAuthorizationTX

Map of strings to T:
* `payload`: String containing base64-encoded AES-GCM encrypted payload (see below)
* `refhash`: String containing hex encoding of 256-bit block hash (64 chars)
* `reftxidx`: Integer containing LookupAuthorizationRequest transaction ID
* `signature`: String containing PEM-encoded signature of `payload`

The plaintext (binary) payload consists of a PEM-encoded symmetric key,
followed by a 256 bit blockhash, followed by a 32-bit transaction identifier.
This references a published Patient transaction. The symmetric key can be used
to decrypt the Patient payload. This payload is encrypted using a symmetric key
derived from the public keys published in the ConsortiumMember and
ExternalLookup transactions transitively referenced through `refhash` and
`reftxidx`.

# TXRequestResponse

Returned by all POST endpoints. Use the HTTP code to determine whether the
operation was a success or not; this payload is free-form.

* optional `failreason`: String with freeform cause of failure
