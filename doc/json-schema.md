# Catena JSON schemata

# InspectResult

Returned by the `/inspect` endpoint. Array of BlockDetails structures.

## BlockDetails

Map of strings to T:
* `version`: Integer corresponding to block spec version
* `transactions`: Array of TransactionDetails structures
* `hash`: String containing hex encoding of 256-bit block hash (64 chars)
* `prev`: String containing hex encoding of 256-bit block hash (64 chars)
* `utc`: Integer block creation UTC timestamp
* `bytes`: Integer size of the block in bytes

`prev` is included so that the genesis block can always be distinguished by its
previous hash of all ones, and so that the previous hash is available when only
a partial range of blocks are being detailed.

## TransactionDetails

Map of strings to T:
* `type`: String containing human-readble transaction type
* optional `sigbytes`: Integer size of the signature in bytes
* optional `signerspec`: String containing TXSpec of signer
* optional `pubkey`: String containing PEM-encoded public key
* optional `subtype`: Integer corresponding to transaction subtype
* optional `subjectspec`: String containing TXSpec of subject
* optional `payload`: payload, dependent on transaction type and subtype
* optional `envpayload`: String containing hex encoding of encrypted payload

`payload` and `encpayload` are mutually exclusive.

# UserStatusResult

Returned by the `/ustatus` endpoint. Freeform JSON payload.

# NewConsortiumMemberTX

Accepted by the `/member` endpoint. Map of strings to T:
* `pubkey`: String containing PEM-encoded public key
* `regspec`: String containing TXSpec of registering ConsortiumMember
* `payload`: JSON payload containing freeform details related to this entity
* optional `privkey`: String containing PEM-encoded private key for `regspec`

This will be signed using the consortium key referenced by `regspec`,
which must have its private key loaded in the catena agent if not supplied.

# NewExternalLookupTX

Accepted by the `/exlookup` endpoint. Map of strings to T:
* `pubkey`: String containing PEM-encoded public key (user lookup key)
* `regspec`: String containing TXSpec of registering ConsortiumMember
* `lookuptype`: Integer, lookup type
* `payload`: String payload, dependent on lookuptype
* optional `privkey`: String containing PEM-encoded private key for `regspec`

This will be signed using the consortium key referenced by `regspec`,
which must have its private key loaded in the catena agent if not supplied.

# NewLookupAuthorizationRequestTX

Map of strings to T:
* `extspec`: String containing TXSpec of requested ExternalLookup
* `reqspec`: String containing TXSpec of requesting ConsortiumMember
* `payload`: JSON payload containing freeform details related to this request
* optional `privkey`: String containing PEM-encoded private key for `reqspec`

This will be signed using the consortium key referenced by `reqspec`,
which must have its private key loaded in the catena agent if not supplied.

# NewLookupAuthorizationTX

Map of strings to T:
* `refspec`: String containing TXSpec of subject LookupAuthReq
* `uspec`: String containing TXSpec of unveiled User
* `symkey`: String containing hex-encoded AES key used to encrypt User payload
* optional `privkey`: String containing PEM-encoded private key for `refspec`

This will be signed using the patient lookup key referenced by `refspec`,
which must have its private key loaded in the catena agent if not supplied.

# NewUserTX

Map of strings to T:
* `pubkey`: String containing PEM-encode public key (user delegation auth key)
* `regspec`: String containing TXSpec of registering ConsortiumMember
* `payload`: JSON payload containing freeform details related to this request
(encrypted before being written to the ledger)
* optional `privkey`: String containing PEM-encoded private key for `regspec`

This will be signed using the consortium key specified by `regspec`, which must
have its private key loaded in the catena agent if not supplied.

## NewUserTXResponse

Map of strings to T:
* `symkey`: String containing hex-encoded AES key used to encrypt User payload.

# NewUserDelegationTX

Map of strings to T:
* `statustype`: Integer identifying the type of status being delegated
* `delspec`: String containing TXSpec of delegated ConsortiumMember
* `uspec`: String containing TXSpec of delegating User
* `payload`: JSON payload containing freeform details related to this request
* optional `privkey`: String containing PEM-encoded private key for `uspec`

This will be signed using the delegation authentication key specified by
`uspec`, which must have its private key loaded in the catena agent if not
supplied.

# NewUserStatusTX
Map of strings to T:
* `usdspec`: String containing TXSpec of authorizing UserStatusDelegation
* `payload`: JSON payload containing freeform details related to this request.
* optional `privkey`: String containing PEM-encoded private key for `usdspec`

This will be signed using the consortium key referenced by `usdspec`,
which must have its private key loaded in the catena agent if not supplied.

# TXRequestResponse

Returned by all POST endpoints. Use the HTTP code to determine whether the
operation was a success or not; this payload is free-form.

* optional `failreason`: String with freeform cause of failure
