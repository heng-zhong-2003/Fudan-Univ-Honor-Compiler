#include "fdmjast.h"
#include "util.h"
#include <stdio.h>

A_pos
A_Pos (int l, int c)
{
  A_pos pos = checked_malloc (sizeof (*pos));
  pos->line = l;
  pos->pos = c;
  return pos;
}

A_type
A_Type (A_pos pos, A_dataType t, string id)
{
  A_type ty = checked_malloc (sizeof (*ty));
  ty->pos = pos;
  ty->t = t;
  ty->id = id;
  return ty;
}

A_prog
A_Prog (A_pos p, A_mainMethod m, A_classDeclList cdl)
{
  A_prog prg = checked_malloc (sizeof (*prg));
  prg->pos = p;
  prg->m = m;
  prg->cdl = cdl;
  return prg;
}

A_mainMethod
A_MainMethod (A_pos pos, A_varDeclList vdl, A_stmList stml)
{
  A_mainMethod mc = checked_malloc (sizeof (*mc));
  mc->pos = pos;
  mc->vdl = vdl;
  mc->sl = stml;
  return mc;
}

A_classDecl
A_ClassDecl (A_pos pos, string class_id, string parent_id, A_varDeclList vdl,
             A_methodDeclList mdl)
{
  A_classDecl cdl = checked_malloc (sizeof (*cdl));
  cdl->pos = pos;
  cdl->id = class_id;
  cdl->parentID = parent_id;
  cdl->vdl = vdl;
  cdl->mdl = mdl;
  return cdl;
}

A_classDeclList
A_ClassDeclList (A_classDecl h, A_classDeclList t)
{
  A_classDeclList cdl = checked_malloc (sizeof (*cdl));
  cdl->head = h;
  cdl->tail = t;
  return cdl;
}

A_varDecl
A_VarDecl (A_pos pos, A_type t, string v, A_expList el)
{
  A_varDecl vd = checked_malloc (sizeof (*vd));
  vd->pos = pos;
  vd->t = t;
  vd->v = v;
  vd->elist = el;
  return vd;
}

A_varDeclList
A_VarDeclList (A_varDecl h, A_varDeclList t)
{
  A_varDeclList vdl = checked_malloc (sizeof (*vdl));
  vdl->head = h;
  vdl->tail = t;
  return vdl;
}

A_methodDecl
A_MethodDecl (A_pos pos, A_type t, string id, A_formalList fl,
              A_varDeclList vdl, A_stmList sl)
{
  A_methodDecl md = checked_malloc (sizeof (*md));
  md->pos = pos;
  md->t = t;
  md->id = id;
  md->fl = fl;
  md->vdl = vdl;
  md->sl = sl;
  return md;
}

A_methodDeclList
A_MethodDeclList (A_methodDecl head, A_methodDeclList tail)
{
  A_methodDeclList mdl = checked_malloc (sizeof (*mdl));
  mdl->head = head;
  mdl->tail = tail;
  return mdl;
}

A_formal
A_Formal (A_pos pos, A_type t, string id)
{
  A_formal f = checked_malloc (sizeof (*f));
  f->pos = pos;
  f->t = t;
  f->id = id;
  return f;
}

A_formalList
A_FormalList (A_formal head, A_formalList tail)
{
  A_formalList fl = checked_malloc (sizeof (*fl));
  fl->head = head;
  fl->tail = tail;
  return fl;
}

A_stmList
A_StmList (A_stm h, A_stmList t)
{
  A_stmList stm = checked_malloc (sizeof (*stm));
  stm->head = h;
  stm->tail = t;
  return (stm);
}

A_stm
A_NestedStm (A_pos pos, A_stmList sl)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_nestedStm;
  s->u.ns = sl;
  return s;
}

A_stm
A_IfStm (A_pos pos, A_exp e, A_stm s1, A_stm s2)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_ifStm;
  s->u.if_stat.e = e;
  s->u.if_stat.s1 = s1;
  s->u.if_stat.s2 = s2;
  return s;
}

A_stm
A_WhileStm (A_pos pos, A_exp e, A_stm ws)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_whileStm;
  s->u.while_stat.e = e;
  s->u.while_stat.s = ws;
  return s;
}

/* March 20, 2023 redundant grammar rule removed: erase array_pos
A_stm A_AssignStm(A_pos pos, A_exp arr, A_exp array_pos, A_exp value) {
  A_stm s=checked_malloc(sizeof(*s));
  s->pos=pos;
  s->kind=A_assignStm;
  s->u.assign.arr=arr;
  s->u.assign.pos=array_pos;
  s->u.assign.value=value;
  return s;
}
*/

// March 20, 2023. updated
A_stm
A_AssignStm (A_pos pos, A_exp arr, A_exp value)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_assignStm;
  s->u.assign.arr = arr;
  s->u.assign.value = value;
  return s;
}

A_stm
A_ArrayInit (A_pos pos, A_exp arr, A_expList el)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_arrayInit, s->u.array_init.arr = arr;
  s->u.array_init.init_values = el;
  return s;
}

A_stm
A_CallStm (A_pos pos, A_exp obj, string fun, A_expList el)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_callStm, s->u.call_stat.obj = obj;
  s->u.call_stat.fun = fun;
  s->u.call_stat.el = el;
  return s;
}

A_stm
A_Continue (A_pos pos)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_continue;
  return s;
}

A_stm
A_Break (A_pos pos)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_break;
  return s;
}

A_stm
A_Return (A_pos pos, A_exp e)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_return;
  s->u.e = e;
  return s;
}

A_stm
A_Putnum (A_pos pos, A_exp e)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_putnum;
  s->u.e = e;
  return s;
}

A_stm
A_Putch (A_pos pos, A_exp e)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_putch;
  s->u.e = e;
  return s;
}

A_stm
A_Putarray (A_pos pos, A_exp e1, A_exp e2)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_putarray;
  s->u.putarray.e1 = e1;
  s->u.putarray.e2 = e2;
  return s;
}

A_stm
A_Starttime (A_pos pos)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_starttime;
  return s;
}

A_stm
A_Stoptime (A_pos pos)
{
  A_stm s = checked_malloc (sizeof (*s));
  s->pos = pos;
  s->kind = A_stoptime;
  return s;
}

A_exp
A_OpExp (A_pos pos, A_exp e1, A_binop op, A_exp e2)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_opExp;
  e->u.op.left = e1;
  e->u.op.oper = op;
  e->u.op.right = e2;
  return e;
}

A_exp
A_ArrayExp (A_pos pos, A_exp arr, A_exp arr_pos)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_arrayExp;
  e->u.array_pos.arr = arr;
  e->u.array_pos.arr_pos = arr_pos;
  return e;
}

A_exp
A_CallExp (A_pos pos, A_exp obj, string fun, A_expList el)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_callExp;
  e->u.call.obj = obj;
  e->u.call.fun = fun;
  e->u.call.el = el;
  return e;
}

A_exp
A_ClassVarExp (A_pos pos, A_exp obj, string var)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_classVarExp;
  e->u.classvar.obj = obj;
  e->u.classvar.var = var;
  return e;
}

A_exp
A_NumConst (A_pos pos, float n)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_numConst;
  e->u.num = n;
  return e;
}

A_exp
A_BoolConst (A_pos pos, bool b)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_boolConst;
  e->u.b = b;
  return e;
}

A_exp
A_LengthExp (A_pos pos, A_exp e1)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_lengthExp;
  e->u.e = e1;
  return e;
}

A_exp
A_IdExp (A_pos pos, string id)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_idExp;
  e->u.v = id;
  return e;
}

A_exp
A_ThisExp (A_pos pos)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_thisExp;
  return e;
}

A_exp
A_NewIntArrExp (A_pos pos, A_exp size)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_newIntArrExp;
  e->u.e = size;
  return e;
}

A_exp
A_NewFloatArrExp (A_pos pos, A_exp size)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_newFloatArrExp;
  e->u.e = size;
  return e;
}

A_exp
A_NewObjExp (A_pos pos, string id)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_newObjExp;
  e->u.v = id;
  return e;
}

A_exp
A_NotExp (A_pos pos, A_exp val)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_notExp;
  e->u.e = val;
  return e;
}

A_exp
A_MinusExp (A_pos pos, A_exp val)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_minusExp;
  e->u.e = val;
  return e;
}

A_exp
A_EscExp (A_pos pos, A_stmList sl, A_exp val)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_escExp;
  e->u.escExp.ns = sl;
  e->u.escExp.exp = val;
  return e;
}

A_exp
A_Getnum (A_pos pos)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_getnum;
  return e;
}

A_exp
A_Getch (A_pos pos)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_getch;
  return e;
}

A_exp
A_Getarray (A_pos pos, A_exp arr)
{
  A_exp e = checked_malloc (sizeof (*e));
  e->pos = pos;
  e->kind = A_getarray;
  e->u.e = arr;
  return e;
}

A_expList
A_ExpList (A_exp head, A_expList tail)
{
  A_expList el = checked_malloc (sizeof (*el));
  el->head = head;
  el->tail = tail;
  return el;
}
