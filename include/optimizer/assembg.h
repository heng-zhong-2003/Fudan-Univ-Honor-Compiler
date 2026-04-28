#pragma once

#include "assem.h"
#include "assemblock.h"
#include "graph.h"
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
G_nodeList AS_Create_bg (AS_blockList);

/* get bg */
G_graph AS_Bg_graph ();
S_table AS_Bg_env ();

/* print bg */
void AS_Show_bg (FILE *, G_nodeList);
