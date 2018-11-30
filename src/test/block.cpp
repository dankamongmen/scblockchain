#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlocks, CatenaBlocksGenesisBlock){
	CatenaBlocks cbs;
	cbs.loadFile(GENESISBLOCK_EXTERNAL);
}
