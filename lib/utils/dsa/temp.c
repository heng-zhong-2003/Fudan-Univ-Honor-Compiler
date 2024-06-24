/*
 * temp.c - functions to create and manipulate temporary variables which are
 *      used in the IR tree representation before it has been determined
 *      which variables are to go into registers.
 *
 */

#include "temp.h"
#include "symbol.h"
#include "table.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static S_table temp_table = NULL, reg_table = NULL;
static int temps = 100;
static int labels = 0;
static FILE *outfile;

Temp_temp
Temp_reg (int name, T_type type)
{
  if (!reg_table)
    reg_table = S_empty ();
  char r[20];
  if (type == T_int)
    {
      sprintf (r, "r%d", name);
    }
  else
    {
      sprintf (r, "s%d", name);
    }
  Temp_temp p = S_look (reg_table, S_Symbol (String (r)));
  if (p)
    {
      p->type = type; // reuse
      return p;
    }
  p = (Temp_temp)checked_malloc (sizeof (*p));
  p->num = name;
  p->type = type;
  Temp_enter (Temp_name (), p, String (r));
  S_enter (reg_table, S_Symbol (String (r)), p);
  return p;
}

Temp_temp
Temp_newtemp (T_type type)
{
  if (!temp_table)
    temp_table = S_empty ();
  char r[16];
  sprintf (r, "%d", temps);
  Temp_temp p = S_look (temp_table, S_Symbol (String (r)));
  if (p)
    {
      p->type = type; // reuse
      temps++;
      return p;
    }
  p = (Temp_temp)checked_malloc (sizeof (*p));
  p->num = temps++;
  p->type = type;
  Temp_enter (Temp_name (), p, String (r));
  S_enter (temp_table, S_Symbol (String (r)), p);
  return p;
}

void
Temp_resettemp ()
{
  temps = 100;
}

void
Temp_resetlabel ()
{
  labels = 0;
}

Temp_temp
Temp_namedtemp (int name, T_type type)
{
  if (!temp_table)
    temp_table = S_empty ();
  char r[16];
  sprintf (r, "%d", name);
  Temp_temp p = S_look (temp_table, S_Symbol (String (r)));
  if (p)
    {
      p->type = type; // reuse
      return p;
    }
  p = (Temp_temp)checked_malloc (sizeof (*p));
  p->num = name;
  p->type = type;
  if (name >= temps)
    temps = name + 1;
  Temp_enter (Temp_name (), p, String (r));
  S_enter (temp_table, S_Symbol (String (r)), p);
  return p;
}

Temp_temp this () { return Temp_namedtemp (99, T_int); }

Temp_tempList
Temp_TempList (Temp_temp h, Temp_tempList t)
{
  Temp_tempList p = (Temp_tempList)checked_malloc (sizeof (*p));
  p->head = h;
  p->tail = t;
  return p;
}

Temp_tempList
Temp_TempListSplice (Temp_tempList a, Temp_tempList b)
{
  Temp_tempList p;
  if (a == NULL)
    return b;
  if (b == NULL)
    return a;
  for (p = a; p->tail != NULL; p = p->tail)
    ;
  p->tail = b;
  return a;
}

// add one to another list (if not already there)
static Temp_tempList
Temp_TempListadd (Temp_tempList tl, Temp_temp t)
{
  for (Temp_tempList l = tl; l != NULL; l = l->tail)
    {
      if (l->head == t)
        return tl; // nothing to add
    }
  return Temp_TempList (t, tl); // else add to the head
}

// do a simple intersect (add each one at a time)
Temp_tempList
Temp_TempListInter (Temp_tempList a, Temp_tempList b)
{
  bool found;
  Temp_tempList scan = a;
  Temp_tempList result = NULL;
  while (scan)
    {
      found = FALSE;
      for (Temp_tempList l = b; l != NULL; l = l->tail)
        {
          if (scan->head == l->head)
            {
              found = TRUE;
              break;
            }
        }
      if (found)
        result = Temp_TempList (scan->head, result);
      scan = scan->tail;
    }
  return result;
}

// do a simple union (add each one at a time)
Temp_tempList
Temp_TempListUnion (Temp_tempList a, Temp_tempList b)
{
  for (; b != NULL; b = b->tail)
    a = Temp_TempListadd (a, b->head);
  return a;
}

// Implement a list difference a-b (for each on in a, scan b)
Temp_tempList
Temp_TempListDiff (Temp_tempList a, Temp_tempList b)
{
  bool found;
  Temp_tempList scan = a;
  Temp_tempList result = NULL;
  while (scan)
    {
      found = FALSE;
      for (Temp_tempList l = b; l != NULL; l = l->tail)
        {
          if (scan->head == l->head)
            {
              found = TRUE;
              break;
            }
        }
      if (!found)
        result = Temp_TempList (scan->head, result);
      scan = scan->tail;
    }
  return result;
}

// a simple eq test using diff twice
bool
Temp_TempListEq (Temp_tempList a, Temp_tempList b)
{
  if (Temp_TempListDiff (a, b) != NULL)
    return FALSE;
  else
    {
      if (Temp_TempListDiff (b, a) != NULL)
        return FALSE;
      else
        return TRUE;
    }
}

bool
Temp_TempInTempList (Temp_temp l, Temp_tempList ll)
{
  for (Temp_tempList a = ll; a; a = a->tail)
    {
      if (l == a->head)
        return TRUE;
    }
  return FALSE;
}

Temp_label
Temp_newlabel_prefix (char c)
{
  char buf[100];
  sprintf (buf, "%c%d", c, labels++);
  return Temp_namedlabel (String (buf));
}

Temp_label
Temp_newlabel (void)
{
  char buf[100];
  sprintf (buf, "L%d", labels++);
  return Temp_namedlabel (String (buf));
}

/* The label will be created only if it is not found. */
Temp_label
Temp_namedlabel (string s)
{
  return S_Symbol (s);
}

string
Temp_labelstring (Temp_label s)
{
  return S_name (s);
}

Temp_labelList
Temp_LabelList (Temp_label h, Temp_labelList t)
{
  Temp_labelList p = (Temp_labelList)checked_malloc (sizeof (*p));
  p->head = h;
  p->tail = t;
  return p;
}

Temp_labelList
Temp_LabelListSplice (Temp_labelList a, Temp_labelList b)
{
  Temp_labelList p;
  if (a == NULL)
    return b;
  if (b == NULL)
    return a;
  for (p = a; p->tail != NULL; p = p->tail)
    ;
  p->tail = b;
  return a;
}

// add one to another list (if not already there)
static Temp_labelList
Temp_LabelListadd (Temp_labelList tl, Temp_label t)
{
  for (Temp_labelList l = tl; l != NULL; l = l->tail)
    {
      if (l->head == t)
        return tl; // nothing to add
    }
  return Temp_LabelList (t, tl); // else add to the head
}

// do a simple intersect (add each one at a time)
Temp_labelList
Temp_LabelListInter (Temp_labelList a, Temp_labelList b)
{
  bool found;
  Temp_labelList scan = a;
  Temp_labelList result = NULL;
  while (scan)
    {
      found = FALSE;
      for (Temp_labelList l = b; l != NULL; l = l->tail)
        {
          if (scan->head == l->head)
            {
              found = TRUE;
              break;
            }
        }
      if (found)
        result = Temp_LabelList (scan->head, result);
      scan = scan->tail;
    }
  return result;
}

// do a simple union (add each one at a time)
Temp_labelList
Temp_LabelListUnion (Temp_labelList a, Temp_labelList b)
{
  for (; b != NULL; b = b->tail)
    a = Temp_LabelListadd (a, b->head);
  return a;
}

// Implement a list difference a-b (for each on in a, scan b)
Temp_labelList
Temp_LabelListDiff (Temp_labelList a, Temp_labelList b)
{
  bool found;
  Temp_labelList scan = a;
  Temp_labelList result = NULL;
  while (scan)
    {
      found = FALSE;
      for (Temp_labelList l = b; l != NULL; l = l->tail)
        {
          if (scan->head == l->head)
            {
              found = TRUE;
              break;
            }
        }
      if (!found)
        result = Temp_LabelList (scan->head, result);
      scan = scan->tail;
    }
  return result;
}

// a simple eq test using diff twice
bool
Temp_LabelListEq (Temp_labelList a, Temp_labelList b)
{
  if (Temp_LabelListDiff (a, b) != NULL)
    return FALSE;
  else
    {
      if (Temp_LabelListDiff (b, a) != NULL)
        return FALSE;
      else
        return TRUE;
    }
}

bool
Temp_LabelInLabelList (Temp_label l, Temp_labelList ll)
{
  for (Temp_labelList a = ll; a; a = a->tail)
    {
      if (l == a->head)
        return TRUE;
    }
  return FALSE;
}

Temp_map
newMap (TAB_table tab, Temp_map under)
{
  Temp_map m = checked_malloc (sizeof (*m));
  m->tab = tab;
  m->under = under;
  return m;
}

Temp_map
Temp_empty (void)
{
  return newMap (TAB_empty (), NULL);
}

Temp_map
Temp_layerMap (Temp_map over, Temp_map under)
{
  if (over == NULL)
    return under;
  else
    return newMap (over->tab, Temp_layerMap (over->under, under));
}

void
Temp_enter (Temp_map m, Temp_temp t, string s)
{
  assert (m && m->tab);
  TAB_enter (m->tab, t, s);
}

string
Temp_look (Temp_map m, Temp_temp t)
{
  string s;
  assert (m && m->tab);
  s = TAB_look (m->tab, t);
  if (s)
    return s;
  else if (m->under)
    return Temp_look (m->under, t);
  else
    return NULL;
}

void
showit (Temp_temp t, string r)
{
  string s;
  if (t->type == T_int)
    s = String ("int");
  else if (t->type == T_float)
    s = String ("float");
  else
    assert (0);
  fprintf (outfile, "t%d, type=%s\n", t->num, s);
}

void
Temp_dumpMap (FILE *out, Temp_map m)
{
  outfile = out;
  TAB_dump (m->tab, (void (*) (void *, void *))showit);
  if (m->under)
    {
      fprintf (out, "---------\n");
      Temp_dumpMap (out, m->under);
    }
}

Temp_map
Temp_name (void)
{
  static Temp_map m = NULL;
  if (!m)
    m = Temp_empty ();
  return m;
}

/* a small test
int main() {
  Temp_temp t1 = Temp_newtemp(T_int);
  Temp_temp t2 = Temp_newtemp(T_float);
  Temp_temp t3 = this();
  Temp_temp t4 = Temp_namedtemp(80, T_float);
  Temp_temp t5 = Temp_namedtemp(80, T_int);
  Temp_dumpMap(stdout, Temp_name());
}
*/
