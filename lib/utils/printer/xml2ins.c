#include "xml2ins.h"
#include "llvmir.h"
#include "lxml.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <strings.h>

#define __DEBUG
#undef __DEBUG

static string
getT_type (T_type type)
{
  return type == T_int ? "T_int" : "T_float";
}

static XMLNode *
xmlgetchildnode (XMLNode *node, string tag)
{
  if (!node || node->children.size == 0)
    return NULL;
#ifdef __DEBUG
  fprintf (stderr, "looking for: xmlgetchildnode: %s\n", tag);
#endif
  XMLNodeList *list = XMLNode_children (node, tag);
  if (!list || list->size == 0)
    return NULL; // empty list (list not holding any data)
#ifdef __DEBUG
  fprintf (stderr, "Got it: xmlgetchildnode: %s, innert:%s\n",
           list->data[0]->tag, list->data[0]->inner_text);
#endif
  return XMLNodeList_at (list, 0);
}

static T_type
xmlinstype (XMLNode *node)
{
  string txt = node->inner_text;
  if (!strcmp (txt, "T_int"))
    return T_int;
  else if (!strcmp (txt, "T_float"))
    return T_float;
  else
    assert (0);
}

static Temp_temp
xmlinstemp (XMLNode *t)
{
  assert (t || !strcmp (t->tag, "temp"));
#ifdef __DEBUG
  fprintf (stderr, "---tmp---xmlinstemp: %s\n", t->tag);
#endif
  XMLNode *name = xmlgetchildnode (t, "name");
  assert (name || !strcmp (name->tag, "name"));
  XMLNode *type = xmlgetchildnode (t, "type");
  assert (type || !strcmp (type->tag, "type"));
  return Temp_namedtemp (atoi (name->inner_text), xmlinstype (type));
}

static Temp_tempList
xmlinstemplist (XMLNode *l)
{
  XMLNode *t;
  if (!l || l->children.size == 0)
    return NULL;
#ifdef __DEBUG
  fprintf (stderr, "---tmp---xmlinstemplist: %s, size=%d\n", l->tag,
           l->children.size);
#endif
  assert (l || !strcmp (l->tag, "templist"));
  if (l->children.size == 0)
    return NULL;
  if (l->children.size == 1)
    {
      t = xmlgetchildnode (l, "temp");
      assert (t || !strcmp (t->tag, "temp"));
      return Temp_TempList (xmlinstemp (t), NULL);
    }
  if (l->children.size == 2)
    {
      t = xmlgetchildnode (l, "temp");
      assert (t || !strcmp (t->tag, "temp"));
      XMLNode *l1 = xmlgetchildnode (l, "templist");
      assert (l1 || !strcmp (l1->tag, "templist"));
      return Temp_TempList (xmlinstemp (t), xmlinstemplist (l1));
    }
  fprintf (stderr, "Invalid TempList Element.\n");
  exit - 2;
}

static Temp_labelList
xmlinslabellist (XMLNode *jumps)
{
#ifdef __DEBUG
  fprintf (stderr, "---tmp---xmlinslabellist: %s, size=%d\n", jumps->tag,
           jumps->children.size);
#endif
  assert (jumps || !strcmp (jumps->tag, "labelist"));
  if (jumps->children.size == 0)
    return NULL;
  if (jumps->children.size == 1)
    return Temp_LabelList (
        Temp_namedlabel (jumps->children.data[0]->inner_text), NULL);
  if (jumps->children.size == 2)
    {
      XMLNode *l1 = xmlgetchildnode (jumps, "labellist");
      return Temp_LabelList (
          Temp_namedlabel (jumps->children.data[0]->inner_text),
          xmlinslabellist (l1));
    }
  fprintf (stderr, "Invalid LabelList Element.\n");
  exit - 2;
}

static LL_instr
xmlgetins (XMLNode *ins)
{
  Temp_tempList dsttemplist = NULL, srctemplist = NULL;
  LL_targets LL_jumps;
  assert (ins);
  string tag = ins->tag;
  if (!strcmp (tag, "oper"))
    {
      XMLNode *assem = xmlgetchildnode (ins, "assem");
      assert (assem);
#ifdef __DEBUG
      fprintf (stderr, "--ins OPER: %s\n", assem->inner_text);
#endif
      XMLNode *dst = xmlgetchildnode (ins, "dst");
      if (dst)
        dsttemplist = xmlinstemplist (dst->children.data[0]);
      XMLNode *src = xmlgetchildnode (ins, "src");
      if (src)
        srctemplist = xmlinstemplist (src->children.data[0]);
      XMLNode *jumps = xmlgetchildnode (ins, "jumps");
      if (jumps)
        LL_jumps = LL_Targets (xmlinslabellist (jumps->children.data[0]));
      else
        LL_jumps = LL_Targets (NULL);
      return LL_Oper (String (assem->inner_text), dsttemplist, srctemplist,
                      LL_jumps);
    }
  else if (!strcmp (tag, "label"))
    {
      XMLNode *assem = xmlgetchildnode (ins, "assem");
      assert (assem->inner_text);
#ifdef __DEBUG
      fprintf (stderr, "--ins LABEL: %s\n", assem->inner_text);
#endif
      XMLNode *temp_label = xmlgetchildnode (ins, "temp_label");
      return LL_Label (String (assem->inner_text),
                       Temp_namedlabel (temp_label->inner_text));
    }
  else if (!strcmp (tag, "move"))
    {
      XMLNode *assem = xmlgetchildnode (ins, "assem");
      assert (assem->inner_text);
#ifdef __DEBUG
      fprintf (stderr, "--ins MOVE: %s\n", assem->inner_text);
#endif
      XMLNode *dst = xmlgetchildnode (ins, "dst");
      if (dst)
        dsttemplist = xmlinstemplist (dst->children.data[0]);
      XMLNode *src = xmlgetchildnode (ins, "src");
      if (src)
        srctemplist = xmlinstemplist (src->children.data[0]);
      return LL_Move (String (assem->inner_text), dsttemplist, srctemplist);
    }
  else
    {
      fprintf (stderr, "Invalid tag=%s\n", tag);
      exit (-1);
    }
}

LL_instrList
insxml2func (XMLNode *l)
{
  LL_instrList aslist = NULL;
  LL_instr ins;

  assert (l && !strcmp (l->tag, "function"));
#ifdef __DEBUG
  fprintf (stderr, "---Entering function : %s=%s\n", l->tag,
           l->attributes.data->value);
#endif
  if (l->children.size == 0)
    {
      return NULL;
    }
  for (int i = 0; i < l->children.size; i++)
    {
      ins = xmlgetins (l->children.data[i]);
      aslist = LL_splice (aslist, LL_InstrList (ins, NULL));
    }
#ifdef __DEBUG
  fprintf (stderr, "---Ending function : %s=%s\n", l->tag,
           l->attributes.data->value);
#endif
  return aslist;
}
