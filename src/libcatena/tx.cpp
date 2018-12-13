#include <cstring>
#include <cstdint>
#include <iostream>
#include "libcatena/utility.h"
#include "libcatena/hash.h"
#include "libcatena/sig.h"
#include "libcatena/tx.h"

namespace Catena {

bool ConsortiumMemberTX::extract(const unsigned char* data, unsigned len){
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for siglen in " << len << std::endl;
		return true;
	}
	siglen = nbo_to_ulong(data, 2);
	data += 2;
	len -= 2;
	if(len < sizeof(signerhash) + sizeof(signidx)){
		std::cerr << "no room for sigspec in " << len << std::endl;
		return true;
	}
	memcpy(signerhash, data, sizeof(signerhash));
	data += sizeof(signerhash);
	len -= sizeof(signerhash);
	signidx = nbo_to_ulong(data, sizeof(signidx));
	data += sizeof(signidx);
	len -= sizeof(signidx);
	if(len < siglen){
		std::cerr << "no room for signature in " << len << std::endl;
		return true;
	}
	memcpy(signature, data, siglen);
	data += siglen;
	len -= siglen;
	// FIXME handle remaning data?
	return false;
}

bool NoOpTX::extract(const unsigned char* data __attribute__ ((unused)),
			unsigned len __attribute__ ((unused))){
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
