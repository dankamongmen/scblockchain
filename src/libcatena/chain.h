#ifndef CATENA_LIBCATENA_CHAIN
#define CATENA_LIBCATENA_CHAIN

#include <json.hpp>
#include <libcatena/truststore.h>
#include <libcatena/block.h>
#include <libcatena/sig.h>

namespace Catena {

class BlockValidationException : public std::runtime_error {
public:
BlockValidationException() : std::runtime_error("error validating block"){}
};

// The ledger (one or more CatenaBlocks on disk) as indexed in memory. The
// Chain can have blocks added to it, either produced locally or received over
// the network. Blocks will be validated before being added. Once added, the
// block will be written to disk (appended to the existing ledger).
class Chain {
public:
Chain() = default;
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

// Dump outstanding transactions in a human-readable format
std::ostream& DumpOutstanding(std::ostream& s) const;

// serialize and flush outstanding transactions
std::pair<std::unique_ptr<const unsigned char[]>, size_t> SerializeOutstanding();

void AddSigningKey(const Keypair& kp);

// Generate and sign new transactions, to be added to the ledger.
void AddNoOp();
void AddConsortiumMember(const std::string& pubfname, nlohmann::json& payload);

friend std::ostream& operator<<(std::ostream& stream, const Chain& chain);

private:
TrustStore tstore;
Blocks blocks;
Block outstanding;

void LoadBuiltinKeys();
};

}

#endif
