#pragma once

#include "env.h"
#include "fdmjast.h"
#include "semant.h"
#include "symbol.h"
// #include "types.h"
#include <stdio.h>

/* Traverse class decl trees and record all class and members decl */
void I_recClassDeclList (FILE *out, A_classDeclList cdl, S_table clstbl);
void I_recClassDecl (FILE *out, A_classDecl cd, S_table clstbl);
void I_recVarDeclList (FILE *out, A_varDeclList vdl, S_table vtbl);
void I_recVarDecl (FILE *out, A_varDecl vd, S_table vtbl);
void I_recMethodDeclList (FILE *out, A_methodDeclList mdl, string fromcls,
                          S_table mtbl);
void I_recMethodDecl (FILE *out, A_methodDecl md, string fromcls,
                      S_table mtbl);
Ty_fieldList I_recFormalList (FILE *out, A_formalList fl, S_table partbl);
Ty_field I_recFormal (FILE *out, A_formal f, S_table partbl);

/* Check inheritance and overriding */
void I_chkClassDeclList (FILE *out, A_classDeclList cdl, S_table clstbl);
void I_chkClassDecl (FILE *out, A_classDecl cd, S_table clstbl);

/* Record the class member vars and methods into Unified Object Record */
void I_uorClassDeclList (FILE *out, A_classDeclList cdl, S_table voff,
                         S_table moff);
void I_uorClassDecl (FILE *out, A_classDecl cd, S_table voff, S_table moff);
void I_uorVarDeclList (FILE *out, A_varDeclList vdl, S_table voff);
void I_uorVarDecl (FILE *out, A_varDecl vd, S_table voff);
void I_uorMethodDeclList (FILE *out, A_methodDeclList mdl, S_table moff);
void I_uorMethodDecl (FILE *out, A_methodDecl md, S_table moff);

/* auxiliary functions for condition testing */
int I_fieldListEq (Ty_fieldList fl1, Ty_fieldList fl2);
int I_tyEq (Ty_ty t1, Ty_ty t2);
int I_strictTyEq (Ty_ty t1, Ty_ty t2);
Ty_ty I_A_type2Ty_ty (A_type aty, A_pos defpos, string failvar,
                      string failfun);
