#include <cstdint>
#include <iostream>
#include "libcatena/tx.h"

enum TXTypes {
	NoOp = 0x0,
	ConsortiumMember = 0x1,
};

// Each transaction starts with a 16-bit unsigned type. Returns true on any
// failure to parse, or if the length is wrong for the transaction.
bool CatenaTX::extract(const char* data, unsigned len){
	uint16_t txtype;
	if(len < sizeof(txtype)){
		std::cerr << "no room for transaction type in " << len << " bytes" << std::endl;
		return true;
	}
	txtype = *data++ * 0x100;
	txtype += *data++;
	len -= sizeof(txtype);
	switch(txtype){
	case NoOp:
		if(len){
			std::cerr << "payload too large for noop: " << len << " bytes" << std::endl;
			return true;
		}
		break;
	case ConsortiumMember:
		// FIXME
		break;
	default:
		std::cerr << "unknown transaction type " << txtype << std::endl;
		return true;
	}
	return false;
}
