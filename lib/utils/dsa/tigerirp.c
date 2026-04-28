/* IR+ for 2024 with types */
#include "tigerirp.h"

T_funcDecl
T_FuncDecl (string name, Temp_tempList tl, T_stm s, T_type ret_type)
{
  T_funcDecl p = (T_funcDecl)checked_malloc (sizeof *p);
  p->name = name;
  p->args = tl;
  p->stm = s;
  p->ret_type = ret_type;
  return p;
}

T_funcDeclList
T_FuncDeclList (T_funcDecl head, T_funcDeclList tail)
{
  T_funcDeclList p = (T_funcDeclList)checked_malloc (sizeof *p);
  p->head = head;
  p->tail = tail;
  return p;
}

T_expList
T_ExpList (T_exp head, T_expList tail)
{
  T_expList p = (T_expList)checked_malloc (sizeof *p);
  p->head = head;
  p->tail = tail;
  return p;
}

T_stmList
T_StmList (T_stm head, T_stmList tail)
{
  T_stmList p = (T_stmList)checked_malloc (sizeof *p);
  p->head = head;
  p->tail = tail;
  return p;
}

T_stm
T_Seq (T_stm left, T_stm right)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_SEQ;
  p->u.SEQ.left = left;
  p->u.SEQ.right = right;
  return p;
}

T_stm
T_Label (Temp_label label)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_LABEL;
  p->u.LABEL = label;
  return p;
}

T_stm
T_Jump (Temp_label label)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_JUMP;
  p->u.JUMP.jump = label;
  return p;
}

T_stm
T_Cjump (T_relOp op, T_exp left, T_exp right, Temp_label t, Temp_label f)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_CJUMP;
  p->u.CJUMP.op = op;
  if (left->type == T_float || right->type == T_float)
    {
      left = T_Cast (left, T_float);
      right = T_Cast (right, T_float);
    }
  p->u.CJUMP.left = left;
  p->u.CJUMP.right = right;
  p->u.CJUMP.t = t;
  p->u.CJUMP.f = f;
  return p;
}

T_stm
T_Move (T_exp dst, T_exp src)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_MOVE;
  p->u.MOVE.dst = dst;
  p->u.MOVE.src = src;
  return p;
}

T_stm
T_Return (T_exp exp)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_RETURN;
  p->u.EXP = exp;
  return p;
}

T_stm
T_Exp (T_exp exp)
{
  T_stm p = (T_stm)checked_malloc (sizeof *p);
  p->kind = T_EXP;
  p->u.EXP = exp;
  return p;
}

T_exp
T_Binop (T_binOp op, T_exp left, T_exp right)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_BINOP;
  p->type = T_int;
  if (left->type == T_float || right->type == T_float)
    {
      left = T_Cast (left, T_float);
      right = T_Cast (right, T_float);
      p->type = T_float;
    }
  p->u.BINOP.op = op;
  p->u.BINOP.left = left;
  p->u.BINOP.right = right;
  return p;
}

T_exp
T_Mem (T_exp exp, T_type type)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_MEM;
  p->type = type;
  p->u.MEM = exp;
  return p;
}

T_exp
T_Temp (Temp_temp temp)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_TEMP;
  p->type = temp->type; // the type of a TEMP Exp is the same as the type of
                        // the TEMP
  p->u.TEMP = temp;
  return p;
}

T_exp
T_Eseq (T_stm stm, T_exp exp)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_ESEQ;
  p->type = exp->type;
  p->u.ESEQ.stm = stm;
  p->u.ESEQ.exp = exp;
  return p;
}

T_exp
T_Name (Temp_label name)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_NAME;
  p->type = T_int;
  p->u.NAME = name;
  return p;
}

T_exp
T_IntConst (int i)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_CONST;
  p->type = T_int;
  p->u.CONST.i = i;
  return p;
}

T_exp
T_FloatConst (float f)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_CONST;
  p->type = T_float;
  p->u.CONST.f = f;
  return p;
}

T_exp
T_Call (string id, T_exp obj, T_expList args, T_type type)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_CALL;
  p->type = type;
  p->u.CALL.id = id;
  p->u.CALL.obj = obj;
  p->u.CALL.args = args;
  return p;
}

T_exp
T_ExtCall (string id, T_expList args, T_type type)
{
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_ExtCALL;
  p->type = type; // the type of an ExtCall is the same as the type of the
                  // function. If the external returns void, use T_int
  p->u.ExtCALL.extfun = id;
  p->u.ExtCALL.args = args;
  return p;
}

T_exp
T_Cast (T_exp exp, T_type type)
{
  if (exp->type == type)
    return exp; // no need to cast
  T_exp p = (T_exp)checked_malloc (sizeof *p);
  p->kind = T_CAST;
  p->type = type;
  p->u.CAST = exp;
  return p;
}

T_relOp
T_notRel (T_relOp r)
{
  switch (r)
    {
    case T_eq:
      return T_ne;
    case T_ne:
      return T_eq;
    case T_lt:
      return T_ge;
    case T_ge:
      return T_lt;
    case T_gt:
      return T_le;
    case T_le:
      return T_gt;
    case T_ult:
      return T_uge;
    case T_uge:
      return T_ult;
    case T_ule:
      return T_ugt;
    case T_ugt:
      return T_ule;
    }
  assert (0);
  return 0;
}

T_relOp
T_commute (T_relOp r)
{
  switch (r)
    {
    case T_eq:
      return T_eq;
    case T_ne:
      return T_ne;
    case T_lt:
      return T_gt;
    case T_ge:
      return T_le;
    case T_gt:
      return T_lt;
    case T_le:
      return T_ge;
    case T_ult:
      return T_ugt;
    case T_uge:
      return T_ule;
    case T_ule:
      return T_uge;
    case T_ugt:
      return T_ult;
    }
  assert (0);
  return 0;
}
