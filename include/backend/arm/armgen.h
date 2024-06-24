#pragma once

#include "assem.h"
#include "graph.h"
#include "llvmir.h"
#include "util.h"

typedef enum AS_type_ AS_type;

typedef enum AS_cmpCnd_ AS_cmpCnd;

AS_type ARM_gettype (LL_instr ins);
/* First, remove all phi functions in LLVM ir, before translating to arm asm */
// void ARM_llvmDePhi (LL_instrList il);
AS_instrList ARM_armprolog (LL_instrList il);
AS_instrList ARM_armbody (LL_instrList il);
AS_instrList ARM_armepilog (LL_instrList il);
