#ifndef CATENA_LIBCATENA_CHAIN
#define CATENA_LIBCATENA_CHAIN

#include <nlohmann/json_fwd.hpp>
#include <libcatena/truststore.h>
#include <libcatena/block.h>
#include <libcatena/sig.h>

namespace Catena {

class BlockValidationException : public std::runtime_error {
public:
BlockValidationException() : std::runtime_error("error validating block"){}
BlockValidationException(const std::string& s) : std::runtime_error(s){}
};

class NetworkException : public std::runtime_error {
public:
NetworkException() : std::runtime_error("network error"){}
NetworkException(const std::string& s) : std::runtime_error(s){}
};

// The ledger (one or more CatenaBlocks on disk) as indexed in memory. The
// Chain can have blocks added to it, either produced locally or received over
// the network. Blocks will be validated before being added. Once added, the
// block will be written to disk (appended to the existing ledger).
class Chain {
public:
Chain() :
  RPCport(0) {}
virtual ~Chain() = default;

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

int PatientCount() const {
	return lmap.PatientCount();
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

// Enable RPC service on the specified port. Returns false if RPC service is
// already enabled. Returns true on successful initiation of RPC service,
// though peer connections have not necessarily been established. Throws
// exceptions on failure to initiate service. chainfile must specify a valid
// X.509 certificate chain; peers must present a cert trusted by this chain.
// The peerfile may be null, in which case no outbound connections will be
// attempted until peers are discovered. If peerfile is not null, it must
// contain one peer per line, specified as an IPv4 or IPv6 address and optional
// ":port" suffix. If a port is not specified for a peer, the RPC service port
// is assumed. Failure to parse this file, if specified, throws an exception.
// FIXME probably need to accept our own cert+key files, ugh. might be able to
//  have our own cert as the last in the cachainfile
bool EnableRPC(int port, const std::string& chainfile, const char* peerfile);

// FIXME should probably return pair including ConsortiumMemberSummary
std::vector<PatientSummary> ConsortiumPatients(const TXSpec& cmspec) const {
	return lmap.ConsortiumPatients(cmspec);
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

// Retrieve the most recent PatientStatus published for this patient of this
// type. Returns a trivial JSON object if no such statuses have been published.
// Throws InvalidTXSpec if no such patient exists.
nlohmann::json PatientStatus(const TXSpec& patspec, unsigned stype) const;

// Generate and sign new transactions, to be added to the ledger.
void AddNoOp();
void AddConsortiumMember(const TXSpec& keyspec, const unsigned char* pkey,
				size_t plen, const nlohmann::json& payload);
void AddExternalLookup(const TXSpec& keyspec, const unsigned char* pkey,
		size_t plen, const std::string& extid, unsigned lookuptype);
void AddLookupAuthReq(const TXSpec& cmspec, const TXSpec& elspec, const nlohmann::json& payload);
void AddLookupAuth(const TXSpec& elspec, const TXSpec& patspec, const SymmetricKey& symkey);
void AddPatient(const TXSpec& cmspec, const unsigned char* pkey, size_t plen,
		const SymmetricKey& symkey, const nlohmann::json& payload);
void AddPatientStatus(const TXSpec& psdspec, const nlohmann::json& payload);
void AddPatientStatusDelegation(const TXSpec& cmspec, const TXSpec& patspec,
				int stype, const nlohmann::json& payload);

// Return a JSON object containing details regarding the specified block range.
// Pass -1 for end to specify only the start of the range.
nlohmann::json InspectJSON(int start, int end) const;

// Return a copy of the blockchain details for the specified block range.
// Pass -1 for end to specify only the start of the range.
std::vector<BlockDetail> Inspect(int start, int end) const;

friend std::ostream& operator<<(std::ostream& stream, const Chain& chain);

private:
TrustStore tstore;
LedgerMap lmap;
Blocks blocks;
Block outstanding;
int RPCport; // if 0, RPC service is currently disabled

void LoadBuiltinKeys();
};

}

#endif
