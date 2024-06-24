#pragma once

#include "fdmjast.h"
#include <stdio.h>

void printX_Pos (FILE *, A_pos);
void printX_Type (FILE *, A_type);

void printX_Prog (FILE *, A_prog);
void printX_MainMethod (FILE *, A_mainMethod);
void printX_VarDecl (FILE *, A_varDecl);
void printX_VarDeclList (FILE *, A_varDeclList);
void printX_ClassDecl (FILE *, A_classDecl);
void printX_ClassDeclList (FILE *, A_classDeclList);
void printX_MethodDecl (FILE *, A_methodDecl);
void printX_MethodDeclList (FILE *, A_methodDeclList);
void printX_Formal (FILE *, A_formal);
void printX_FormalList (FILE *, A_formalList);

void printX_StmList (FILE *, A_stmList);
void printX_Stm (FILE *, A_stm);

void printX_NestedStm (FILE *, A_stm);
void printX_IfStm (FILE *, A_stm);
void printX_WhileStm (FILE *, A_stm);
void printX_AssignStm (FILE *, A_stm);
void printX_ArrayInit (FILE *, A_stm);
void printX_CallStm (FILE *, A_stm);
void printX_Continue (FILE *, A_stm);
void printX_Break (FILE *, A_stm);
void printX_Return (FILE *, A_stm);
void printX_Putnum (FILE *, A_stm);
void printX_Putch (FILE *, A_stm);
void printX_Putarray (FILE *, A_stm);
void printX_Starttime (FILE *, A_stm);
void printX_Stoptime (FILE *, A_stm);

void printX_Exp (FILE *, A_exp);
void printX_ExpList (FILE *, A_expList);

void printX_OpExp (FILE *, A_exp);
void printX_ArrayExp (FILE *, A_exp);
void printX_CallExp (FILE *, A_exp);
void printX_ClassVarExp (FILE *, A_exp);
void printX_BoolConst (FILE *, A_exp);
void printX_NumConst (FILE *, A_exp);
void printX_LengthExp (FILE *, A_exp);
void printX_IdExp (FILE *, A_exp);
void printX_ThisExp (FILE *, A_exp);
void printX_NewIntArrExp (FILE *, A_exp);
void printX_NewFloatArrExp (FILE *, A_exp);
void printX_NewObjExp (FILE *, A_exp);
void printX_NotExp (FILE *, A_exp);
void printX_MinusExp (FILE *, A_exp);
void printX_EscExp (FILE *, A_exp);
void printX_Getnum (FILE *, A_exp);
void printX_Getch (FILE *, A_exp);
void printX_Getarray (FILE *, A_exp);
