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

void Chain::AddConsortiumMember(const std::string& pubfname, nlohmann::json& payload){
	std::cout << "fname: " << pubfname << " payload: " << payload << "\n";
	// FIXME
}

}
