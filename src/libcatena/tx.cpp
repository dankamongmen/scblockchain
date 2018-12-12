#include <cstring>
#include <cstdint>
#include <iostream>
#include "libcatena/hash.h"
#include "libcatena/sig.h"
#include "libcatena/tx.h"

namespace Catena {

enum ConsortiumSigTypes {
	InternalSigner = 0x0000,
	LedgerSigner = 0x0001,
};

bool Transaction::extractConsortiumMember(const unsigned char* data, unsigned len){
	uint16_t sigtype;
	if(len < sizeof(sigtype)){
		std::cerr << "no room for consortium sigtype in " << len << std::endl;
		return true;
	}
	len -= sizeof(sigtype);
	sigtype = *data++ * 0x100;
	sigtype += *data++;
	switch(sigtype){
	case InternalSigner:{
		uint32_t signidx; // internal signer index
		unsigned char sig[SIGLEN];
		if(len < sizeof(sig) + sizeof(signidx)){
			std::cerr << "no room for internal signature in " << len << std::endl;
			return true;
		}
		memcpy(sig, data, sizeof(sig));
		data += sizeof(sig);
		len -= sizeof(sig);
		break;
	}case LedgerSigner:{
		uint32_t signidx; // ledger signer transaction index
		if(len < SIGLEN + HASHLEN + sizeof(signidx)){
			std::cerr << "no room for ledger signature in " << len << std::endl;
			return true;
		}
		break;
	}default:
		std::cerr << "unknown consortium sigtype " << sigtype << std::endl;
		return true;
	}
	return false;
}

enum TXTypes {
	NoOp = 0x0000,
	ConsortiumMember = 0x0001,
};

// Each transaction starts with a 16-bit unsigned type. Returns true on any
// failure to parse, or if the length is wrong for the transaction.
bool Transaction::extract(const unsigned char* data, unsigned len){
	uint16_t txtype;
	if(len < sizeof(txtype)){
		std::cerr << "no room for transaction type in " << len << std::endl;
		return true;
	}
	len -= sizeof(txtype);
	txtype = *data++ * 0x100;
	txtype += *data++;
	// FIXME replace this with a map of the enum to function pointers?
	switch(txtype){
	case NoOp:
		uint16_t nooptype;
		if(len != sizeof(nooptype)){
			std::cerr << "payload too large for noop: " << len << std::endl;
			return true;
		}
		break;
	case ConsortiumMember:
		return extractConsortiumMember(data, len);
	default:
		std::cerr << "unknown transaction type " << txtype << std::endl;
		return true;
	}
	return false;
}

}
