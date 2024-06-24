#include "llvmgen.h"
#include "assem.h"
#include "dbg.h"
#include "llvmir.h"
#include "print_stm.h"
#include "symbol.h"
#include "temp.h"
#include "tigerirp.h"
#include "tile.h"
#include "util.h"
#include <complex.h>
#include <stdio.h>
#include <string.h>

#define LLVMGEN_DEBUG
#undef LLVMGEN_DEBUG

#define INSTRLEN 200
#define OPLEN 8

// These are for collecting the AS instructions into a list (i.e., iList).
//   last is the last instruction in ilist
static LL_instrList iList = NULL, last = NULL;
static void munchStm (T_stm stm);
static Temp_temp munchExp (T_exp exp);
static Temp_tempList munchArgLst (int argord, T_expList arglst);
static void cnd2Str (char *destStr, T_relOp cnd, T_type ty);
static void bop2Str (char *destStr, T_binOp bop, T_type ty);
static void Ttype2Str (char *destStr, T_type ty);
static int tempListLen (Temp_tempList tmplst);
static void catFormalArgLst (Temp_tempList pars, char *destStr, int i);
static int stmCall = 0;

static char *
temp2Str (Temp_temp tmp)
{
  char *ret = calloc (32, sizeof (char));
  sprintf (ret, "TMP%d#%s", tmp->num, tmp->type == T_int ? "INT" : "FLOAT");
  return ret;
}

static void
emit (LL_instr inst)
{
  if (last)
    {
      // add the instruction to the (nonempty) ilist to the end
      last = last->tail = LL_InstrList (inst, NULL);
    }
  else
    {
      // if this is the first instruction, make it the head of the list
      last = iList = LL_InstrList (inst, NULL);
    }
}

static Temp_tempList
TL (Temp_temp t, Temp_tempList tl)
{
  return Temp_TempList (t, tl);
}

static Temp_tempList
TLS (Temp_tempList a, Temp_tempList b)
{
  return Temp_TempListSplice (a, b);
}

static Temp_labelList
LL (Temp_label l, Temp_labelList ll)
{
  return Temp_LabelList (l, ll);
}

/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE FOLLOWING FUNCTION FOR HW9_10 */
/* ********************************************************/

LL_instrList
llvmbody (T_stmList stmList)
{
  if (!stmList)
    return NULL;
  iList = last = NULL;
  while (stmList)
    {
      munchStm (stmList->head);
      stmList = stmList->tail;
    }
  return iList;
}

static void
munchStm (T_stm stm)
{
  // static int AAA = 1;
  // DBGPRT ("%d, ", AAA);
  // ++AAA;
  // DBGPRT ("stmKind: %d; ", stm->kind);
  TL_tileKind patt = TL_matchTile (TL_StmConstr (stm));
  Temp_labelList jmplst = NULL;
  Temp_temp cmptmp = NULL, subtmp1 = NULL, subtmp2 = NULL, midtmp1 = NULL;
  char *asmStr1 = checked_malloc (INSTRLEN * sizeof (char)),
       *asmStr2 = checked_malloc (INSTRLEN * sizeof (char)),
       *cndStr = checked_malloc (OPLEN * sizeof (char)),
       *tyStr = checked_malloc (8 * sizeof (char));
  memset (asmStr1, '\0', INSTRLEN);
  memset (asmStr2, '\0', INSTRLEN);
  memset (cndStr, '\0', OPLEN);
  memset (tyStr, '\0', 8);
  switch (patt)
    {
    case TL_stmLabel:
      sprintf (asmStr1, "%s:", Temp_labelstring (stm->u.LABEL));
      // DBGPRT ("%s\n", asmStr1);
      emit (LL_Label (asmStr1, stm->u.LABEL));
      break;
    case TL_stmUncondBranch:
      sprintf (asmStr1, "br label %%`j0");
      jmplst = Temp_LabelList (stm->u.JUMP.jump, NULL);
      emit (LL_Oper (asmStr1, NULL, NULL, LL_Targets (jmplst)));
      break;
    case TL_stmCondBranchTempConst:
      cmptmp = Temp_newtemp (T_int);
      jmplst = Temp_LabelList (stm->u.CJUMP.t,
                               Temp_LabelList (stm->u.CJUMP.f, NULL));
      cnd2Str (cndStr, stm->u.CJUMP.op, stm->u.CJUMP.left->type);
      if (stm->u.CJUMP.left->type == T_int
          && stm->u.CJUMP.right->type == T_int)
        {
          if (stm->u.CJUMP.left->kind == T_TEMP
              && stm->u.CJUMP.right->kind == T_TEMP)
            {
              sprintf (asmStr1, "%%`d0 = icmp %s i64 %%`s0, %%`s1", cndStr);
              emit (LL_Oper (
                  asmStr1, Temp_TempList (cmptmp, NULL),
                  Temp_TempList (
                      stm->u.CJUMP.left->u.TEMP,
                      Temp_TempList (stm->u.CJUMP.right->u.TEMP, NULL)),
                  NULL));
            }
          else if (stm->u.CJUMP.left->kind == T_CONST
                   && stm->u.CJUMP.right->kind == T_TEMP)
            {
              sprintf (asmStr1, "%%`d0 = icmp %s i64 %d, %%`s0", cndStr,
                       stm->u.CJUMP.left->u.CONST.i);
              emit (LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL),
                             Temp_TempList (stm->u.CJUMP.right->u.TEMP, NULL),
                             NULL));
            }
          else if (stm->u.CJUMP.left->kind == T_TEMP
                   && stm->u.CJUMP.right->kind == T_CONST)
            {
              sprintf (asmStr1, "%%`d0 = icmp %s i64 %%`s0, %d", cndStr,
                       stm->u.CJUMP.right->u.CONST.i);
              emit (LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL),
                             Temp_TempList (stm->u.CJUMP.left->u.TEMP, NULL),
                             NULL));
            }
          else
            {
              sprintf (asmStr1, "%%`d0 = icmp %s i64 %d, %d", cndStr,
                       stm->u.CJUMP.left->u.CONST.i,
                       stm->u.CJUMP.right->u.CONST.i);
              emit (
                  LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL), NULL, NULL));
            }
        }
      else if (stm->u.CJUMP.left->type == T_float
               && stm->u.CJUMP.right->type == T_float)
        {
          if (stm->u.CJUMP.left->kind == T_TEMP
              && stm->u.CJUMP.right->kind == T_TEMP)
            {
              sprintf (asmStr1, "%%`d0 = fcmp %s double %%`s0, %%`s1", cndStr);
              emit (LL_Oper (
                  asmStr1, Temp_TempList (cmptmp, NULL),
                  Temp_TempList (
                      stm->u.CJUMP.left->u.TEMP,
                      Temp_TempList (stm->u.CJUMP.right->u.TEMP, NULL)),
                  NULL));
            }
          else if (stm->u.CJUMP.left->kind == T_CONST
                   && stm->u.CJUMP.right->kind == T_TEMP)
            {
              sprintf (asmStr1, "%%`d0 = fcmp %s double %f, %%`s0", cndStr,
                       stm->u.CJUMP.left->u.CONST.f);
              emit (LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL),
                             Temp_TempList (stm->u.CJUMP.right->u.TEMP, NULL),
                             NULL));
            }
          else if (stm->u.CJUMP.left->kind == T_TEMP
                   && stm->u.CJUMP.right->kind == T_CONST)
            {
              sprintf (asmStr1, "%%`d0 = fcmp %s double %%`s0, %f", cndStr,
                       stm->u.CJUMP.right->u.CONST.f);
              emit (LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL),
                             Temp_TempList (stm->u.CJUMP.left->u.TEMP, NULL),
                             NULL));
            }
          else
            {
              sprintf (asmStr1, "%%`d0 = fcmp %s double %f, %f", cndStr,
                       stm->u.CJUMP.left->u.CONST.f,
                       stm->u.CJUMP.right->u.CONST.f);
              emit (
                  LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL), NULL, NULL));
            }
        }
      else
        MYASRT (0);
      sprintf (asmStr2, "br i1 %%`s0, label %%`j0, label %%`j1");
      emit (LL_Oper (asmStr2, NULL, Temp_TempList (cmptmp, NULL),
                     LL_Targets (jmplst)));
      break;
    case TL_stmCondBranchOther:
      /* T_Cjump (cnd, <expLeft>, <expRight>, labelTrueJDest, labelFalseJDest)
       */
      subtmp1 = munchExp (stm->u.CJUMP.left);
      subtmp2 = munchExp (stm->u.CJUMP.right);
      cnd2Str (cndStr, stm->u.CJUMP.op, subtmp1->type);
      cmptmp = Temp_newtemp (T_int);
      jmplst = Temp_LabelList (stm->u.CJUMP.t,
                               Temp_LabelList (stm->u.CJUMP.f, NULL));
      if (subtmp1->type == T_int && subtmp2->type == T_int)
        sprintf (asmStr1, "%%`d0 = icmp %s i64 %%`s0, %%`s1", cndStr);
      else if (subtmp1->type == T_float && subtmp2->type == T_float)
        sprintf (asmStr1, "%%`d0 = fcmp %s double %%`s0, %%`s1", cndStr);
      else
        MYASRT (0);
      emit (LL_Oper (asmStr1, Temp_TempList (cmptmp, NULL),
                     Temp_TempList (subtmp1, Temp_TempList (subtmp2, NULL)),
                     NULL));
      sprintf (asmStr2, "br i1 %%`s0, label %%`j0, label %%`j1");
      emit (LL_Oper (asmStr2, NULL, Temp_TempList (cmptmp, NULL),
                     LL_Targets (jmplst)));
      break;
    case TL_stmMoveMallocTemp:
      /* T_Move (T_Temp (temp), T_ExtCall ("malloc", <expSize>, type)) */
      /* <expSize>, the first argument for calling malloc */
      subtmp1 = munchExp (stm->u.MOVE.src->u.ExtCALL.args->head);
      /* This subtmp2 is for receiving the i64* type return value of malloc */
      midtmp1 = Temp_newtemp (T_int);
      sprintf (asmStr1, "%%`d0 = call i64* @malloc(i64 %%`s0)");
      emit (LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL),
                     Temp_TempList (subtmp1, NULL), NULL));
      sprintf (asmStr2, "%%`d0 = ptrtoint i64* %%`s0 to i64");
      emit (LL_Oper (asmStr2, Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                     Temp_TempList (midtmp1, NULL), NULL));
      break;
    case TL_stmMoveLoad:
      /* T_Move (T_Temp (tempDest), T_Mem (<expAddr>, type)) */
      subtmp1 = munchExp (stm->u.MOVE.src->u.MEM);
      Ttype2Str (tyStr, stm->u.MOVE.dst->u.TEMP->type);
      sprintf (asmStr2, "%%`d0 = inttoptr i64 %%`s0 to i64*");
      midtmp1 = Temp_newtemp (T_int);
      // DBGPRT ("midtmp1: %s\n", temp2Str (midtmp1));
      // DBGPRT ("movdst: %s\n", temp2Str (stm->u.MOVE.dst->u.TEMP));
      // DBGPRT ("tystr: %s\n", tyStr);
      emit (LL_Oper (asmStr2, Temp_TempList (midtmp1, NULL),
                     Temp_TempList (subtmp1, NULL), NULL));
      sprintf (asmStr1, "%%`d0 = load %s, i64* %%`s0", tyStr);
      emit (LL_Oper (asmStr1, Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                     Temp_TempList (midtmp1, NULL), NULL));
      break;
    case TL_stmMoveStore:
      /* T_Move (T_Mem (<expAddr>, type), <expSrc>) */
      subtmp1 = munchExp (stm->u.MOVE.dst->u.MEM); /* i64 dest temp */
      subtmp2 = munchExp (stm->u.MOVE.src);
      midtmp1 = Temp_newtemp (T_int); /* i64* dest temp */
      /* type of dest mem content */
      Ttype2Str (tyStr, stm->u.MOVE.src->type);
      /* NEED INSPECT */
      sprintf (asmStr1, "%%`d0 = inttoptr i64 %%`s0 to i64*");
      // DBGPRT ("%s, TL_stmMoveLoad, temp %d\n", __func__, midtmp1->num);
      emit (LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL),
                     Temp_TempList (subtmp1, NULL), NULL));
      sprintf (asmStr2, "store %s %%`s0, i64* %%`s1", tyStr);
      emit (LL_Oper (asmStr2, NULL,
                     Temp_TempList (subtmp2, Temp_TempList (midtmp1, NULL)),
                     NULL));
      break;
    case TL_stmMoveTempConst:
      /* T_Move (T_Temp (labelDest),
       *         [T_Temp (labelSrc)/T_[Int/Float]Const (numSrc)]) */
      MYASRT (stm->u.MOVE.src->type == stm->u.MOVE.dst->type);
      Ttype2Str (tyStr, stm->u.MOVE.dst->type);
      if (stm->u.MOVE.src->kind == T_TEMP)
        {
          if (stm->u.MOVE.src->type == T_int)
            sprintf (asmStr1, "%%`d0 = add %s %%`s0, 0", tyStr);
          else
            sprintf (asmStr1, "%%`d0 = fadd %s %%`s0, 0.0", tyStr);
          emit (LL_Oper (asmStr1,
                         Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                         Temp_TempList (stm->u.MOVE.src->u.TEMP, NULL), NULL));
        }
      else if (stm->u.MOVE.src->kind == T_CONST)
        {
          if (stm->u.MOVE.src->type == T_int)
            {
              sprintf (asmStr1, "%%`d0 = add i64 %d, 0",
                       stm->u.MOVE.src->u.CONST.i);
              emit (LL_Oper (asmStr1,
                             Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                             NULL, NULL));
            }
          else
            {
              sprintf (asmStr1, "%%`d0 = fadd double %f, 0.0",
                       stm->u.MOVE.src->u.CONST.f);
              emit (LL_Oper (asmStr1,
                             Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                             NULL, NULL));
            }
        }
      else
        MYASRT (0);
      break;
    case TL_stmMoveToTemp:
      /* T_Move (T_Temp (tempDest), <expSrc>) */
      MYASRT (stm->u.MOVE.src->type == stm->u.MOVE.dst->type);
      subtmp1 = munchExp (stm->u.MOVE.src);
      if (stm->u.MOVE.dst->type == T_int)
        {
          sprintf (asmStr1, "%%`d0 = add i64 %%`s0, 0");
          emit (LL_Oper (asmStr1,
                         Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
        }
      else
        {
          sprintf (asmStr1, "%%`d0 = fadd double %%`s0, 0.0");
          emit (LL_Oper (asmStr1,
                         Temp_TempList (stm->u.MOVE.dst->u.TEMP, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
        }
      break;
    case TL_stmTExp:
      /* T_Exp (<exp>) */
      stmCall = 1;
      munchExp (stm->u.EXP);
      stmCall = 0;
      break;
    case TL_stmRet:
      {
        /* T_Return (<exp>) */
        // DBGPRT ("beg\n");
        subtmp1 = munchExp (stm->u.EXP);
        LL_targets targets
            = LL_Targets (Temp_LabelList (Temp_newlabel_prefix ('Z'), NULL));
        Ttype2Str (tyStr, stm->u.EXP->type);
        sprintf (asmStr1, "ret %s %%`s0", tyStr);
        emit (LL_Oper (asmStr1, NULL, Temp_TempList (subtmp1, NULL), targets));
        // DBGPRT ("end\n");
      }
      break;
    default:
      MYASRT (0);
      break;
    }
}

static Temp_temp
munchExp (T_exp exp)
{
  // DBGPRT ("expKind: %d; ", exp->kind);
  TL_tileKind patt = TL_matchTile (TL_ExpConstr (exp));
  Temp_temp rsltTmp = NULL, subtmp1 = NULL, subtmp2 = NULL, midtmp1 = NULL;
  Temp_tempList arglst = NULL;
  char *asmStr1 = checked_malloc (INSTRLEN * sizeof (char)),
       *asmStr2 = checked_malloc (INSTRLEN * sizeof (char)),
       *bopStr = checked_malloc (OPLEN * sizeof (char)),
       *tyStr = checked_malloc (8 * sizeof (char));
  memset (asmStr1, '\0', INSTRLEN);
  memset (asmStr2, '\0', INSTRLEN);
  memset (tyStr, '\0', 8);
  switch (patt)
    {
    case TL_expBinopTempConst:
      /* T_Binop (oper, [T_Temp (tempL)/T_[Int/Float]Const (numL)],
       *          [T_Temp (tempR)/T_[Int/Float]Const (numR)]) */
      MYASRT (exp->u.BINOP.left->type == exp->u.BINOP.right->type);
      rsltTmp = Temp_newtemp (exp->u.BINOP.left->type);
      bop2Str (bopStr, exp->u.BINOP.op, exp->u.BINOP.left->type);
      if (exp->u.BINOP.left->kind == T_CONST
          && exp->u.BINOP.right->kind == T_CONST)
        {
          if (exp->u.BINOP.left->type == T_int)
            sprintf (asmStr1, "%%`d0 = %s i64 %d, %d", bopStr,
                     exp->u.BINOP.left->u.CONST.i,
                     exp->u.BINOP.right->u.CONST.i);
          else
            sprintf (asmStr1, "%%`d0 = %s double %f, %f", bopStr,
                     exp->u.BINOP.left->u.CONST.f,
                     exp->u.BINOP.right->u.CONST.f);
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL), NULL, NULL));
        }
      else if (exp->u.BINOP.left->kind == T_CONST
               && exp->u.BINOP.right->kind == T_TEMP)
        {
          if (exp->u.BINOP.left->type == T_int)
            sprintf (asmStr1, "%%`d0 = %s i64 %d, %%`s0", bopStr,
                     exp->u.BINOP.left->u.CONST.i);
          else
            sprintf (asmStr1, "%%`d0 = %s double %f, %%`s0", bopStr,
                     exp->u.BINOP.left->u.CONST.f);
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (exp->u.BINOP.right->u.TEMP, NULL),
                         NULL));
        }
      else if (exp->u.BINOP.left->kind == T_TEMP
               && exp->u.BINOP.right->kind == T_CONST)
        {
          if (exp->u.BINOP.left->type == T_int)
            sprintf (asmStr1, "%%`d0 = %s i64 %%`s0, %d", bopStr,
                     exp->u.BINOP.right->u.CONST.i);
          else
            sprintf (asmStr1, "%%`d0 = %s double %%`s0, %f", bopStr,
                     exp->u.BINOP.right->u.CONST.f);
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (exp->u.BINOP.left->u.TEMP, NULL),
                         NULL));
        }
      else
        {
          if (exp->u.BINOP.left->type == T_int)
            sprintf (asmStr1, "%%`d0 = %s i64 %%`s0, %%`s1", bopStr);
          else
            sprintf (asmStr1, "%%`d0 = %s double %%`s0, %%`s1", bopStr);
          emit (LL_Oper (
              asmStr1, Temp_TempList (rsltTmp, NULL),
              Temp_TempList (exp->u.BINOP.left->u.TEMP,
                             Temp_TempList (exp->u.BINOP.right->u.TEMP, NULL)),
              NULL));
        }
      return rsltTmp;
      break;
    case TL_expBinopOther:
      /* T_Binop (oper, <expL>, <expR>) */
      subtmp1 = munchExp (exp->u.BINOP.left);
      subtmp2 = munchExp (exp->u.BINOP.right);
      MYASRT (subtmp1->type == subtmp2->type);
      rsltTmp = Temp_newtemp (subtmp1->type);
      bop2Str (bopStr, exp->u.BINOP.op, subtmp1->type);
      Ttype2Str (tyStr, subtmp1->type);
      sprintf (asmStr1, "%%`d0 = %s %s %%`s0, %%`s1", bopStr, tyStr);
      emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                     Temp_TempList (subtmp1, Temp_TempList (subtmp2, NULL)),
                     NULL));
      return rsltTmp;
      break;
    case TL_expMemTempConst:
      /* T_Mem ([T_Temp (tempAddr)/T_IntConst (numAddr)], type) */
      MYASRT (exp->u.MEM->type == T_int);
      rsltTmp = Temp_newtemp (exp->type);
      /* the i64*-typed version of memory address */
      midtmp1 = Temp_newtemp (T_int);
      /* Type of the content in this memory location */
      Ttype2Str (tyStr, exp->type);
      if (exp->u.MEM->kind == T_TEMP)
        {
          /* NEED INSPECT */
          sprintf (asmStr1, "%%`d0 = inttoptr i64 %%`s0 to i64*");
          // DBGPRT ("%s, TL_expMemTempConst, exp.u.mem.kind==T_TEMP, temp
          // %d\n",
          //         __func__, exp->u.MEM->u.TEMP->num);
          emit (LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL),
                         Temp_TempList (exp->u.MEM->u.TEMP, NULL), NULL));
        }
      else if (exp->u.MEM->kind == T_CONST)
        {
          sprintf (asmStr1, "%%`d0 = inttoptr i64 %d to i64*",
                   exp->u.MEM->u.CONST.i);
          emit (LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL), NULL, NULL));
        }
      else
        MYASRT (0);
      sprintf (asmStr2, "%%`d0 = load %s, i64* %%`s0", tyStr);
      emit (LL_Oper (asmStr2, Temp_TempList (rsltTmp, NULL),
                     Temp_TempList (midtmp1, NULL), NULL));
      return rsltTmp;
      break;
    case TL_expMemOther:
      /* T_Mem (<expAdr>, type) */
      subtmp1 = munchExp (exp->u.MEM);
      MYASRT (subtmp1->type == T_int);
      rsltTmp = Temp_newtemp (exp->type);
      midtmp1 = Temp_newtemp (T_int);
      Ttype2Str (tyStr, exp->type);
      sprintf (asmStr1, "%%`d0 = inttoptr i64 %%`s0 to i64*");
      // DBGPRT ("%s, TL_expMemOther, temp %d\n", __func__, midtmp1->num);
      emit (LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL),
                     Temp_TempList (subtmp1, NULL), NULL));
      sprintf (asmStr2, "%%`d0 = load %s, i64* %%`s0", tyStr);
      emit (LL_Oper (asmStr2, Temp_TempList (rsltTmp, NULL),
                     Temp_TempList (midtmp1, NULL), NULL));
      return rsltTmp;
      break;
    case TL_expTemp:
      /* T_Temp (temp) */
      return exp->u.TEMP;
      break;
    case TL_expName:
      /* T_Name (label) */
      rsltTmp = Temp_newtemp (T_int);
      sprintf (asmStr1, "%%`d0 = ptrtoint i64* @%s to i64",
               Temp_labelstring (exp->u.NAME));
      emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL), NULL, NULL));
      return rsltTmp;
      break;
    case TL_expIntConst:
      rsltTmp = Temp_newtemp (T_int);
      sprintf (asmStr1, "%%`d0 = add i64 %d, 0", exp->u.CONST.i);
      emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL), NULL, NULL));
      return rsltTmp;
      break;
    case TL_expFloatConst:
      rsltTmp = Temp_newtemp (T_float);
      sprintf (asmStr1, "%%`d0 = fadd double %f, 0.0", exp->u.CONST.f);
      emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL), NULL, NULL));
      return rsltTmp;
      break;
    case TL_expCall:
      /* T_Call (strFuncName, <expFunc>, <expListArgs>, typeRet) */
      /* address of the called method */
      if (stmCall)
        {
          stmCall = 0;
          subtmp1 = munchExp (exp->u.CALL.obj);
          arglst = munchArgLst (0, exp->u.CALL.args);
          rsltTmp = NULL;
          midtmp1 = Temp_newtemp (T_int);
          sprintf (asmStr2, "%%`d0 = inttoptr i64 %%`s0 to i64*");
          emit (LL_Oper (asmStr2, Temp_TempList (midtmp1, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
          sprintf (asmStr1, "call void %%`s0(");
          catFormalArgLst (arglst, asmStr1, 1);
          strcat (asmStr1, ")");
          emit (
              LL_Oper (asmStr1, NULL, Temp_TempList (midtmp1, arglst), NULL));
          return rsltTmp;
        }
      else
        {
          subtmp1 = munchExp (exp->u.CALL.obj);
          // DBGPRT ("temp1\n");
          arglst = munchArgLst (0, exp->u.CALL.args);
          // DBGPRT ("temp2\n");
          midtmp1 = Temp_newtemp (T_int);
          rsltTmp = Temp_newtemp (exp->type);
          Ttype2Str (tyStr, exp->type); /* return type */
          sprintf (asmStr2, "%%`d0 = inttoptr i64 %%`s0 to i64*");
          emit (LL_Oper (asmStr2, Temp_TempList (midtmp1, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
          sprintf (asmStr1, "%%`d0 = call %s %%`s0(", tyStr);
          catFormalArgLst (arglst, asmStr1, 1);
          strcat (asmStr1, ")");
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (midtmp1, arglst), NULL));
          return rsltTmp;
        }
      break;
    case TL_expExtCall:
      /* T_ExtCall (strExtFuncName, <expListArgs>, type) */
      if (stmCall)
        {
          stmCall = 0;
          arglst = munchArgLst (0, exp->u.ExtCALL.args);
          rsltTmp = NULL;
          sprintf (asmStr1, "call void @%s(", exp->u.ExtCALL.extfun);
          catFormalArgLst (arglst, asmStr1, 0);
          strcat (asmStr1, ")");
          emit (LL_Oper (asmStr1, NULL, arglst, NULL));
          return rsltTmp;
        }
      if (strcmp (exp->u.ExtCALL.extfun, "malloc") == 0)
        {
          rsltTmp = Temp_newtemp (T_int);
          midtmp1 = Temp_newtemp (T_int);
          arglst = munchArgLst (0, exp->u.ExtCALL.args);
          sprintf (asmStr1, "%%`d0 = call i64* @malloc(i64 %%`s0)");
          emit (
              LL_Oper (asmStr1, Temp_TempList (midtmp1, NULL), arglst, NULL));
          sprintf (asmStr2, "%%`d0 = ptrtoint i64* %%`s0 to i64");
          emit (LL_Oper (asmStr2, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (midtmp1, NULL), NULL));
        }
      else
        {
          arglst = munchArgLst (0, exp->u.ExtCALL.args);
          rsltTmp = Temp_newtemp (exp->type);
          Ttype2Str (tyStr, exp->type);
          sprintf (asmStr1, "%%`d0 = call %s @%s(", tyStr,
                   exp->u.ExtCALL.extfun);
          catFormalArgLst (arglst, asmStr1, 0);
          strcat (asmStr1, ")");
          emit (
              LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL), arglst, NULL));
        }
      return rsltTmp;
      break;
    case TL_expCast:
      /* T_Cast (<expOrigin>, type) */
      subtmp1 = munchExp (exp->u.CAST);
      if (exp->u.CAST->type == exp->type)
        rsltTmp = subtmp1;
      else if (exp->u.CAST->type == T_int && exp->type == T_float) /* i->f */
        {
          rsltTmp = Temp_newtemp (exp->type);
          sprintf (asmStr1, "%%`d0 = sitofp i64 %%`s0 to double");
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
        }
      else /* f->i */
        {
          rsltTmp = Temp_newtemp (exp->type);
          sprintf (asmStr1, "%%`d0 = fptosi double %%`s0 to i64");
          emit (LL_Oper (asmStr1, Temp_TempList (rsltTmp, NULL),
                         Temp_TempList (subtmp1, NULL), NULL));
        }
      return rsltTmp;
      break;
    }
}

static Temp_tempList
munchArgLst (int argord, T_expList arglst)
{
  if (arglst == NULL)
    return NULL;
  else
    return Temp_TempList (munchExp (arglst->head),
                          munchArgLst (argord + 1, arglst->tail));
}

static void
cnd2Str (char *destStr, T_relOp cnd, T_type ty)
{
  switch (cnd)
    {
    case T_eq:
      strcpy (destStr, ty == T_int ? "eq" : "oeq");
      break;
    case T_ne:
      strcpy (destStr, ty == T_int ? "ne" : "one");
      break;
    case T_lt:
      strcpy (destStr, ty == T_int ? "slt" : "olt");
      break;
    case T_gt:
      strcpy (destStr, ty == T_int ? "sgt" : "ogt");
      break;
    case T_le:
      strcpy (destStr, ty == T_int ? "sle" : "ole");
      break;
    case T_ge:
      strcpy (destStr, ty == T_int ? "sge" : "oge");
      break;
    case T_ult:
    case T_ule:
    case T_ugt:
    case T_uge:
      MYASRT (0);
      break;
    }
}

static void
bop2Str (char *destStr, T_binOp bop, T_type ty)
{
  switch (bop)
    {
    case T_plus:
      strcpy (destStr, ty == T_int ? "add" : "fadd");
      break;
    case T_minus:
      strcpy (destStr, ty == T_int ? "sub" : "fsub");
      break;
    case T_mul:
      strcpy (destStr, ty == T_int ? "mul" : "fmul");
      break;
    case T_div:
      strcpy (destStr, ty == T_int ? "sdiv" : "fdiv");
      break;
    case T_and:
    case T_or:
    case T_lshift:
    case T_rshift:
    case T_arshift:
    case T_xor:
      MYASRT (0);
      break;
    }
}

static void
Ttype2Str (char *destStr, T_type ty)
{
  switch (ty)
    {
    case T_int:
      strcpy (destStr, "i64");
      break;
    case T_float:
      strcpy (destStr, "double");
      break;
    }
}

static int
tempListLen (Temp_tempList tmplst)
{
  if (tmplst == NULL)
    return 0;
  else
    return 1 + tempListLen (tmplst->tail);
}

static void
catFormalArgLst (Temp_tempList pars, char *destStr, int i)
{
  if (pars == NULL)
    return;
  char par[20] = { '\0' };
  char tystr[10] = { '\0' };
  Ttype2Str (tystr, pars->head->type);
  if (pars->tail == NULL)
    sprintf (par, "%s %%`s%d", tystr, i);
  else
    sprintf (par, "%s %%`s%d, ", tystr, i);
  strcat (destStr, par);
  catFormalArgLst (pars->tail, destStr, i + 1);
}

/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE ABOVE FUNCTION FOR HW9_10 */
/* ********************************************************/

/* The following are some auxiliary functions to be used by the main */
// This function is to make the beginning of the function that jumps to the
// beginning label (lbeg) of a block (in case the lbeg is NOT at the
// beginning of the block).
LL_instrList
llvmbodybeg (Temp_label lbeg)
{
  if (!lbeg)
    return NULL;
  iList = last = NULL;
  Temp_label lstart = Temp_newlabel ();
  string ir = (string)checked_malloc (IR_MAXLEN);
  sprintf (ir, "%s:", Temp_labelstring (lstart));
  emit (LL_Label (ir, lstart));
  ir = (string)checked_malloc (IR_MAXLEN);
  sprintf (ir, "br label %%`j0");
  emit (LL_Oper (ir, NULL, NULL, LL_Targets (LL (lbeg, NULL))));
  return iList;
}

// This function is to make the prolog of the function that takes the method
// name and the arguments WE ARE MISSING THE RETURN TYPE in tigherirp.h. YOU
// NEED TO ADD IT!
LL_instrList
llvmprolog (string methodname, Temp_tempList args, T_type rettype)
{
#ifdef LLVMGEN_DEBUG
  fprintf (stderr, "llvmprolog: methodname=%s, rettype=%d\n", methodname,
           rettype);
#endif
  if (!methodname)
    return NULL;
  iList = last = NULL;
  string ir = (string)checked_malloc (IR_MAXLEN);
  if (rettype == T_int)
    sprintf (ir, "define i64 @%s(", methodname);
  else if (rettype == T_float)
    sprintf (ir, "define double @%s(", methodname);
#ifdef LLVMGEN_DEBUG
  fprintf (stderr, "llvmprolog: ir=%s\n", ir);
#endif
  if (args)
    {
      int i = 0;
      for (Temp_tempList arg = args; arg; arg = arg->tail, i += 1)
        {
          if (i != 0)
            sprintf (ir + strlen (ir), ", ");
          if (arg->head->type == T_int)
            {
              sprintf (ir + strlen (ir), "i64 %%`s%d", i);
#ifdef LLVMGEN_DEBUG
              fprintf (stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif
            }
          else if (arg->head->type == T_float)
            {
#ifdef LLVMGEN_DEBUG
              fprintf (stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif
              sprintf (ir + strlen (ir), "double %%`s%d", i);
            }
#ifdef LLVMGEN_DEBUG
          fprintf (stderr, "llvmprolog args: arg=%s\n", ir);
#endif
        }
    }
  sprintf (ir + strlen (ir), ") {");
#ifdef LLVMGEN_DEBUG
  fprintf (stderr, "llvmprolog final: ir=%s\n", ir);
#endif
  emit (LL_Oper (ir, NULL, args, NULL));
  return iList;
}

// This function is to make the epilog of the function that jumps to the end
// label (lend) of a block
LL_instrList
llvmepilog (Temp_label lend)
{
  if (!lend)
    return NULL;
  iList = last = NULL;
  // string ir = (string)checked_malloc (IR_MAXLEN);
  // sprintf (ir, "%s:", Temp_labelstring (lend));
  // emit (LL_Label (ir, lend));
  // if (retty == T_int)
  //   emit (LL_Oper ("ret i64 -1", NULL, NULL, NULL));
  // else
  //   emit (LL_Oper ("ret double -1.0", NULL, NULL, NULL));
  emit (LL_Oper ("}", NULL, NULL, NULL));
  return iList;
}
