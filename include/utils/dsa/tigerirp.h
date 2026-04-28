#pragma once

#include "symbol.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

/*
 * tigerirp.h - Definitions for intermediate representation (IR) plus trees.
 *
 * This tree.h is an extension of tiger IR with the following
 * additional concepts and constructs:
 *
 * 1. Function declarations are explicitly list given.
 * 2. Two built-in functions: alloca and phi (as in LLVM IR).
 * 3. For FDMJ 2024, added data types (int, float, etc.) into the expression.
 */

/* A piece of code (in a file) is a list of function declarations,
 * which includes a (unique) main function declaration
 */

/* The following is to define statements and expressions */
typedef struct T_stm_ *T_stm;
typedef struct T_exp_ *T_exp;

typedef struct T_expList_ *T_expList;
struct T_expList_
{
  T_exp head;
  T_expList tail;
};

typedef struct T_stmList_ *T_stmList;
struct T_stmList_
{
  T_stm head;
  T_stmList tail;
};

/* A function is a list of temps as arguments, and a list of statements */
typedef struct T_funcDecl_ *T_funcDecl;
typedef struct T_funcDeclList_ *T_funcDeclList;
struct T_funcDecl_
{
  string name;
  Temp_tempList args;
  T_stm stm;
  T_type ret_type;
};
struct T_funcDeclList_
{
  T_funcDecl head;
  T_funcDeclList tail;
};

typedef enum
{
  T_plus,
  T_minus,
  T_mul,
  T_div,
  T_and,
  T_or,
  T_lshift,
  T_rshift,
  T_arshift,
  T_xor
} T_binOp;

typedef enum
{
  T_eq,
  T_ne,
  T_lt,
  T_gt,
  T_le,
  T_ge,
  T_ult,
  T_ule,
  T_ugt,
  T_uge
} T_relOp;

struct T_stm_
{
  enum
  {
    T_SEQ,
    T_LABEL,
    T_JUMP,
    T_CJUMP,
    T_MOVE,
    T_EXP,
    T_RETURN
  } kind;
  union
  {
    struct
    {
      T_stm left, right;
    } SEQ;
    Temp_label LABEL;
    struct
    {
      Temp_label jump;
    } JUMP;
    struct
    {
      T_relOp op;
      T_exp left, right;
      Temp_label t, f;
    } CJUMP;
    struct
    {
      T_exp dst, src;
    } MOVE;
    T_exp EXP;
  } u;
};

struct T_exp_
{
  enum
  {
    T_BINOP,
    T_MEM,
    T_TEMP,
    T_ESEQ,
    T_NAME,
    T_CONST,
    T_CALL,
    T_ExtCALL,
    T_CAST
  } kind;
  T_type type;
  union
  {
    struct
    {
      T_binOp op;
      T_exp left, right;
    } BINOP;
    T_exp MEM;
    Temp_temp TEMP;
    struct
    {
      T_stm stm;
      T_exp exp;
    } ESEQ;
    Temp_label NAME;
    union
    {
      int i;
      float f;
    } CONST;
    struct
    {
      string id;
      T_exp obj;
      T_expList args;
    } CALL;
    struct
    {
      string extfun;
      T_expList args;
    } ExtCALL;
    T_exp CAST;
  } u;
};

T_expList T_ExpList (T_exp head, T_expList tail);
T_stmList T_StmList (T_stm head, T_stmList tail);

T_funcDeclList T_FuncDeclList (T_funcDecl, T_funcDeclList);
T_funcDecl T_FuncDecl (string, Temp_tempList, T_stm, T_type);

T_stm T_Seq (T_stm left, T_stm right);
T_stm T_Label (Temp_label);
T_stm T_Jump (Temp_label);
T_stm T_Cjump (T_relOp op, T_exp left, T_exp right, Temp_label t,
               Temp_label f);
T_stm T_Move (T_exp, T_exp);
T_stm T_Exp (T_exp);
T_stm T_Return (T_exp);

T_exp T_Binop (T_binOp, T_exp, T_exp); // ret: T_int or T_float (inferred)
T_exp T_Mem (T_exp, T_type); // ret: T_int (ptr auto cast) or T_float, arg:
                             // type stored in memroy (T_int or T_float)
T_exp T_Temp (Temp_temp);    // ret: T_int or T_float
T_exp T_Eseq (T_stm, T_exp); // ret: type of T_exp (inferred)
T_exp T_Name (Temp_label);   // ret: T_int (ptr auto cast)
T_exp T_IntConst (int);      // ret: T_int
T_exp T_FloatConst (float);  // ret: T_float
T_exp T_Call (string, T_exp, T_expList,
              T_type); // ret: T_int or T_float (if void, use T_int), arg:
                       // T_int (ptr auto cast) or T_float
T_exp T_ExtCall (string, T_expList,
                 T_type); // ret: T_int or T_float (if void, use T_int), arg:
                          // T_int (ptr auto cast) or T_float
T_exp T_Cast (T_exp, T_type); // ret: T_int or T_float, arg: type to be cast

T_relOp T_notRel (T_relOp);  /* a op b  ==   not(a notRel(op) b)  */
T_relOp T_commute (T_relOp); /* a op b  ==  b commute(op) a     */
