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

bool NoOpTX::Extract(const unsigned char* data __attribute__ ((unused)),
			unsigned len __attribute__ ((unused))){
	return false;
}

std::ostream& NoOpTX::TXOStream(std::ostream& s) const {
	return s << "NoOp";
}

nlohmann::json NoOpTX::JSONify() const {
	return nlohmann::json({{"type", "NoOp"}});
}

std::pair<std::unique_ptr<unsigned char[]>, size_t> NoOpTX::Serialize() const {
	std::unique_ptr<unsigned char[]> ret(new unsigned char[2]);
	TXType_to_nbo(TXTypes::NoOp, ret.get());
	return std::make_pair(std::move(ret), 2);
}

// Each transaction starts with a 16-bit unsigned type. Returns nullptr on any
// failure to parse, or if the length is wrong for the transaction.
std::unique_ptr<Transaction> Transaction::lexTX(const unsigned char* data, unsigned len,
					const CatenaHash& blkhash, unsigned txidx){
	uint16_t txtype;
	if(len < sizeof(txtype)){
		std::cerr << "no room for transaction type in " << len << std::endl;
		return 0;
	}
	txtype = nbo_to_ulong(data, sizeof(txtype));
	len -= sizeof(txtype);
	data += sizeof(txtype);
	Transaction* tx;
	switch(static_cast<TXTypes>(txtype)){
	case TXTypes::NoOp:
		tx = new NoOpTX(blkhash, txidx);
		break;
	case TXTypes::ConsortiumMember:
		tx = new ConsortiumMemberTX(blkhash, txidx);
		break;
	case TXTypes::ExternalLookup:
		tx = new ExternalLookupTX(blkhash, txidx);
		break;
	case TXTypes::User:
		tx = new UserTX(blkhash, txidx);
		break;
	case TXTypes::UserStatus:
		tx = new UserStatusTX(blkhash, txidx);
		break;
	case TXTypes::LookupAuthReq:
		tx = new LookupAuthReqTX(blkhash, txidx);
		break;
	case TXTypes::LookupAuth:
		tx = new LookupAuthTX(blkhash, txidx);
		break;
	case TXTypes::UserStatusDelegation:
		tx = new UserStatusDelegationTX(blkhash, txidx);
		break;
	default:
		std::cerr << "unknown transaction type " << txtype << std::endl;
		return nullptr;
	}
	if(tx->Extract(data, len)){
		delete tx;
		return nullptr;
	}
	return std::unique_ptr<Transaction>(tx);
}

}
