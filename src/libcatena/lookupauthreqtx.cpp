#include <iostream>
#include <libcatena/lookupauthreqtx.h>

namespace Catena {

bool LookupAuthReqTX::Extract(const unsigned char* data, unsigned len) {
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
	// FIXME verify that specs are valid? verify payload is valid JSON?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool LookupAuthReqTX::Validate(TrustStore& tstore) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	// FIXME store metadata?
	return false;
}

std::ostream& LookupAuthReqTX::TXOStream(std::ostream& s) const {
	s << "LookupAuthReq (" << siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " requester: " << signerhash << "." << signeridx << "\n";
	// FIXME s << " elookup: " << signerhash << "." << signeridx << "\n";
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;

}

std::pair<std::unique_ptr<unsigned char[]>, size_t> LookupAuthReqTX::Serialize() const {
	size_t len = 4 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::LookupAuthReq, ret.get());
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

nlohmann::json LookupAuthReqTX::JSONify() const {
	nlohmann::json ret({{"type", "LookupAuthReq"}});
	ret["sigbytes"] = siglen;
	ret["signerhash"] = hashOString(signerhash);
	ret["signeridx"] = signeridx;
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	// EL TXSpec
	return ret;
}

}
