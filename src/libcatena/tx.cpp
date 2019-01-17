#include <cstring>
#include <cstdint>
#include <iostream>
#include <libcatena/externallookuptx.h>
#include <libcatena/lookupauthreqtx.h>
#include <libcatena/ustatus.h>
#include <libcatena/usertx.h>
#include <libcatena/utility.h>
#include <libcatena/member.h>
#include <libcatena/tx.h>

namespace Catena {

// Each transaction starts with a 16-bit unsigned type. Throws
// TransactionException on an invalid or unknown type.
std::unique_ptr<Transaction> Transaction::LexTX(const unsigned char* data, unsigned len,
					const CatenaHash& blkhash, unsigned txidx){
	uint16_t txtype;
	if(len < sizeof(txtype)){
		throw TransactionException("too small for transaction type field");
	}
	txtype = nbo_to_ulong(data, sizeof(txtype));
	len -= sizeof(txtype);
	data += sizeof(txtype);
  std::unique_ptr<Transaction> tx;
	switch(static_cast<TXTypes>(txtype)){
	case TXTypes::ConsortiumMember:
		tx = std::make_unique<ConsortiumMemberTX>(blkhash, txidx);
		break;
	case TXTypes::ExternalLookup:
		tx = std::make_unique<ExternalLookupTX>(blkhash, txidx);
		break;
	case TXTypes::User:
		tx = std::make_unique<UserTX>(blkhash, txidx);
		break;
	case TXTypes::UserStatus:
		tx = std::make_unique<UserStatusTX>(blkhash, txidx);
		break;
	case TXTypes::LookupAuthReq:
		tx = std::make_unique<LookupAuthReqTX>(blkhash, txidx);
		break;
	case TXTypes::LookupAuth:
		tx = std::make_unique<LookupAuthTX>(blkhash, txidx);
		break;
	case TXTypes::UserStatusDelegation:
		tx = std::make_unique<UserStatusDelegationTX>(blkhash, txidx);
		break;
	default:
		throw TransactionException("unknown transaction type " + std::to_string(txtype));
	}
	tx->Extract(data, len);
	return tx;
}

}
