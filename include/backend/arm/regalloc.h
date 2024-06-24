#include "assem.h"
#include "assemflowgraph.h"
#include "assemig.h"
#include "assemliveness.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

typedef struct REG_allocRet REG_allocRet_t;
struct REG_allocRet
{
  AS_instrList il;
  Temp_map map;
};
REG_allocRet_t REG_regalloc (AS_instrList il, G_nodeList ig,
                             G_nodeList igCopy);
