#ifndef CATENA_LIBCATENA_CHAIN
#define CATENA_LIBCATENA_CHAIN

#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <libcatena/externallookuptx.h>
#include <libcatena/truststore.h>
#include <libcatena/exceptions.h>
#include <libcatena/block.h>
#include <libcatena/peer.h>
#include <libcatena/rpc.h>

namespace Catena {

class Keypair;

// The ledger (one or more CatenaBlocks on disk) as indexed in memory. The
// Chain can have blocks added to it, either produced locally or received over
// the network. Blocks will be validated before being added. Once added, the
// block will be written to disk (appended to the existing ledger).
class Chain {
public:
Chain() = default;

// Constructing a Chain requires lexing and validating blocks. On a logic error
// within the chain, a BlockValidationException exception is thrown. Exceptions
// can also be thrown for file I/O error. An empty file is acceptable.
Chain(const std::string& fname);

// A Chain instantiated from memory will not write out new blocks.
Chain(const void* data, unsigned len);

// Throw the same exceptions as Chain(), otherwise returning the number of
// added blocks.
unsigned loadFile(const std::string& fname);
unsigned loadData(const void* data, unsigned len);

// Dump the trust store contents in a human-readable format
std::ostream& DumpTrustStore(std::ostream& s) const {
	return s << tstore;
}

unsigned GetBlockCount() const {
	return blocks.GetBlockCount();
}

unsigned OutstandingTXCount() const {
	return outstanding.TransactionCount();
}

unsigned TXCount() const {
	return blocks.TXCount();
}

time_t MostRecentBlock() const {
	return blocks.GetLastUTC();
}

CatenaHash MostRecentBlockHash() const {
	auto count = blocks.GetBlockCount();
	if(count == 0){
		CatenaHash ret;
		ret.fill(0xff);
		return ret;
	}
	return blocks.HashByIdx(count - 1);
}

int PubkeyCount() const {
	return tstore.PubkeyCount();
}

// Total size of the serialized chain, in bytes (does not include outstandings)
size_t Size() const {
	return blocks.Size();
}

int LookupRequestCount() const {
	return lmap.LookupRequestCount();
}

int LookupRequestCount(bool authorized) const {
	return lmap.LookupRequestCount(authorized);
}

int ExternalLookupCount() const {
	return lmap.ExternalLookupCount();
}

int StatusDelegationCount() const {
	return lmap.StatusDelegationCount();
}

int UserCount() const {
	return lmap.UserCount();
}

int ConsortiumMemberCount() const {
	return lmap.ConsortiumMemberCount();
}

std::vector<ConsortiumMemberSummary> ConsortiumMembers() const {
	return lmap.ConsortiumMembers();
}

ConsortiumMemberSummary ConsortiumMember(const TXSpec& tx) const {
	return lmap.ConsortiumMember(tx);
}

// FIXME should probably return pair including ConsortiumMemberSummary
std::vector<UserSummary> ConsortiumUsers(const TXSpec& cmspec) const {
	return lmap.ConsortiumUsers(cmspec);
}

// Only good until some mutating call is made, beware!
const Block& OutstandingTXs() const;

// serialize outstanding transactions
std::pair<std::unique_ptr<const unsigned char[]>, size_t>
  SerializeOutstanding() const;

// Serialize outstanding transactions into a block, add it to the ledger, and
// flush the transactions assuming everything worked.
void CommitOutstanding();

// Flush (drop) any outstanding transactions.
void FlushOutstanding();

void AddPrivateKey(const KeyLookup& kl, const Keypair& kp) {
	tstore.AddKey(&kp, kl);
}

// Retrieve the most recent UserStatus published for this user of this type.
// Returns a trivial JSON object if no such statuses have been published.
// Throws InvalidTXSpec if no such user exists.
nlohmann::json UserStatus(const TXSpec& uspec, unsigned stype) const;

// Generate and sign new transactions, to be added to the ledger. Each of these
// will result in a new outstanding transaction, plus a broadcast. The versions
// without a key supplied require the specified private key to be loaded in the
// truststore.
void AddConsortiumMember(const TXSpec& keyspec, const unsigned char* pubkey,
				size_t publen, const nlohmann::json& payload, const void* privkey,
        size_t privlen);

void AddConsortiumMember(const TXSpec& keyspec, const unsigned char* pubkey,
				size_t publen, const nlohmann::json& payload){
  AddConsortiumMember(keyspec, pubkey, publen, payload, nullptr, 0);
}

void AddExternalLookup(const TXSpec& keyspec, const unsigned char* pubkey,
		size_t publen, const std::string& extid, ExtIDTypes lookuptype,
    const void* privkey, size_t privlen);

void AddExternalLookup(const TXSpec& keyspec, const unsigned char* pubkey,
		size_t publen, const std::string& extid, ExtIDTypes lookuptype){
  AddExternalLookup(keyspec, pubkey, publen, extid, lookuptype, nullptr, 0);
}

void AddLookupAuthReq(const TXSpec& cmspec, const TXSpec& elspec, const nlohmann::json& payload,
    const void* privkey, size_t privlen);

void AddLookupAuthReq(const TXSpec& cmspec, const TXSpec& elspec,
				const nlohmann::json& payload){
  AddLookupAuthReq(cmspec, elspec, payload, nullptr, 0);
}

void AddLookupAuth(const TXSpec& larspec, const TXSpec& uspec, const SymmetricKey& symkey);
void AddUser(const TXSpec& cmspec, const unsigned char* pkey, size_t plen,
		const SymmetricKey& symkey, const nlohmann::json& payload);
void AddUserStatus(const TXSpec& usdspec, const nlohmann::json& payload);
void AddUserStatusDelegation(const TXSpec& cmspec, const TXSpec& uspec,
				int stype, const nlohmann::json& payload);

void AddTransaction(std::unique_ptr<Transaction> tx);

// Return a JSON object containing details regarding the specified block range.
// Pass -1 for end to specify only the start of the range.
nlohmann::json InspectJSON(int start, int end) const;

// Return a copy of the blockchain details for the specified block range.
// Pass -1 for end to specify only the start of the range.
std::vector<BlockDetail> Inspect(int start, int end) const;

// Return details for the specified block hash.
BlockDetail Inspect(const CatenaHash& hash) const;

// Enable p2p rpc networking. Throws NetworkException if already enabled for
// this ledger, or a variety of other possible problems.
void EnableRPC(const RPCServiceOptions& opts);

// Return details about the RPC p2p network peers. Throws NetworkException if
// p2p networking has not been enabled.
std::vector<PeerInfo> Peers() const;

std::vector<ConnInfo> Conns() const;

// Throws NetworkException if RPC networking has not been enabled
void AddPeers(const std::string& peerfile);

// Returns 0 if RPC networking has not been enabled
int RPCPort() const;

std::vector<std::string> AdvertisedAddresses() const {
  if(rpcnet){
    return rpcnet->Advertisement();
  }
  return std::vector<std::string>{};
}

// Safe to call only if RPC networking has been enabled (RPCPort() != 0)
void PeerCount(int* defined, int* maxactive) const {
	return rpcnet->PeerCount(defined, maxactive);
}

int ActiveConnCount() const {
	return rpcnet->ActiveConnCount();
}

// Get the node's RPC name. Safe to call only if RPC networking has been enabled.
std::pair<std::string, std::string> RPCName() const {
	return rpcnet->Name();
}

RPCServiceStats RPCStats() const {
  return rpcnet->Stats();
}

friend std::ostream& operator<<(std::ostream& stream, const Chain& chain);

private:
TrustStore tstore;
LedgerMap lmap;
Blocks blocks;
Block outstanding;
std::unique_ptr<RPCService> rpcnet;
std::mutex lock;

void LoadBuiltinKeys();
};

}

#endif
