/*
 * printtree.c - functions to print out intermediate representation (IR) trees.
 *
 */
#include "print_stm.h"
#include "symbol.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"
#include <stdio.h>

/* local function prototype */
static void pr_tree_exp (FILE *out, T_exp exp, int d);

static void
indent (FILE *out, int d)
{
  int i;
  for (i = 0; i <= d; i++)
    fprintf (out, " ");
}

static char bin_oper[][12]
    = { "T_plus", "T_minus", "T_mul", "T_div", "T_and", "T_or" };

static char rel_oper[][12]
    = { "T_eq", "T_ne", "T_lt", "T_gt", "T_le", "T_ge" };

static void
pr_stm (FILE *out, T_stm stm, int d)
{
  switch (stm->kind)
    {
    case T_SEQ:
      indent (out, d);
      pr_stm (out, stm->u.SEQ.left, d + 1);
      fprintf (out, ",\n");
      pr_stm (out, stm->u.SEQ.right, d + 1);
      fprintf (out, ")");
      break;
    case T_LABEL:
      indent (out, d - 2);
      fprintf (out, "%s:", S_name (stm->u.LABEL));
      break;
    case T_JUMP:
      indent (out, d);
      fprintf (out, "Jump ");
      fprintf (out, "%s", S_name (stm->u.JUMP.jump));
      break;
    case T_CJUMP:
      indent (out, d);
      fprintf (out, "Cjump %s", rel_oper[stm->u.CJUMP.op]);
      fprintf (out, ", ");
      pr_tree_exp (out, stm->u.CJUMP.left, d + 1);
      fprintf (out, ", ");
      pr_tree_exp (out, stm->u.CJUMP.right, d + 1);
      fprintf (out, ", ");
      fprintf (out, "%s, ", S_name (stm->u.CJUMP.t));
      fprintf (out, "%s", S_name (stm->u.CJUMP.f));
      break;
    case T_MOVE:
      indent (out, d);
      fprintf (out, "Move ");
      pr_tree_exp (out, stm->u.MOVE.dst, d + 1);
      fprintf (out, ", ");
      pr_tree_exp (out, stm->u.MOVE.src, d + 1);
      break;
    case T_EXP:
      indent (out, d);
      pr_tree_exp (out, stm->u.EXP, d);
      break;
    case T_RETURN:
      indent (out, d);
      fprintf (out, "Return ");
      pr_tree_exp (out, stm->u.EXP, d + 1);
      break;
    }
}

static void
pr_templist (FILE *out, Temp_tempList tl, int d)
{
  indent (out, d);
  if (!tl)
    {
      fprintf (out, "NULL");
      return;
    }
  fprintf (out, "t%s", Temp_look (Temp_name (), tl->head));
  tl = tl->tail;
  if (tl)
    fprintf (out, ", ");
  else
    fprintf (out, ", ");
  pr_templist (out, tl, d);
  return;
}

static void
pr_explist (FILE *out, T_expList el, int d)
{
  if (!el)
    {
      //  fprintf(out, "NULL");
      return;
    }
  // indent(out, d);
  pr_tree_exp (out, el->head, -1);
  el = el->tail;
  if (el)
    fprintf (out, ", ");
  pr_explist (out, el, d);
  return;
}

static void
pr_tree_exp (FILE *out, T_exp exp, int d)
{
  switch (exp->kind)
    {
    case T_BINOP:
      fprintf (out, "Binop(%s, ", bin_oper[exp->u.BINOP.op]);
      pr_tree_exp (out, exp->u.BINOP.left, 0);
      fprintf (out, ", ");
      pr_tree_exp (out, exp->u.BINOP.right, 0);
      fprintf (out, ")");
      break;
    case T_MEM:
      fprintf (out, "Mem(");
      pr_tree_exp (out, exp->u.MEM, 0);
      fprintf (out, ")");
      break;
    case T_TEMP:
      fprintf (out, "t%s", Temp_look (Temp_name (), exp->u.TEMP));
      break;
    case T_ESEQ:
      fprintf (out, "T_Eseq(\n");
      pr_stm (out, exp->u.ESEQ.stm, d + 1);
      fprintf (out, ",\n");
      pr_tree_exp (out, exp->u.ESEQ.exp, d + 1);
      fprintf (out, ")");
      break;
    case T_NAME:
      fprintf (out, "%s", S_name (exp->u.NAME));
      break;
    case T_CONST:
      if (exp->type == T_int)
        fprintf (out, "%i", exp->u.CONST.i);
      else
        fprintf (out, "%f", exp->u.CONST.f);
      break;
    case T_CALL:
      {
        pr_tree_exp (out, exp->u.CALL.obj, 0);
        fprintf (out, ".%s ", exp->u.CALL.id);
        T_expList args = exp->u.CALL.args;
        if (!args)
          break;
        pr_explist (out, args, 0);
      }
      break;
    case T_ExtCALL:
      {
        T_expList args = exp->u.ExtCALL.args;
        fprintf (out, "%s ", exp->u.ExtCALL.extfun);
        if (args)
          {
            pr_explist (out, args, 0);
          }
      }
      break;
    case T_CAST:
      indent (out, d);
      fprintf (out, "Cast(");
      pr_tree_exp (out, exp->u.CAST, d + 1);
      fprintf (out, ")");
      break;
    } /* end of switch */
  fprintf (out, ":");
  if (exp->type == T_int)
    fprintf (out, "int");
  else
    fprintf (out, "float");
}

void
printStm_StmList (FILE *out, T_stmList stmList, int d)
{
  T_stmList l = stmList;
  if (l)
    {
      indent (out, d);
      pr_stm (out, l->head, d);
      fprintf (out, "\n");
      l = l->tail;
      printStm_StmList (out, l, d);
    }
  return;
}
