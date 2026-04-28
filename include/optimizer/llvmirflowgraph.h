#pragma once

#include "graph.h"
#include "llvmir.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Temp_tempList LL_FG_def (G_node n);
Temp_tempList LL_FG_use (G_node n);
bool LL_FG_isMove (G_node n);
G_graph LL_FG_AssemFlowGraph (LL_instrList il);
void LL_FG_Showinfo (FILE *, LL_instr, Temp_map);
void LL_FG_show (LL_instr ins);
