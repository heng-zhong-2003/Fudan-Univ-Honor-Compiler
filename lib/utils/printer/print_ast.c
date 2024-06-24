#include "print_ast.h"
#include "fdmjast.h"
#include <stdio.h>

#define __DEBUG
#undef __DEBUG

/* */
void
printX_Pos (FILE *out, A_pos pos)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Pos...\n");
#endif
  if (!pos)
    return;
  fprintf (out, "<pos><line>%d</line><col>%d</col></pos>\n", pos->line,
           pos->pos);
  return; // don't need to print position
}

/* */
void
printX_Type (FILE *out, A_type t)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Type...\n");
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
printX_Prog (FILE *out, A_prog p)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Prog...\n");
#endif
  fprintf (stdout, "<?xml version=\"1.0\"?>\n");
  fprintf (stdout, "<Program>\n");
  printX_Pos (out, p->pos);
  if (p->m)
    printX_MainMethod (out, p->m);
  else
    fprintf (out, "Error: There's no main class!\n");
  if (p->cdl)
    printX_ClassDeclList (out, p->cdl);
  fprintf (stdout, "</Program>\n");
  fflush (out);
  return;
}

/* */
void
printX_MainMethod (FILE *out, A_mainMethod main)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_MainMethod...\n");
#endif
  fprintf (out, "<main>\n");
  printX_Pos (stdout, main->pos);
  if (main->vdl)
    printX_VarDeclList (out, main->vdl);
  if (main->sl)
    printX_StmList (out, main->sl);
  fprintf (out, "</main>\n");
  return;
}

/* */
void
printX_VarDecl (FILE *out, A_varDecl vd)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_VarDecl...\n");
#endif
  if (!vd)
    return;
  printX_Pos (out, vd->pos);
  fprintf (out, "<Type>\n");
  printX_Type (out, vd->t);
  fprintf (out, "</Type>\n<id>");
  fprintf (out, "%s", vd->v);
  fprintf (out, "</id>");
  if (vd->elist)
    {
      if (vd->t->t == A_intType || vd->t->t == A_floatType)
        {
          fprintf (out, "<initValue>");
          // fprintf(out, "=");
          printX_Exp (out, vd->elist->head);
          fprintf (out, "</initValue>");
          if (vd->elist->tail)
            fprintf (out, ": Initialize an integer with an array?!\n");
        }
      else if (vd->t->t == A_intArrType || vd->t->t == A_floatArrType)
        {
          fprintf (out, "<initValueList>\n");
          // fprintf(out, "={");
          printX_ExpList (out, vd->elist);
          fprintf (out, "</initValueList>\n");
        }
      else
        fprintf (out, ": Initialize non array?!\n");
    }
  // fprintf(out, ";\n");
  return;
}

/* */
void
printX_VarDeclList (FILE *out, A_varDeclList vdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_VarDecList...\n");
#endif
  if (!vdl)
    return;
  fprintf (out, "<varDeclList>\n<varDecl>\n");
  printX_VarDecl (out, vdl->head);
  fprintf (out, "</varDecl>\n");
  if (vdl->tail)
    printX_VarDeclList (out, vdl->tail);
  fprintf (out, "</varDeclList>\n");
  return;
}

/* */
void
printX_ClassDecl (FILE *out, A_classDecl cd)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ClassDecl...\n");
#endif
  if (!cd)
    return;
  printX_Pos (out, cd->pos);
  fprintf (out, "<className>%s</className>\n", cd->id);
  // fprintf(out, "<class>\n");
  if (cd->parentID)
    fprintf (out, "<extends>%s</extends>\n", cd->parentID);
  // fprintf(out, "{\n");
  if (cd->vdl)
    printX_VarDeclList (out, cd->vdl);
  if (cd->mdl)
    printX_MethodDeclList (out, cd->mdl);
  // fprintf(out, "</class>\n");
  return;
}

/* */
void
printX_ClassDeclList (FILE *out, A_classDeclList cdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ClassDeclList...\n");
#endif
  if (!cdl)
    return;
  fprintf (out, "<classDeclList>\n<classDecl>\n");
  printX_ClassDecl (out, cdl->head);
  fprintf (out, "</classDecl>\n");
  if (cdl->tail)
    printX_ClassDeclList (out, cdl->tail);
  fprintf (out, "</classDeclList>\n");
  return;
}

/* */
void
printX_MethodDecl (FILE *out, A_methodDecl md)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_MethodDecl...\n");
#endif
  if (!md)
    return;
  printX_Pos (out, md->pos);
  // fprintf(out, "public ");
  fprintf (out, "<type>\n");
  printX_Type (out, md->t);
  fprintf (out, "</type>\n<name>\n");
  fprintf (out, "%s", md->id);
  fprintf (out, "</name>\n");
  // fprintf(out, "(");
  if (md->fl)
    printX_FormalList (out, md->fl);
  // fprintf(out, ") {\n");
  if (md->vdl)
    printX_VarDeclList (out, md->vdl);
  if (md->sl)
    printX_StmList (out, md->sl);
  return;
}

/* */
void
printX_MethodDeclList (FILE *out, A_methodDeclList mdl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_MethodDeclList...\n");
#endif
  if (!mdl)
    return;
  fprintf (out, "<methodDeclList>\n<methodDecl>\n");
  printX_MethodDecl (out, mdl->head);
  fprintf (out, "</methodDecl>\n");
  if (mdl->tail)
    printX_MethodDeclList (out, mdl->tail);
  fprintf (out, "</methodDeclList>\n");
  return;
}

/* */
void
printX_Formal (FILE *out, A_formal f)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Formal...\n");
#endif
  if (!f)
    return;
  printX_Pos (out, f->pos);
  fprintf (out, "<type>\n");
  printX_Type (out, f->t);
  fprintf (out, "</type>\n");
  fprintf (out, "<id>");
  fprintf (out, "%s", f->id);
  fprintf (out, "</id>\n");
  return;
}

/* */
void
printX_FormalList (FILE *out, A_formalList fl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_FormalList...\n");
#endif
  if (!fl)
    return;
  fprintf (out, "<formalList>\n<formal>\n");
  printX_Formal (out, fl->head);
  fprintf (out, "</formal>");
  if (fl->tail)
    {
      // fprintf(out, ", ");
      printX_FormalList (out, fl->tail);
    }
  fprintf (out, "</formalList>\n");
  return;
}

/* */
void
printX_StmList (FILE *out, A_stmList sl)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_StmList...\n");
#endif
  if (!sl)
    return;
  fprintf (out, "<stmList>\n");
  printX_Stm (out, sl->head);
  if (sl->tail)
    printX_StmList (out, sl->tail);
  fprintf (out, "</stmList>\n");
  return;
}

/* */
void
printX_Stm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Stm...\n");
#endif
  if (!s)
    return;
  switch (s->kind)
    {
    case A_nestedStm:
      printX_NestedStm (out, s);
      break;
    case A_ifStm:
      printX_IfStm (out, s);
      break;
    case A_whileStm:
      printX_WhileStm (out, s);
      break;
    case A_assignStm:
      printX_AssignStm (out, s);
      break;
    case A_arrayInit:
      printX_ArrayInit (out, s);
      break;
    case A_callStm:
      printX_CallStm (out, s);
      break;
    case A_continue:
      printX_Continue (out, s);
      break;
    case A_break:
      printX_Break (out, s);
      break;
    case A_return:
      printX_Return (out, s);
      break;
    case A_putnum:
      printX_Putnum (out, s);
      break;
    case A_putarray:
      printX_Putarray (out, s);
      break;
    case A_putch:
      printX_Putch (out, s);
      break;
    case A_starttime:
      printX_Starttime (out, s);
      break;
    case A_stoptime:
      printX_Stoptime (out, s);
      break;
    default:
      fprintf (out, "Unknown statement kind!");
      break;
    }
  return;
}

/* */
void
printX_NestedStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NestedStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_nestedStm)
    fprintf (out, "Not a nested stm!\n");
  else
    {
      fprintf (out, "<nestedStm>\n");
      printX_Pos (out, s->pos);
      //    fprintf(out, "{\n");
      printX_StmList (out, s->u.ns);
      fprintf (out, "</nestedStm>\n");
      //    fprintf(out, "}\n");
    }
  return;
}

/* */
void
printX_IfStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_IfStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_ifStm)
    fprintf (out, "Not a if stm!\n");
  else
    {
      fprintf (out, "<if>\n");
      printX_Pos (out, s->pos);
      printX_Exp (out, s->u.if_stat.e);
      fprintf (out, "<then>\n");
      printX_Stm (out, s->u.if_stat.s1);
      fprintf (out, "</then>\n");
      if (s->u.if_stat.s2)
        {
          fprintf (out, "<else>\n");
          printX_Stm (out, s->u.if_stat.s2);
          fprintf (out, "</else>\n");
        };
      fprintf (out, "</if>\n");
    }
  return;
}

/* */
void
printX_WhileStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_WhileStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_whileStm)
    fprintf (out, "Not a while stm!\n");
  else
    {
      fprintf (out, "<while>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "<cond>\n");
      printX_Exp (out, s->u.while_stat.e);
      fprintf (out, "</cond>\n");
      if (!s->u.while_stat.s)
        // fprintf(out, ");\n");
        {
        }
      else
        {
          // fprintf(out, ")\n");
          printX_Stm (out, s->u.while_stat.s);
        }
      fprintf (out, "</while>\n");
    }
  return;
}

/* */
void
printX_AssignStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_AssignStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_assignStm)
    fprintf (out, "Not an assign stm!\n");
  else
    {
      fprintf (out, "<assign>\n<left>\n");
      printX_Exp (out, s->u.assign.arr);
      fprintf (out, "</left>\n");
      /* March 20, 2023. Remove redundant grammar rule.. hence the ast struct
          if (s->u.assign.pos) { //this is an array position
            fprintf(out, "[");
            printX_Exp(out, s->u.assign.pos);
            fprintf(out, "]");
          }
      */
      fprintf (out, "<right>\n");
      printX_Exp (out, s->u.assign.value);
      fprintf (out, "</right>\n");
      fprintf (out, "</assign>\n");
    }
  return;
}

/* */
void
printX_ArrayInit (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ArrayInit...\n");
#endif
  if (s->kind != A_arrayInit)
    fprintf (out, "Not an array init stm!\n");
  else
    {
      fprintf (out, "<arrayInit>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "<left>");
      printX_Exp (out, s->u.array_init.arr);
      fprintf (out, "</left>\n");
      //    fprintf(out, "[");
      //    fprintf(out, "]");
      //    fprintf(out, "={");
      fprintf (out, "<right>\n");
      printX_ExpList (out, s->u.array_init.init_values);
      fprintf (out, "</right>\n");
      fprintf (out, "</arrayInit>\n");
    }
  return;
}

/* */
void
printX_CallStm (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_CallStm...\n");
#endif
  if (s->kind != A_callStm)
    fprintf (out, "Not a call stm!\n");
  else
    {
      fprintf (out, "<call>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "<obj>\n");
      printX_Exp (out, s->u.call_stat.obj);
      fprintf (out, "</obj>\n<fun>\n");
      fprintf (out, "%s", s->u.call_stat.fun);
      fprintf (out, "</fun>\n<args>\n");
      printX_ExpList (out, s->u.call_stat.el);
      fprintf (out, "</args>\n");
      fprintf (out, "</call>\n");
    }
  return;
}

/* */
void
printX_Continue (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Continue...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_continue)
    fprintf (out, "Not a continue stm!\n");
  else
    {
      fprintf (out, "<continue>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "</continue>\n");
    }
  return;
}

/* */
void
printX_Break (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Break...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_break)
    fprintf (out, "Not a break stm!\n");
  else
    {
      fprintf (out, "<break>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "</break>\n");
    }
  return;
}

/* */
void
printX_Return (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Return...\n");
#endif
  if (s->kind != A_return)
    fprintf (out, "Not a return stm!\n");
  else
    {
      fprintf (out, "<return>\n");
      printX_Pos (out, s->pos);
      if (s->u.e)
        printX_Exp (out, s->u.e);
      fprintf (out, "</return>\n");
    }
  return;
}

/* */
void
printX_Putnum (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Putin...\n");
#endif
  if (s->kind != A_putnum)
    fprintf (out, "Not a putnum stm!\n");
  else
    {
      fprintf (out, "<putnum>\n");
      printX_Pos (out, s->pos);
      if (s->u.e)
        printX_Exp (out, s->u.e);
      fprintf (out, "</putnum>\n");
    }
  return;
}

/* */
void
printX_Putarray (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Putarray...\n");
#endif
  if (s->kind != A_putarray)
    fprintf (out, "Not a putarray stm!\n");
  else
    {
      fprintf (out, "<putarray>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "<arr>\n");
      printX_Exp (out, s->u.putarray.e1);
      fprintf (out, "</arr>\n<length>\n");
      printX_Exp (out, s->u.putarray.e2);
      fprintf (out, "</length>\n</putarray>\n");
    }
  return;
}

/* */
void
printX_Putch (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Putch...\n");
#endif
  if (s->kind != A_putch)
    fprintf (out, "Not a putch stm!\n");
  else
    {
      fprintf (out, "<putch>\n");
      printX_Pos (out, s->pos);
      if (s->u.e)
        printX_Exp (out, s->u.e);
      fprintf (out, "</putch>\n");
    }
  return;
}

/* */
void
printX_Starttime (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_StarttimeStm...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_starttime)
    fprintf (out, "Not a starttime stm!\n");
  else
    {
      fprintf (out, "<starttime>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "</starttime>\n");
    }
  return;
}

/* */
void
printX_Stoptime (FILE *out, A_stm s)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_stoptime...\n");
#endif
  if (!s)
    return;
  if (s->kind != A_stoptime)
    fprintf (out, "Not a stoptime stm!\n");
  else
    {
      fprintf (out, "<stoptime>\n");
      printX_Pos (out, s->pos);
      fprintf (out, "</stoptime>\n");
    }
  return;
}

/* */
void
printX_Exp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Exp...\n");
#endif
  if (!e)
    return;
  switch (e->kind)
    {
    case A_opExp:
      printX_OpExp (out, e);
      break;
    case A_arrayExp:
      printX_ArrayExp (out, e);
      break;
    case A_callExp:
      printX_CallExp (out, e);
      break;
    case A_classVarExp:
      printX_ClassVarExp (out, e);
      break;
    case A_boolConst:
      printX_BoolConst (out, e);
      break;
    case A_numConst:
      printX_NumConst (out, e);
      break;
    case A_lengthExp:
      printX_LengthExp (out, e);
      break;
    case A_idExp:
      printX_IdExp (out, e);
      break;
    case A_thisExp:
      printX_ThisExp (out, e);
      break;
    case A_newIntArrExp:
      printX_NewIntArrExp (out, e);
      break;
    case A_newFloatArrExp:
      printX_NewFloatArrExp (out, e);
      break;
    case A_newObjExp:
      printX_NewObjExp (out, e);
      break;
    case A_notExp:
      printX_NotExp (out, e);
      break;
    case A_minusExp:
      printX_MinusExp (out, e);
      break;
    case A_escExp:
      printX_EscExp (out, e);
      break;
    case A_getnum:
      printX_Getnum (out, e);
      break;
    case A_getch:
      printX_Getch (out, e);
      break;
    case A_getarray:
      printX_Getarray (out, e);
      break;
    default:
      fprintf (out, "Unknown expression kind!\n");
      break;
    }
  return;
}

/* */
void
printX_ExpList (FILE *out, A_expList el)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ExpList...\n");
#endif
  if (!el)
    return;
  fprintf (out, "<expList>\n");
  printX_Exp (out, el->head);
  if (el->tail)
    {
      // fprintf(out, ", ");
      printX_ExpList (out, el->tail);
    }
  fprintf (out, "</expList>\n");
  return;
}

/* */
void
printX_OpExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_OpExp...\n");
#endif
  if (!e)
    return;
  fprintf (out, "<opExp>\n");
  printX_Pos (out, e->pos);
  fprintf (out, "<left>\n");
  printX_Exp (out, e->u.op.left);
  fprintf (out, "</left>\n<op>");
  switch (e->u.op.oper)
    {
    case A_and:
      fprintf (out, "AND");
      break;
    case A_or:
      fprintf (out, "OR");
      break;
    case A_less:
      fprintf (out, "LESS THAN");
      break;
    case A_le:
      fprintf (out, "LESS THAN OR EQUAL TO");
      break;
    case A_greater:
      fprintf (out, "GREATER THAN");
      break;
    case A_ge:
      fprintf (out, "GREATER THAN OR EQUAL TO");
      break;
    case A_eq:
      fprintf (out, "EQUAL TO");
      break;
    case A_ne:
      fprintf (out, "NOT EQUAL TO");
      break;
    case A_plus:
      fprintf (out, "PLUS");
      break;
    case A_minus:
      fprintf (out, "MINUS");
      break;
    case A_times:
      fprintf (out, "TIMES");
      break;
    case A_div:
      fprintf (out, "DIVIDES");
      break;
    }
  fprintf (out, "</op>\n<right>\n");
  printX_Exp (out, e->u.op.right);
  fprintf (out, "</right>\n</opExp>\n");
  return;
}

/* */
void
printX_ArrayExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_OpExp...\n");
#endif
  if (!e)
    return;
  fprintf (out, "<arrExp>\n");
  printX_Pos (out, e->pos);
  fprintf (out, "<arr>\n");
  printX_Exp (out, e->u.array_pos.arr);
  fprintf (out, "</arr>\n<index>\n");
  printX_Exp (out, e->u.array_pos.arr_pos);
  fprintf (out, "</index>\n</arrExp>\n");
  return;
}

/* */
void
printX_CallExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_CallExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_callExp)
    fprintf (out, "Not Call exp!\n");
  else
    {
      fprintf (out, "<callExp>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "<obj>\n");
      printX_Exp (out, e->u.call.obj);
      fprintf (out, "</obj>\n<fun>\n");
      fprintf (out, "%s", e->u.call.fun);
      fprintf (out, "</fun>\n<par>\n");
      printX_ExpList (out, e->u.call.el);
      fprintf (out, "</par>\n</callExp>\n");
    }
  return;
}

/* */
void
printX_ClassVarExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ClassVarExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_classVarExp)
    fprintf (out, "Not classVar exp!\n");
  else
    {
      fprintf (out, "<classVarExp>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "<obj>\n");
      printX_Exp (out, e->u.classvar.obj);
      fprintf (out, "</obj>\n<var>\n");
      fprintf (out, "%s", e->u.classvar.var);
      fprintf (out, "</var>\n</classVarExp>\n");
      /* remove redundant arrpos. March 20, 2023
           if (e->u.classvar.arrpos!=NULL) {
          fprintf(out, "[");
            printX_Exp(out, e->u.classvar.arrpos);
            fprintf(out, "]");
           }
      */
    }
  return;
}

/* */
void
printX_BoolConst (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_BoolConst...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_boolConst)
    fprintf (out, "Not Bool constant!\n");
  else
    {
      if (e->u.b)
        {
          fprintf (out, "<true>\n");
          printX_Pos (out, e->pos);
          fprintf (out, "</true>\n");
        }
      else
        {
          fprintf (out, "<false>\n");
          printX_Pos (out, e->pos);
          fprintf (out, "</false>\n");
        }
    }
  return;
}

/* */
void
printX_NumConst (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NumConst...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_numConst)
    fprintf (out, "Not Num constant!\n");
  else
    {
      fprintf (out, "<num>\n");
      printX_Pos (out, e->pos);
      if (e->u.num == (int)e->u.num)
        fprintf (out, "<value>%d</value></num>\n", (int)e->u.num);
      else
        fprintf (out, "<value>%f</value></num>\n", e->u.num);
    }
  return;
}

/* */
void
printX_LengthExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_LengthExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_lengthExp)
    fprintf (out, "Not length exp!\n");
  else
    {
      fprintf (out, "<lengthOf>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</lengthOf>\n");
    }
  return;
}

/* */
void
printX_IdExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_IdExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_idExp)
    fprintf (out, "Not ID exp!\n");
  else
    {
      fprintf (out, "<id>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "<string>%s</string></id>\n", e->u.v);
    }
  return;
}

/* */
void
printX_ThisExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_ThisExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_thisExp)
    fprintf (out, "Not This exp!\n");
  else
    {
      fprintf (out, "<this>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "</this>\n");
    }
  return;
}

/* */
void
printX_NewIntArrExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NewIntArrExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newIntArrExp)
    fprintf (out, "Not NewIntArray Exp!\n");
  else
    {
      fprintf (out, "<newInt>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</newInt>\n");
    }
  return;
}

/* */
void
printX_NewFloatArrExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NewFloatArrExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newFloatArrExp)
    fprintf (out, "Not NewFloatArray Exp!\n");
  else
    {
      fprintf (out, "<newFloat>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</newFloat>\n");
    }
  return;
}

/* */
void
printX_NewObjExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NewObjExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_newObjExp)
    fprintf (out, "Not newObj exp!\n");
  else
    {
      fprintf (out, "<newObj>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "<id>%s</id></newObj>\n", e->u.v);
    }
  return;
}

/* */
void
printX_NotExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_NotExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_notExp)
    fprintf (out, "Not Not exp!\n");
  else
    {
      fprintf (out, "<notExp>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</notExp>\n");
    }
  return;
}

/* */
void
printX_MinusExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_MinusExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_minusExp)
    fprintf (out, "Not Minus exp!\n");
  else
    {
      fprintf (out, "<minusExp>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</minusExp>\n");
    }
  return;
}

/* */
void
printX_EscExp (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_EscExp...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_escExp)
    fprintf (out, "Not Esc exp!\n");
  else
    {
      fprintf (out, "<escExp>\n");
      printX_Pos (out, e->pos);
      printX_StmList (out, e->u.escExp.ns);
      printX_Exp (out, e->u.escExp.exp);
      fprintf (out, "</escExp>\n");
    }
  return;
}

/* */
void
printX_Getnum (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Getnum...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getnum)
    fprintf (out, "Not getnum exp!\n");
  else
    {
      fprintf (out, "<getnum>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "</getnum>\n");
    }
  return;
}

/* */
void
printX_Getch (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Getch...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getch)
    fprintf (out, "Not getch exp!\n");
  else
    {
      fprintf (out, "<getch>\n");
      printX_Pos (out, e->pos);
      fprintf (out, "</getch>\n");
    }
  return;
}

/* */
void
printX_Getarray (FILE *out, A_exp e)
{
#ifdef __DEBUG
  fprintf (out, "Entering printX_Getarray...\n");
#endif
  if (!e)
    return;
  if (e->kind != A_getarray)
    fprintf (out, "Not getarray exp!\n");
  else
    {
      fprintf (out, "<getarray>\n");
      printX_Pos (out, e->pos);
      printX_Exp (out, e->u.e);
      fprintf (out, "</getarray>\n");
    }
  return;
}
