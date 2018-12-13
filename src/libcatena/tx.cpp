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

unsigned long nbo_to_ulong(const unsigned char* data, int bytes){
	unsigned long ret = 0;
	while(bytes-- > 0){
		ret *= 0x100;
		ret += *data++;
	}
	return ret;
}

bool ConsortiumMemberTX::extract(const unsigned char* data, unsigned len){
	uint16_t sigtype;
	if(len < sizeof(sigtype)){
		std::cerr << "no room for consortium sigtype in " << len << std::endl;
		return true;
	}
	sigtype = nbo_to_ulong(data, sizeof(sigtype));
	data += sizeof(sigtype);
	len -= sizeof(sigtype);
	uint32_t signidx;
	switch(sigtype){
	case InternalSigner:{
		if(len < sizeof(signature) + sizeof(signidx)){
			std::cerr << "no room for internal signature in " << len << std::endl;
			return true;
		}
		memcpy(signature, data, sizeof(signature));
		data += sizeof(signature);
		len -= sizeof(signature);
		signidx = nbo_to_ulong(data, sizeof(signidx));
		data += sizeof(signidx);
		len -= sizeof(signidx);
		break;
	}case LedgerSigner:{
		if(len < sizeof(signature) + sizeof(signerhash) + sizeof(signidx)){
			std::cerr << "no room for ledger signature in " << len << std::endl;
			return true;
		}
		memcpy(signature, data, sizeof(signature));
		data += sizeof(signature);
		len -= sizeof(signature);
		memcpy(signerhash, data, sizeof(signerhash));
		data += sizeof(signerhash);
		len -= sizeof(signerhash);
		signidx = nbo_to_ulong(data, sizeof(signidx));
		data += sizeof(signidx);
		len -= sizeof(signidx);
		break;
	}default:
		std::cerr << "unknown consortium sigtype " << sigtype << std::endl;
		return true;
	}
	return false;
}

bool NoOpTX::extract(const unsigned char* data, unsigned len){
	uint16_t nooptype;
	if(len != sizeof(nooptype)){
		std::cerr << "payload too large for noop: " << len << std::endl;
		return true;
	}
	nooptype = nbo_to_ulong(data, sizeof(nooptype));
	data += sizeof(nooptype);
	len -= sizeof(nooptype);
	if(nooptype){
		std::cerr << "warning: invalid noop type " << nooptype << std::endl;
		return true;
	}
	return false;
}

enum TXTypes {
	NoOp = 0x0000,
	ConsortiumMember = 0x0001,
};

// Each transaction starts with a 16-bit unsigned type. Returns nullptr on any
// failure to parse, or if the length is wrong for the transaction.
std::unique_ptr<Transaction> Transaction::lexTX(const unsigned char* data, unsigned len){
	uint16_t txtype;
	if(len < sizeof(txtype)){
		std::cerr << "no room for transaction type in " << len << std::endl;
		return 0;
	}
	txtype = nbo_to_ulong(data, sizeof(txtype));
	len -= sizeof(txtype);
	data += sizeof(txtype);
	Transaction* tx;
	switch(txtype){
	case NoOp:
		tx = new NoOpTX;
		break;
	case ConsortiumMember:
		tx = new ConsortiumMemberTX;
		break;
	default:
		std::cerr << "unknown transaction type " << txtype << std::endl;
		return nullptr;
	}
	if(tx->extract(data, len)){
		delete tx;
		return nullptr;
	}
	return std::unique_ptr<Transaction>(tx);
}

}
