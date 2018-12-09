#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

#include <memory>
#include <utility>

// A contiguous chain of zero or more CatenaBlock objects
class CatenaBlocks {
public:
CatenaBlocks() : blockcount(0) {};

// Load blocks from the specified chunk of memory. Returns false on parsing
// error, or if there were no blocks.
bool loadData(const char *data, unsigned len);
// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded. Return value is the same as loadData.
bool loadFile(const std::string& s);
unsigned getBlockCount();

private:
int blockcount;
int verifyData(const char *data, unsigned len);
};

// A descriptor of a single block, and logic to serialize blocks
class CatenaBlock {
public:
CatenaBlock() = default;
virtual ~CatenaBlock() = default;
static const int BLOCKHEADERLEN = 96;
static const int BLOCKVERSION = 0;

// Returns allocated block with serialized data, and size of serialized data
static std::pair<std::unique_ptr<const char[]>, unsigned> serializeBlock();

static bool extractHeader(struct CatenaBlockHeader*, const char*, unsigned);
};

#define HASHLEN 32 // length of hash outputs in bytes

#endif
