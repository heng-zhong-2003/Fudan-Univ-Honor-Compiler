#include "translate.h"
#include "env.h"
#include "fdmjast.h"
#include "semant.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

extern int SEM_ARCH_SIZE;
extern Tr_expList transA_ExpList (FILE *out, A_expList el);

/* patchList */

typedef struct patchList_ *patchList;
struct patchList_
{
  Temp_label *head;
  patchList tail;
};

static patchList
PatchList (Temp_label *head, patchList tail)
{
  patchList p = checked_malloc (sizeof (*p));
  p->head = head;
  p->tail = tail;
  return p;
}

void
doPatch (patchList pl, Temp_label tl)
{
  for (; pl; pl = pl->tail)
    *(pl->head) = tl;
}

patchList
joinPatch (patchList first, patchList second)
{
  if (!first)
    return second;
  if (!second)
    return first;
  patchList tmp = first;
  while (tmp->tail)
    tmp = tmp->tail;
  tmp->tail = second;
  return first;
}

/* Tr_exp */

typedef struct Cx_ *Cx;

struct Cx_
{
  patchList trues;
  patchList falses;
  T_stm stm;
};

struct Tr_exp_
{
  enum
  {
    Tr_ex,
    Tr_nx,
    Tr_cx
  } kind;
  union
  {
    T_exp ex;
    T_stm nx;
    Cx cx;
  } u;
};

static Tr_exp
Tr_Ex (T_exp ex)
{
  Tr_exp exp = checked_malloc (sizeof (*exp));
  exp->kind = Tr_ex;
  exp->u.ex = ex;
  return exp;
}

static Tr_exp
Tr_Nx (T_stm nx)
{
  Tr_exp exp = checked_malloc (sizeof (*exp));
  exp->kind = Tr_nx;
  exp->u.nx = nx;
  return exp;
}

static Tr_exp
Tr_Cx (patchList trues, patchList falses, T_stm stm)
{
  Tr_exp exp = checked_malloc (sizeof (*exp));
  exp->kind = Tr_cx;
  exp->u.cx = checked_malloc (sizeof (*(exp->u.cx)));
  exp->u.cx->trues = trues;
  exp->u.cx->falses = falses;
  exp->u.cx->stm = stm;
  return exp;
}

static T_exp
unEx (Tr_exp exp)
{
  if (!exp)
    return NULL;
  switch (exp->kind)
    {
    case Tr_ex:
      return exp->u.ex;
    case Tr_cx:
      {
        Temp_temp r = Temp_newtemp (T_int);
        Temp_label t = Temp_newlabel ();
        Temp_label f = Temp_newlabel ();
        Temp_label e = Temp_newlabel ();
        doPatch (exp->u.cx->trues, t);
        doPatch (exp->u.cx->falses, f);
        return T_Eseq (
            T_Seq (exp->u.cx->stm,
                   T_Seq (T_Label (t),
                          T_Seq (T_Move (T_Temp (r), T_IntConst (1)),
                                 T_Seq (T_Jump (e),
                                        T_Seq (T_Label (f),
                                               T_Seq (T_Move (T_Temp (r),
                                                              T_IntConst (0)),
                                                      T_Label (e))))))),
            T_Temp (r));
      }
    case Tr_nx:
      return T_Eseq (exp->u.nx, T_IntConst (0));
    default:
      assert (0);
    }
}

static T_stm
unNx (Tr_exp exp)
{
  if (!exp)
    return NULL;
  switch (exp->kind)
    {
    case Tr_ex:
      return T_Exp (exp->u.ex);
    case Tr_cx:
      return exp->u.cx->stm;
    case Tr_nx:
      return exp->u.nx;
    default:
      assert (0);
    }
}

static Cx
unCx (Tr_exp exp)
{
  if (!exp)
    return NULL;
  switch (exp->kind)
    {
    case Tr_ex:
      {
        T_stm stm = T_Cjump (T_ne, unEx (exp), T_IntConst (0), NULL, NULL);
        patchList trues = PatchList (&stm->u.CJUMP.t, NULL);
        patchList falses = PatchList (&stm->u.CJUMP.f, NULL);
        Tr_exp cx = Tr_Cx (trues, falses, stm);
        return cx->u.cx;
      }
    case Tr_cx:
      return exp->u.cx;
    default:
      assert (0);
    }
}

/* Tr_expList */

// struct Tr_expList_
// {
//   Tr_exp head;
//   Tr_expList tail;
// };

/* TODO: translate */

// ***********
// * methods *
// ***********

T_funcDeclList
Tr_FuncDeclList (T_funcDecl fd, T_funcDeclList fdl)
{
  if (fd == NULL)
    return fdl;
  return T_FuncDeclList (fd, fdl);
}

T_funcDecl
Tr_MainMethod (Tr_exp vdl, Tr_exp sl)
{
  return T_FuncDecl (String ("main"), NULL, unNx (Tr_StmList (vdl, sl)),
                     T_int);
}

T_funcDecl
Tr_ClassMethod (string name, Temp_tempList paras, Tr_exp vdl, Tr_exp sl,
                Ty_ty retty)
{
  return T_FuncDecl (name, paras, unNx (Tr_StmList (vdl, sl)),
                     retty->kind == Ty_float ? T_float : T_int);
}

T_funcDeclList
Tr_ChainFuncDeclList (T_funcDeclList first, T_funcDeclList second)
{
  if (first == NULL)
    return second;
  else
    return T_FuncDeclList (first->head,
                           Tr_ChainFuncDeclList (first->tail, second));
}

// **********
// *  stms  *
// **********

Tr_exp
Tr_StmList (Tr_exp head, Tr_exp tail)
{
  if (head == NULL && tail == NULL)
    return NULL;
  else if (head == NULL)
    return Tr_Nx (unNx (tail));
  else if (tail == NULL)
    return Tr_Nx (unNx (head));
  else
    return Tr_Nx (T_Seq (unNx (head), unNx (tail)));
}

Tr_exp
Tr_IfStm (Tr_exp test, Tr_exp then, Tr_exp elsee)
{
  if (then == NULL && elsee == NULL)
    {
      Temp_label join = Temp_newlabel ();
      Cx tstcx = unCx (test);
      doPatch (tstcx->falses, join);
      doPatch (tstcx->trues, join);
      return Tr_Nx (T_Seq (tstcx->stm, T_Label (join)));
    }
  else if (then == NULL)
    {
      Temp_label join = Temp_newlabel (), flslbl = Temp_newlabel ();
      Cx tstcx = unCx (test);
      doPatch (tstcx->falses, flslbl);
      doPatch (tstcx->trues, join);
      return Tr_Nx (
          T_Seq (tstcx->stm, T_Seq (T_Label (flslbl),
                                    T_Seq (unNx (elsee), T_Label (join)))));
    }
  else if (elsee == NULL)
    {
      Temp_label trulbl = Temp_newlabel ();
      Temp_label join = Temp_newlabel ();
      Cx tstcx = unCx (test);
      doPatch (tstcx->falses, join);
      doPatch (tstcx->trues, trulbl);
      return Tr_Nx (
          T_Seq (tstcx->stm, T_Seq (T_Label (trulbl),
                                    T_Seq (unNx (then), T_Label (join)))));
    }
  else
    {
      Temp_label trulbl = Temp_newlabel ();
      Temp_label flslbl = Temp_newlabel ();
      Temp_label join = Temp_newlabel ();
      Cx tstcx = unCx (test);
      doPatch (tstcx->falses, flslbl);
      doPatch (tstcx->trues, trulbl);
      return Tr_Nx (T_Seq (
          tstcx->stm, T_Seq (T_Label (trulbl),
                             T_Seq (unNx (then),
                                    T_Seq (T_Jump (join),
                                           T_Seq (T_Label (flslbl),
                                                  T_Seq (unNx (elsee),
                                                         T_Label (join))))))));
    }
}

Tr_exp
Tr_WhileStm (Tr_exp test, Tr_exp loop, Temp_label whiletest,
             Temp_label whileend)
{
  Cx tstcx = unCx (test);
  if (loop == NULL)
    {
      doPatch (tstcx->trues, whiletest);
      doPatch (tstcx->falses, whileend);
      return Tr_Nx (
          T_Seq (T_Label (whiletest), T_Seq (tstcx->stm, T_Label (whileend))));
    }
  else
    {
      Temp_label looplbl = Temp_newlabel ();
      doPatch (tstcx->trues, looplbl);
      doPatch (tstcx->falses, whileend);
      return Tr_Nx (T_Seq (
          T_Label (whiletest),
          T_Seq (tstcx->stm,
                 T_Seq (T_Label (looplbl),
                        T_Seq (unNx (loop), T_Seq (T_Jump (whiletest),
                                                   T_Label (whileend)))))));
    }
}

Tr_exp
Tr_AssignStm (Tr_exp location, Tr_exp value)
{
  // DBGPRT ("Tr_AssignStm begin\n");
  T_exp loctexp = unEx (location), valtexp = unEx (value);
  MYASRT (loctexp && valtexp);
  if (loctexp->kind != valtexp->kind)
    return Tr_Nx (T_Move (loctexp, T_Cast (valtexp, loctexp->type)));
  else
    return Tr_Nx (T_Move (loctexp, valtexp));
}

Tr_exp
Tr_Continue (Temp_label whiletest)
{
  return Tr_Nx (T_Jump (whiletest));
}

Tr_exp
Tr_Break (Temp_label whileend)
{
  return Tr_Nx (T_Jump (whileend));
}

Tr_exp
Tr_Return (Tr_exp ret)
{
  return Tr_Nx (T_Return (unEx (ret)));
}

Tr_exp
Tr_Putint (Tr_exp exp)
{
  return Tr_Nx (T_Exp (T_ExtCall (
      "putint", T_ExpList (T_Cast (unEx (exp), T_int), NULL), T_int)));
}

Tr_exp
Tr_Putfloat (Tr_exp exp)
{
  return Tr_Nx (T_Exp (T_ExtCall (
      "putfloat", T_ExpList (T_Cast (unEx (exp), T_float), NULL), T_int)));
}

Tr_exp
Tr_Putch (Tr_exp exp)
{
  return Tr_Nx (T_Exp (T_ExtCall (
      "putch", T_ExpList (T_Cast (unEx (exp), T_int), NULL), T_int)));
}

Tr_exp
Tr_Starttime ()
{
  return Tr_Nx (T_Exp (T_ExtCall (String ("starttime"), NULL, T_int)));
}

Tr_exp
Tr_Stoptime ()
{
  return Tr_Nx (T_Exp (T_ExtCall (String ("stoptime"), NULL, T_int)));
}

Tr_exp
Tr_VarDeclInt (Temp_temp tmp, int ival)
{
  return Tr_Nx (T_Move (T_Temp (tmp), T_IntConst (ival)));
}

Tr_exp
Tr_VarDeclFloat (Temp_temp tmp, float fval)
{
  return Tr_Nx (T_Move (T_Temp (tmp), T_FloatConst (fval)));
}

// **********
// *  exps  *
// **********

Tr_expList
Tr_ExpList (Tr_exp head, Tr_expList tail)
{
  if (head == NULL)
    return tail;
  Tr_expList newel = checked_malloc (sizeof (struct Tr_expList_));
  newel->head = head;
  newel->tail = tail;
  return newel;
}

Tr_exp
Tr_OpExp (A_binop op, Tr_exp left, Tr_exp right)
{
  MYASRT (left != NULL && right != NULL);
  T_stm cmpexstm = NULL;
  Cx cx1 = NULL, cx2 = NULL;
  Temp_label rghtlbl = NULL;
  patchList truptlst = NULL, flsptlst = NULL;
  switch (op)
    {
    case A_and:
      cx1 = unCx (left);
      cx2 = unCx (right);
      rghtlbl = Temp_newlabel ();
      doPatch (cx1->trues, rghtlbl);
      truptlst = cx2->trues;
      flsptlst = joinPatch (cx1->falses, cx2->falses);
      return Tr_Cx (truptlst, flsptlst,
                    T_Seq (cx1->stm, T_Seq (T_Label (rghtlbl), cx2->stm)));
    case A_or:
      cx1 = unCx (left);
      cx2 = unCx (right);
      rghtlbl = Temp_newlabel ();
      doPatch (cx1->falses, rghtlbl);
      truptlst = joinPatch (cx1->trues, cx2->trues);
      flsptlst = cx2->falses;
      return Tr_Cx (truptlst, flsptlst,
                    T_Seq (cx1->stm, T_Seq (T_Label (rghtlbl), cx2->stm)));
    case A_less:
      // DBGPRT ("temp\n");
      cmpexstm = T_Cjump (T_lt, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      // DBGPRT ("temp2\n");
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_le:
      cmpexstm = T_Cjump (T_le, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_greater:
      cmpexstm = T_Cjump (T_gt, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_ge:
      cmpexstm = T_Cjump (T_ge, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_eq:
      cmpexstm = T_Cjump (T_eq, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_ne:
      cmpexstm = T_Cjump (T_ne, unEx (left), unEx (right), NULL, NULL);
      truptlst = PatchList (&(cmpexstm->u.CJUMP.t), NULL);
      flsptlst = PatchList (&(cmpexstm->u.CJUMP.f), NULL);
      return Tr_Cx (truptlst, flsptlst, cmpexstm);
      break;
    case A_plus:
      return Tr_Ex (T_Binop (T_plus, unEx (left), unEx (right)));
      break;
    case A_minus:
      return Tr_Ex (T_Binop (T_minus, unEx (left), unEx (right)));
      break;
    case A_times:
      return Tr_Ex (T_Binop (T_mul, unEx (left), unEx (right)));
      break;
    case A_div:
      return Tr_Ex (T_Binop (T_div, unEx (left), unEx (right)));
      break;
    default:
      MYASRT (0);
      break;
    }
}

Tr_exp
Tr_BoolConst (bool b)
{
  return Tr_Ex (T_IntConst (b));
}

Tr_exp
Tr_NumConst (float num, T_type type)
{
  switch (type)
    {
    case T_int:
      return Tr_Ex (T_IntConst ((int)num));
    case T_float:
      return Tr_Ex (T_FloatConst (num));
    default:
      MYASRT (0);
    }
}

Tr_exp
Tr_IdExp (Temp_temp tmp)
{
  return Tr_Ex (T_Temp (tmp));
}

Tr_exp
Tr_NotExp (Tr_exp exp)
{
  Cx expcx = unCx (exp);
  return Tr_Cx (expcx->falses, expcx->trues, expcx->stm);
}

Tr_exp
Tr_MinusExp (Tr_exp exp)
{
  return Tr_Ex (T_Binop (T_minus, T_IntConst (0), unEx (exp)));
}

Tr_exp
Tr_EscExp (Tr_exp stm, Tr_exp exp)
{
  // DBGPRT ("begin Tr_EscExp\n");
  if (stm == NULL)
    return Tr_Ex (unEx (exp));
  else
    {
      // DBGPRT ("temp\n");
      return Tr_Ex (T_Eseq (unNx (stm), unEx (exp)));
    }
}

Tr_exp
Tr_Getfloat ()
{
  return Tr_Ex (T_ExtCall (String ("getfloat"), NULL, T_float));
}

Tr_exp
Tr_Getch ()
{
  return Tr_Ex (T_ExtCall (String ("getch"), NULL, T_int));
}

Tr_exp
Tr_Cast (Tr_exp exp, T_type type)
{
  Tr_exp retVal = Tr_Ex (T_Cast (unEx (exp), type));
  // DBGPRT ("%s, type %s\n", __func__,
  //         retVal->u.ex->type == T_float ? "float" : "int");
  return retVal;
}

/********
 * HW 8 *
 ********/

Tr_exp
Tr_ArrayInit (Tr_exp arr, Tr_expList init, T_type type)
{
  if (init == NULL)
    return NULL;
  int len = Tr_expListLen (init);
  Temp_temp tmp = Temp_newtemp (T_int);
  T_stm allocStm = T_Move (
      T_Temp (tmp),
      T_Binop (
          T_plus,
          T_ExtCall ("malloc",
                     T_ExpList (T_IntConst ((len + 1) * SEM_ARCH_SIZE), NULL),
                     T_int),
          T_IntConst (SEM_ARCH_SIZE)));
  // length of the array stored at the first slot
  T_stm lenStm = T_Move (
      T_Mem (T_Binop (T_plus, T_Temp (tmp), T_IntConst (-1 * SEM_ARCH_SIZE)),
             T_int),
      T_IntConst (len));
  Tr_expList lstiter = init;
  T_stm initStm = T_Seq (allocStm, lenStm);
  for (int curr = 0; lstiter != NULL; ++curr, lstiter = lstiter->tail)
    {
      // DBGPRT ("%s, type: %s\n", __func__, type == T_int ? "T_int" :
      // "T_float");
      T_exp memExp = T_Mem (
          T_Binop (T_plus, T_Temp (tmp), T_IntConst (curr * SEM_ARCH_SIZE)),
          type);
      T_exp srcExp = unEx (lstiter->head)->type == type
                         ? unEx (lstiter->head)
                         : T_Cast (unEx (lstiter->head), type);
      initStm = T_Seq (initStm, T_Move (memExp, srcExp));
    }
  initStm = T_Seq (initStm, T_Move (unEx (arr), T_Temp (tmp)));
  return Tr_Nx (initStm);
}

int
Tr_expListLen (Tr_expList lst)
{
  if (lst == NULL)
    return 0;
  else
    return (1 + Tr_expListLen (lst->tail));
}

Tr_exp
Tr_Putarray (Tr_exp pos, Tr_exp arr)
{
  return Tr_Nx (T_Exp (T_ExtCall (
      "putarray",
      T_ExpList (T_Cast (unEx (pos), T_int), T_ExpList (unEx (arr), NULL)),
      T_int)));
}

Tr_exp
Tr_Putfarray (Tr_exp pos, Tr_exp arr)
{
  return Tr_Nx (T_Exp (T_ExtCall (
      "putfarray",
      T_ExpList (T_Cast (unEx (pos), T_int), T_ExpList (unEx (arr), NULL)),
      T_int)));
}

Tr_exp
Tr_CallStm (string meth, Tr_exp thiz, int offset, Tr_expList el, T_type type)
{
  T_exp memExp = T_Mem (
      T_Binop (T_plus, unEx (thiz), T_IntConst (offset /* * SEM_ARCH_SIZE*/)),
      T_int);
  T_expList argsTexp = T_ExpList (unEx (Tr_TExpDeEseq (unEx (thiz))),
                                  Tr_TrExpList2TExpList (el));
  return Tr_Ex (T_Call (meth, memExp, argsTexp, type));
}

Tr_exp
Tr_CallExp (string meth, Tr_exp thiz, int offset, Tr_expList el, T_type type)
{
  Temp_temp tmp = Temp_newtemp (type);
  T_exp memExp = T_Mem (
      T_Binop (T_plus, unEx (thiz), T_IntConst (offset /* * SEM_ARCH_SIZE*/)),
      T_int);
  T_exp retVExp = T_Call (meth, memExp,
                          T_ExpList (unEx (Tr_TExpDeEseq (unEx (thiz))),
                                     Tr_TrExpList2TExpList (el)),
                          type);
  // Method call exp return value needed. Stored to tmp.
  T_exp eseqed = T_Eseq (T_Move (T_Temp (tmp), retVExp), T_Temp (tmp));
  return Tr_Ex (eseqed);
}

Tr_exp
Tr_ClassVarExp (Tr_exp thiz, int offset, T_type type)
{
  return Tr_Ex (T_Mem (
      T_Binop (T_plus, unEx (thiz), T_IntConst (offset /* * SEM_ARCH_SIZE*/)),
      type));
}

Tr_exp
Tr_LengthExp (Tr_exp arr)
{
  // Array length stored in the "-1 th" element at head.
  return Tr_ArrayExp (arr, Tr_Ex (T_IntConst (-1)), T_int);
}

Tr_exp
Tr_ArrayExp (Tr_exp arr, Tr_exp pos, T_type type)
{
  T_exp offset = T_Binop (T_mul, T_Cast (unEx (pos), T_int),
                          T_IntConst (SEM_ARCH_SIZE));
  T_exp addr = T_Binop (T_plus, unEx (arr), offset);
  return Tr_Ex (T_Mem (addr, type));
}

Tr_exp
Tr_ThisExp (Temp_temp tmp)
{
  return Tr_Ex (T_Temp (tmp));
}

Tr_exp
Tr_NewArrExp (Tr_exp size)
{
  Temp_temp tgtTmp = Temp_newtemp (T_int);
  Temp_temp lenTmp = Temp_newtemp (T_int);
  T_exp allocLen = T_Temp (lenTmp);
  // T_stm for initializing the length
  T_stm initLStm = T_Move (allocLen, T_Cast (unEx (size), T_int));
  T_stm allocStm = T_Move (
      T_Temp (tgtTmp),
      T_Binop (T_plus,
               T_ExtCall ("malloc",
                          T_ExpList (T_Binop (T_mul,
                                              T_Binop (T_plus, allocLen,
                                                       T_IntConst (1)),
                                              T_IntConst (SEM_ARCH_SIZE)),
                                     NULL),
                          T_int),
               T_IntConst (SEM_ARCH_SIZE)));
  T_stm asgnLStm = T_Move (T_Mem (T_Binop (T_plus, T_Temp (tgtTmp),
                                           T_IntConst (-1 * SEM_ARCH_SIZE)),
                                  T_int),
                           allocLen);
  T_stm escStm = T_Seq (initLStm, T_Seq (allocStm, asgnLStm));
  return Tr_Ex (T_Eseq (escStm, T_Temp (tgtTmp)));
}

Tr_exp
Tr_NewObjExp (FILE *out, E_enventry clsent, S_table vofftbl, S_table mofftbl,
              Tr_exp size)
{
  // DBGPRT ("%s called\n", __func__);
  Temp_temp tmp = Temp_newtemp (T_int);
  if (size == 0)
    return Tr_Ex (T_Temp (tmp));
  // DBGPRT ("%s, size: %d\n", __func__, unEx (size)->u.CONST.i);
  T_stm allocStm = T_Move (
      T_Temp (tmp),
      T_ExtCall ("malloc",
                 T_ExpList (T_IntConst (unEx (size)->u.CONST.i), NULL),
                 T_int));
  T_stm initOStm = NULL; // Statement for initializing the object
  Tr_initTrStmList initSLIt = NULL, initOSL = NULL;
  S_table thisVtbl = clsent->u.cls.vtbl, thisMtbl = clsent->u.cls.mtbl;
  S_symbol top = NULL;
  binder bd = NULL;
  // Seq-ed Stms that initialize variables
  // for (top = thisVtbl->top, bd = S_getBinder (thisVtbl, top); top != NULL;
  //      top = (S_symbol)bd->prevtop)
  for (top = thisVtbl->top; top != NULL;
       top = ((binder)S_getBinder (thisVtbl, top))->prevtop)
    {
      bd = S_getBinder (thisVtbl, top);
      E_enventry varent = bd->value;
      // DBGPRT ("%s, var init %s\n", __func__, S_name ((S_symbol)top));
      if (varent->u.var.vd->elist == NULL)
        continue;
      int *thisVOff = S_look (vofftbl, top);
      T_exp memLocEx
          = T_Mem (T_Binop (T_plus, T_Temp (tmp),
                            T_IntConst (*thisVOff /* * SEM_ARCH_SIZE*/)),
                   varent->u.var.ty->kind == Ty_float ? T_float : T_int);
      Tr_expList initVExp = transA_ExpList (out, varent->u.var.vd->elist);
      Tr_exp asgnStm = NULL;
      if (varent->u.var.ty->kind == Ty_array)
        asgnStm = Tr_ArrayInit (
            Tr_Ex (memLocEx), initVExp,
            varent->u.var.ty->u.array->kind == Ty_float ? T_float : T_int);
      else
        asgnStm = Tr_Nx (
            T_Move (memLocEx, T_Cast (unEx (initVExp->head), memLocEx->type)));
      T_stm newInitS = /* T_Seq (unNx (asgnStm), NULL); */ NULL;
      initOSL = Tr_ConsInitTrStmList (asgnStm, initOSL);
      // DBGPRT ("initOSL AFTER cons: %p\n", initOSL);
    }
  // Seq-ed Stms that initialize methods
  // for (top = thisMtbl->top, bd = S_getBinder (thisMtbl, top); top != NULL;
  //      top = bd->prevtop)
  for (top = thisMtbl->top; top != NULL;
       top = ((binder)S_getBinder (thisMtbl, top))->prevtop)
    {
      // DBGPRT ("%s, meth init\n", __func__);
      bd = S_getBinder (thisMtbl, top);
      E_enventry methent = bd->value;
      S_symbol mthClsId = methent->u.meth.from;
      int *thisMOff = S_look (mofftbl, top);
      T_exp memLocEx = T_Mem (
          T_Binop (T_plus, T_Temp (tmp), T_IntConst (*thisMOff)), T_int);
      char *combname = checked_malloc (128 * sizeof (char));
      sprintf (combname, "%s$%s", S_name (mthClsId), S_name (top));
      T_stm asgnMStm = T_Move (memLocEx, T_Name (Temp_namedlabel (combname)));
      initOSL = Tr_ConsInitTrStmList (Tr_Nx (asgnMStm), initOSL);
    }
  /*
  return Tr_Ex (
      T_Eseq (T_Seq (allocStm, T_Seq (initVStm, T_Seq (initMStm, NULL))),
              T_Temp (tmp)));
  return Tr_Ex (
      T_Eseq (T_Seq (allocStm, T_Seq (initVStm, initMStm)), T_Temp (tmp))); */
  initOStm = unNx (Tr_InitTrStmList2TStm (allocStm, initOSL));
  return Tr_Ex (T_Eseq (initOStm, T_Temp (tmp)));
}

Tr_exp
Tr_InitTrStmList2TStm (T_stm allocStm, Tr_initTrStmList isl)
{
  if (allocStm == NULL)
    return NULL;
  T_stm initStm = allocStm;
  // DBGPRT ("%p\n", isl);
  for (; isl != NULL; isl = isl->tail)
    {
      // DBGPRT ("temp\n");
      initStm = T_Seq (initStm, unNx (isl->head));
    }
  return Tr_Nx (initStm);
}

Tr_initTrStmList
Tr_ConsInitTrStmList (Tr_exp head, Tr_initTrStmList tail)
{
  Tr_initTrStmList newNode
      = checked_malloc (sizeof (struct Tr_initTrStmList_ *));
  newNode->head = head;
  newNode->tail = tail;
  return newNode;
}

Tr_exp
Tr_Getarray (Tr_exp exp)
{
  return Tr_Ex (T_ExtCall ("getarray", T_ExpList (unEx (exp), NULL), T_int));
}

Tr_exp
Tr_Getfarray (Tr_exp exp)
{
  return Tr_Ex (T_ExtCall ("getfarray", T_ExpList (unEx (exp), NULL), T_int));
}

Tr_exp
Tr_TExpDeEseq (T_exp exp)
{
  if (exp->kind == T_ESEQ)
    return Tr_TExpDeEseq (exp->u.ESEQ.exp);
  else
    return Tr_Ex (exp);
}

T_expList
Tr_TrExpList2TExpList (Tr_expList trexlst)
{
  if (trexlst == NULL)
    return NULL;
  else
    return T_ExpList (unEx (trexlst->head),
                      Tr_TrExpList2TExpList (trexlst->tail));
}
