# catena -- ShareCare blockchain for health information

## Building

The default target is `all`, which will build all binaries and docs. To run
unit tests, use the `test` target, which will build any necessary dependencies.

### Build requirements

* C++ compiler and GNU Make 4.2.1+
** Tested with clang++ 7.0.1 and g++ 8.2.0
* Google Test (libgtest-dev)
* OpenSSL 1.1+ (libopenssl-dev)
* GNU Libmicrohttpd 0.9.62+ (libmicrohttpd-dev)
* GNU Readline 6.3+ (libreadline-dev)

External projects included in this project include:

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

The following commands are available when catena is invoked interactively (this
list can be accessed by running the `help` command):

* `help`: summary of available commands
* `quit`: exit catena
* `show`: print the chain in a human-readable format
* `tstore`: print the trust store (known keys) in a human-readable format
* `outstanding`: print outstanding transactions in a human-readable format
* `commit`: coalesce outstanding transactions into a block and add it to ledger
* `flush`: flush outstanding transactions
* `noop`: generate a NoOp transaction
* `member`: generate a ConsortiumMember transaction. takes as its argument a
filename containing the new member's public key, and an arbitrary JSON-encoded
payload. Use single quotes to enclose the payload, escaping any single quotes
within the payload.

Use of the `member` command requires a private key having been loaded with the
`-u` option.

### HTTP services of cantena

The following endpoints are provided:

* `/show`: HTML equivalent of the `show` command
* `/tstore`: HTML equivalent of the `tstore` command

## Key operations

### Generating ECDSA material

* Generate ECDSA key at `outfile.pem`: `openssl ecparam -name secp256k1 -genkey -noout -out outfile.pem -param_enc explicit`
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
* Extract public ECDSA key from `outfile.pem` to `outfile.pub`: `openssl ec -in outfile.pem -pubout -out outfile.pub`

## Runtime data

Included are two files, `genesisblock` and `test/genesisblock-test`. The former
is the first block of the official ledger as run in the Headway Catena Network.
The latter contains a ConsortiumMemberTX record using the testing keys in
`test/ecdsa.pem` and `test/ecdsa.pub`.
