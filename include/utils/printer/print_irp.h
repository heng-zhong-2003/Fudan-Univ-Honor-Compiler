#pragma once

#include "tigerirp.h"

typedef enum
{
  IRP_parentheses,
  IRP_xml
} IRP_format;
void printIRP_set (IRP_format);

void printIRP_FuncDeclList (FILE *, T_funcDeclList);
void printIRP_FuncDecl (FILE *, T_funcDecl);
void printIRP_StmList (FILE *, T_stmList, int);
