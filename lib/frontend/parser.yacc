/* 1. declarations */

/* included code */
%{
// #define DEBUG 1
// #define UMDBG 1
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "fdmjast.h"

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

extern A_prog root;

%}

/* yylval */
%union {
    A_pos pos;
    A_type type;
    A_prog prog;
    A_mainMethod mainMethod;
    A_classDecl classDecl;
    A_classDeclList classDeclList;
    A_methodDecl methodDecl;
    A_methodDeclList methodDeclList;
    A_formal formal;
    A_formalList formalList;
    A_varDecl varDecl;
    A_varDeclList varDeclList;
    A_stmList stmList;
    A_stm stm;
    A_exp exp;
    A_expList expList;
}

/* terminal symbols */
%token<exp> ID NUM BTRUE BFALSE
%token<pos> PLUS MINUS TIMES DIV OR AND LESS LE GREATER GE EQ NE ILGLID
%token<pos> PUBLIC INT FLOAT MAIN IF ELSE WHILE CONTINUE BREAK RETURN CLASS
%token<pos> EXTENDS NEW PUTNUM PUTCH PUTARRAY STARTTIME STOPTIME LENGTH GETNUM
%token<pos> GETCH GETARRAY THIS
%token<pos> '(' ')' '{' '}' '[' ']' '=' ';' ',' '!'

/* non-termianl symbols */
%type<prog> PROG
%type<type> TYPE
%type<mainMethod> MAINMETHOD
%type<classDecl> CLASSDECL
%type<classDeclList> CLASSDECLLIST
%type<methodDecl> METHODDECL
%type<methodDeclList> METHODDECLLIST
/* %type<formal> FORMAL */
%type<formalList> FORMALLIST FORMALREST
%type<varDecl> VARDECL
%type<varDeclList> VARDECLLIST
%type<stm> STM
%type<stmList> STMLIST
%type<exp> EXP CONST
%type<expList> EXPLIST EXPREST CONSTLIST CONSTREST

/* start symbol */
%start PROG

/* precedence */
%left IF
%left ELSE
%left ILGLCOMM
%right '='
%left OR
%left AND
%left EQ NE
%left LESS LE GREATER GE
%left PLUS MINUS
%left TIMES DIV
%right UMINUS '!'
%left '[' ']' '(' ')' '.'

%% /* 2. rules */

PROG: MAINMETHOD CLASSDECLLIST {
    #ifdef DEBUG
    fprintf(stderr, "Prog rule reached\n");
    #endif
    root = A_Prog(A_Pos($1->pos->line, $1->pos->pos), $1, $2);
    $$ = root;
} ;

MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}' {
    #ifdef DEBUG
    fprintf(stderr, "VarDecList: %p, StmList: %p\n", $7, $8);
    #endif
    $$ = A_MainMethod($1, $7, $8);
} ;

VARDECLLIST: /* empty */ {
    $$ = NULL;
} | VARDECL VARDECLLIST {
    $$ = A_VarDeclList($1, $2);
} ;

VARDECL: CLASS ID ID ';' {
    $$ = A_VarDecl($1, A_Type($1, A_idType, $2->u.v), $3->u.v, NULL);
} | INT ID ';' {
    $$ = A_VarDecl($1, A_Type($1, A_intType, NULL), $2->u.v, NULL);
} | INT ID '=' CONST ';' {
    $$ = A_VarDecl($1, A_Type($1, A_intType, NULL), $2->u.v,
                   A_ExpList($4, NULL));
} | INT '[' ']' ID ';' {
    $$ = A_VarDecl($1, A_Type($1, A_intArrType, NULL), $4->u.v, NULL);
} | INT '[' ']' ID '=' '{' CONSTLIST '}' ';' {
    $$ = A_VarDecl($1, A_Type($1, A_intArrType, NULL), $4->u.v, $7);
} | FLOAT ID ';' {
    $$ = A_VarDecl($1, A_Type($1, A_floatType, NULL), $2->u.v, NULL);
} | FLOAT ID '=' CONST ';' {
    $$ = A_VarDecl($1, A_Type($1, A_floatType, NULL), $2->u.v,
                   A_ExpList($4, NULL));
} | FLOAT '[' ']' ID ';' {
    $$ = A_VarDecl($1, A_Type($1, A_floatArrType, NULL), $4->u.v, NULL);
} | FLOAT '[' ']' ID '=' '{' CONSTLIST '}' ';' {
    $$ = A_VarDecl($1, A_Type($1, A_floatArrType, NULL), $4->u.v, $7);
} | INT ILGLID ';' { // ilglid
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | INT ILGLID '=' CONST ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | INT '[' ']' ILGLID ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | INT '[' ']' ILGLID '=' '{' CONSTLIST '}' ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | FLOAT ILGLID ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | FLOAT ILGLID '=' CONST ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | FLOAT '[' ']' ILGLID ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | FLOAT '[' ']' ILGLID '=' '{' CONSTLIST '}' ';' {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("error: illegal identifier\n"); exit(0);
    $$ = NULL;
} | CLASS ID ID  { // nosemi
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | INT ID  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | INT ID '=' CONST  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | INT '[' ']' ID  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | INT '[' ']' ID '=' '{' CONSTLIST '}'  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | FLOAT ID  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | FLOAT ID '=' CONST  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | FLOAT '[' ']' ID  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} | FLOAT '[' ']' ID '=' '{' CONSTLIST '}'  {
    fprintf(stderr, "line %d ", $1->line);
    yyerror("expected \';\'\n");
    exit(0);
} ;

CONST: NUM {
    $$ = A_NumConst(A_Pos($1->pos->line, $1->pos->pos), $1->u.num);
} | MINUS NUM %prec UMINUS {
    #ifdef UMDBG
    fprintf(stderr, "CONST > UMINUS\n");
    #endif
    $$ = A_MinusExp($1, A_NumConst(A_Pos($2->pos->line, $2->pos->pos),
                                   $2->u.num));
} ;

CONSTLIST: /* empty */ {
    $$ = NULL;
} | CONST CONSTREST {
    $$ = A_ExpList($1, $2);
} ;

CONSTREST: /* empty */ {
    $$ = NULL;
} | ',' CONST CONSTREST {
    $$ = A_ExpList($2, $3);
} ;

STMLIST: /* empty */ {
    $$ = NULL;
} | STM STMLIST {
    $$ = A_StmList($1, $2);
} ;

STM: '{' STMLIST '}' {
    $$ = A_NestedStm($1, $2);
} | IF '(' EXP ')' STM ELSE STM {
    $$ = A_IfStm($1, $3, $5, $7);
} | IF '(' EXP ')' STM %prec IF {
    $$ = A_IfStm($1, $3, $5, NULL);
} | WHILE '(' EXP ')' STM {
    $$ = A_WhileStm($1, $3, $5);
} | WHILE '(' EXP ')' ';' {
    $$ = A_WhileStm($1, $3, NULL);
} | EXP '=' EXP ';' {
    $$ = A_AssignStm(A_Pos($1->pos->line, $1->pos->pos), $1, $3);
} | EXP '[' ']' '=' '{' EXPLIST '}' ';' {
    $$ = A_ArrayInit(A_Pos($1->pos->line, $1->pos->pos), $1, $6);
} | EXP '.' ID '(' EXPLIST ')' ';' {
    $$ = A_CallStm(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v, $5);
} | CONTINUE ';' {
    $$ = A_Continue($1);
} | BREAK ';' {
    $$ = A_Break($1);
} | RETURN EXP ';' {
    $$ = A_Return($1, $2);
} | PUTNUM '(' EXP ')' ';' {
    $$ = A_Putnum($1, $3);
} | PUTCH '(' EXP ')' ';' {
    $$ = A_Putch($1, $3);
} | PUTARRAY '(' EXP ',' EXP ')' ';' {
    $$ = A_Putarray($1, $3, $5);
} | STARTTIME '(' ')' ';' {
    $$ = A_Starttime($1);
} | STOPTIME '(' ')' ';' {
    $$ = A_Stoptime($1);
} ;

EXP: NUM {
    $$ = A_NumConst(A_Pos($1->pos->line, $1->pos->pos), $1->u.num);
} | BTRUE {
    $$ = A_BoolConst(A_Pos($1->pos->line, $1->pos->pos), $1->u.b);
} | BFALSE {
    $$ = A_BoolConst(A_Pos($1->pos->line, $1->pos->pos), $1->u.b);
} | LENGTH '(' EXP ')' {
    $$ = A_LengthExp($1, $3);
} | GETNUM '(' ')' {
    $$ = A_Getnum($1);
} | GETCH '(' ')' {
    $$ = A_Getch($1);
} | GETARRAY '(' EXP ')' {
    $$ = A_Getarray($1, $3);
} | ID {
    #ifdef DEBUG
    fprintf(stderr, "EXP > ID: %s\n", $1->u.v);
    #endif
    $$ = A_IdExp(A_Pos($1->pos->line, $1->pos->pos), $1->u.v);
} | THIS {
    $$ = A_ThisExp($1);
} | NEW INT '[' EXP ']' {
    $$ = A_NewIntArrExp($1, $4);
} | NEW FLOAT '[' EXP ']' {
    $$ = A_NewFloatArrExp($1, $4);
} | NEW ID '(' ')' {
    $$ = A_NewObjExp($1, $2->u.v);
} | EXP PLUS EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_plus, $3);
} | EXP MINUS EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_minus, $3);
} | EXP TIMES EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_times, $3);
} | EXP DIV EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_div, $3);
} | EXP AND EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_and, $3);
} | EXP OR EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_or, $3);
} | EXP LESS EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_less, $3);
} | EXP LE EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_le, $3);
} | EXP GREATER EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_greater, $3);
} | EXP GE EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_ge, $3);
} | EXP EQ EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_eq, $3);
} | EXP NE EXP {
    $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_ne, $3);
} | '!' EXP {
    $$ = A_NotExp($1, $2);
} | MINUS EXP %prec UMINUS {
    $$ = A_MinusExp($1, $2);
} | '(' EXP ')' {
    $2->pos = $1;
    $$ = $2;
} | '(' '{' STMLIST '}' EXP ')' {
    $$ = A_EscExp($1, $3, $5);
} | EXP '.' ID {
    $$ = A_ClassVarExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v);
} | EXP '.' ID '(' EXPLIST ')' {
    $$ = A_CallExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v, $5);
} | EXP '[' EXP ']' {
    $$ = A_ArrayExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3);
} | ',' %prec ILGLCOMM {
    fprintf(stderr, "line %d ",$1->line);
    yyerror("error: unexpected comma \',\'\n");
    exit(0);
} ;

EXPLIST: /* empty */ {
    $$ = NULL;
} | EXP EXPREST {
    $$ = A_ExpList($1, $2);
} ;

EXPREST: /* empty */ {
    $$ = NULL;
} | ',' EXP EXPREST {
    $$ = A_ExpList($2, $3);
}

CLASSDECLLIST: /* empty */ {
    $$ = NULL;
} | CLASSDECL CLASSDECLLIST {
    $$ = A_ClassDeclList($1, $2);
} ;

CLASSDECL: PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}' {
    $$ = A_ClassDecl($1, $3->u.v, $5->u.v, $7, $8);
} | PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}' {
    $$ = A_ClassDecl($1, $3->u.v, NULL, $5, $6);
}

METHODDECLLIST: /* empty */ {
    $$ = NULL;
} | METHODDECL METHODDECLLIST {
    $$ = A_MethodDeclList($1, $2);
} ;

METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}' {
    $$ = A_MethodDecl($1, $2, $3->u.v, $5, $8, $9);
} ;

TYPE: CLASS ID {
    $$ = A_Type($1, A_idType, $2->u.v);
} | INT {
    $$ = A_Type($1, A_intType, NULL);
} | INT '[' ']' {
    $$ = A_Type($1, A_intArrType, NULL);
} | FLOAT {
    $$ = A_Type($1, A_floatType, NULL);
} | FLOAT '[' ']' {
    $$ = A_Type($1, A_floatArrType, NULL);
} ;

FORMALLIST: /* empty */ {
    $$ = NULL;
} | TYPE ID  FORMALREST {
    $$ = A_FormalList(A_Formal(A_Pos($1->pos->line, $1->pos->pos),
                               $1, $2->u.v),
                      $3);
} ;

FORMALREST: /* empty */ {
    $$ = NULL;
} | ',' TYPE ID FORMALREST {
    $$ = A_FormalList(A_Formal($2->pos, $2, $3->u.v), $4);
} ;

%% /* 3. programs */

void
yyerror(char *s)
{
    fprintf(stderr, "%s\n",s);
}

int
yywrap()
{
    return(1);
}
