#include <iostream>
#include <libcatena/lookupauthreqtx.h>
#include <libcatena/chain.h>
#include <libcatena/hash.h>

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
	if(len < signerhash.size() + 4){
		std::cerr << "no room for subjectspec in " << len << std::endl;
		return true;
	}
	subjectidx = nbo_to_ulong(data + signerhash.size(), 4);
	// FIXME verify that subjectspec is valid? verify payload is valid JSON?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool LookupAuthReqTX::Validate(TrustStore& tstore, PatientMap& pmap) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	TXSpec elspec;
	memcpy(elspec.first.data(), payload.get(), elspec.first.size());
	elspec.second = subjectidx;
	auto cmspec = TXSpec(signerhash, signeridx);
	pmap.AddLookupReq({blockhash, txidx}, elspec, cmspec);
	return false;
}

std::ostream& LookupAuthReqTX::TXOStream(std::ostream& s) const {
	s << "LookupAuthReq (" << siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " requester: " << signerhash << "." << signeridx << "\n";
	s << " subject: ";
	hashOStream(s, payload.get()) << "." << subjectidx << "\n";
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
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	ss.str(std::string());
	hashOStream(ss, payload.get());
	ss << "." << subjectidx;
	ret["subjectspec"] = ss.str();
	auto pload = std::string(reinterpret_cast<const char*>(GetJSONPayload()), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	return ret;
}

bool LookupAuthTX::Extract(const unsigned char* data, unsigned len) {
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
	if(len == 0){
		std::cerr << "no room for payload in " << len << std::endl;
		return true;
	}
	// FIXME verify that elspec is valid?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

nlohmann::json LookupAuthTX::JSONify() const {
	nlohmann::json ret({{"type", "LookupAuth"}});
	ret["sigbytes"] = siglen;
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	ss.str(std::string());
	HexOutput(ss, payload.get(), payloadlen);
        ret["encpayload"] = ss.str();
	return ret;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t> LookupAuthTX::Serialize() const {
	size_t len = 4 + siglen + signerhash.size() + sizeof(signeridx) +
		payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::LookupAuth, ret.get());
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

std::ostream& LookupAuthTX::TXOStream(std::ostream& s) const {
	s << "LookupAuth (" << siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " authorizes: " << signerhash << "." << signeridx << "\n";
	s << " payload is encrypted";
	return s;

}

bool LookupAuthTX::Validate(TrustStore& tstore, PatientMap& pmap) {
	// Bears a TXSpec for a LookupAuthReq TX; need to check it to get the
	// ExternalLookup with the actual signing key.
	auto& lar = pmap.LookupReq({signerhash, signeridx});
	TXSpec elspec = lar.ELSpec();
	if(tstore.Verify(elspec, payload.get(), payloadlen, signature, siglen)){
		return true;
	}
	/* FIXME catch exceptions here, maybe do this in Extract()?
	auto cmspec = lar.CMSpec();
	auto key = tstore.DeriveSymmetricKey(elspec, cmspec);
	auto ptext = tstore.Decrypt(payload.get(), payloadlen, key);
	if(ptext.second < elspec.first.size() + 4){
		throw BlockValidationException("plaintext too small for txspec");
	}
	TXSpec patspec;
	memcpy(patspec.first.data(), ptext.first.get(), patspec.first.size());
	patspec.second = nbo_to_ulong(ptext.first.get() + patspec.first.size(), 4);
	// FIXME do something with patspec? verify it is patient? */
	lar.Authorize();
	return false;
}

}
