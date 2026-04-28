#pragma once

#include "graph.h"
#include "llvmir.h"
#include "llvmirbg.h"
#include "llvmirblock.h"
#include "llvmirliveness.h"
#include "symbol.h"
#include "util.h"

LL_instrList LL_instrList_to_SSA (LL_instrList bodyil, G_nodeList lg,
                                  G_nodeList bg);
