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
	return s;
}

std::pair<std::unique_ptr<const unsigned char[]>, size_t>
Chain::SerializeOutstanding(){
	unsigned char lasthash[HASHLEN];
	blocks.GetLastHash(lasthash);
	auto p = outstanding.serializeBlock(lasthash);
	HexOutput(std::cout, p.first.get(), p.second) << std::endl;
	return p;
	// FIXME need kill off outstanding
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
	std::cout << "payload: " << payload << "\n";
	std::cout << "payloaddump: " << payload.dump() << "\n";
	auto serialjson = payload.dump();
	size_t len = plen + 2 + serialjson.length();
	unsigned char buf[len];
	unsigned char *targ = buf;
	targ = ulong_to_nbo(plen, targ, 2);
	memcpy(targ, pkey, plen);
	targ += plen;
	memcpy(targ, serialjson.c_str(), serialjson.length());
	auto sig = tstore.Sign(buf, len);
	if(sig.second == 0){
		return; // FIXME throw exception? make Sign throw exception?
	}
	// FIXME need to get signer hash and signer idx
	size_t totlen = len + sig.second + 4 + HASHLEN + 2;
	unsigned char txbuf[totlen];
	targ = ulong_to_nbo(sig.second, txbuf, 2);
memset(targ, 0, HASHLEN + 4);
targ += HASHLEN + 4; // FIXME need signer hash/idx!
	memcpy(targ, sig.first.get(), sig.second);
	targ += sig.second;
	memcpy(targ, buf, len);
	auto tx = std::unique_ptr<ConsortiumMemberTX>(new ConsortiumMemberTX());
	if(tx.get()->extract(txbuf, totlen)){
		return; // FIXME throw exception?
	}
	outstanding.AddTransaction(std::move(tx));
}

}
