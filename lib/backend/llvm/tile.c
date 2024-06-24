#include "tile.h"
#include "dbg.h"
#include "temp.h"
#include "util.h"
#include <string.h>

TL_tileKind TL_findLargestTile (int *matchArr, int *tileSize);

int TL_tileSize[] = {
  [TL_stmLabel] = 2,
  [TL_stmUncondBranch] = 2,
  [TL_stmCondBranchTempConst] = 8,
  [TL_stmCondBranchOther] = 4,
  [TL_stmMoveMallocTemp] = 6,
  // [TL_stmMoveMallocOther] = 4,
  [TL_stmMoveLoad] = 5,
  [TL_stmMoveStore] = 5,
  // [TL_stmMoveBothMem] = 5,
  [TL_stmMoveTempConst] = 5,
  // [TL_stmMoveOther] = 1,
  [TL_stmMoveToTemp] = 3,
  [TL_stmTExp] = 1,
  [TL_stmRet] = 1,
  [TL_expBinopTempConst] = 6,
  [TL_expBinopOther] = 2,
  [TL_expMemTempConst] = 4,
  [TL_expMemOther] = 2,
  [TL_expTemp] = 2,
  [TL_expIntConst] = 2,
  [TL_expFloatConst] = 2,
  [TL_expCall] = 3,
  [TL_expExtCall] = 3,
  [TL_expCast] = 2,
};

TL_tileKind
TL_matchTile (TL_stmExp stmexp)
{
  int matchPat[TL_tileKindLENGTH] = { 0 };
  T_stm stm = NULL;
  T_exp exp = NULL;
  switch (stmexp->kind)
    {
    case TL_stm:
      {
        stm = stmexp->u.stm;
        if (stm->kind == T_LABEL)
          matchPat[TL_stmLabel] = 1;
        if (stm->kind == T_JUMP)
          matchPat[TL_stmUncondBranch] = 1;
        if (stm->kind == T_CJUMP)
          {
            matchPat[TL_stmCondBranchOther] = 1;
            if ((stm->u.CJUMP.left->kind == T_TEMP
                 || stm->u.CJUMP.left->kind == T_CONST)
                && (stm->u.CJUMP.right->kind == T_TEMP
                    || stm->u.CJUMP.right->kind == T_CONST))
              matchPat[TL_stmCondBranchTempConst] = 1;
          }
        if (stm->kind == T_MOVE)
          {
            // matchPat[TL_stmMoveOther] = 1;
            if (stm->u.MOVE.src->kind == T_ExtCALL
                && strcmp (stm->u.MOVE.src->u.ExtCALL.extfun, "malloc") == 0)
              {
                // matchPat[TL_stmMoveMallocOther] = 1;
                if (stm->u.MOVE.dst->kind == T_TEMP)
                  matchPat[TL_stmMoveMallocTemp] = 1;
              }
            if (stm->u.MOVE.dst->kind == T_TEMP
                && stm->u.MOVE.src->kind == T_MEM)
              matchPat[TL_stmMoveLoad] = 1;
            if (stm->u.MOVE.dst->kind == T_MEM)
              matchPat[TL_stmMoveStore] = 1;
            // if (stm->u.MOVE.dst->kind == T_MEM
            //     && stm->u.MOVE.src->kind == T_MEM)
            //   matchPat[TL_stmMoveBothMem] = 1;
            if (stm->u.MOVE.dst->kind == T_TEMP
                && (stm->u.MOVE.src->kind == T_TEMP
                    || stm->u.MOVE.src->kind == T_CONST))
              matchPat[TL_stmMoveTempConst] = 1;
            if (stm->u.MOVE.dst->kind == T_TEMP)
              matchPat[TL_stmMoveToTemp] = 1;
          }
        if (stm->kind == T_EXP)
          matchPat[TL_stmTExp] = 1;
        if (stm->kind == T_RETURN)
          matchPat[TL_stmRet] = 1;
        break;
      }
    case TL_exp:
      {
        exp = stmexp->u.exp;
        if (exp->kind == T_BINOP)
          {
            matchPat[TL_expBinopOther] = 1;
            if ((exp->u.BINOP.left->kind == T_TEMP
                 || exp->u.BINOP.left->kind == T_CONST)
                && (exp->u.BINOP.right->kind == T_TEMP
                    || exp->u.BINOP.right->kind == T_CONST))
              matchPat[TL_expBinopTempConst] = 1;
          }
        if (exp->kind == T_MEM)
          {
            matchPat[TL_expMemOther] = 1;
            if (exp->u.MEM->kind == T_TEMP || exp->u.MEM->kind == T_CONST)
              matchPat[TL_expMemTempConst] = 1;
          }
        if (exp->kind == T_TEMP)
          matchPat[TL_expTemp] = 1;
        if (exp->kind == T_NAME)
          matchPat[TL_expName] = 1;
        if (exp->kind == T_CONST && exp->type == T_int)
          matchPat[TL_expIntConst] = 1;
        if (exp->kind == T_CONST && exp->type == T_float)
          matchPat[TL_expFloatConst] = 1;
        if (exp->kind == T_CALL)
          matchPat[TL_expCall] = 1;
        if (exp->kind == T_ExtCALL)
          matchPat[TL_expExtCall] = 1;
        if (exp->kind == T_CAST)
          matchPat[TL_expCast] = 1;
      }
      break;
    }
  return TL_findLargestTile (matchPat, TL_tileSize);
}

TL_tileKind
TL_findLargestTile (int *matchArr, int *tileSize)
{
  TL_tileKind ret = (TL_tileKind)0;
  int metFirst = 0;
  for (int i = 0; i < TL_tileKindLENGTH; ++i)
    {
      if (!metFirst && matchArr[i])
        {
          metFirst = 1;
          ret = i;
        }
      else if (metFirst && matchArr[i] && tileSize[i] > tileSize[ret])
        ret = i;
      else
        ;
    }
  MYASRT (metFirst);
  return ret;
}

TL_stmExp
TL_StmConstr (T_stm stm)
{
  TL_stmExp ret = checked_malloc (sizeof (struct TL_stmExp_));
  ret->kind = TL_stm;
  ret->u.stm = stm;
  return ret;
}

TL_stmExp
TL_ExpConstr (T_exp exp)
{
  TL_stmExp ret = checked_malloc (sizeof (struct TL_stmExp_));
  ret->kind = TL_exp;
  ret->u.exp = exp;
  return ret;
}
