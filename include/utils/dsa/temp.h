#pragma once

#include "symbol.h"
#include "util.h"
#include <stdio.h>

typedef enum
{
  T_int,
  T_float
} T_type;

typedef struct Temp_temp_ *Temp_temp;
struct Temp_temp_
{
  int num;
  T_type type;
};

Temp_temp Temp_reg (int num, T_type ty);
Temp_temp Temp_newtemp (T_type type);
void Temp_resettemp ();
Temp_temp Temp_namedtemp (int name, T_type type);
Temp_temp this ();

typedef struct Temp_tempList_ *Temp_tempList;
struct Temp_tempList_
{
  Temp_temp head;
  Temp_tempList tail;
};
Temp_tempList Temp_TempList (Temp_temp h, Temp_tempList t);
Temp_tempList Temp_TempListSplice (Temp_tempList a, Temp_tempList b);
Temp_tempList Temp_TempListInter (Temp_tempList a, Temp_tempList b);
Temp_tempList Temp_TempListUnion (Temp_tempList a, Temp_tempList b);
Temp_tempList Temp_TempListDiff (Temp_tempList a, Temp_tempList b);
bool Temp_TempListEq (Temp_tempList a, Temp_tempList b);
bool Temp_TempInTempList (Temp_temp l, Temp_tempList ll);

typedef S_symbol Temp_label;
Temp_label Temp_newlabel (void);
Temp_label Temp_newlabel_prefix (char c);
Temp_label Temp_namedlabel (string name);
string Temp_labelstring (Temp_label s);
void Temp_resetlabel ();

typedef struct Temp_labelList_ *Temp_labelList;
struct Temp_labelList_
{
  Temp_label head;
  Temp_labelList tail;
};
Temp_labelList Temp_LabelList (Temp_label h, Temp_labelList t);
Temp_labelList Temp_LabelListSplice (Temp_labelList a, Temp_labelList b);
Temp_labelList Temp_LabelListInter (Temp_labelList a, Temp_labelList b);
Temp_labelList Temp_LabelListUnion (Temp_labelList a, Temp_labelList b);
Temp_labelList Temp_LabelListDiff (Temp_labelList a, Temp_labelList b);
bool Temp_LabelListEq (Temp_labelList a, Temp_labelList b);
bool Temp_LabelInLabelList (Temp_label l, Temp_labelList ll);

typedef struct Temp_map_ *Temp_map;
struct Temp_map_
{
  TAB_table tab;
  Temp_map under;
};
Temp_map Temp_empty (void);
Temp_map Temp_layerMap (Temp_map over, Temp_map under);
void Temp_enter (Temp_map m, Temp_temp t, string s);
string Temp_look (Temp_map m, Temp_temp t);
void Temp_dumpMap (FILE *out, Temp_map m);

Temp_map Temp_name (void);
