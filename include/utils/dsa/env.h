#pragma once

#include "dbg.h"
#include "fdmjast.h"
#include "symbol.h"
#include "temp.h"
#include "util.h"

typedef enum
{
  Ty_int,
  Ty_float,
  Ty_array,
  Ty_name
} Ty_kind;

typedef struct Ty_ty_ *Ty_ty;
typedef struct Ty_field_ *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

struct Ty_ty_
{
  Ty_kind kind;
  union
  {
    Ty_fieldList record;
    Ty_ty array;
    S_symbol name;
  } u;
};

struct Ty_field_
{
  S_symbol name;
  Ty_ty ty;
};
struct Ty_fieldList_
{
  Ty_field head;
  Ty_fieldList tail;
};

Ty_ty Ty_Int (void);
Ty_ty Ty_Float (void);
Ty_ty Ty_Array (Ty_ty ty);
Ty_ty Ty_Name (S_symbol sym);

Ty_field Ty_Field (S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList (Ty_field head, Ty_fieldList tail);

void Ty_print (FILE *out, Ty_ty t);

typedef struct E_enventry_ *E_enventry;

typedef enum
{
  E_transInit,
  E_transFind,
  E_transFill
} E_status;

typedef enum
{
  E_varEntry,
  E_classEntry,
  E_methodEntry
} E_enventryKind;

struct E_enventry_
{
  E_enventryKind kind;
  union
  {
    struct
    {
      A_varDecl vd;
      Ty_ty ty;
      Temp_temp tmp;
    } var;
    struct
    {
      A_classDecl cd;
      S_symbol fa;
      E_status status;
      S_table vtbl;
      S_table mtbl;
    } cls;
    struct
    {
      A_methodDecl md;
      S_symbol from;
      Ty_ty ret;
      Ty_fieldList fl;
    } meth;
  } u;
};

E_enventry E_VarEntry (A_varDecl vd, Ty_ty ty, Temp_temp tmp);
E_enventry E_ClassEntry (A_classDecl cd, S_symbol fa, E_status status,
                         S_table vtbl, S_table mtbl);
E_enventry E_MethodEntry (A_methodDecl md, S_symbol from, Ty_ty ret,
                          Ty_fieldList fl);
