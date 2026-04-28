#pragma once

#include "util.h"

typedef struct A_pos_ *A_pos; // position information
typedef struct A_type_ *A_type;
typedef struct A_prog_ *A_prog;
typedef struct A_mainMethod_ *A_mainMethod;
typedef struct A_classDecl_ *A_classDecl;
typedef struct A_classDeclList_ *A_classDeclList;
typedef struct A_methodDecl_ *A_methodDecl;
typedef struct A_methodDeclList_ *A_methodDeclList;
typedef struct A_formal_ *A_formal;
typedef struct A_formalList_ *A_formalList;
typedef struct A_varDecl_ *A_varDecl;
typedef struct A_varDeclList_ *A_varDeclList;
typedef struct A_stmList_ *A_stmList;
typedef struct A_stm_ *A_stm;
typedef struct A_exp_ *A_exp;
typedef struct A_expList_ *A_expList;

struct A_pos_
{
  int line, pos;
};

typedef enum
{
  A_intType,
  A_floatType,
  A_idType,
  A_intArrType,
  A_floatArrType
} A_dataType;

struct A_type_
{
  A_pos pos;
  A_dataType t;
  string id; // for class type, otherwise ignore
};

struct A_prog_
{
  A_pos pos;
  A_mainMethod m;
  A_classDeclList cdl;
};

struct A_mainMethod_
{
  A_pos pos;
  A_varDeclList vdl;
  A_stmList sl;
};

struct A_methodDecl_
{
  A_pos pos;
  A_type t;  // return type
  string id; // method name
  A_formalList fl;
  A_varDeclList vdl;
  A_stmList sl;
};

struct A_methodDeclList_
{
  A_methodDecl head;
  A_methodDeclList tail;
};

struct A_classDecl_
{
  A_pos pos;
  string id;       // class name
  string parentID; // parent class id if not NULL
  A_varDeclList vdl;
  A_methodDeclList mdl;
};

struct A_classDeclList_
{
  A_classDecl head;
  A_classDeclList tail;
};

struct A_formal_
{
  A_pos pos;
  A_type t;
  string id;
};

struct A_formalList_
{
  A_formal head;
  A_formalList tail;
};

struct A_varDecl_
{
  A_pos pos;
  A_type t;
  string v;
  A_expList elist; // this must be a list of num's for array type
};

struct A_varDeclList_
{
  A_varDecl head;
  A_varDeclList tail;
};

struct A_stmList_
{
  A_stm head;
  A_stmList tail;
};

typedef enum
{
  A_nestedStm,
  A_ifStm,
  A_whileStm,
  A_assignStm,
  A_arrayInit,
  A_callStm,
  A_continue,
  A_break,
  A_return,
  // A_putint,
  A_putnum,
  A_putarray,
  A_putch,
  A_starttime,
  A_stoptime
} A_stmKind;

struct A_stm_
{
  A_pos pos;
  A_stmKind kind;
  union
  {
    A_stmList ns;
    struct
    {
      A_exp e;
      A_stm s1, s2;
    } if_stat;
    struct
    {
      A_exp e;
      A_stm s;
    } while_stat;
    struct
    {
      A_exp arr; // left must be a location for an array type
      // A_exp pos; //array position /* March 20, 2023
      // This is redundant due to erasure of redundant
      // rule in the grammar */
      A_exp value;
    } assign;
    struct
    {
      A_exp arr; ////left must be a location for an array type
      A_expList init_values;
    } array_init;
    struct
    {
      A_exp obj;    // object
      string fun;   // function name
      A_expList el; // parameters
    } call_stat;
    struct
    {
      A_exp e1, e2;
    } putarray;
    A_exp e;
  } u;
};

typedef enum
{
  A_and,
  A_or,
  A_less,
  A_le,
  A_greater,
  A_ge,
  A_eq,
  A_ne,
  A_plus,
  A_minus,
  A_times,
  A_div
} A_binop;

typedef enum
{
  A_opExp,
  A_arrayExp,
  A_callExp,
  A_classVarExp,
  A_boolConst,
  A_numConst,
  A_idExp,
  A_thisExp,
  A_lengthExp,
  A_newIntArrExp,
  A_newFloatArrExp,
  A_newObjExp,
  A_notExp,
  A_minusExp,
  A_escExp,
  A_getnum,
  A_getch,
  A_getarray
} A_expKind;

struct A_exp_
{
  A_pos pos;
  A_expKind kind;
  union
  {
    struct
    {
      A_exp left;
      A_binop oper;
      A_exp right;
    } op;
    struct
    {
      A_exp arr; // this must evaluate to an array
      A_exp arr_pos;
    } array_pos;
    struct
    {
      A_exp obj;  // this must evaluate to object
      string fun; // this is the name of the function if present
      A_expList el;
    } call;
    struct
    {
      A_exp obj;  // this must evaluate to object
      string var; // this is the name of the variable
    } classvar;
    struct
    {
      A_stmList ns;
      A_exp exp;
    } escExp;
    float num;
    bool b;
    string v;
    A_exp e;
  } u;
};

struct A_expList_
{
  A_exp head;
  A_expList tail;
};

A_pos A_Pos (int, int);
A_type A_Type (A_pos, A_dataType, string);
A_prog A_Prog (A_pos, A_mainMethod, A_classDeclList);
A_mainMethod A_MainMethod (A_pos, A_varDeclList, A_stmList);
A_classDecl A_ClassDecl (A_pos, string, string, A_varDeclList,
                         A_methodDeclList);
A_classDeclList A_ClassDeclList (A_classDecl, A_classDeclList);
A_varDecl A_VarDecl (A_pos, A_type, string v, A_expList);
A_varDeclList A_VarDeclList (A_varDecl, A_varDeclList);
A_methodDecl A_MethodDecl (A_pos, A_type, string, A_formalList, A_varDeclList,
                           A_stmList);
A_methodDeclList A_MethodDeclList (A_methodDecl, A_methodDeclList);
A_formal A_Formal (A_pos, A_type, string);
A_formalList A_FormalList (A_formal, A_formalList);

A_stmList A_StmList (A_stm, A_stmList);
A_stm A_NestedStm (A_pos, A_stmList);
A_stm A_IfStm (
    A_pos, A_exp, A_stm,
    A_stm); // if the second A_stm is not empty, then there is an else part
A_stm A_WhileStm (A_pos, A_exp, A_stm);
A_stm A_AssignStm (
    A_pos, A_exp,
    A_exp); // Updated March 20, 2023. Removed second exp: OLD: if second A_exp
            // is not empty, then it's array position assignment
A_stm A_ArrayInit (A_pos, A_exp, A_expList);
A_stm A_CallStm (A_pos, A_exp, string, A_expList);
A_stm A_Continue (A_pos);
A_stm A_Break (A_pos);
A_stm A_Return (A_pos, A_exp);
A_stm A_Putnum (A_pos, A_exp);
A_stm A_Putch (A_pos, A_exp);
A_stm A_Putarray (A_pos, A_exp, A_exp);
A_stm A_Starttime (A_pos);
A_stm A_Stoptime (A_pos);

A_expList A_ExpList (A_exp, A_expList);
A_exp A_OpExp (A_pos, A_exp, A_binop, A_exp);
A_exp A_ArrayExp (A_pos, A_exp, A_exp);
A_exp A_CallExp (A_pos, A_exp, string, A_expList);
A_exp A_ClassVarExp (A_pos, A_exp,
                     string); // Updated March 20, 2023. Remove pos
A_exp A_NumConst (A_pos, float);
A_exp A_BoolConst (A_pos, bool);
A_exp A_LengthExp (A_pos, A_exp);
A_exp A_IdExp (A_pos, string);
A_exp A_ThisExp (A_pos);
A_exp A_NewIntArrExp (A_pos, A_exp);
A_exp A_NewFloatArrExp (A_pos, A_exp);
A_exp A_NewObjExp (A_pos, string);
A_exp A_NotExp (A_pos, A_exp);
A_exp A_MinusExp (A_pos, A_exp);
A_exp A_EscExp (A_pos, A_stmList, A_exp);
A_exp A_Getnum (A_pos);
A_exp A_Getch (A_pos);
A_exp A_Getarray (A_pos, A_exp);
