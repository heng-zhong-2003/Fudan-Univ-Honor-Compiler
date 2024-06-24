#pragma once

#include "graph.h"
#include "llvmir.h"
#include "llvmirblock.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

/* Block graph: graph on AS_ basic blocks.
   This is useful to find dominance relations, etc. */

/* input AS_blockList after instruction selection for each block
   in the C_Block, generate a basic blocks graph called bg */
G_nodeList LL_Create_bg (LL_blockList);

/* get bg */
G_graph LL_Bg_graph ();
S_table LL_Bg_env ();

/* print bg */
void LL_Show_bg (FILE *, G_nodeList);
