#include "print_src.h"
#include "fdmjast.h"
#include <stdio.h>

#define __DEBUG
#undef __DEBUG

/* */
void
printA_Pos (FILE *out, A_pos pos)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Pos...\n");
#endif
  return; // don't need to print position
}

/* */
void
printA_Type (FILE *out, A_type t)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Type...\n");
#endif
  switch (t->t)
    {
    case A_intType:
      fprintf (out, "int");
      break;
    case A_floatType:
      fprintf (out, "float");
      break;
    case A_idType:
      fprintf (out, "class %s", t->id);
      break;
    case A_intArrType:
      fprintf (out, "int[]");
      break;
    case A_floatArrType:
      fprintf (out, "float[]");
      break;
    default:
      fprintf (out, "Unknown data type! ");
      break;
    }
  return;
}

/* */
void
printA_Prog (FILE *out, A_prog p)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Prog...\n");
#endif
  if (p->m)
    printA_MainMethod (out, p->m);
  else
    fprintf (out, "Error: There's no main class!\n");
  if (p->cdl)
    printA_ClassDeclList (out, p->cdl);
  fflush (out);
  return;
}

/* */
void
printA_MainMethod (FILE *out, A_mainMethod main)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_MainMethod...\n");
#endif
  fprintf (out, "public int main () {\n");
  if (main->vdl)
    printA_VarDeclList (out, main->vdl);
  if (main->sl)
    printA_StmList (out, main->sl);
  fprintf (out, "}\n");
  return;
}

/* */
void
printA_VarDecl (FILE *out, A_varDecl vd)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_VarDecl...\n");
#endif
  if (!vd)
    return;
  printA_Pos (out, vd->pos);
  printA_Type (out, vd->t);
  fprintf (out, " %s", vd->v);
  if (vd->elist)
    {
      if (vd->t->t == A_intType || vd->t->t == A_floatType)
        {
          fprintf (out, "=");
          printA_Exp (out, vd->elist->head);
          if (vd->elist->tail)
            fprintf (out, ": Initialize an integer with an array?!\n");
        }
      else if (vd->t->t == A_intArrType || vd->t->t == A_floatArrType)
        {
          fprintf (out, "={");
          printA_ExpList (out, vd->elist);
          fprintf (out, "}");
        }
      else
        fprintf (out, ": Initialize non array?!\n");
    }
  fprintf (out, ";\n");
  return;
}

/* */
void
printA_VarDeclList (FILE *out, A_varDeclList vdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_VarDecList...\n");
#endif
  if (!vdl)
    return;
  printA_VarDecl (out, vdl->head);
  if (vdl->tail)
    printA_VarDeclList (out, vdl->tail);
  return;
}

/* */
void
printA_ClassDecl (FILE *out, A_classDecl cd)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ClassDecl...\n");
#endif
  if (!cd)
    return;
  printA_Pos (out, cd->pos);
  fprintf (out, "public class %s ", cd->id);
  if (cd->parentID)
    fprintf (out, "extends %s ", cd->parentID);
  fprintf (out, "{\n");
  if (cd->vdl)
    printA_VarDeclList (out, cd->vdl);
  if (cd->mdl)
    printA_MethodDeclList (out, cd->mdl);
  fprintf (out, "}\n");
  return;
}

/* */
void
printA_ClassDeclList (FILE *out, A_classDeclList cdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ClassDeclList...\n");
#endif
  if (!cdl)
    return;
  printA_ClassDecl (out, cdl->head);
  if (cdl->tail)
    printA_ClassDeclList (out, cdl->tail);
  return;
}

/* */
void
printA_MethodDecl (FILE *out, A_methodDecl md)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_MethodDecl...\n");
#endif
  if (!md)
    return;
  printA_Pos (out, md->pos);
  fprintf (out, "public ");
  printA_Type (out, md->t);
  fprintf (out, " %s", md->id);
  fprintf (out, "(");
  if (md->fl)
    printA_FormalList (out, md->fl);
  fprintf (out, ") {\n");
  if (md->vdl)
    printA_VarDeclList (out, md->vdl);
  if (md->sl)
    printA_StmList (out, md->sl);
  fprintf (out, "}\n");
  return;
}

/* */
void
printA_MethodDeclList (FILE *out, A_methodDeclList mdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_MethodDeclList...\n");
#endif
  if (!mdl)
    return;
  printA_MethodDecl (out, mdl->head);
  if (mdl->tail)
    printA_MethodDeclList (out, mdl->tail);
  return;
}

/* */
void
printA_Formal (FILE *out, A_formal f)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Formal...\n");
#endif
  if (!f)
    return;
  printA_Pos (out, f->pos);
  printA_Type (out, f->t);
  fprintf (out, " %s", f->id);
  return;
}

/* */
void
printA_FormalList (FILE *out, A_formalList fl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_FormalList...\n");
#endif
  if (!fl)
    return;
  printA_Formal (out, fl->head);
  if (fl->tail)
    {
      fprintf (out, ", ");
      printA_FormalList (out, fl->tail);
    }
  return;
}

/* */
void
printA_StmList (FILE *out, A_stmList sl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_StmList...\n");
#endif
  if (!sl)
    return;
  printA_Stm (out, sl->head);
  if (sl->tail)
    printA_StmList (out, sl->tail);
  return;
}

/* */
void
printA_Stm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Stm...\n");
#endif
  if (!s)
    return;
  switch (s->kind)
    {
    case A_nestedStm:
      printA_NestedStm (out, s);
      break;
    case A_ifStm:
      printA_IfStm (out, s);
      break;
    case A_whileStm:
      printA_WhileStm (out, s);
      break;
    case A_assignStm:
      printA_AssignStm (out, s);
      break;
    case A_arrayInit:
      printA_ArrayInit (out, s);
      break;
    case A_callStm:
      printA_CallStm (out, s);
      break;
    case A_continue:
      printA_Continue (out, s);
      break;
    case A_break:
      printA_Break (out, s);
      break;
    case A_return:
      printA_Return (out, s);
      break;
    case A_putnum:
      printA_Putnum (out, s);
      break;
    case A_putarray:
      printA_Putarray (out, s);
      break;
    case A_putch:
      printA_Putch (out, s);
      break;
    case A_starttime:
      printA_Starttime (out, s);
      break;
    case A_stoptime:
      printA_Stoptime (out, s);
      break;
    default:
      fprintf (out, "Unknown statement kind!");
      break;
    }
  return;
}

/* */
void
printA_NestedStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NestedStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_nestedStm)
    fprintf (out, "Not a nested stm!\n");
  else
    {
      fprintf (out, "{\n");
      printA_StmList (out, s->u.ns);
      fprintf (out, "}\n");
    }
  return;
}

/* */
void
printA_IfStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_IfStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_ifStm)
    fprintf (out, "Not a if stm!\n");
  else
    {
      fprintf (out, "if (");
      printA_Exp (out, s->u.if_stat.e);
      fprintf (out, ")\n");
      printA_Stm (out, s->u.if_stat.s1);
      if (s->u.if_stat.s2)
        {
          fprintf (out, "else\n");
          printA_Stm (out, s->u.if_stat.s2);
        };
    }
  return;
}

/* */
void
printA_WhileStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_WhileStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_whileStm)
    fprintf (out, "Not a while stm!\n");
  else
    {
      fprintf (out, "while (");
      printA_Exp (out, s->u.while_stat.e);
      if (!s->u.while_stat.s)
        fprintf (out, ");\n");
      else
        {
          fprintf (out, ")\n");
          printA_Stm (out, s->u.while_stat.s);
        }
    }
  return;
}

/* */
void
printA_AssignStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_AssignStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_assignStm)
    fprintf (out, "Not an assign stm!\n");
  else
    {
      printA_Exp (out, s->u.assign.arr);
      /* March 20, 2023. Remove redundant grammar rule.. hence the ast struct
          if (s->u.assign.pos) { //this is an array position
            fprintf(out, "[");
            printA_Exp(out, s->u.assign.pos);
            fprintf(out, "]");
          }
      */
      fprintf (out, "=");
      printA_Exp (out, s->u.assign.value);
      fprintf (out, ";\n");
    }
  return;
}

/* */
void
printA_ArrayInit (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ArrayInit...\n");
#endif
  if (s->kind != A_arrayInit)
    fprintf (out, "Not an array init stm!\n");
  else
    {
      printA_Exp (out, s->u.array_init.arr);
      fprintf (out, "[");
      fprintf (out, "]");
      fprintf (out, "={");
      printA_ExpList (out, s->u.array_init.init_values);
      fprintf (out, "};\n");
    }
  return;
}

/* */
void
printA_CallStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_CallStm...\n");
#endif
  if (s->kind != A_callStm)
    fprintf (out, "Not a call stm!\n");
  else
    {
      printA_Exp (out, s->u.call_stat.obj);
      fprintf (out, ".%s(", s->u.call_stat.fun);
      printA_ExpList (out, s->u.call_stat.el);
      fprintf (out, ");\n");
    }
  return;
}

/* */
void
printA_Continue (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Continue...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_continue)
    fprintf (out, "Not a continue stm!\n");
  else
    fprintf (out, "continue;\n");
  return;
}

/* */
void
printA_Break (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Break...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_break)
    fprintf (out, "Not a break stm!\n");
  else
    fprintf (out, "break;\n");
  return;
}

/* */
void
printA_Return (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Return...\n");
#endif
  if (s->kind != A_return)
    fprintf (out, "Not a return stm!\n");
  else
    {
      fprintf (out, "return ");
      if (s->u.e)
        printA_Exp (out, s->u.e);
      fprintf (out, ";\n");
    }
  return;
}

/* */
void
printA_Putnum (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Putin...\n");
#endif
  if (s->kind != A_putnum)
    fprintf (out, "Not a putnum stm!\n");
  else
    {
      fprintf (out, "putnum(");
      if (s->u.e)
        printA_Exp (out, s->u.e);
      fprintf (out, ");\n");
    }
  return;
}

/* */
void
printA_Putarray (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Putarray...\n");
#endif
  if (s->kind != A_putarray)
    fprintf (out, "Not a putarray stm!\n");
  else
    {
      fprintf (out, "putarray(");
      printA_Exp (out, s->u.putarray.e1);
      fprintf (out, ", ");
      printA_Exp (out, s->u.putarray.e2);
      fprintf (out, ");\n");
    }
  return;
}

/* */
void
printA_Putch (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Putch...\n");
#endif
  if (s->kind != A_putch)
    fprintf (out, "Not a putch stm!\n");
  else
    {
      fprintf (out, "putch(");
      if (s->u.e)
        printA_Exp (out, s->u.e);
      fprintf (out, ");\n");
    }
  return;
}

/* */
void
printA_Starttime (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_StarttimeStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_starttime)
    fprintf (out, "Not a starttime stm!\n");
  else
    fprintf (out, "starttime();\n");
  return;
}

/* */
void
printA_Stoptime (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_stoptime...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_stoptime)
    fprintf (out, "Not a stoptime stm!\n");
  else
    fprintf (out, "stoptime();\n");
  return;
}

/* */
void
printA_Exp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Exp...\n");
#endif
  if (!e)
    return;
  switch (e->kind)
    {
    case A_opExp:
      printA_OpExp (out, e);
      break;
    case A_arrayExp:
      printA_ArrayExp (out, e);
      break;
    case A_callExp:
      printA_CallExp (out, e);
      break;
    case A_classVarExp:
      printA_ClassVarExp (out, e);
      break;
    case A_boolConst:
      printA_BoolConst (out, e);
      break;
    case A_numConst:
      printA_NumConst (out, e);
      break;
    case A_lengthExp:
      printA_LengthExp (out, e);
      break;
    case A_idExp:
      printA_IdExp (out, e);
      break;
    case A_thisExp:
      printA_ThisExp (out, e);
      break;
    case A_newIntArrExp:
      printA_NewIntArrExp (out, e);
      break;
    case A_newFloatArrExp:
      printA_NewFloatArrExp (out, e);
      break;
    case A_newObjExp:
      printA_NewObjExp (out, e);
      break;
    case A_notExp:
      printA_NotExp (out, e);
      break;
    case A_minusExp:
      printA_MinusExp (out, e);
      break;
    case A_escExp:
      printA_EscExp (out, e);
      break;
    case A_getnum:
      printA_Getnum (out, e);
      break;
    case A_getch:
      printA_Getch (out, e);
      break;
    case A_getarray:
      printA_Getarray (out, e);
      break;
    default:
      fprintf (out, "Unknown expression kind!\n");
      break;
    }
  return;
}

/* */
void
printA_ExpList (FILE *out, A_expList el)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ExpList...\n");
#endif
  if (!el)
    return;
  printA_Exp (out, el->head);
  if (el->tail)
    {
      fprintf (out, ", ");
      printA_ExpList (out, el->tail);
    }
  return;
}

/* */
void
printA_OpExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_OpExp...\n");
#endif
  if (!e)
    return;
  fprintf (out, "(");
  printA_Exp (out, e->u.op.left);
  switch (e->u.op.oper)
    {
    case A_and:
      fprintf (out, "&&");
      break;
    case A_or:
      fprintf (out, "||");
      break;
    case A_less:
      fprintf (out, "<");
      break;
    case A_le:
      fprintf (out, "<=");
      break;
    case A_greater:
      fprintf (out, ">");
      break;
    case A_ge:
      fprintf (out, ">=");
      break;
    case A_eq:
      fprintf (out, "==");
      break;
    case A_ne:
      fprintf (out, "!=");
      break;
    case A_plus:
      fprintf (out, "+");
      break;
    case A_minus:
      fprintf (out, "-");
      break;
    case A_times:
      fprintf (out, "*");
      break;
    case A_div:
      fprintf (out, "/");
      break;
    }
  printA_Exp (out, e->u.op.right);
  fprintf (out, ")");
  return;
}

/* */
void
printA_ArrayExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_OpExp...\n");
#endif
  if (!e)
    return;
  printA_Exp (out, e->u.array_pos.arr);
  fprintf (out, "[");
  printA_Exp (out, e->u.array_pos.arr_pos);
  fprintf (out, "]");
  return;
}

/* */
void
printA_CallExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_CallExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_callExp)
    fprintf (out, "Not Call exp!\n");
  else
    {
      printA_Exp (out, e->u.call.obj);
      fprintf (out, ".%s(", e->u.call.fun);
      printA_ExpList (out, e->u.call.el);
      fprintf (out, ")");
    }
  return;
}

/* */
void
printA_ClassVarExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ClassVarExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_classVarExp)
    fprintf (out, "Not classVar exp!\n");
  else
    {
      printA_Exp (out, e->u.classvar.obj);
      fprintf (out, ".%s", e->u.classvar.var);
      /* remove redundant arrpos. March 20, 2023
           if (e->u.classvar.arrpos!=NULL) {
          fprintf(out, "[");
            printA_Exp(out, e->u.classvar.arrpos);
            fprintf(out, "]");
           }
      */
    }
  return;
}

/* */
void
printA_BoolConst (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_BoolConst...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_boolConst)
    fprintf (out, "Not Bool constant!\n");
  else
    {
      if (e->u.b)
        fprintf (out, "true");
      else
        fprintf (out, "false");
    }
  return;
}

/* */
void
printA_NumConst (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NumConst...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_numConst)
    fprintf (out, "Not Num constant!\n");
  else
    {
      if (e->u.num == (int)e->u.num)
        fprintf (out, "%d", (int)e->u.num);
      else
        fprintf (out, "%f", e->u.num);
    }
  return;
}

/* */
void
printA_LengthExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_LengthExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_lengthExp)
    fprintf (out, "Not length exp!\n");
  else
    {
      fprintf (out, "length(");
      printA_Exp (out, e->u.e);
      fprintf (out, ")");
    }
  return;
}

/* */
void
printA_IdExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_IdExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_idExp)
    fprintf (out, "Not ID exp!\n");
  else
    {
      fprintf (out, "%s", e->u.v);
    }
  return;
}

/* */
void
printA_ThisExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_ThisExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_thisExp)
    fprintf (out, "Not This exp!\n");
  else
    fprintf (out, "this");
  return;
}

/* */
void
printA_NewIntArrExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NewIntArrExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newIntArrExp)
    fprintf (out, "Not NewIntArray Exp!\n");
  else
    {
      fprintf (out, "new int[");
      printA_Exp (out, e->u.e);
      fprintf (out, "]");
    }
  return;
}

/* */
void
printA_NewFloatArrExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NewFloatArrExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newFloatArrExp)
    fprintf (out, "Not NewFloatArray Exp!\n");
  else
    {
      fprintf (out, "new float[");
      printA_Exp (out, e->u.e);
      fprintf (out, "]");
    }
  return;
}

/* */
void
printA_NewObjExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NewObjExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newObjExp)
    fprintf (out, "Not newObj exp!\n");
  else
    fprintf (out, "new %s()", e->u.v);
  return;
}

/* */
void
printA_NotExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_NotExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_notExp)
    fprintf (out, "Not Not exp!\n");
  else
    {
      fprintf (out, "!(");
      printA_Exp (out, e->u.e);
      fprintf (out, ")");
    }
  return;
}

/* */
void
printA_MinusExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_MinusExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_minusExp)
    fprintf (out, "Not Minus exp!\n");
  else
    {
      fprintf (out, "-");
      printA_Exp (out, e->u.e);
    }
  return;
}

/* */
void
printA_EscExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_EscExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_escExp)
    fprintf (out, "Not Esc exp!\n");
  else
    {
      fprintf (out, "({");
      printA_StmList (out, e->u.escExp.ns);
      fprintf (out, "} ");
      printA_Exp (out, e->u.escExp.exp);
      fprintf (out, ")");
    }
  return;
}

/* */
void
printA_Getnum (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Getnum...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getnum)
    fprintf (out, "Not getnum exp!\n");
  else
    fprintf (out, "getnum()");
  return;
}

/* */
void
printA_Getch (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Getch...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getch)
    fprintf (out, "Not getch exp!\n");
  else
    fprintf (out, "getch()");
  return;
}

/* */
void
printA_Getarray (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printA_Getarray...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getarray)
    fprintf (out, "Not getarray exp!\n");
  else
    {
      fprintf (out, "getarray(");
      printA_Exp (out, e->u.e);
      fprintf (out, ")");
    }
  return;
}
