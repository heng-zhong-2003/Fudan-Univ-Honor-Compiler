#pragma once

#include "llvmir.h"
#include "symbol.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

typedef struct LL_block_ *LL_block;
typedef struct LL_blockList_ *LL_blockList;
struct LL_block_
{
  LL_instrList instrs;
  Temp_label label;
  Temp_labelList succs;
};
struct LL_blockList_
{
  LL_block head;
  LL_blockList tail;
};

LL_blockList LL_BlockList (LL_block head, LL_blockList tail);
LL_blockList LL_BlockSplice (LL_blockList a, LL_blockList b);
LL_block LL_Block (LL_instrList instrs);

LL_instrList LL_traceSchedule (LL_blockList bl, LL_instrList prolog,
                               LL_instrList epilog, bool optimize);
