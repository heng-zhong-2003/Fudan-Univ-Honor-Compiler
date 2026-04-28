#pragma once

#include "symbol.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */

typedef struct LL_targets_ *LL_targets;
struct LL_targets_
{
  Temp_labelList labels;
};
LL_targets LL_Targets (Temp_labelList labels);

typedef struct LL_instr_ *LL_instr;
struct LL_instr_
{
  enum
  {
    LL_OPER,
    LL_LABEL,
    LL_MOVE
  } kind;
  union
  {
    struct
    {
      string assem;
      Temp_tempList dst, src;
      LL_targets jumps;
    } OPER;
    struct
    {
      string assem;
      Temp_label label;
    } LABEL;
    struct
    {
      string assem;
      Temp_tempList dst, src;
    } MOVE;
  } u;
};
LL_instr LL_Oper (string a, Temp_tempList d, Temp_tempList s, LL_targets j);
LL_instr LL_Label (string a, Temp_label label);
LL_instr LL_Move (string a, Temp_tempList d, Temp_tempList s);
void LL_format (char *, string, Temp_tempList, Temp_tempList, LL_targets,
                Temp_map);
void LL_print (FILE *out, LL_instr i, Temp_map m);

typedef struct LL_instrList_ *LL_instrList;
struct LL_instrList_
{
  LL_instr head;
  LL_instrList tail;
};
LL_instrList LL_InstrList (LL_instr head, LL_instrList tail);
LL_instrList LL_splice (LL_instrList a, LL_instrList b);
void LL_printInstrList (FILE *out, LL_instrList iList, Temp_map m);

typedef struct LL_proc_ *LL_proc;
struct LL_proc_
{
  string prolog;
  LL_instrList body;
  string epilog;
};
LL_proc LL_Proc (string p, LL_instrList b, string e);
