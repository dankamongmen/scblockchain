#ifndef CATENA_LIBCATENA_BLOCK
#define CATENA_LIBCATENA_BLOCK

#include <memory>
#include <vector>
#include <utility>
#include <ostream>
#include <libcatena/lookupauthreqtx.h>
#include <libcatena/truststore.h>
#include <libcatena/hash.h>
#include <libcatena/tx.h>

namespace Catena {

// NOT the on-disk packed format
struct BlockHeader {
	CatenaHash hash;
	CatenaHash prev;
	unsigned version;
	unsigned totlen;
	unsigned txcount;
	uint64_t utc;
	unsigned txidx; // transaction index within block, derived on read
};

struct BlockDetail {
public:
BlockDetail(const BlockHeader& bhdr, unsigned offset,
		std::unique_ptr<unsigned char[]> bytes,
		std::vector<std::unique_ptr<Transaction>> trans) :
  bhdr(bhdr),
  offset(offset),
  bytes(std::move(bytes)),
  transactions(std::move(trans)) {}

BlockHeader bhdr;
unsigned offset;
std::unique_ptr<unsigned char[]> bytes;
std::vector<std::unique_ptr<Transaction>> transactions;

friend std::ostream& operator<<(std::ostream& stream, const BlockDetail& b);
};

// A contiguous chain of zero or more BlockHeaders
class Blocks {
public:
Blocks() = default;
virtual ~Blocks() = default;

// FIXME why aren't these two just constructors? they should only be called once.
// Load blocks from the specified chunk of memory. Returns true on parsing
// error. Any present blocks are discarded.
bool LoadData(const void* data, unsigned len, LedgerMap& lmap, TrustStore& tstore);
// Load blocks from the specified file. Propagates I/O exceptions. Any present
// blocks are discarded. Return value is the same as loadData.
bool LoadFile(const std::string& s, LedgerMap& lmap, TrustStore& tstore);

// Parse, validate, and finally add the block to the ledger.
bool AppendBlock(const unsigned char* block, size_t blen, LedgerMap& lmap, TrustStore& tstore);

unsigned GetBlockCount() const {
	return offsets.size();
}

// Total size of the serialized chain, in bytes (does not include outstandings)
size_t Size() const {
	if(offsets.empty()){
		return 0;
	}
	return offsets.back() + headers.back().totlen;
}

unsigned TXCount() const {
	return std::accumulate(headers.begin(), headers.end(), 0,
		[](int total, const BlockHeader& bhdr){
			return total + bhdr.txcount;
		});
}

void GetLastHash(CatenaHash& hash) const;

time_t GetLastUTC() const {
	if(headers.empty()){
		return -1;
	}
	return headers.back().utc;
}

CatenaHash HashByIdx(int idx) const {
	return headers.at(idx).hash;
}

// FIXME slow, O(N) on N blocks
int IdxByHash(const CatenaHash& hash) const {
	auto ret = std::find_if(headers.begin(), headers.end(),
			[hash](const BlockHeader& b){
				return b.hash == hash;
			});
	if(ret == headers.end()){
		throw std::out_of_range("no such block");
	}
	return ret - headers.begin();
}

// Pass -1 for end to leave the end unspecified. Start and end are inclusive.
std::vector<BlockDetail> Inspect(int start, int end) const;

friend std::ostream& operator<<(std::ostream& stream, const Blocks& b);

private:
std::vector<unsigned> offsets;
std::vector<BlockHeader> headers;
int VerifyData(const unsigned char* data, unsigned len,
		LedgerMap& lmap, TrustStore& tstore);
std::string filename; // for in-memory chains, "", otherwise name from LoadFile
std::vector<unsigned char> memledger; // non-empty iff filename.empty()
};

// A descriptor of a single block, and logic to serialize blocks
class Block {
public:
Block() = default;
virtual ~Block() = default;
// FIXME can we not make these constexpr?
static const int BLOCKHEADERLEN = 96;
static const int BLOCKVERSION = 0;

// Returns allocated block with serialized data, and size of serialized data.
// Updates prevhash with hash of serialized block.
std::pair<std::unique_ptr<const unsigned char[]>, size_t>
	SerializeBlock(CatenaHash& prevhash) const;

// Throws InvalidBlockException on errors
static void ExtractHeader(BlockHeader* chdr, const unsigned char* data,
		unsigned len, const CatenaHash& prevhash, uint64_t prevutc);

std::vector<std::unique_ptr<Transaction>>
  Inspect(const unsigned char* b, const BlockHeader* bhdr);

// Pass nullptr as tstore to not validate transactions / update metadata
bool ExtractBody(const BlockHeader* chdr, const unsigned char* data,
    unsigned len, LedgerMap* lmap, TrustStore* tstore);

int TransactionCount() const {
	return transactions.size();
}

void AddTransaction(std::unique_ptr<Transaction> tx);

void Flush();

friend std::ostream& operator<<(std::ostream& stream, const Block& b);

private:
std::vector<std::unique_ptr<Transaction>> transactions;
};

}

#endif
