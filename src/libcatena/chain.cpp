#include "libcatena/chain.h"
#include "libcatena/block.h"

namespace Catena {

Chain::Chain(const std::string& fname){
	if(blocks.loadFile(fname)){
		throw BlockValidationException();
	}
}

// A Chain instantiated from memory will not write out new blocks.
Chain::Chain(const void* data, unsigned len){
	if(blocks.loadData(data, len)){
		throw BlockValidationException();
	}
}

}
