#include <cstring>
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
	// FIXME need kill off outstanding
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

void Chain::AddConsortiumMember(const unsigned char* pkey, size_t plen, nlohmann::json& payload){
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
	size_t totlen = len + sig.second + 4 + HASHLEN + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
	memcpy(targ, signer.first.data(), HASHLEN);
	targ += HASHLEN;
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

void Chain::AddExternalLookup(const unsigned char* pkey, size_t plen,
			const std::string& extid, unsigned lookuptype){
	(void)pkey;
	(void)plen;
	(void)extid;
	(void)lookuptype;
	// FIXME
}

}
