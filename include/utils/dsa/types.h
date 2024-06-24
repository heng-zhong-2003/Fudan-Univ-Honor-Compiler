#pragma once

#include "symbol.h"
#include "util.h"

/*
 * types.h -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

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

void Ty_print (Ty_ty t);
