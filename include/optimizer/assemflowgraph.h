#pragma once

#include "assem.h"
#include "graph.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Temp_tempList AS_FG_def (G_node n);
Temp_tempList AS_FG_use (G_node n);
bool AS_FG_isMove (G_node n);
G_graph AS_FG_AssemFlowGraph (AS_instrList il);
void AS_FG_Showinfo (FILE *, AS_instr, Temp_map);
void AS_FG_show (AS_instr ins);
