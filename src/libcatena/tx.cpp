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
	ExternalLookup = 0x0002,
	Patient = 0x0003,
	PatientStatus = 0x0004,
	LookupAuthReq = 0x0005,
	LookupAuth = 0x0006,
	PatientStatusDelegation = 0x0007,
};

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
	bhash = blockhash;
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

nlohmann::json NoOpTX::JSONify() const {
	return nlohmann::json({{"type", "NoOp"}});
}

std::pair<std::unique_ptr<unsigned char[]>, size_t> NoOpTX::Serialize() const {
	std::unique_ptr<unsigned char[]> ret(new unsigned char[2]);
	ulong_to_nbo(NoOp, ret.get(), 2);
	return std::make_pair(std::move(ret), 2);
}

std::ostream& ConsortiumMemberTX::TXOStream(std::ostream& s) const {
	s << "ConsortiumMember (" << siglen << "b signature, " << payloadlen << "b payload, "
		<< keylen << "b key)\n";
	s << " signer: " << signerhash << "[" << signeridx << "]\n";
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
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

bool ExternalLookupTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for lookup type in " << len << std::endl;
		return true;
	}
	lookuptype = nbo_to_ulong(data, 2);
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
	if(len < 2){
		std::cerr << "no room for siglen in " << len << std::endl;
		return true;
	}
	siglen = nbo_to_ulong(data, 2);
	data += 2;
	len -= 2;
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

bool ExternalLookupTX::Validate(TrustStore& tstore) {
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
	bhash = blockhash;
	Keypair kp(data, keylen);
	tstore.addKey(&kp, {bhash, txidx});
	return false;
}

std::ostream& ExternalLookupTX::TXOStream(std::ostream& s) const {
	s << "ExternalLookup (type " << lookuptype << ", " << siglen
		<< "b signature, " << payloadlen << "b payload, "
		<< keylen << "b key)\n";
	s << " registrar: " << signerhash << "[" << signeridx << "]\n";
	s << " payload: ";
	std::copy(GetPayload(), GetPayload() + GetPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
ExternalLookupTX::Serialize() const {
	size_t len = 6 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = ulong_to_nbo(ExternalLookup, ret.get(), 2);
	data = ulong_to_nbo(lookuptype, data, 2);
	memcpy(data, signerhash.data(), signerhash.size());
	data += signerhash.size();
	data = ulong_to_nbo(signeridx, data, 4);
	data = ulong_to_nbo(siglen, data, 2);
	memcpy(data, signature, siglen);
	data += siglen;
	memcpy(data, payload.get(), payloadlen);
	data += payloadlen;
	return std::make_pair(std::move(ret), len);
}

nlohmann::json ExternalLookupTX::JSONify() const {
	nlohmann::json ret({{"type", "ExternalLookup"}});
	ret["sigbytes"] = siglen;
	ret["signerhash"] = hashOString(signerhash);
	ret["signeridx"] = signeridx;
	ret["payload"] = std::string(reinterpret_cast<const char*>(GetPayload()), GetPayloadLength());
	auto pubkey = std::string(reinterpret_cast<const char*>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	ret["subtype"] = lookuptype;
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
	switch(txtype){
	case NoOp:
		tx = new NoOpTX(blkhash, txidx);
		break;
	case ConsortiumMember:
		tx = new ConsortiumMemberTX(blkhash, txidx);
		break;
	case ExternalLookup:
		tx = new ExternalLookupTX(blkhash, txidx);
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
