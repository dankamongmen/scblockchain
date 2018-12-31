#include <cstring>
#include "libcatena/externallookuptx.h"
#include "libcatena/lookupauthreqtx.h"
#include "libcatena/patienttx.h"
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
	if(blocks.LoadFile(fname, tstore)){
		throw BlockValidationException();
	}
}

// A Chain instantiated from memory will not write out new blocks.
Chain::Chain(const void* data, unsigned len){
	LoadBuiltinKeys();
	if(blocks.LoadData(data, len, tstore)){
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
	if(blocks.AppendBlock(p.first.get(), p.second, tstore)){
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

void Chain::AddConsortiumMember(const unsigned char* pkey, size_t plen,
				const nlohmann::json& payload){
	// FIXME verify that pkey is a valid public key
	auto serialjson = payload.dump();
	size_t len = plen + 2 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, serialjson.c_str(), serialjson.length());
	KeyLookup signer;
	auto sig = tstore.Sign(buf, len, &signer);
	if(sig.second == 0){
		throw SigningException("couldn't sign payload");
	}
	size_t totlen = len + sig.second + 4 + signer.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, signer.first.data(), signer.first.size());
	targ += signer.first.size();
	targ = ulong_to_nbo(signer.second, targ, 4);
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
	KeyLookup signer; // FIXME enforce cmspec-ified key
	auto sig = tstore.Sign(buf, len, &signer);
	if(sig.second == 0){
		throw SigningException("couldn't sign payload");
	}
	size_t totlen = len + sig.second + 4 + signer.first.size() + 2;
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
	KeyLookup signer;
	(void)cmspec; // FIXME use cmspec to choose signer
	auto sig = tstore.Sign(buf, len, &signer);
	if(sig.second == 0){
		throw SigningException("couldn't sign payload");
	}
	size_t totlen = len + sig.second + 4 + signer.first.size() + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2); // siglen
	memcpy(targ, signer.first.data(), signer.first.size()); // sighash
	targ += signer.first.size();
	targ = ulong_to_nbo(signer.second, targ, 4); // sigidx
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
void Chain::AddLookupAuth(const TXSpec& elspec, const SymmetricKey& symkey){
	(void)symkey;
	(void)elspec; // FIXME
}

void Chain::AddPatientStatus(const TXSpec& psdspec, const nlohmann::json& payload){
	(void)psdspec;
	(void)payload; // FIXME
}

}
