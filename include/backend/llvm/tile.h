#pragma once

#include "tigerirp.h"

/* Comment: IR tree pattern. <xxx>: not processed in this call but in next
 *            recursive call
 * Item:    tile kind */
typedef enum TL_tileKind_
{
  /*********
   * T_stm *
   *********/
  /* T_Label (label) */
  TL_stmLabel,
  /* T_Jump (labelJDest) */
  TL_stmUncondBranch,
  /* T_Cjump (cnd, [T_Temp (labelL)/T_[Int/Float]Const (numL)],
   *          [T_Temp (labelR)/T_[Int/Float]Const (numR)],
   *          labelTrueJDest, labelFalseJDest) */
  TL_stmCondBranchTempConst,
  /* T_Cjump (cnd, <expLeft>, <expRight>, labelTrueJDest, labelFalseJDest) */
  TL_stmCondBranchOther,
  /* T_Move (T_Temp (temp), T_ExtCall ("malloc", <expSize>, type)) */
  TL_stmMoveMallocTemp,
  /* T_Move (expDest, T_ExtCall ("malloc", <expSize>, type))
   *   where expDest is a T_exp other than a T_Temp (temp)*/
  // TL_stmMoveMallocOther,
  /* T_Move (T_Temp (tempDest), T_Mem (<expAddr>, type)) */
  TL_stmMoveLoad,
  /* T_Move (T_Mem (<expAddr>, type), <expSrc>) */
  TL_stmMoveStore,
  /* T_Move (T_Mem (<expAddrDest>, type), T_Mem (<expAddrSrc>, type)) */
  // TL_stmMoveBothMem,
  /* T_Move (T_Temp (labelDest),
   *         [T_Temp (labelSrc)/T_[Int/Float]Const (numSrc)]) */
  TL_stmMoveTempConst,
  /* T_Move (T_Temp (tempDest), <expSrc>) */
  TL_stmMoveToTemp,
  /* T_Exp (<exp>) */
  TL_stmTExp,
  /* T_Return (<exp>) */
  TL_stmRet,
  /*********
   * T_exp *
   *********/
  /* T_Binop (oper, [T_Temp (tempL)/T_[Int/Float]Const (numL)],
   *          [T_Temp (tempR)/T_[Int/Float]Const (numR)]) */
  TL_expBinopTempConst,
  /* T_Binop (oper, <expL>, <expR>) */
  TL_expBinopOther,
  /* T_Mem ([T_Temp (tempAddr)/T_IntConst (numAddr)], type) */
  TL_expMemTempConst,
  /* T_Mem (<expAdr>, type) */
  TL_expMemOther,
  /* T_Temp (temp) */
  TL_expTemp,
  /* T_Name (label) */
  TL_expName,
  /* T_IntConst (i) */
  TL_expIntConst,
  /* T_FloatConst (f) */
  TL_expFloatConst,
  /* T_Call (strFuncName, <expFunc>, <expListArgs>, typeRet) */
  TL_expCall,
  /* T_ExtCall (strExtFuncName, <expListArgs>, type) */
  TL_expExtCall,
  /* T_Cast (<expOrigin>, type) */
  TL_expCast,
  /* number of substantial items in this enum */
  TL_tileKindLENGTH,
} TL_tileKind;

typedef enum TL_stmExpKind_
{
  TL_stm,
  TL_exp
} TL_stmExpKind;

typedef struct TL_stmExp_
{
  TL_stmExpKind kind;
  union
  {
    T_stm stm;
    T_exp exp;
  } u;
} *TL_stmExp;

TL_tileKind TL_matchTile (TL_stmExp stmexp);
TL_stmExp TL_StmConstr (T_stm stm);
TL_stmExp TL_ExpConstr (T_exp exp);
