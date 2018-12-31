#include <iostream>
#include <libcatena/patienttx.h>
#include <libcatena/utility.h>

namespace Catena {

bool PatientTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for signature type in " << len << std::endl;
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
	// FIXME verify that key is valid?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool PatientTX::Validate(TrustStore& tstore, PatientMap& pmap) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	Keypair kp(payload.get() + 2, keylen);
	tstore.addKey(&kp, {blockhash, txidx});
	(void)pmap; // FIXME store Patient metadata?
	return false;
}

std::ostream& PatientTX::TXOStream(std::ostream& s) const {
	s << "Patient (" << siglen << "b signature, " << payloadlen
		<< "b payload, " << keylen << "b key)\n";
	s << " registrar: " << signerhash << "." << signeridx << "\n";
	s << " payload is encrypted";
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
PatientTX::Serialize() const {
	size_t len = 4 + signerhash.size() + sizeof(signeridx) + siglen + payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::Patient, ret.get());
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

nlohmann::json PatientTX::JSONify() const {
	nlohmann::json ret({{"type", "Patient"}});
	ret["sigbytes"] = siglen;
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	ss.str(std::string());
	HexOutput(ss, GetPayload(), GetPayloadLength());
	ret["encpayload"] = ss.str();
	auto pubkey = std::string(reinterpret_cast<const char*>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	return ret;
}

bool PatientStatusDelegationTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit signature length
		std::cerr << "no room for signature type in " << len << std::endl;
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
	if(len < signerhash.size() + sizeof(cmidx) + 4){
		std::cerr << "no room for cmspec+stype in " << len << std::endl;
		return true;
	}
	cmidx = nbo_to_ulong(data + signerhash.size(), 4);
	statustype = nbo_to_ulong(data + signerhash.size() + 4, 4);
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool PatientStatusDelegationTX::Validate(TrustStore& tstore, PatientMap& pmap) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	(void)pmap; // FIXME store PatientStatusDelegation metadata?
	return false;
}

std::ostream& PatientStatusDelegationTX::TXOStream(std::ostream& s) const {
	s << "PatientStatusDelegation (type " << statustype << " "
		<< siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " delegator: " << signerhash << "." << signeridx << "\n";
	s << " delegate: ";
	hashOStream(s, payload.get()) << "." << cmidx << "\n";
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
PatientStatusDelegationTX::Serialize() const {
	size_t len = 4 + signerhash.size() + sizeof(signeridx) + siglen + payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::PatientStatusDelegation, ret.get());
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

nlohmann::json PatientStatusDelegationTX::JSONify() const {
	nlohmann::json ret({{"type", "PatientStatusDelegation"}});
	ret["sigbytes"] = siglen;
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	ss.str(std::string());
        hashOStream(ss, payload.get());
        ss << "." << cmidx;
        ret["subjectspec"] = ss.str();
	ret["subtype"] = statustype;
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	return ret;
}
}
