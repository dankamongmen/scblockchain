#include <cstring>
#include <libcatena/externallookuptx.h>
#include <libcatena/lookupauthreqtx.h>
#include <libcatena/newversiontx.h>
#include <libcatena/ustatus.h>
#include <libcatena/builtin.h>
#include <libcatena/utility.h>
#include <libcatena/usertx.h>
#include <libcatena/member.h>
#include <libcatena/chain.h>
#include <libcatena/block.h>
#include <libcatena/tx.h>

namespace Catena {

// Called during Chain() constructor
void Chain::LoadBuiltinKeys(){
	BuiltinKeys bkeys;
	bkeys.AddToTrustStore(tstore);
}

Chain::Chain(const std::string& fname){
	LoadBuiltinKeys();
	if(blocks.LoadFile(fname, lmap, tstore)){
		throw BlockValidationException();
	}
}

// A Chain instantiated from memory will not write out new blocks.
Chain::Chain(const void* data, unsigned len){
	LoadBuiltinKeys();
	if(blocks.LoadData(data, len, lmap, tstore)){
		throw BlockValidationException();
	}
}

const Block& Chain::OutstandingTXs() const {
	return outstanding;
}

std::pair<std::unique_ptr<const unsigned char[]>, size_t>
Chain::SerializeOutstanding() const {
	CatenaHash lasthash;
	blocks.GetLastHash(lasthash);
	auto p = outstanding.SerializeBlock(lasthash);
	return p;
}

// This needs to operator atomically -- either while trying to commit the
// transactions, we can't modify the outstanding table (including accepting
// new transactions), or we need hide the outstanding transactions and then
// merge them back on failure.
void Chain::CommitOutstanding(){
	auto p = SerializeOutstanding();
	if(blocks.AppendBlock(p.first.get(), p.second, lmap, tstore)){
		throw BlockValidationException();
	}
	FlushOutstanding();
}

void Chain::FlushOutstanding(){
	outstanding.Flush();
}

void Chain::AddNewVersion(){
	outstanding.AddTransaction(std::make_unique<NewVersionTX>());
}

void Chain::AddConsortiumMember(const TXSpec& keyspec, const unsigned char* pkey,
				size_t plen, const nlohmann::json& payload){
	// FIXME verify that pkey is a valid public key
	auto serialjson = payload.dump();
	size_t len = plen + 2 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len, keyspec);
	size_t totlen = len + sig.second + 4 + keyspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, keyspec.first.data(), keyspec.first.size());
	targ += keyspec.first.size();
	targ = ulong_to_nbo(keyspec.second, targ, 4);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<ConsortiumMemberTX>(new ConsortiumMemberTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

// Get full block information about the specified range
std::vector<BlockDetail> Chain::Inspect(int start, int end) const {
	return blocks.Inspect(start, end);
}

BlockDetail Chain::Inspect(const CatenaHash& hash) const {
	auto idx = blocks.IdxByHash(hash);
	auto details = blocks.Inspect(idx, idx + 1);
	return std::move(details.at(0));
}

void Chain::AddLookupAuthReq(const TXSpec& cmspec, const TXSpec& elspec,
				const nlohmann::json& payload){
	auto serialjson = payload.dump();
	// Signed payload is ExternalLookup TXSpec + JSON
	size_t len = elspec.first.size() + 4 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	memcpy(targ, elspec.first.data(), elspec.first.size());
	targ += elspec.first.size();
	targ = ulong_to_nbo(elspec.second, targ, 4);
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len, cmspec);
	size_t totlen = len + sig.second + 4 + cmspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, cmspec.first.data(), cmspec.first.size());
	targ += cmspec.first.size();
	targ = ulong_to_nbo(cmspec.second, targ, 4);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<LookupAuthReqTX>(new LookupAuthReqTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

void Chain::AddExternalLookup(const TXSpec& keyspec, const unsigned char* pkey,
		size_t plen, const std::string& extid, unsigned lookuptype){
	// FIXME verify that pkey is a valid public key
	size_t len = plen + 2 + extid.size();
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, extid.c_str(), extid.size());
	auto sig = tstore.Sign(buf, len, keyspec);
	size_t totlen = len + sig.second + 4 + keyspec.first.size() + 4;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(lookuptype, txbuf, 2);
	memcpy(targ, keyspec.first.data(), keyspec.first.size());
	targ += keyspec.first.size();
	targ = ulong_to_nbo(keyspec.second, targ, 4);
	targ = ulong_to_nbo(sig.second, targ, 2);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<ExternalLookupTX>(new ExternalLookupTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

void Chain::AddUser(const TXSpec& cmspec, const unsigned char* pkey,
			size_t plen, const SymmetricKey& symkey,
			const nlohmann::json& payload){
	// FIXME verify that pkey is a valid public key
	auto serialjson = payload.dump(); // Only the JSON payload is encrypted
	auto etext = tstore.Encrypt(serialjson.data(), serialjson.length(), symkey);
	// etext.second is encrypted length, etext.first is ciphertext + IV
	// The public key, keylen, IV, and encrypted payload are all signed
	size_t len = plen + 2 + etext.second;
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, etext.first.get(), etext.second);
	auto sig = tstore.Sign(buf, len, cmspec);
	size_t totlen = len + sig.second + 4 + cmspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2); // siglen
	memcpy(targ, cmspec.first.data(), cmspec.first.size()); // sighash
	targ += cmspec.first.size();
	targ = ulong_to_nbo(cmspec.second, targ, 4); // sigidx
	memcpy(targ, sig.first.get(), sig.second); // signature
	targ += sig.second;
	memcpy(targ, buf, len); // signed payload (pkeylen, pkey, ciphertext)
	auto tx = std::unique_ptr<UserTX>(new UserTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

// FIXME can only work with AES256 (keytype 1) currently
void Chain::AddLookupAuth(const TXSpec& larspec, const TXSpec& uspec, const SymmetricKey& symkey){
	const auto& lar = lmap.LookupReq(larspec);
	TXSpec elspec = lar.ELSpec();
	TXSpec cmspec = lar.CMSpec();
	SymmetricKey derivedkey = tstore.DeriveSymmetricKey(elspec, cmspec);
	// Encrypted payload is User TXSpec, 16-bit keytype, key
	auto plainlen = symkey.size() + 2 + uspec.first.size() + 4;
	unsigned char plaintext[plainlen], *targ;
	memcpy(plaintext, uspec.first.data(), uspec.first.size());
	targ = plaintext + uspec.first.size();
	targ = ulong_to_nbo(uspec.second, targ, 4);
	targ = ulong_to_nbo(static_cast<unsigned>(LookupAuthTX::Keytype::AES256), targ, 2);
	memcpy(targ, symkey.data(), symkey.size());
	auto etext = tstore.Encrypt(plaintext, plainlen, derivedkey);
	auto sig = tstore.Sign(etext.first.get(), etext.second, elspec);
	size_t totlen = etext.second + sig.second + 4 + larspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2); // siglen
	memcpy(targ, larspec.first.data(), larspec.first.size()); // sighash
	targ += larspec.first.size();
	targ = ulong_to_nbo(larspec.second, targ, 4); // sigidx
	memcpy(targ, sig.first.get(), sig.second); // signature
	targ += sig.second;
	memcpy(targ, etext.first.get(), etext.second); // signed, encrypted payload
	auto tx = std::unique_ptr<LookupAuthTX>(new LookupAuthTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

void Chain::AddUserStatus(const TXSpec& usdspec, const nlohmann::json& payload){
	const auto& psd = lmap.LookupDelegation(usdspec);
	TXSpec cmspec = psd.CMSpec();
	auto serialjson = payload.dump();
	// Signed payload is UserStatusDelegation TXSpec + JSON
	size_t len = usdspec.first.size() + 4 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	memcpy(targ, usdspec.first.data(), usdspec.first.size());
	targ += usdspec.first.size();
	targ = ulong_to_nbo(usdspec.second, targ, 4);
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len, cmspec);
	size_t totlen = len + sig.second + 4 + cmspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, cmspec.first.data(), cmspec.first.size());
	targ += cmspec.first.size();
	targ = ulong_to_nbo(cmspec.second, targ, 4);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<UserStatusTX>(new UserStatusTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

void Chain::AddUserStatusDelegation(const TXSpec& cmspec, const TXSpec& uspec,
				int stype, const nlohmann::json& payload){
	auto serialjson = payload.dump();
	// FIXME verify that uspec and cmspec are appropriate
	size_t len = serialjson.length() + 4 + 4 + cmspec.first.size();
	unsigned char buf[len];
	unsigned char *targ = buf;
	memcpy(targ, cmspec.first.data(), cmspec.first.size());
	targ += cmspec.first.size();
	targ = ulong_to_nbo(cmspec.second, targ, 4);
	targ = ulong_to_nbo(stype, targ, 4);
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len, uspec);
	size_t totlen = len + sig.second + 4 + uspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, uspec.first.data(), uspec.first.size());
	targ += uspec.first.size();
	targ = ulong_to_nbo(uspec.second, targ, 4);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<UserStatusDelegationTX>(new UserStatusDelegationTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

nlohmann::json Chain::UserStatus(const TXSpec& uspec, unsigned stype) const {
	const auto& u = lmap.LookupUser(uspec);
	return u.Status(stype);
}

}
