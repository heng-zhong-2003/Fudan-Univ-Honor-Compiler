#pragma once

#include "graph.h"
#include "llvmir.h"
#include "llvmirflowgraph.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

G_nodeList LL_Liveness (G_nodeList);
void LL_Show_Liveness (FILE *, G_nodeList);
Temp_tempList LL_FG_Out (G_node);
Temp_tempList LL_FG_In (G_node);
