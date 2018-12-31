#include <cstring>
#include "libcatena/externallookuptx.h"
#include "libcatena/lookupauthreqtx.h"
#include "libcatena/patienttx.h"
#include "libcatena/pstatus.h"
#include "libcatena/builtin.h"
#include "libcatena/utility.h"
#include "libcatena/chain.h"
#include "libcatena/block.h"
#include "libcatena/tx.h"

namespace Catena {

// Called during Chain() constructor
void Chain::LoadBuiltinKeys(){
	BuiltinKeys bkeys;
	bkeys.AddToTrustStore(tstore);
}

Chain::Chain(const std::string& fname){
	LoadBuiltinKeys();
	if(blocks.LoadFile(fname, pmap, tstore)){
		throw BlockValidationException();
	}
}

// A Chain instantiated from memory will not write out new blocks.
Chain::Chain(const void* data, unsigned len){
	LoadBuiltinKeys();
	if(blocks.LoadData(data, len, pmap, tstore)){
		throw BlockValidationException();
	}
}

std::ostream& operator<<(std::ostream& stream, const Chain& chain){
	stream << chain.blocks;
	return stream;
}

std::ostream& Chain::DumpOutstanding(std::ostream& s) const {
	s << outstanding;
	auto p = SerializeOutstanding();
	return HexOutput(s, p.first.get(), p.second) << std::endl;
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
	if(blocks.AppendBlock(p.first.get(), p.second, pmap, tstore)){
		throw BlockValidationException();
	}
	FlushOutstanding();
}

void Chain::FlushOutstanding(){
	outstanding.Flush();
}

void Chain::AddSigningKey(const Keypair& kp){
	const KeyLookup& kl = tstore.GetLookup(kp);
	tstore.addKey(&kp, kl);
}

void Chain::AddNoOp(){
	outstanding.AddTransaction(std::make_unique<NoOpTX>());
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

void Chain::AddExternalLookup(const unsigned char* pkey, size_t plen,
			const std::string& extid, unsigned lookuptype){
	// FIXME verify that pkey is a valid public key
	size_t len = plen + 2 + extid.size();
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, extid.c_str(), extid.size());
	KeyLookup signer;
	auto sig = tstore.Sign(buf, len, &signer);
	if(sig.second == 0){
		throw SigningException("couldn't sign payload");
	}
	size_t totlen = len + sig.second + 4 + signer.first.size() + 4;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(lookuptype, txbuf, 2);
	memcpy(targ, signer.first.data(), signer.first.size());
	targ += signer.first.size();
	targ = ulong_to_nbo(signer.second, targ, 4);
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

void Chain::AddPatient(const TXSpec& cmspec, const unsigned char* pkey,
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
	auto tx = std::unique_ptr<PatientTX>(new PatientTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

// FIXME can only work with AES256 (keytype 1) currently
void Chain::AddLookupAuth(const TXSpec& larspec, const TXSpec& patspec, const SymmetricKey& symkey){
	const auto& lar = pmap.LookupReq(larspec);
	TXSpec elspec = lar.ELSpec();
	TXSpec cmspec = lar.CMSpec();
	SymmetricKey derivedkey = tstore.DeriveSymmetricKey(elspec, cmspec);
	// Encrypted payload is Patient TXSpec, 16-bit keytype, key
	auto plainlen = symkey.size() + 2 + patspec.first.size() + 4;
	unsigned char plaintext[plainlen], *targ;
	memcpy(plaintext, patspec.first.data(), patspec.first.size());
	targ = plaintext + patspec.first.size();
	targ = ulong_to_nbo(patspec.second, targ, 4);
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

void Chain::AddPatientStatus(const TXSpec& psdspec, const nlohmann::json& payload){
	const auto& psd = pmap.LookupDelegation(psdspec);
	TXSpec cmspec = psd.CMSpec();
	auto serialjson = payload.dump();
	// Signed payload is PatientStatusDelegation TXSpec + JSON
	size_t len = psdspec.first.size() + 4 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	memcpy(targ, psdspec.first.data(), psdspec.first.size());
	targ += psdspec.first.size();
	targ = ulong_to_nbo(psdspec.second, targ, 4);
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
	auto tx = std::unique_ptr<PatientStatusTX>(new PatientStatusTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

void Chain::AddPatientStatusDelegation(const TXSpec& cmspec, const TXSpec& patspec,
				int stype, const nlohmann::json& payload){
	auto serialjson = payload.dump();
	// FIXME verify that patspec and cmspec are appropriate
	size_t len = serialjson.length() + 4 + 4 + cmspec.first.size();
	unsigned char buf[len];
	unsigned char *targ = buf;
	memcpy(targ, cmspec.first.data(), cmspec.first.size());
	targ += cmspec.first.size();
	targ = ulong_to_nbo(cmspec.second, targ, 4);
	targ = ulong_to_nbo(stype, targ, 4);
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len, patspec);
	size_t totlen = len + sig.second + 4 + patspec.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, patspec.first.data(), patspec.first.size());
	targ += patspec.first.size();
	targ = ulong_to_nbo(patspec.second, targ, 4);
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<PatientStatusDelegationTX>(new PatientStatusDelegationTX());
	if(tx.get()->Extract(txbuf, totlen)){
		throw BlockValidationException("couldn't unpack transaction");
	}
	outstanding.AddTransaction(std::move(tx));
}

nlohmann::json Chain::PatientStatus(const TXSpec& patspec, unsigned stype) const {
	const auto& pat = pmap.LookupPatient(patspec);
	return pat.Status(stype);
}

}
