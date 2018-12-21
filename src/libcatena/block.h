#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

#include <memory>
#include <vector>
#include <utility>
#include <ostream>
#include <libcatena/truststore.h>
#include <libcatena/hash.h>
#include <libcatena/tx.h>

namespace Catena {

// NOT the on-disk packed format
struct BlockHeader {
	unsigned char hash[HASHLEN];
	unsigned char prev[HASHLEN];
	unsigned version;
	unsigned totlen;
	unsigned txcount;
	uint64_t utc;
};

// A contiguous chain of zero or more BlockHeaders
class Blocks {
public:
Blocks() = default;
virtual ~Blocks() = default;

// Load blocks from the specified chunk of memory. Returns true on parsing
// error. Any present blocks are discarded.
bool LoadData(const void* data, unsigned len, TrustStore& tstore);
// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded. Return value is the same as loadData.
bool LoadFile(const std::string& s, TrustStore& tstore);

// Parse, validate, and finally add the block to the ledger.
bool AppendBlock(const unsigned char* block, size_t blen, TrustStore& tstore);

unsigned GetBlockCount() const {
	return offsets.size();
}

void GetLastHash(unsigned char* hash) const;

friend std::ostream& operator<<(std::ostream& stream, const Blocks& b);

private:
std::vector<unsigned> offsets;
std::vector<BlockHeader> headers;
int VerifyData(const unsigned char* data, unsigned len, TrustStore& tstore);
};

// A descriptor of a single block, and logic to serialize blocks
class Block {
public:
Block() = default;
virtual ~Block() = default;
static const int BLOCKHEADERLEN = 96;
static const int BLOCKVERSION = 0;

// Returns allocated block with serialized data, and size of serialized data.
// Updates prevhash with hash of serialized block.
std::pair<std::unique_ptr<const unsigned char[]>, size_t>
	SerializeBlock(unsigned char* prevhash) const;

static bool ExtractHeader(BlockHeader* chdr, const unsigned char* data,
	unsigned len, const unsigned char* prevhash, uint64_t prevutc);

bool ExtractBody(BlockHeader* chdr, const unsigned char* data, unsigned len,
			TrustStore& tstore);

int TransactionCount() const {
	return transactions.size();
}

void AddTransaction(std::unique_ptr<Transaction> tx);

friend std::ostream& operator<<(std::ostream& stream, const Block& b);

private:
std::vector<std::unique_ptr<Transaction>> transactions;
};

}

#endif
