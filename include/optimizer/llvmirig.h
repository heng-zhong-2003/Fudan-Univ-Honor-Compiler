#pragma once

#include "graph.h"
#include "llvmir.h"
#include "llvmirflowgraph.h"
#include "llvmirliveness.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void LL_Enter_ig (Temp_temp, Temp_temp);
G_nodeList LL_Create_ig (G_nodeList);
void LL_Show_ig (FILE *, G_nodeList);
void LL_Create_ig_Code (FILE *out, G_nodeList ig);
G_node LL_Look_ig (Temp_temp t);
