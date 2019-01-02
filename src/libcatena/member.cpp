#include <iostream> // FIXME convert std::cerr to exceptions
#include <libcatena/utility.h>
#include <libcatena/member.h>

namespace Catena {

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
	// FIXME verify that key is valid?
	// Key length is part of the signed payload, so don't advance data
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
	return false;
}

bool ConsortiumMemberTX::Validate(TrustStore& tstore, LedgerMap& lmap){
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	auto jsonstr = std::string(GetJSONPayload(), GetJSONPayloadLength());
	Keypair kp(payload.get() + 2, keylen);
	tstore.AddKey(&kp, {blockhash, txidx});
	lmap.AddConsortiumMember({blockhash, txidx}, nlohmann::json::parse(jsonstr));
	return false;
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
	std::stringstream ss;
	ss << signerhash << "." << signeridx;
	ret["signerspec"] = ss.str();
	auto pload = std::string(GetJSONPayload(), GetJSONPayloadLength());
	ret["payload"] = nlohmann::json::parse(pload);
	auto pubkey = std::string(reinterpret_cast<const char *>(GetPubKey()), keylen);
	ret["pubkey"] = pubkey;
	return ret;
}

}
