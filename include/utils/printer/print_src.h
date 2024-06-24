#pragma once

#include "fdmjast.h"
#include <stdio.h>

void printA_Pos (FILE *, A_pos);
void printA_Type (FILE *, A_type);

void printA_Prog (FILE *, A_prog);
void printA_MainMethod (FILE *, A_mainMethod);
void printA_VarDecl (FILE *, A_varDecl);
void printA_VarDeclList (FILE *, A_varDeclList);
void printA_ClassDecl (FILE *, A_classDecl);
void printA_ClassDeclList (FILE *, A_classDeclList);
void printA_MethodDecl (FILE *, A_methodDecl);
void printA_MethodDeclList (FILE *, A_methodDeclList);
void printA_Formal (FILE *, A_formal);
void printA_FormalList (FILE *, A_formalList);

void printA_StmList (FILE *, A_stmList);
void printA_Stm (FILE *, A_stm);

void printA_NestedStm (FILE *, A_stm);
void printA_IfStm (FILE *, A_stm);
void printA_WhileStm (FILE *, A_stm);
void printA_AssignStm (FILE *, A_stm);
void printA_ArrayInit (FILE *, A_stm);
void printA_CallStm (FILE *, A_stm);
void printA_Continue (FILE *, A_stm);
void printA_Break (FILE *, A_stm);
void printA_Return (FILE *, A_stm);
void printA_Putnum (FILE *, A_stm);
void printA_Putch (FILE *, A_stm);
void printA_Putarray (FILE *, A_stm);
void printA_Starttime (FILE *, A_stm);
void printA_Stoptime (FILE *, A_stm);

void printA_Exp (FILE *, A_exp);
void printA_ExpList (FILE *, A_expList);

void printA_OpExp (FILE *, A_exp);
void printA_ArrayExp (FILE *, A_exp);
void printA_CallExp (FILE *, A_exp);
void printA_ClassVarExp (FILE *, A_exp);
void printA_BoolConst (FILE *, A_exp);
void printA_NumConst (FILE *, A_exp);
void printA_LengthExp (FILE *, A_exp);
void printA_IdExp (FILE *, A_exp);
void printA_ThisExp (FILE *, A_exp);
void printA_NewIntArrExp (FILE *, A_exp);
void printA_NewFloatArrExp (FILE *, A_exp);
void printA_NewObjExp (FILE *, A_exp);
void printA_NotExp (FILE *, A_exp);
void printA_MinusExp (FILE *, A_exp);
void printA_EscExp (FILE *, A_exp);
void printA_Getnum (FILE *, A_exp);
void printA_Getch (FILE *, A_exp);
void printA_Getarray (FILE *, A_exp);
