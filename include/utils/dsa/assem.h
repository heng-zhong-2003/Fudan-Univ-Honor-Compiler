#pragma once

#include "symbol.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */

typedef struct AS_targets_ *AS_targets;
struct AS_targets_
{
  Temp_labelList labels;
};
AS_targets AS_Targets (Temp_labelList labels);

typedef struct AS_instr_ *AS_instr;
struct AS_instr_
{
  enum
  {
    I_OPER,
    I_LABEL,
    I_MOVE
  } kind;
  union
  {
    struct
    {
      string assem;
      Temp_tempList dst, src;
      AS_targets jumps;
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
AS_instr AS_Oper (string a, Temp_tempList d, Temp_tempList s, AS_targets j);
AS_instr AS_Label (string a, Temp_label label);
AS_instr AS_Move (string a, Temp_tempList d, Temp_tempList s);
void AS_format (char *, string, Temp_tempList, Temp_tempList, AS_targets,
                Temp_map);
void AS_print (FILE *out, AS_instr i, Temp_map m);

typedef struct AS_instrList_ *AS_instrList;
struct AS_instrList_
{
  AS_instr head;
  AS_instrList tail;
};
AS_instrList AS_InstrList (AS_instr head, AS_instrList tail);
AS_instrList AS_splice (AS_instrList a, AS_instrList b);
void AS_printInstrList (FILE *out, AS_instrList iList, Temp_map m);

typedef struct AS_proc_ *AS_proc;
struct AS_proc_
{
  string prolog;
  AS_instrList body;
  string epilog;
};
AS_proc AS_Proc (string p, AS_instrList b, string e);
