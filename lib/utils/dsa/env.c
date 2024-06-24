#include "env.h"

static struct Ty_ty_ tyint = { Ty_int };
Ty_ty
Ty_Int (void)
{
  return &tyint;
}

static struct Ty_ty_ tyfloat = { Ty_float };
Ty_ty
Ty_Float (void)
{
  return &tyfloat;
}

Ty_ty
Ty_Array (Ty_ty ty)
{
  Ty_ty p = checked_malloc (sizeof (*p));
  p->kind = Ty_array;
  p->u.array = ty;
  return p;
}

Ty_ty
Ty_Name (S_symbol sym)
{
  Ty_ty p = checked_malloc (sizeof (*p));
  p->kind = Ty_name;
  p->u.name = sym;
  return p;
}

Ty_field
Ty_Field (S_symbol name, Ty_ty ty)
{
  Ty_field p = checked_malloc (sizeof (*p));
  p->name = name;
  p->ty = ty;
  return p;
}

Ty_fieldList
Ty_FieldList (Ty_field head, Ty_fieldList tail)
{
  Ty_fieldList p = checked_malloc (sizeof (*p));
  p->head = head;
  p->tail = tail;
  return p;
}

/* printing functions - used for debugging */
static char str_ty[][12] = { "int", "float", "vector", "class", "void" };

/* This will infinite loop on mutually recursive types */
void
Ty_print (FILE *out, Ty_ty t)
{
  assert (t);
  fprintf (out, "%s", str_ty[t->kind]);
  if (t->kind == Ty_array)
    {
      fprintf (out, "<");
      Ty_print (out, t->u.array);
      fprintf (out, ">");
    }
  else if (t->kind == Ty_name)
    {
      fprintf (out, " %s", S_name (t->u.name));
    }
}

E_enventry
E_VarEntry (A_varDecl vd, Ty_ty ty, Temp_temp tmp)
{
  E_enventry entry = checked_malloc (sizeof (*entry));
  entry->kind = E_varEntry;
  entry->u.var.vd = vd;
  entry->u.var.ty = ty;
  entry->u.var.tmp = tmp;
  return entry;
}

E_enventry
E_ClassEntry (A_classDecl cd, S_symbol fa, E_status status, S_table vtbl,
              S_table mtbl)
{
  E_enventry entry = checked_malloc (sizeof (*entry));
  entry->kind = E_classEntry;
  entry->u.cls.cd = cd;
  entry->u.cls.fa = fa;
  entry->u.cls.status = status;
  entry->u.cls.vtbl = vtbl;
  entry->u.cls.mtbl = mtbl;
  return entry;
}

E_enventry
E_MethodEntry (A_methodDecl md, S_symbol from, Ty_ty ret, Ty_fieldList fl)
{
  E_enventry entry = checked_malloc (sizeof (*entry));
  entry->kind = E_methodEntry;
  entry->u.meth.md = md;
  entry->u.meth.from = from;
  entry->u.meth.ret = ret;
  entry->u.meth.fl = fl;
  return entry;
}
