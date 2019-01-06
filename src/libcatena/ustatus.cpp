#include <iostream>
#include <libcatena/ustatus.h>

namespace Catena {

bool UserStatusTX::Extract(const unsigned char* data, unsigned len) {
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
	if(len < signerhash.size() + 4){
		std::cerr << "no room for subjectspec in " << len << std::endl;
		return true;
	}
	usdidx = nbo_to_ulong(data + signerhash.size(), 4);
	// FIXME verify that subjectspec is valid? verify payload is valid JSON?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool UserStatusTX::Validate(TrustStore& tstore, LedgerMap& lmap) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	TXSpec usdspec;
	memcpy(usdspec.first.data(), payload.get(), usdspec.first.size());
	usdspec.second = usdidx;
	auto& usd = lmap.LookupDelegation(usdspec);
	const auto& uspec = usd.USpec();
	auto& pat = lmap.LookupUser(uspec);
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	pat.SetStatus(usd.StatusType(), nlohmann::json::parse(pload));
	return false;
}

std::ostream& UserStatusTX::TXOStream(std::ostream& s) const {
	s << "UserStatus (" << siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " publisher: " << signerhash << "." << signeridx << "\n";
	// FIXME want referenced patient spec and status type
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;

}

std::pair<std::unique_ptr<unsigned char[]>, size_t> UserStatusTX::Serialize() const {
	size_t len = 4 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::UserStatus, ret.get());
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

nlohmann::json UserStatusTX::JSONify() const {
	nlohmann::json ret({{"type", "UserStatus"}});
	ret["sigbytes"] = siglen;
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	// FIXME return referenced patient spec and status type
	ss.str(std::string());
        hashOStream(ss, payload.get());
        ss << "." << usdidx;
        ret["subjectspec"] = ss.str();
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	return ret;
}

}
