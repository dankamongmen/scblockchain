#ifndef CATENA_LIBCATENA_HASH
#define CATENA_LIBCATENA_HASH

#include <ostream>
#include <libcatena/utility.h>

namespace Catena {

static constexpr int HASHLEN = 32; // length of hash outputs in bytes

// Do a class rather than alias so ADL finds the operator<<() overload when
// trying to print a CatenaHash from outside the Catena namespace.
struct CatenaHash : std::array<unsigned char, HASHLEN>{

using std::array<unsigned char, HASHLEN>::array; // inherit default constructor

template<class... U>
CatenaHash(U&&... u) :
	std::array<unsigned char, HASHLEN>{std::forward<U>(u)...} {}

inline friend std::ostream& operator<<(std::ostream& s, const CatenaHash& hash){
	HexOutput(s, hash.data(), hash.size());
	return s;
}

// FIXME can surely do this much faster with some precomputed comparison
bool IsGenesis() const {
	for(auto i = 0u ; i < size() ; ++i){
		if(data()[i] != 0xff){
			return false;
		}
	}
	return true;
}

};

void catenaHash(const void* in, unsigned len, CatenaHash& hash);
// This might be a bit overbroad, and capture things unexpectedly...? find out
std::string hashOString(const CatenaHash& hash);

// Naked interface to hashing functions. hash must be at least HASHLEN bytes.
void catenaHash(const void* in, unsigned len, void* hash);
std::ostream& hashOStream(std::ostream& s, const void* hash);

// Hex representation of block hash. Throws ConvertInputException on lex error.
// Encoded bytes may be followed by arbitrary material.
CatenaHash StrToCatenaHash(const std::string& s);

struct TXSpec : std::pair<CatenaHash, unsigned>{

using std::pair<CatenaHash, unsigned>::pair; // inherit default constructor

TXSpec(const CatenaHash& hash, unsigned idx) :
	std::pair<CatenaHash, unsigned>(hash, idx) {}

TXSpec(const std::string& s) {
	TXSpec tx = StrToTXSpec(s);
	*this = tx;
}

inline friend std::ostream& operator<<(std::ostream& s, const TXSpec& t){
	s << t.first << "." << t.second;
	return s;
}

// Canonical format is hex representation of block hash, followed by period,
// followed by transaction index with optional leading zeroes. No other content
// is allowed. Throws ConvertInputException on any lexing error.
static TXSpec StrToTXSpec(const std::string& s);

};

}

#endif
