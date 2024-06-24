/*
 * FDMJ 2024 with types
 * printtree.c - functions to print out intermediate representation (IR) trees.
 * This is trying to print parenthesized as well as XML form
 * Use a global flag to control what format to print.
 *
 */
#include "util.h"
#include <stdio.h>
//#include "symbol.h"
#include "print_irp.h"
#include "temp.h"
#include "tigerirp.h"

#define __PRING_IRP_DEBUG
#undef __PRING_IRP_DEBUG

static IRP_format format = IRP_parentheses;

void
printIRP_set (IRP_format f)
{
  format = f;
}

static string
getT_type (T_type type)
{
  return type == T_int ? "T_int" : "T_float";
}

/* local function prototype */
static void pr_exp (FILE *out, T_exp exp, int d);

static void
indent (FILE *out, int d)
{
  int i;
  if (format == IRP_xml)
    return; // don't indent if XML
  for (i = 0; i <= d; i++)
    fprintf (out, "  ");
}

static char bin_oper[][12]
    = { "T_plus", "T_minus", "T_mul", "T_div", "T_and", "T_or" };

static char rel_oper[][12]
    = { "T_eq", "T_ne", "T_lt", "T_gt", "T_le", "T_ge" };

static char stm_kind[][12] = { "T_SEQ",  "T_LABEL", "T_JUMP",  "T_CJUMP",
                               "T_MOVE", "T_EXP",   "T_RETURN" };

static void
pr_stm (FILE *out, T_stm stm, int d)
{
  if (!stm)
    {
      return;
    }
#ifdef __PRING_IRP_DEBUG
  printf ("---Entering pr_stm: %s\n", stm_kind[stm->kind]);
#endif
  switch (stm->kind)
    {
    case T_SEQ:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Seq(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Seq>\n");
          break;
        }
      pr_stm (out, stm->u.SEQ.left, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, ",\n");
      pr_stm (out, stm->u.SEQ.right, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</Seq>\n");
          break;
        }
      break;
    case T_LABEL:
      indent (out, d);
      if (format == IRP_parentheses)
        fprintf (out, "T_Label(Temp_namedlabel(String(\"%s\")))",
                 Temp_labelstring (stm->u.LABEL));
      else
        fprintf (out, "<Label>%s</Label>", Temp_labelstring (stm->u.LABEL));
      break;
    case T_JUMP:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Jump(");
          fprintf (out, "Temp_namedlabel(String(\"%s\"))",
                   Temp_labelstring (stm->u.JUMP.jump));
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "<Jump>\n");
          fprintf (out, "<label>%s</label>",
                   Temp_labelstring (stm->u.JUMP.jump));
          fprintf (out, "</Jump>\n");
          break;
        }
      break;
    case T_CJUMP:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Cjump(%s,\n", rel_oper[stm->u.CJUMP.op]);
          pr_exp (out, stm->u.CJUMP.left, d + 1);
          fprintf (out, ",\n");
          pr_exp (out, stm->u.CJUMP.right, d + 1);
          fprintf (out, ",\n");
          indent (out, d + 1);
          fprintf (out, "Temp_namedlabel(String(\"%s\")),\n",
                   Temp_labelstring (stm->u.CJUMP.t));
          indent (out, d + 1);
          fprintf (out, "Temp_namedlabel(String(\"%s\"))",
                   Temp_labelstring (stm->u.CJUMP.f));
          fprintf (out, "\n");
          indent (out, d);
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "<Cjump>\n");
          fprintf (out, "<op>%s</op>\n", rel_oper[stm->u.CJUMP.op]);
          fprintf (out, "<left>\n");
          pr_exp (out, stm->u.CJUMP.left, d + 1);
          fprintf (out, "</left>\n");
          fprintf (out, "<right>\n");
          pr_exp (out, stm->u.CJUMP.right, d + 1);
          fprintf (out, "</right>\n");
          fprintf (out, "<true>%s</true>\n",
                   Temp_labelstring (stm->u.CJUMP.t));
          fprintf (out, "<false>%s</false>\n",
                   Temp_labelstring (stm->u.CJUMP.f));
          fprintf (out, "</Cjump>\n");
          break;
        }
      break;
    case T_MOVE:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Move(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Move><left>\n");
          break;
        }
      pr_exp (out, stm->u.MOVE.dst, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, ",\n");
      else
        fprintf (out, "</left>\n<right>");
      pr_exp (out, stm->u.MOVE.src, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</right></Move>\n");
          break;
        }
      break;
    case T_EXP:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Exp(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Exp>\n");
          break;
        }
      pr_exp (out, stm->u.EXP, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</Exp>\n");
          break;
        }
      break;
    case T_RETURN:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Return(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Return>\n");
          break;
        }
      pr_exp (out, stm->u.EXP, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</Return>\n");
          break;
        }
      break;
    }
#ifdef __PRING_IRP_DEBUG
  printf ("---Exiting pr_stm: %s\n", stm_kind[stm->kind]);
#endif
}

static void
pr_temp (FILE *out, Temp_temp t, int d)
{
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, "Temp_namedtemp(%s,%s)", Temp_look (Temp_name (), t),
               getT_type (t->type));
      break;
    case IRP_xml:
      fprintf (stdout, "<temp><name>%s</name><type>%s</type></temp>",
               Temp_look (Temp_name (), t), getT_type (t->type));
      break;
    }
  return;
}

static void
pr_templist (FILE *out, Temp_tempList tl, int d)
{
  indent (out, d);
  if (!tl)
    {
      if (format == IRP_parentheses)
        fprintf (out, "NULL");
      return;
    }
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, "Temp_TempList(");
      pr_temp (out, tl->head, d);
      break;
    case IRP_xml:
      fprintf (out, "<TempList>\n");
      pr_temp (out, tl->head, d);
      break;
    }
  tl = tl->tail;
  if (format == IRP_parentheses)
    fprintf (out, ",\n");
  pr_templist (out, tl, d + 1);
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, ")");
      break;
    case IRP_xml:
      fprintf (out, "</TempList>\n");
      break;
    }
  return;
}

static void
pr_explist (FILE *out, T_expList el, int d)
{
  indent (out, d);
  if (!el)
    {
      if (format == IRP_parentheses)
        fprintf (out, "NULL");
      return;
    }
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, "T_ExpList(\n");
      pr_exp (out, el->head, d + 1);
      break;
    case IRP_xml:
      fprintf (out, "<ExpList>\n");
      pr_exp (out, el->head, d + 1);
      break;
    }
  el = el->tail;
  if (format == IRP_parentheses)
    fprintf (out, ",\n");
  pr_explist (out, el, d + 1);
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, ")");
      break;
    case IRP_xml:
      fprintf (out, "</ExpList>\n");
      break;
    }
  return;
}

static char exp_kind[][12]
    = { "T_BINOP", "T_MEM",  "T_TEMP",    "T_ESEQ", "T_NAME",
        "T_CONST", "T_CALL", "T_ExtCALL", "T_CAST" };

static void
pr_exp (FILE *out, T_exp exp, int d)
{
  if (!exp)
    {
      return;
    }
#ifdef __PRING_IRP_DEBUG
  fprintf (stderr, "---Entering pr_exp: %s\n", exp_kind[exp->kind]);
#endif
  switch (exp->kind)
    {
    case T_BINOP:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Binop(%s,\n", bin_oper[exp->u.BINOP.op]);
          break;
        case IRP_xml:
          fprintf (out, "<Binop><type>%s</type>\n", getT_type (exp->type));
          fprintf (out, "<op>%s</op>\n", bin_oper[exp->u.BINOP.op]);
          break;
        }
      if (format == IRP_xml)
        fprintf (out, "<left>\n");
      pr_exp (out, exp->u.BINOP.left, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, ",\n");
      else
        fprintf (out, "</left>\n<right>\n");
      pr_exp (out, exp->u.BINOP.right, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      else
        fprintf (out, "</right>\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</Binop>\n");
          break;
        }
      break;
    case T_MEM:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Mem(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Mem><type>%s</type>\n", getT_type (exp->type));
          break;
        }
      pr_exp (out, exp->u.MEM, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ", %s)", getT_type (exp->type));
          break;
        case IRP_xml:
          fprintf (out, "</Mem>\n");
          break;
        }
      break;
    case T_TEMP:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Temp(");
          break;
        case IRP_xml:
          fprintf (out, "<Temp>\n");
          break;
        }
      pr_temp (out, exp->u.TEMP, d + 1);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ")");
          break;
        case IRP_xml:
          fprintf (out, "</Temp>\n");
          break;
        }
      break;
    case T_ESEQ:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Eseq(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Eseq><type>%s</type>\n<Stm>\n",
                   getT_type (exp->type));
          break;
        }
      pr_stm (out, exp->u.ESEQ.stm, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, ",\n");
      else
        fprintf (out, "</Stm>\n<Exp>\n");
      pr_exp (out, exp->u.ESEQ.exp, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      else
        fprintf (out, "</Exp>\n");
      indent (out, d);
      if (format == IRP_parentheses)
        fprintf (out, ")");
      else
        fprintf (out, "</Eseq>\n");
      break;
    case T_NAME:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Name(Temp_namedlabel(String(\"%s\")))",
                   Temp_labelstring (exp->u.NAME));
          break;
        case IRP_xml:
          fprintf (out, "<Name>%s</Name>", Temp_labelstring (exp->u.NAME));
          break;
        }
      break;
    case T_CONST:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          if (exp->type == T_int)
            fprintf (out, "T_IntConst(%i)", exp->u.CONST.i);
          else
            fprintf (out, "T_FloatConst(%f)", exp->u.CONST.f);
          break;
        case IRP_xml:
          if (exp->type == T_int)
            fprintf (out, "<IntConst>%i</IntConst>", exp->u.CONST.i);
          else
            fprintf (out, "<FloatConst>%f</FloatConst>", exp->u.CONST.f);
          break;
        }
      break;
    case T_CALL:
      {
        T_expList args = exp->u.CALL.args;
        indent (out, d);
        switch (format)
          {
          case IRP_parentheses:
            fprintf (out, "T_Call(String(\"%s\"),\n", exp->u.CALL.id);
            break;
          case IRP_xml:
            fprintf (out, "<Call>\n<id>%s</id>\n<obj>", exp->u.CALL.id);
            break;
          }
        pr_exp (out, exp->u.CALL.obj, d + 1);
        if (format == IRP_parentheses)
          fprintf (out, ",\n");
        else
          fprintf (out, "</obj>\n<args>\n");
        pr_explist (out, args, d + 1);
        if (format == IRP_parentheses)
          fprintf (out, "\n");
        else
          fprintf (out, "</args>\n");
        indent (out, d);
        if (format == IRP_parentheses)
          fprintf (out, ", %s)", getT_type (exp->type));
        else
          fprintf (out, "<ret_type>%s</ret_type>\n</Call>\n",
                   getT_type (exp->type));
      }
      break;
    case T_ExtCALL:
      indent (out, d);
      T_expList args = exp->u.ExtCALL.args;
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_ExtCall(String(\"%s\"),\n", exp->u.ExtCALL.extfun);
          break;
        case IRP_xml:
          fprintf (out, "<ExtCall><id>%s</id><args>\n", exp->u.ExtCALL.extfun);
          break;
        }
      pr_explist (out, args, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      else
        fprintf (out, "</args>");
      indent (out, d);
      if (format == IRP_parentheses)
        fprintf (out, ",%s)", getT_type (exp->type));
      else
        fprintf (out, "<ret_type>%s</ret_type></ExtCall>\n",
                 getT_type (exp->type));
      break;
    case T_CAST:
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_Cast(\n");
          break;
        case IRP_xml:
          fprintf (out, "<Cast>");
          break;
        }
      pr_exp (out, exp->u.CAST, d + 1);
      if (format == IRP_parentheses)
        fprintf (out, "\n");
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, ",%s)", getT_type (exp->type));
          break;
        case IRP_xml:
          fprintf (out, "<type>%s</type>", getT_type (exp->type));
          fprintf (out, "</Cast>\n");
          break;
        }
      break;
    } /* end of switch */
  if (format == IRP_parentheses)
    fprintf (out, "/*%s*/", getT_type (exp->type));
#ifdef __PRING_IRP_DEBUG
  fprintf (stderr, "---Exiting pr_exp: %s\n", exp_kind[exp->kind]);
#endif
  return;
}

void
printIRP_StmList (FILE *out, T_stmList stmList, int d)
{
  T_stmList l = stmList;
  if (l)
    {
      indent (out, d);
      switch (format)
        {
        case IRP_parentheses:
          fprintf (out, "T_StmList(\n");
          break;
        case IRP_xml:
          fprintf (out, "<StmList>\n");
          break;
        }
      pr_stm (out, l->head, d);
      l = l->tail;
      if (l)
        {
          if (format == IRP_parentheses)
            fprintf (out, ",\n");
          printIRP_StmList (out, l, d);
          if (format == IRP_parentheses)
            fprintf (out, "\n");
          indent (out, d);
          switch (format)
            {
            case IRP_parentheses:
              fprintf (out, ")");
              break;
            case IRP_xml:
              fprintf (out, "</StmList>\n");
              break;
            }
        }
      else
        {
          indent (out, d);
          switch (format)
            {
            case IRP_parentheses:
              fprintf (out, ", NULL)");
              break;
            case IRP_xml:
              fprintf (out, "</StmList>\n");
              break;
            }
        }
    }
  else
    {
      if (format == IRP_parentheses)
        {
          indent (out, d);
          fprintf (out, "NULL");
        }
    }
  return;
}

void
printIRP_FuncDecl (FILE *out, T_funcDecl funcDecl)
{
  Temp_tempList t = funcDecl->args;
  T_stm s = funcDecl->stm;

  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, "T_FuncDecl(String(\"%s\"),\n", funcDecl->name);
      break;
    case IRP_xml:
      fprintf (out, "<FuncDecl>\n<name>%s</name>\n", funcDecl->name);
      break;
    }

  pr_templist (out, t, 0);

  switch (format)
    {
    case IRP_parentheses:
      if (s)
        {
          fprintf (out, ",\n");
          pr_stm (out, s, 0);
          fprintf (out, "\n)");
        }
      else
        {
          fprintf (out, ",\n");
          indent (out, 0);
          fprintf (out, "NULL\n)");
        }
      break;
    case IRP_xml:
      pr_stm (out, s, 0);
      fprintf (out, "</FuncDecl>\n");
      break;
    }

  return;
}

void
printIRP_FuncDeclList (FILE *out, T_funcDeclList funcDeclList)
{
  T_funcDeclList l = funcDeclList;
  if (!l)
    {
      if (format == IRP_parentheses)
        fprintf (out, "NULL");
      return;
    }
  switch (format)
    {
    case IRP_parentheses:
      fprintf (out, "T_FuncDeclList(\n");
      break;
    case IRP_xml:
      fprintf (out, "<FuncDeclList>\n");
      break;
    }

  printIRP_FuncDecl (out, l->head);
  l = l->tail;

  switch (format)
    {
    case IRP_parentheses:
      if (l)
        {
          fprintf (out, ",\n");
          printIRP_FuncDeclList (out, l);
          fprintf (out, "\n");
          fprintf (out, ")");
        }
      else
        {
          fprintf (out, ", NULL)");
        }
      break;
    case IRP_xml:
      printIRP_FuncDeclList (out, l);
      fprintf (out, "</FuncDeclList>\n");
      break;
    }
  return;
}
