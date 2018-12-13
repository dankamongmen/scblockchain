#include <cstring>
#include "libcatena/builtin.h"
#include "libcatena/chain.h"
#include "libcatena/block.h"

namespace Catena {

void Chain::LoadBuiltinKeys(){
	BuiltinKeys bkeys;
	bkeys.AddToTrustStore(tstore);
}

Chain::Chain(const std::string& fname){
	LoadBuiltinKeys();
	if(blocks.loadFile(fname)){
		throw BlockValidationException();
	}
}

// A Chain instantiated from memory will not write out new blocks.
Chain::Chain(const void* data, unsigned len){
	LoadBuiltinKeys();
	if(blocks.loadData(data, len)){
		throw BlockValidationException();
	}
}

}
