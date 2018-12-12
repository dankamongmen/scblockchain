#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

#include <memory>
#include <vector>
#include <utility>
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

// Load blocks from the specified chunk of memory. Returns false on parsing
// error, or if there were no blocks. Any present blocks are discarded.
bool loadData(const void* data, unsigned len);
// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded. Return value is the same as loadData.
bool loadFile(const std::string& s);

unsigned getBlockCount(){
	return offsets.size();
}

private:
std::vector<unsigned> offsets;
std::vector<BlockHeader> headers;
int verifyData(const unsigned char* data, unsigned len);
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
static std::pair<std::unique_ptr<const char[]>, unsigned>
	serializeBlock(unsigned char* prevhash);

static bool extractHeader(BlockHeader* chdr, const unsigned char* data,
	unsigned len, const unsigned char* prevhash, uint64_t prevutc);
static bool extractBody(BlockHeader* chdr, const unsigned char* data, unsigned len);

private:
std::vector<Transaction*> transactions;
};

}

#endif
