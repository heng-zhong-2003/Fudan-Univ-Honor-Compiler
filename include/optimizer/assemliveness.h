#pragma once

#include "assem.h"
#include "assemflowgraph.h"
#include "graph.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

G_nodeList AS_Liveness (G_nodeList);
void AS_Show_Liveness (FILE *, G_nodeList);
Temp_tempList AS_FG_Out (G_node);
Temp_tempList AS_FG_In (G_node);
