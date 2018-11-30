#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

// A contiguous chain of zero or more CatenaBlock objects
class CatenaBlocks {
public:
CatenaBlocks() : blockcount(0) {};

// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded, regardless of result.
void loadFile(const std::string& s);
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
