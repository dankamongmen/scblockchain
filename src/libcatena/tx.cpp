#include <cstring>
#include <cstdint>
#include <iostream>
#include "libcatena/externallookuptx.h"
#include "libcatena/lookupauthreqtx.h"
#include "libcatena/patienttx.h"
#include "libcatena/utility.h"
#include "libcatena/hash.h"
#include "libcatena/sig.h"
#include "libcatena/tx.h"

namespace Catena {

TXSpec Transaction::StrToTXSpec(const std::string& s){
	TXSpec ret;
	// 2 chars for each hash byte, 1 for period, 1 min for txindex
	if(s.size() < 2 * ret.first.size() + 2){
		throw ConvertInputException("too small for txspec: " + s);
	}
	if(s[2 * ret.first.size()] != '.'){
		throw ConvertInputException("expected '.': " + s);
	}
	for(size_t i = 0 ; i < ret.first.size() ; ++i){
		char c1 = s[i * 2];
		char c2 = s[i * 2 + 1];
		auto hexasc_to_val = [](char nibble){
			if(nibble >= 'a' && nibble <= 'f'){
				return nibble - 'a' + 10;
			}else if(nibble >= 'A' && nibble <= 'F'){
				return nibble - 'A' + 10;
			}else if(nibble >= '0' && nibble <= '9'){
				return nibble - '0';
			}
			throw ConvertInputException("bad hex digit: " + nibble);
		};
		ret.first[i] = hexasc_to_val(c1) * 16 + hexasc_to_val(c2);
	}
	ret.second = StrToLong(s.substr(2 * ret.first.size() + 1), 0, 0xffffffff);
	return ret;
}

bool ConsortiumMemberTX::Extract(const unsigned char* data, unsigned len){
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for siglen in " << len << std::endl;
		return true;
	}
	siglen = nbo_to_ulong(data, 2);
	data += 2;
	len -= 2;
	if(len < signerhash.size() + sizeof(signeridx)){
		std::cerr << "no room for sigspec in " << len << std::endl;
		return true;
	}
	memcpy(signerhash.data(), data, signerhash.size());
	data += signerhash.size();
	len -= signerhash.size();
	signeridx = nbo_to_ulong(data, sizeof(signeridx));
	data += sizeof(signeridx);
	len -= sizeof(signeridx);
	if(len < siglen || siglen > sizeof(signature)){
		std::cerr << "no room for signature in " << len << std::endl;
		return true;
	}
	memcpy(signature, data, siglen);
	data += siglen;
	len -= siglen;
	if(len < 2){
		std::cerr << "no room for keylen in " << len << std::endl;
		return true;
	}
	keylen = nbo_to_ulong(data, 2);
	if(keylen + 2 > len){
		std::cerr << "no room for key in " << len << std::endl;
		return true;
	}
	// FIXME verify that key is valid? verify payload is valid JSON?
	// Key length is part of the signed payload, so don't advance data
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
	Keypair kp(payload.get() + 2, keylen);
	tstore.addKey(&kp, {blockhash, txidx});
	return false;
}

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

std::ostream& ConsortiumMemberTX::TXOStream(std::ostream& s) const {
	s << "ConsortiumMember (" << siglen << "b signature, " << payloadlen << "b payload, "
		<< keylen << "b key)\n";
	s << " signer: " << signerhash << "." << signeridx << "\n";
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
ConsortiumMemberTX::Serialize() const {
	size_t len = 4 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::ConsortiumMember, ret.get());
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

nlohmann::json ConsortiumMemberTX::JSONify() const {
	nlohmann::json ret({{"type", "ConsortiumMember"}});
	ret["sigbytes"] = siglen;
	ret["signerhash"] = hashOString(signerhash);
	ret["signeridx"] = signeridx;
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	auto pubkey = std::string(reinterpret_cast<const char*>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	return ret;
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
	case TXTypes::Patient:
		tx = new PatientTX(blkhash, txidx);
		break;
	case TXTypes::LookupAuthReq:
		tx = new LookupAuthReqTX(blkhash, txidx);
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
