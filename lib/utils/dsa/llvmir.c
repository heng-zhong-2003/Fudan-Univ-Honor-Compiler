/*
 * mipscodegen.c - Functions to translate to Assem-instructions for
 *       the Jouette assembly language using Maximal Munch.
 */

#include "llvmir.h"
#include "dbg.h"
#include "symbol.h"

LL_targets
LL_Targets (Temp_labelList labels)
{
  LL_targets p = checked_malloc (sizeof *p);
  p->labels = labels;
  return p;
}

LL_instr
LL_Oper (string a, Temp_tempList d, Temp_tempList s, LL_targets j)
{
  LL_instr p = (LL_instr)checked_malloc (sizeof *p);
  p->kind = LL_OPER;
  p->u.OPER.assem = a;
  p->u.OPER.dst = d;
  p->u.OPER.src = s;
  p->u.OPER.jumps = j;
  return p;
}

LL_instr
LL_Label (string a, Temp_label label)
{
  LL_instr p = (LL_instr)checked_malloc (sizeof *p);
  p->kind = LL_LABEL;
  p->u.LABEL.assem = a;
  p->u.LABEL.label = label;
  return p;
}

LL_instr
LL_Move (string a, Temp_tempList d, Temp_tempList s)
{
  LL_instr p = (LL_instr)checked_malloc (sizeof *p);
  p->kind = LL_MOVE;
  p->u.MOVE.assem = a;
  p->u.MOVE.dst = d;
  p->u.MOVE.src = s;
  return p;
}

LL_instrList
LL_InstrList (LL_instr head, LL_instrList tail)
{
  LL_instrList p = (LL_instrList)checked_malloc (sizeof *p);
  p->head = head;
  p->tail = tail;
  return p;
}

/* put list b at the end of list a */
LL_instrList
LL_splice (LL_instrList a, LL_instrList b)
{
  LL_instrList p;
  if (a == NULL)
    return b;
  for (p = a; p->tail != NULL; p = p->tail)
    ;
  p->tail = b;
  return a;
}

static Temp_temp
nthTemp (Temp_tempList list, int i)
{
  assert (list);
  if (i == 0)
    return list->head;
  else
    return nthTemp (list->tail, i - 1);
}

static Temp_label
nthLabel (Temp_labelList list, int i)
{
  assert (list);
  if (i == 0)
    return list->head;
  else
    return nthLabel (list->tail, i - 1);
}

/* first param is string created by this function by reading 'assem' string
 * and replacing `d `s and `j stuff.
 * Last param is function to use to determine what to do with each temp.
 */
static void
format (char *result, string assem, Temp_tempList dst, Temp_tempList src,
        LL_targets jumps, Temp_map m)
{
  char *p;
  int i = 0; /* offset to result string */
  for (p = assem; p && *p != '\0'; p++)
    if (*p == '`')
      switch (*(++p))
        {
        case 's':
          {
            int n = atoi (++p);
            string s = Temp_look (m, nthTemp (src, n));
            strcpy (result + i, "r");
            strcpy (result + i + 1, s);
            i += (strlen (s) + 1);
          }
          break;
        case 'd':
          {
            int n = atoi (++p);
            string s = Temp_look (m, nthTemp (dst, n));
            strcpy (result + i, "r");
            strcpy (result + i + 1, s);
            i += (strlen (s) + 1);
          }
          break;
        case 'j':
          assert (jumps);
          {
            int n = atoi (++p);
            // DBGPRT ("jump label: %s\n", S_name (jumps->labels->head));
            string s = Temp_labelstring (nthLabel (jumps->labels, n));
            strcpy (result + i, s);
            i += strlen (s);
          }
          break;
        case '`':
          result[i] = '`';
          i++;
          break;
        default:
          assert (0);
        }
    else
      {
        result[i] = *p;
        i++;
      }
  result[i] = '\0';
}

void
LL_format (char *result, string assem, Temp_tempList dst, Temp_tempList src,
           LL_targets jumps, Temp_map m)
{
  format (result, assem, dst, src, jumps, m);

  strcpy (result + strlen (result), "; d: ");
  Temp_tempList l = dst;
  while (l)
    {
      strcpy (result + strlen (result), "r");
      strcpy (result + strlen (result), Temp_look (m, l->head));
      l = l->tail;
      if (l)
        strcpy (result + strlen (result), ", ");
      else
        strcpy (result + strlen (result), "; ");
    }

  strcpy (result + strlen (result), "s: ");
  l = src;
  while (l)
    {
      strcpy (result + strlen (result), "r");
      strcpy (result + strlen (result), Temp_look (m, l->head));
      l = l->tail;
      if (l)
        strcpy (result + strlen (result), ", ");
    }
}

void
LL_print (FILE *out, LL_instr i, Temp_map m)
{
  char r[200]; /* result */
  switch (i->kind)
    {
    case LL_OPER:
      format (r, i->u.OPER.assem, i->u.OPER.dst, i->u.OPER.src,
              i->u.OPER.jumps, m);
      fprintf (out, "%s", r);
      break;
    case LL_LABEL:
      format (r, i->u.LABEL.assem, NULL, NULL, NULL, m);
      fprintf (out, "%s", r);
      /* i->u.LABEL->label); */
      break;
    case LL_MOVE:
      format (r, i->u.MOVE.assem, i->u.MOVE.dst, i->u.MOVE.src, NULL, m);
      fprintf (out, "%s", r);
      break;
    }
  fprintf (out, "\n");
}

/* c should be COL_color; temporarily it is not */
void
LL_printInstrList (FILE *out, LL_instrList iList, Temp_map m)
{
  for (; iList; iList = iList->tail)
    {
      // if (iList->head->kind == I_LABEL)
      // DBGPRT ("%s, %s\n", __func__, iList->head->u.LABEL.assem);
      LL_print (out, iList->head, m);
    }
  fprintf (out, "\n");
}

LL_proc
LL_Proc (string p, LL_instrList b, string e)
{
  LL_proc proc = checked_malloc (sizeof (*proc));
  proc->prolog = p;
  proc->body = b;
  proc->epilog = e;
  return proc;
}
