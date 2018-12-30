# Known public keys from the Catena project

## Builtin keys

Builtin public keys are encoded directly into the distributed binary, and
referenced in the ledger using the all-ones hash pointer plus an index.

### Builtin index 0: Headway key #1 (475 bytes)
The corresponding private key is not distributed.
```
-----BEGIN PUBLIC KEY-----
MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////
/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m
fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0
SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB
AgEBA0IABMHATB6cBQYel6ln6rvD+3nQQkv+pCcy5FaZbtgbNzp85uIJlBOdgRqU
c8QhbDlfR85mT+KKpKH6TqQkI990Wm8=
-----END PUBLIC KEY-----
```

## Consortium member keys

Consortium member keys are placed on the ledger in ConsortiumMember
transactions. They are signed by a builtin key, or another consortium member
key from the ledger.

### ShareCare key 1 (174 bytes)
The corresponding private key is not distributed.
```
-----BEGIN PUBLIC KEY-----
MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAERSeRF0M5HkDoGiLri7YRsqaVA7zoLWVE
mBQ7d6b94TpZu3e0QkX2QxmfNK4cRX/saw4dDM9yV2c0RIVe35r0wg==
-----END PUBLIC KEY-----
```

### ShareCare key 2 (475 bytes)
The corresponding private key is not distributed.
```
-----BEGIN PUBLIC KEY-----
MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////
/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m
fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0
SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB
AgEBA0IABOTv5jWDTZHbdQWSVLAu/XDs7lRnznmu70IqP5VxOXId7rQlF6+wPWv9
3TcA+ygpe41Uw5DDEzogV1HdBe9r/wM=
-----END PUBLIC KEY-----
```

### Test consortium member key (174 bytes)
The corresponding private key is in `test/cm-test1.pem`.
```
-----BEGIN PUBLIC KEY-----
MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEmklGZ8BMpE5snXpbpqLvxDcGqSxuAtI2
Raq6hzx2H0+Ie/w1cpXeg8avew0QSJxuTqB8jMCXMmkLBv3jEkYN6w==
-----END PUBLIC KEY-----
```


## Patient keys

Each patient has two keypairs (the Delegation Authorization keypair, and the
Lookup Authorization keypair) and a symmetric key. The Delegation Authorization
public key is written into a Patient transaction. The Lookup Authorization
public key is written into an ExternalLookup transaction. The symmetric key is
encrypted and written into LookupAuth transactions.

The test patient symmetric key is in `test/symmetric-test1.aes`.

### Test patient Delegation Authorization key (475 bytes)
The corresponding private key is in `test/pauth-test1.pem`.
```
-----BEGIN PUBLIC KEY-----
MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////
/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m
fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0
SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB
AgEBA0IABM97jqaKaNs9PDS7yr49Ec4PwqyNKBhVMNGC9cJKy/yx94rbWLMt7AW9
MEpgSYdUrSgG3xkdQEnpZc9k0Tht2QA=
-----END PUBLIC KEY-----
```

### Test patient Lookup Authorization key (475 bytes)
The corresponding private key is in `test/elook-test1.pem`.
```
-----BEGIN PUBLIC KEY-----
MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////
/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m
fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0
SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB
AgEBA0IABBXZN8y7o3wiR9QAsGAs/aIJ3sxqWfG3zB4Q4/Us9fzGwBSKowvHiEN+
5ea/Xj3eFkNUaNlShsucr9+h/+7oBqU=
-----END PUBLIC KEY-----
```
