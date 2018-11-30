#include <gtest/gtest.h>
#include "libcatena/block.h"

#define GENESISBLOCK_EXTERNAL "test/genesisblock"

TEST(CatenaBlock, CatenaBlockGenesisBlock){
	CatenaBlock cb;
	cb.loadFile(GENESISBLOCK_EXTERNAL);
}
