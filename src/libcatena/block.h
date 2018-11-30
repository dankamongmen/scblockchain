#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

class CatenaBlock {

public:
CatenaBlock() = default;
virtual ~CatenaBlock() = default;

// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded, regardless of result.
void loadFile(const std::string& s);
};

#endif
