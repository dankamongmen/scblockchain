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

bool PatientTX::Validate(TrustStore& tstore) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	// FIXME store metadata
	return false;
}

std::ostream& PatientTX::TXOStream(std::ostream& s) const {
	s << "Patient (" << siglen << "b signature, " << payloadlen
		<< "b payload, " << keylen << "b key)\n";
	s << " registrar: " << signerhash << "." << signeridx << "\n";
	// FIXME payload is encrypted!
	s << " payload: ";
	std::copy(GetPayload(), GetPayload() + GetPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
PatientTX::Serialize() const {
	size_t len = 4 + signerhash.size() + sizeof(signeridx) + siglen +
		2 + keylen + payloadlen;
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
	ret["signerhash"] = hashOString(signerhash);
	ret["signeridx"] = signeridx;
	ret["payload"] = std::string(reinterpret_cast<const char*>(GetPayload()), GetPayloadLength());
	auto pubkey = std::string(reinterpret_cast<const char*>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	return ret;
}

}
