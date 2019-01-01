# catena - a distributed ledger for health information

## Building

The default target is `all`, which will build all binaries and docs. To run
unit tests, use the `test` target, which will build any necessary dependencies.

### Build requirements

* C++ compiler and GNU Make 4.2.1+
    * Tested with clang++ 7.0.1 and g++ 8.2.0
* Google Test 1.8.1+ (libgtest-dev)
* OpenSSL 1.1+ (libopenssl-dev)
* GNU Libmicrohttpd 0.9.62+ (libmicrohttpd-dev)
* GNU Readline 6.3+ (libreadline-dev)

External projects (see the `ext/` directory) include:

* nlohmann\_json (https://github.com/nlohmann/json) version 3.4.0 (released 2018-10-30, MIT)

## Running the catena daemon

`catena` requires the `-l ledger` option to specify a ledger file. This ledger
will be validated and imported on startup, and updated during runtime. If the
ledger cannot be validated, `catena` will refuse to start. An empty file can be
provided, resulting in complete download of the ledger from a peer.

`catena` should be started with the `-u pubkey` and `-v privkey` options when
it will be signing transactions. See the "Key operations" section for material
regarding creation of local ("Consortium") keys. The provided public key must
match either a built-in or an on-ledger key.

When started without the `-d` option, the catena agent will remain in the
foreground, providing a readline-driven text UI. This can be used to examine
the loaded ledger and issue API requests directly.

The ledger must never be modified externally while `catena` is running. Doing
so will result in undefined behavior, possibly corrupting the ledger.

HTTP service will be provided on port 80 by default; this can be changed with
the `-p` parameter. Specifying a port of 0 will disable HTTP service.

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
appropriate private key having been loaded with the `-u` option (along with
the corresponding public key with the `-v` option).

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
* GET `/showmembers`: HTML equivalent of `/getmembers`
    * Required query argument: `member`, TXSpec of ConsortiumMember
* GET `/showpstatus`: HTML equivalent of `/pstatus`
    * Accepts same query arguments as `/pstatus`

## Key operations

### Generating ECDSA material

* Generate ECDSA key at `outfile.pem`:
    * `openssl ecparam -name secp256k1 -genkey -noout -out outfile.pem -param_enc explicit`
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
Field Type: prime-field
Prime:
    00:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
    ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:fe:ff:
    ff:fc:2f
A:    0
B:    7 (0x7)
Generator (uncompressed):
    04:79:be:66:7e:f9:dc:bb:ac:55:a0:62:95:ce:87:
    0b:07:02:9b:fc:db:2d:ce:28:d9:59:f2:81:5b:16:
    f8:17:98:48:3a:da:77:26:a3:c4:65:5d:a4:fb:fc:
    0e:11:08:a8:fd:17:b4:48:a6:85:54:19:9c:47:d0:
    8f:fb:10:d4:b8
Order:
    00:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
    ff:fe:ba:ae:dc:e6:af:48:a0:3b:bf:d2:5e:8c:d0:
    36:41:41
Cofactor:  1 (0x1)
```
* Extract public ECDSA key from `outfile.pem` to `outfile.pub`:
    * `openssl ec -in outfile.pem -pubout -out outfile.pub`

## Runtime data

Included are two files, `genesisblock` and `test/ledger-test`. The former is
the first block of the official ledger as run in the Headway Catena Network.
The latter contains several records based off the keys in `test/`.
