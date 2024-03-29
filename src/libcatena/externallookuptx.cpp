#include <iostream>
#include <libcatena/externallookuptx.h>
#include <libcatena/utility.h>

namespace Catena {

void ExternalLookupTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit lookup length
		throw TransactionException("no room for lookup type");
	}
  auto ltype = nbo_to_ulong(data, 2);
	lookuptype = static_cast<ExtIDTypes>(ltype);
	if(lookuptype != ExtIDTypes::SharecareID){
    throw TransactionException("unknown external lookup type");
	}
	data += 2;
	len -= 2;
	if(len < signerhash.size() + sizeof(signeridx)){
		throw TransactionException("no room for signature spec");
	}
	memcpy(signerhash.data(), data, signerhash.size());
	data += signerhash.size();
	len -= signerhash.size();
	signeridx = nbo_to_ulong(data, sizeof(signeridx));
	data += sizeof(signeridx);
	len -= sizeof(signeridx);
	if(len < 2){
		throw TransactionException("no room for signature length");
	}
	siglen = nbo_to_ulong(data, 2);
	data += 2;
	len -= 2;
	if(len < siglen || siglen > sizeof(signature)){
		throw TransactionException("no room for signature");
	}
	memcpy(signature, data, siglen);
	data += siglen;
	len -= siglen;
	if(len < 2){
		throw TransactionException("no room for key length");
	}
	keylen = nbo_to_ulong(data, 2);
	if(keylen + 2 > len){
		throw TransactionException("no room for key");
	}
	// FIXME verify that key is valid? verify payload is valid for type?
	// Key length is part of the signed payload, so don't advance data
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
}

bool ExternalLookupTX::Validate(TrustStore& tstore,
				LedgerMap& lookups) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	const unsigned char* data = payload.get() + 2;
	Keypair kp(data, keylen);
	tstore.AddKey(&kp, {blockhash, txidx});
	lookups.AddExtLookup({signerhash, signeridx});
	return false;
}

std::ostream& ExternalLookupTX::TXOStream(std::ostream& s) const {
	s << "ExternalLookup (type " << static_cast<unsigned>(lookuptype) << ", " << siglen
		<< "b signature, " << payloadlen << "b payload, "
		<< keylen << "b key)\n";
	s << " registrar: " << signerhash << "." << signeridx << "\n";
	s << " payload: ";
	std::copy(GetPayload(), GetPayload() + GetPayloadLength(), std::ostream_iterator<char>(s, ""));
	switch(lookuptype){
    case ExtIDTypes::SharecareID: s << " (SCID)"; break;
		default: s << " (Unknown type)"; break;
	}
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
ExternalLookupTX::Serialize() const {
	size_t len = 6 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::ExternalLookup, ret.get());
	data = ulong_to_nbo(static_cast<unsigned>(lookuptype), data, 2);
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
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	ret["payload"] = std::string(reinterpret_cast<const char*>(GetPayload()), GetPayloadLength());
	auto pubkey = std::string(reinterpret_cast<const char*>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	ret["subtype"] = lookuptype;
	return ret;
}

}
