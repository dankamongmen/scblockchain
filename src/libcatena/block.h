#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

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

// A single block
class CatenaBlock {
public:
CatenaBlock() = default;
virtual ~CatenaBlock() = default;
static const int BLOCKHEADERLEN = 96;
};

#define HASHLEN 32 // length of hash outputs in bytes

// NOT the on-disk packed format
struct CatenaBlockHeader {
	char hash[HASHLEN];
	char prev[HASHLEN];
	unsigned version;
	unsigned totlen;
	unsigned txcount;
	uint64_t utc;
};

#endif
