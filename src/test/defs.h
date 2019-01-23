#ifndef CATENA_TEST_DEFS
#define CATENA_TEST_DEFS

#define GENESISBLOCK_EXTERNAL "genesisblock"
#define MOCKLEDGER "test/ledger-test"
#define MOCKLEDGER_BLOCKS 19
#define MOCKLEDGER_TXS 23
#define MOCKLEDGER_PUBKEYS 10
#define PUBLICKEY \
"-----BEGIN PUBLIC KEY-----\n"\
"MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEmklGZ8BMpE5snXpbpqLvxDcGqSxuAtI2\n"\
"Raq6hzx2H0+Ie/w1cpXeg8avew0QSJxuTqB8jMCXMmkLBv3jEkYN6w==\n"\
"-----END PUBLIC KEY-----"
#define ECDSAKEY "test/cm-test1.pem"
#define CM1_TEST_TX "859640f2a50303dc13eb5ab7dac59f0febcdeb6654310e809d10b7197d7897f4.0"
#define ELOOK_TEST_PUBKEY \
"-----BEGIN PUBLIC KEY-----\n"\
"MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA////////////////\n"\
"/////////////////////v///C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"\
"AAAAAAAAAAAEIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5m\n"\
"fvncu6xVoGKVzocLBwKb/NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0\n"\
"SKaFVBmcR9CP+xDUuAIhAP////////////////////66rtzmr0igO7/SXozQNkFB\n"\
"AgEBA0IABBXZN8y7o3wiR9QAsGAs/aIJ3sxqWfG3zB4Q4/Us9fzGwBSKowvHiEN+\n"\
"5ea/Xj3eFkNUaNlShsucr9+h/+7oBqU=\n"\
"-----END PUBLIC KEY-----"
#define RPC_TEST_PEERS "test/rpc-peers"
#define ELOOK_TEST_PRIVKEY "test/elook-test1.pem"
#define TEST_NODEKEY "test/ca/intermediate/testcorp/private/testnode1.key.pem"
#define TEST_CONFIGURED_PEERS 2

static const char TEST_X509_CHAIN[] = "test/node1chain.pem";

#endif
