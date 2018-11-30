#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

// A contiguous chain of zero or more CatenaBlock objects
class CatenaBlocks {
public:
CatenaBlocks() = default;
virtual ~CatenaBlocks() = default;

// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded, regardless of result.
void loadFile(const std::string& s);

private:
void verifyData(const char *data, unsigned len);
};

// A single block
class CatenaBlock {
public:
CatenaBlock() = default;
virtual ~CatenaBlock() = default;
};

#endif
