# catena - a distributed ledger for health information

## Building

The default target is `all`, which will build all binaries and docs. To run
unit tests, use the `test` target, which will build any necessary dependencies.

### Build requirements

Required version numbers indicate the minimal version tested. Older versions
might or might not work.

* C++ compiler with C++14 support
    * Tested with clang++ 7.0.1 and g++ 8.2.0
* GNU Make 4.2.1+
* Google Test 1.8.1+ (libgtest-dev)
* OpenSSL 1.1+ (libopenssl-dev) (older versions *probably will not* work)
* GNU Libmicrohttpd 0.9.62+ (libmicrohttpd-dev)
* GNU Readline 6.3+ (libreadline-dev)
* JSON for Modern C++ 3.4.0+ (nlohmann-json3-dev)

External projects (see the `ext/` directory) include:

* nlohmann\_json (https://github.com/nlohmann/json) version 3.5.0 (released 2018-12-22, MIT)

Dependencies included do not need to be installed on the host system, but their
presence should not lead to any problems.

## Running the catena daemon

`catena` requires the `-l ledger` option to specify a ledger file. This ledger
will be validated and imported on startup, and updated during runtime. If the
ledger cannot be validated, `catena` will refuse to start. An empty file can be
provided, resulting in complete download of the ledger from a peer.

Catena should be started with the `-k pubkey,txspec` option when it will be
signing transactions. See the "Key operations" section for material regarding
creation of keys suitable for use with Catena. `-k` can be supplied multiple
times to load multiple private keys.

When started without the `-d` option, Catena will remain in the foreground,
providing a readline-driven text UI. This can be used to examine the loaded
ledger and issue API requests directly from the console.

The ledger must never be modified externally while Catena is running. Doing
so will result in undefined behavior, possibly corrupting the ledger.

HTTP service will be provided on port 80 by default; this can be changed with
the `-p` parameter. Specifying a port of 0 will disable HTTP service. There
currently exists no native TLS support, and authentication is not performed.
It is recommended that HTTP clients use TLS terminated in front of Catena.

RPC service will not be provided by default; this can be changed with the `-r`
parameter. Specifying a port of 0 will leave RPC service disabled. This service
runs under TLS (though it is not HTTPS) and requires mutual authentication with
X.509 certificates signed by a common CA. The certificate chain for this CA
must be specified via the `-C` argument. If RPC service is requested, but the
certificate chain is missing or invalid, Catena will refuse to start. It is not
meaningful to provide `-C` without enabling RPC service via `-r`, but it is
also not an error.

A file consisting of initial peers to try can be specified via the `-P`
argument. This file should contain one peer per line, specified as IPv4[:port]
or IPv6[:port]. If a port is not specified, Catena will assume port 40404.
Catena will not wait on successful contact of these peers to start operations,
nor will failure to contact them be considered an exception. If no peers are
specified, Catena will not attempt to contact peers at startup, but will do so
following a connection *from* another peer. Likewise, once connected to peers,
Catena might choose newly-discovered peers, even if specified peers are
available. It is not meaningful to provide `-P` without enabling RPC service
via `-r`, but it is also not an error.

The services provided via HTTP and those provided by RPC are mutually exclusive.
The console client implements a superset of the union of these two services.

### Interactive use of catena

`catena` supports a REPL command line interactive mode with full Readline
support (command line editing, history, etc.). Single quotes are supported, as
is escaping with backslash. The following commands are available when catena is
invoked interactively (this list can be accessed by running the `help`
command):

* `help`: summarize available commands.
* `quit`: exit catena.
* `summary`: summarize the chain in a human-readable format.
* `show`: print the chain in a human-readable format.
* `tstore`: print the trust store (known keys) in a human-readable format.
* `inspect`: print detailed information about a range of the chain.
* `outstanding`: print outstanding transactions in a human-readable format.
* `commit`: coalesce outstanding transactions and add them to ledger.
* `flush`: flush (drop) outstanding transactions.
* `noop`: generate a NoOp transaction.
* `member`: generate a ConsortiumMember transaction. takes as its arguments a
TXSpec for the signing key, a filename containing the new member's public key,
and an arbitrary JSON-encoded payload.
* `getmember`: list all consortium members, or one specified member with
patient details.
* `exlookup`: generate an ExternalLookup transaction. takes as its arguments
a TXSpec for the signing key, an integer specifying the lookup type, a filename
containing the new association's public key, and an external identifier valid
for the specified lookup type.
* `lauthreq`: generate a LookupAuthReq transaction. takes as its arguments
a TXSpec for the requesting ConsortiumMember, a TXSpec for the referenced
ExternalLookup, and a JSON payload.
* `lauth`: generate a LookupAuth transaction. takes as its arguments a TXSpec
for the referenced LookupAuthReq, a TXSpec for the unveiled Patient, and a
filename containing the patient's symmetric key.
* `patient`: generate a Patient transaction. takes as its arguments a filename
containing the new entity's authorization public key, a filename containing the
raw symmetric key, and an arbitrary JSON payload. This payload will be
encrypted.
* `delpstatus`: generate a PatientStatusDelegation transaction. takes as its
arguments a TXSpec for the delegating Patient, a TXSpec for the delegated
ConsortiumMember, and a status type.
* `pstatus`: generate a PatientStatus transaction. takes as its arguments a
TXSpec for the referenced PatientStatusDelegation, and a JSON payload.
* `getpstatus`: show the most recent PatientStatus for the specified patient
and patient status delegation type.

Use of commands that generate signed or encrypted transactions requires an
appropriate private key having been loaded with the `-k` option.

### HTTP services of cantena

The following endpoints are provided. JSON schema are available in
doc/json-schema.md. HTML pages are subject to change, and ought not be scraped
nor parsed for semantic content in clients.

* GET `/`: HTML status page for human consumption (do not scrape/parse). Much
of the same information as is available from the `summary` command.
* GET `/show`: HTML equivalent of the `show` command
* GET `/tstore`: HTML equivalent of the `tstore` command
* GET `/inspect`: JSON equivalent of the `inspect` command
    * Optional query argument: `begin`, integer specifying first block
    * Optional query argument: `end`, integer specifying last block
    * Replies with application/json body of type InspectResult
* GET `/outstanding`: JSON equivalent of the `outstanding` command
    * Replies with application/json body of type InspectResult
* POST `/member`: JSON equivalent of the `member` command
    * Requires an application/json body of type NewConsortiumMemberTX
    * Replies with application/json body of type TXRequestResult
* POST `/exlookup`: JSON equivalent of the `exlookup` command
    * Requires an application/json body of type NewExternalLookupTX
    * Replies with application/json body of type TXRequestResponse
* POST `/lauthreq`: JSON equivalent of the `lauthreq` command
    * Requires an application/json body of type NewLookupAuthorizationRequestTX
    * Replies with application/json body of type TXRequestResponse
* POST `/lauth`: JSON equivalent of the `lauth` command
    * Requires an application/json body of type NewLookupAuthorizationTX
    * Replies with application/json body of type TXRequestResponse
* POST `/patient`: JSON equivalent of the `patient` command
    * Requires an application/json body of type NewPatientTX
    * Replies with application/json body of type NewPatientTXResponse or, on
failure, TXRequestResponse
* POST `/delpstatus`: JSON equivalent of the `delpstatus` command
    * Requires an application/json body of type NewPatientStatusDelegationTX
    * Replies with application/json body of type TXRequestResponse
* POST `/pstatus`: JSON equivalent of the `pstatus` command
    * Requires an application/json body of type NewPatientStatusTX
    * Replies with application/json body of type TXRequestResponse
* GET `/pstatus`: JSON equivalent of the `getpstatus` command
    * Required query argument: `patient`, TXSpec of Patient
    * Required query argument: `stype`, integer specifying delegated status type
    * Replies with application/json body of type PatientStatusResult or, on
failure, TXRequestResponse
* GET `/showpstatus`: HTML equivalent of `/pstatus`
    * Accepts same query arguments as `/pstatus`
* GET `/showmembers`: HTML equivalent of `/getmembers`
    * Required query argument: `member`, TXSpec of ConsortiumMember
* GET `/showblock`: HTML information about a particular block by hash
    * Required query argument: `hash`, blockhash

## Key operations

### Generating ECDSA material

* Generate ECDSA key at `outfile.pem`:
    * `openssl ecparam -name secp256k1 -genkey -noout -out outfile.pem`
* Verify ECDSA keypair at `outfile.pem`:
```
openssl ec -in outfile.pem -text -noout
read EC key
Private-Key: (256 bit)
priv:
    c8:f7:09:54:d7:32:7a:c9:b4:ef:59:50:d4:cd:21:
    0a:fa:72:10:df:36:f1:15:d4:ed:83:3a:45:e1:6e:
    4c:25
pub:
    04:3d:f0:bd:0a:f3:9a:f7:62:97:68:54:43:31:25:
    7f:7c:14:c1:58:8d:ee:b4:c0:8e:ae:39:b5:4f:a5:
    5e:e1:2f:b0:26:59:4b:46:3e:61:ee:94:80:99:9c:
    1f:78:02:37:8d:97:23:c9:18:27:b1:5d:00:b4:71:
    ad:42:3b:7b:74
ASN1 OID: secp256k1
```
* Extract public ECDSA key from `outfile.pem` to `outfile.pub`:
    * `openssl ec -in outfile.pem -pubout -out outfile.pub`

*DO NOT* use the `-param_enc explicit` option to `openssl ec` when extracting
the public key. Catena does not currently properly match private keys to public
keys when this has been used, FIXME.

## Runtime data

Included are two files, `genesisblock` and `test/ledger-test`. The former is
the first block of the official ledger as run in the Headway Catena Network.
The latter contains several records based off the keys in `test/`.
