#include <cstring>
#include <cstdint>
#include <iostream>
#include "libcatena/utility.h"
#include "libcatena/hash.h"
#include "libcatena/sig.h"
#include "libcatena/tx.h"

namespace Catena {

enum TXTypes {
	NoOp = 0x0000,
	ConsortiumMember = 0x0001,
};

bool ConsortiumMemberTX::Extract(const unsigned char* data, unsigned len){
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for siglen in " << len << std::endl;
		return true;
	}
	siglen = nbo_to_ulong(data, 2);
	data += 2;
	len -= 2;
	if(len < HASHLEN + sizeof(signeridx)){
		std::cerr << "no room for sigspec in " << len << std::endl;
		return true;
	}
	for(int i = 0 ; i < HASHLEN ; ++i){
		signerhash[i] = data[i];
	}
	data += HASHLEN;
	len -= HASHLEN;
	signeridx = nbo_to_ulong(data, sizeof(signeridx));
	data += sizeof(signeridx);
	len -= sizeof(signeridx);
	if(len < siglen){
		std::cerr << "no room for signature in " << len << std::endl;
		return true;
	}
	memcpy(signature, data, siglen);
	data += siglen;
	len -= siglen;
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool ConsortiumMemberTX::Validate(TrustStore& tstore){
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	const unsigned char* data = payload.get();
	size_t len = payloadlen;
	uint16_t keylen;
	if(len < sizeof(keylen)){
		return true;
	}
	keylen = nbo_to_ulong(data, sizeof(keylen));
	len -= sizeof(keylen);
	data += sizeof(keylen);
	std::array<unsigned char, sizeof(blockhash)> bhash;
	memcpy(bhash.data(), blockhash, sizeof(blockhash));
	Keypair kp(data, keylen);
	tstore.addKey(&kp, {bhash, txidx});
	return false;
}

bool NoOpTX::Extract(const unsigned char* data __attribute__ ((unused)),
			unsigned len __attribute__ ((unused))){
	return false;
}

std::ostream& NoOpTX::TXOStream(std::ostream& s) const {
	return s << "NoOp";
}

std::pair<std::unique_ptr<unsigned char[]>, size_t> NoOpTX::Serialize() const {
	std::unique_ptr<unsigned char[]> ret(new unsigned char[2]);
	ulong_to_nbo(NoOp, ret.get(), 2);
	return std::make_pair(std::move(ret), 2);
}

std::ostream& ConsortiumMemberTX::TXOStream(std::ostream& s) const {
	s << "ConsortiumMember (" << siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " signer: ";
	hashOStream(s, signerhash);
	s << "[" << signeridx << "]";
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
ConsortiumMemberTX::Serialize() const {
	size_t len = 4 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = ulong_to_nbo(ConsortiumMember, ret.get(), 2);
	data = ulong_to_nbo(siglen, data, 2);
	memcpy(data, signerhash.data(), signerhash.size());
	data += signerhash.size();
	data = ulong_to_nbo(signeridx, data, 4);
	memcpy(data, signature, siglen);
	data += siglen;
	memcpy(data, payload.get(), payloadlen);
	data += payloadlen;
	return std::make_pair(std::move(ret), len);
}

// Each transaction starts with a 16-bit unsigned type. Returns nullptr on any
// failure to parse, or if the length is wrong for the transaction.
std::unique_ptr<Transaction> Transaction::lexTX(const unsigned char* data, unsigned len,
					const unsigned char* hash, unsigned idx){
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
		tx = new NoOpTX(hash, idx);
		break;
	case ConsortiumMember:
		tx = new ConsortiumMemberTX(hash, idx);
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
