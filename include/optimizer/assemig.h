#include "assem.h"
#include "assemflowgraph.h"
#include "assemliveness.h"
#include "graph.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void AS_Enter_ig (Temp_temp, Temp_temp);
G_nodeList AS_Create_ig (G_nodeList);
void AS_Show_ig (FILE *, G_nodeList);
void AS_Create_ig_Code (FILE *out, G_nodeList ig);
G_node AS_Look_ig (Temp_temp t);
