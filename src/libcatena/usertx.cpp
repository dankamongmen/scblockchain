#include <iostream>
#include <libcatena/utility.h>
#include <libcatena/usertx.h>

namespace Catena {

void UserTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit signature length
		throw TransactionException("no room for signature length");
	}
	siglen = nbo_to_ulong(data, 2);
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
	// FIXME verify that key is valid?
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
}

bool UserTX::Validate(TrustStore& tstore, LedgerMap& lmap) {
	if(tstore.Verify({signerhash, signeridx}, payload.get(),
				payloadlen, signature, siglen)){
		return true;
	}
	Keypair kp(payload.get() + 2, keylen);
	tstore.AddKey(&kp, {blockhash, txidx});
	lmap.AddUser({blockhash, txidx}, {signerhash, signeridx});
	return false;
}

std::ostream& UserTX::TXOStream(std::ostream& s) const {
	s << "User (" << siglen << "b signature, " << payloadlen
		<< "b payload, " << keylen << "b key)\n";
	s << " registrar: " << signerhash << "." << signeridx << "\n";
	s << " payload is encrypted";
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
UserTX::Serialize() const {
	size_t len = 4 + signerhash.size() + sizeof(signeridx) + siglen + payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::User, ret.get());
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

nlohmann::json UserTX::JSONify() const {
	nlohmann::json ret({{"type", "User"}});
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

void UserStatusDelegationTX::Extract(const unsigned char* data, unsigned len) {
	if(len < 2){ // 16-bit signature length
		throw TransactionException("no room for signature length");
	}
	siglen = nbo_to_ulong(data, 2);
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
	if(len < siglen || siglen > sizeof(signature)){
		throw TransactionException("no room for signature");
	}
	memcpy(signature, data, siglen);
	data += siglen;
	len -= siglen;
	if(len < signerhash.size() + sizeof(cmidx) + 4){
		throw TransactionException("no room for cmspec + stype");
	}
	cmidx = nbo_to_ulong(data + signerhash.size(), 4);
	statustype = nbo_to_ulong(data + signerhash.size() + 4, 4);
	payload = std::unique_ptr<unsigned char[]>(new unsigned char[len]);
	memcpy(payload.get(), data, len);
	payloadlen = len;
}

bool UserStatusDelegationTX::Validate(TrustStore& tstore, LedgerMap& lmap) {
	TXSpec uspec;
	memcpy(uspec.first.data(), signerhash.data(), signerhash.size());
	uspec.second = signeridx;
	if(tstore.Verify(uspec, payload.get(), payloadlen, signature, siglen)){
		return true;
	}
	TXSpec cmspec;
	memcpy(cmspec.first.data(), payload.get(), cmspec.first.size());
	cmspec.second = cmidx;
	lmap.AddDelegation({blockhash, txidx}, cmspec, uspec, statustype);
	return false;
}

std::ostream& UserStatusDelegationTX::TXOStream(std::ostream& s) const {
	s << "UserStatusDelegation (type " << statustype << " "
		<< siglen << "b signature, " << payloadlen << "b payload)\n";
	s << " delegator: " << signerhash << "." << signeridx << "\n";
	s << " delegate: ";
	hashOStream(s, payload.get()) << "." << cmidx << "\n";
	s << " payload: ";
	std::copy(GetJSONPayload(), GetJSONPayload() + GetJSONPayloadLength(), std::ostream_iterator<char>(s, ""));
	return s;
}

std::pair<std::unique_ptr<unsigned char[]>, size_t>
UserStatusDelegationTX::Serialize() const {
	size_t len = 4 + signerhash.size() + sizeof(signeridx) + siglen + payloadlen;
	std::unique_ptr<unsigned char[]> ret(new unsigned char[len]);
	auto data = TXType_to_nbo(TXTypes::UserStatusDelegation, ret.get());
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

nlohmann::json UserStatusDelegationTX::JSONify() const {
	nlohmann::json ret({{"type", "UserStatusDelegation"}});
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
