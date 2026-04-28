/* 1. declarations */

/* included code */
%{

#include <stdlib.h>
#include "util.h"
#include "fdmjast.h"
#include "parser.h"

int c;
int line = 1;
int pos = 1;
%}

/* start conditions */
%s SNGLCMT MULTCMT

/* regexp nicknames */
id [a-z_A-Z][a-z_A-Z_0-9]*
ilglid [0-9][a-z_A-Z_0-9]*
num [1-9][0-9]*|0|[1-9][0-9]*\.[0-9]*|0\.[0-9]*|[1-9][0-9]*\.|0\.|\.[0-9]*
punc [()\[\]{}=,;.!]

%% /* 2. rules */
<INITIAL>"//" {
    pos += 2;
    BEGIN SNGLCMT;
}
<SNGLCMT>\n {
    line += 1;
    pos = 1;
    BEGIN INITIAL;
}
<SNGLCMT>. {
    pos += 1;
}
<INITIAL>"/*" {
    pos += 2;
    BEGIN MULTCMT;
}
<MULTCMT>"*/" {
    pos += 2;
    BEGIN INITIAL;
}
<MULTCMT>\n {
    line += 1;
    pos = 1;
}
<MULTCMT>. {
    pos += 1;
}
<INITIAL>" "|\t {
    pos += 1;
}
<INITIAL>\n {
    line += 1;
    pos = 1;
}
<INITIAL>\r {}
<INITIAL>"+" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return PLUS;
}
<INITIAL>"-" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return MINUS;
}
<INITIAL>"*" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return TIMES;
}
<INITIAL>"/" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return DIV;
}
<INITIAL>"||" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return OR;
}
<INITIAL>"&&" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return AND;
}
<INITIAL>"<" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return LESS;
}
<INITIAL>"<=" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return LE;
}
<INITIAL>">" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return GREATER;
}
<INITIAL>">=" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return GE;
}
<INITIAL>"==" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return EQ;
}
<INITIAL>"!=" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return NE;
}
<INITIAL>"true" {
    yylval.exp = A_BoolConst(A_Pos(line, pos), 1);
    pos += yyleng;
    return BTRUE;
}
<INITIAL>"false" {
    yylval.exp = A_BoolConst(A_Pos(line, pos), 0);
    pos += yyleng;
    return BFALSE;
}
<INITIAL>"public" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return PUBLIC;
}
<INITIAL>"int" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return INT;
}
<INITIAL>"float" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return FLOAT;
}
<INITIAL>"main" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return MAIN;
}
<INITIAL>"if" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return IF;
}
<INITIAL>"else" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return ELSE;
}
<INITIAL>"while" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return WHILE;
}
<INITIAL>"continue" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return CONTINUE;
}
<INITIAL>"break" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return BREAK;
}
<INITIAL>"class" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return CLASS;
}
<INITIAL>"extends" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return EXTENDS;
}
<INITIAL>"new" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return NEW;
}
<INITIAL>"putnum" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return PUTNUM;
}
<INITIAL>"putch" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return PUTCH;
}
<INITIAL>"putarray" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return PUTARRAY;
}
<INITIAL>"starttime" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return STARTTIME;
}
<INITIAL>"stoptime" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return STOPTIME;
}
<INITIAL>"length" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return LENGTH;
}
<INITIAL>"getnum" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return GETNUM;
}
<INITIAL>"getch" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return GETCH;
}
<INITIAL>"getarray" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return GETARRAY;
}
<INITIAL>"this" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return THIS;
}
<INITIAL>"return" {
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return RETURN;
}
<INITIAL>{punc} {
    yylval.pos = A_Pos(line, pos);
    pos += 1;
    c = yytext[0];
    return c;
}
<INITIAL>{num} {
    yylval.exp = A_NumConst(A_Pos(line, pos), atof(yytext));
    pos += yyleng;
    return NUM;
}
<INITIAL>{id} {
    yylval.exp = A_IdExp(A_Pos(line, pos), String(yytext));
    pos += yyleng;
    return ID;
}
<INITIAL>{ilglid} {
    // fprintf(stderr, "lex ilglid %s\n", yytext);
    yylval.pos = A_Pos(line, pos);
    pos += yyleng;
    return ILGLID;
}

. {return yytext[0];} // this needs to be replaced!

%% /* 3. programs */
