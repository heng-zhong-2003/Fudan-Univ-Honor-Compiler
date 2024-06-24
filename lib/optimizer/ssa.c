#include "ssa.h"
#include "assem.h"
#include "dbg.h"
#include "graph.h"
#include "llvmir.h"
#include "llvmirbg.h"
#include "set.h"
#include "stack.h"
#include "temp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __DEBUG
#undef __DEBUG

#define INSTRLEN 1024
#define VITABSZ 128

typedef struct SSA_varInfo_ *SSA_varInfo_t;
struct SSA_varInfo_
{
  SET_set_t defsites;
  STK_stack_t stack;
  Temp_temp tmp;
};

/* Hash table using integer as key, not pointers */
typedef struct SSA_tabEnt_ *SSA_tabEnt_t;
struct SSA_tabEnt_
{
  int key;
  SSA_varInfo_t value;
  struct SSA_tabEnt_ *next;
};

typedef struct SSA_intVarInfoTab_ *SSA_intVarInfoTab_t;
struct SSA_intVarInfoTab_
{
  int size;
  SSA_tabEnt_t *table;
};

typedef struct SSA_bgNodeInfo_ *SSA_bgNodeInfo_t;
struct SSA_bgNodeInfo_
{
  Temp_tempList in; /* in-nodes */
  Temp_tempList Aorig;
  SSA_intVarInfoTab_t Aphi;
  G_node bgNode; /* the node itself */
  SET_set_t domSet;
  G_node idom;
  G_nodeList children;
  SET_set_t df;
};

static int numGNodes = 0;
static SSA_bgNodeInfo_t *bgNodeInfos = NULL;
static SSA_intVarInfoTab_t varInfoTab = NULL;
static void initGlobs (G_nodeList bg, LL_instrList bodyil, G_nodeList lg);

SSA_varInfo_t SSA_constrVarInfo (int numNodes, Temp_temp t);
SSA_intVarInfoTab_t SSA_constrEmptyTab ();
SSA_varInfo_t SSA_tabLook (SSA_intVarInfoTab_t ht, int key);
void SSA_tabEnter (SSA_intVarInfoTab_t tab, int key, SSA_varInfo_t value);
SSA_bgNodeInfo_t SSA_constrBgNodeInfo (G_node node, int numNodes,
                                       LL_instrList bodyil, G_nodeList lg);
static bool isInTempList (Temp_tempList l, int key);

void SSA_computeDomSet ();
void SSA_computeIdom ();
void SSA_computeDF (G_node n);
string SSA_formatPhi (const char *fmt, ...);
LL_instr SSA_constrPhi (SSA_varInfo_t var, int node);
void SSA_insertPhi ();
bool SSA_isPhi (LL_instr instr);
void SSA_rename (int n);

LL_instrList
LL_instrList_to_SSA (LL_instrList bodyil, G_nodeList lg, G_nodeList bg)
{
  // DBGPRT ("basic block:\n");
  // LL_Show_bg (stderr, bg);
  initGlobs (bg, bodyil, lg);
  SSA_computeDomSet ();
  // DBGPRT ("SSA_computeDomSet finish\n");
  SSA_computeIdom ();
  // DBGPRT ("SSA_computeIdom finish\n");
  SSA_computeDF (bg->head);
  // DBGPRT ("SSA_computeDF finish\n");
  // #ifdef DEBUG
  //   DBGPRT ("dominance frontier:\n");
  //   for (int i = 1; i < numGNodes; ++i)
  //     {
  //       DBGPRT ("\tnode %d: ", i);
  //       DBGPRT ("df = { ");
  //       for (int j = 0; j < numGNodes; ++j)
  //         DBGPRT (" %d", bgNodeInfos[i]->df[j]);
  //       DBGPRT (" }\n");
  //     }
  // #endif
  // LL_printInstrList (stderr, bodyil, Temp_name ());
  SSA_insertPhi ();
  // DBGPRT ("SSA_insertPhi finish\n");
  SSA_rename (0);
  // DBGPRT ("SSA_rename finish\n");
  LL_instrList new_bodyil = NULL, new_bodyil_last = NULL;
  for (G_nodeList _bg = bg; _bg; _bg = _bg->tail)
    {
      LL_block block = (LL_block)G_nodeInfo (_bg->head);
      for (LL_instrList il = block->instrs; il; il = il->tail)
        {
          // DBGPRT ("ins kind %d, ", il->head->kind);
          // LL_print (stderr, il->head, Temp_name ());
          if (new_bodyil == NULL)
            new_bodyil = new_bodyil_last = LL_InstrList (il->head, NULL);
          else
            {
              new_bodyil_last->tail = LL_InstrList (il->head, NULL);
              new_bodyil_last = new_bodyil_last->tail;
            }
        }
    }
  return new_bodyil;
}

static void
initGlobs (G_nodeList bg, LL_instrList bodyil, G_nodeList lg)
{
  numGNodes = bg->head->mygraph->nodecount;
  bgNodeInfos = checked_malloc (numGNodes * sizeof (*bgNodeInfos));
  for (G_nodeList _bg = bg; _bg; _bg = _bg->tail)
    {
      G_node n = _bg->head;
      bgNodeInfos[n->mykey] = SSA_constrBgNodeInfo (n, numGNodes, bodyil, lg);
    }
  varInfoTab = SSA_constrEmptyTab ();
}

void
SSA_computeDomSet ()
{
  for (int i = 1; i < numGNodes; ++i)
    memset (bgNodeInfos[i]->domSet, 1, numGNodes * sizeof (bool));
  bgNodeInfos[0]->domSet[0] = 1;
  for (;;)
    {
      bool diff = 0;
      for (int i = 1; i < numGNodes; ++i)
        {
          G_nodeList predIter = G_pred (bgNodeInfos[i]->bgNode);
          SET_set_t domSetNxt = SET_constrEmpty (numGNodes);
          memset (domSetNxt, 1, numGNodes);
          for (; predIter != NULL; predIter = predIter->tail)
            {
              G_node p = predIter->head;
              domSetNxt = SET_intersect (
                  domSetNxt, bgNodeInfos[p->mykey]->domSet, numGNodes);
            }
          SET_set_t self = SET_constrEmpty (numGNodes);
          self[i] = 1;
          domSetNxt = SET_union (domSetNxt, self, numGNodes);
          if (!SET_eq (domSetNxt, bgNodeInfos[i]->domSet, numGNodes))
            {
              bgNodeInfos[i]->domSet = domSetNxt;
              diff = 1;
            }
        }
      if (!diff)
        break;
    }
}

void
SSA_computeIdom ()
{
  for (int i = 1; i < numGNodes; ++i)
    {
      for (int j = 0; j < numGNodes; ++j)
        {
          if (i == j)
            continue;
          if (bgNodeInfos[i]->domSet[j])
            {
              bool flag = 1;
              for (int k = 0; k < numGNodes; ++k)
                {
                  if (k == i || k == j || !bgNodeInfos[i]->domSet[k])
                    continue;
                  if (bgNodeInfos[k]->domSet[j])
                    {
                      flag = 0;
                      break;
                    }
                }
              if (flag)
                {
                  bgNodeInfos[i]->idom = bgNodeInfos[j]->bgNode;
                  bgNodeInfos[j]->children = G_NodeList (
                      bgNodeInfos[i]->bgNode, bgNodeInfos[j]->children);
                  break;
                }
            }
        }
    }
}

void
SSA_computeDF (G_node n)
{
  SET_set_t s = SET_constrEmpty (numGNodes);
  for (G_nodeList succ = G_succ (n); succ; succ = succ->tail)
    {
      G_node c = succ->head;
      G_node idom = bgNodeInfos[c->mykey]->idom;
      if (idom != NULL && idom->mykey != n->mykey)
        s[c->mykey] = 1;
    }
  for (G_nodeList children = bgNodeInfos[n->mykey]->children; children;
       children = children->tail)
    {
      G_node c = children->head;
      SSA_computeDF (c);
      for (int w = 0; w < numGNodes; ++w)
        {
          if (bgNodeInfos[c->mykey]->df[w])
            {
              if (bgNodeInfos[w]->domSet[n->mykey] == 0 || w == n->mykey)
                s[w] = 1;
            }
        }
    }
  bgNodeInfos[n->mykey]->df = s;
}

string
SSA_formatPhi (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  string buf = (string)checked_malloc (IR_MAXLEN);
  vsprintf (buf, fmt, args);
  va_end (args);
  return buf;
}

LL_instr
SSA_constrPhi (SSA_varInfo_t var, int node)
{
  char *instrStr = checked_malloc (INSTRLEN);
  char *tyStr = var->tmp->type == T_float ? "double " : "i64 ";
  memset (instrStr, 0, INSTRLEN);
  sprintf (instrStr, "%%`d0 = phi %s", tyStr);

  int num = 0;
  Temp_tempList dst = Temp_TempList (var->tmp, NULL);
  Temp_tempList src = NULL;
  Temp_labelList tl = NULL, tl_last = NULL;
  G_nodeList pred = G_pred (bgNodeInfos[node]->bgNode);
  while (pred)
    {
      if (pred->tail)
        strcat (instrStr, SSA_formatPhi ("[%%`s%d, %%`j%d], ", num, num));
      else
        strcat (instrStr, SSA_formatPhi ("[%%`s%d,%%`j%d]", num, num));
      src = Temp_TempList (var->tmp, src);
      LL_block block = (LL_block)G_nodeInfo (pred->head);
      Temp_label label = block->label;
      if (tl_last == NULL)
        tl = tl_last = Temp_LabelList (label, NULL);
      else
        {
          tl_last->tail = Temp_LabelList (label, NULL);
          tl_last = tl_last->tail;
        }
      ++num;
      pred = pred->tail;
    }
  return LL_Oper (instrStr, dst, src, LL_Targets (tl));
}

void
SSA_insertPhi ()
{
  for (int n = 0; n < numGNodes; ++n)
    {
      Temp_tempList A_orig = bgNodeInfos[n]->Aorig;
      for (; A_orig; A_orig = A_orig->tail)
        {
          Temp_temp t = A_orig->head;
          SSA_varInfo_t vInfo = SSA_tabLook (varInfoTab, t->num);
          if (vInfo == NULL)
            {
              vInfo = SSA_constrVarInfo (numGNodes, t);
              SSA_tabEnter (varInfoTab, t->num, vInfo);
              // DBGPRT ("insert var %d with type %s\n", t->num,
              //         t->type == T_float ? "double" : "i64");
            }
          vInfo->defsites[n] = 1;
        }
    }
  for (int i = 0; i < VITABSZ; ++i)
    {
      SSA_tabEnt_t ithVITEnt = varInfoTab->table[i];
      /* Second iteration of algorithm 19.6
       * for each variable */
      for (SSA_tabEnt_t entIter = ithVITEnt; entIter; entIter = entIter->next)
        {
          SSA_tabEnt_t a = entIter;
          SET_set_t W = a->value->defsites;
          int sz = SET_setSize (W, numGNodes);
          /* while W not empty */
          while (sz > 0)
            {
              for (int n = 0; n < numGNodes; ++n)
                {
                  if (!W[n])
                    continue;
                  --sz;
                  W[n] = 0;
                  for (int Y = 0; Y < numGNodes; ++Y)
                    {
                      if (bgNodeInfos[n]->df[Y] == 0)
                        continue;
                      SSA_varInfo_t vInfo
                          = SSA_tabLook (bgNodeInfos[Y]->Aphi, a->key);
                      if (vInfo == NULL
                          && isInTempList (bgNodeInfos[Y]->in, a->key))
                        {
                          // DBGPRT ("%d, %d\n", vInfo == NULL,
                          //         isInTempList (bgNodeInfos[Y]->in,
                          //         a->key));
                          // DBGPRT ("place phi for temp %d at node %d\n",
                          // a->key,
                          //         Y);
                          LL_instr phiIns = SSA_constrPhi (a->value, Y);
                          LL_block block
                              = (LL_block)G_nodeInfo (bgNodeInfos[Y]->bgNode);
                          block->instrs->tail
                              = LL_InstrList (phiIns, block->instrs->tail);

                          SSA_tabEnter (bgNodeInfos[Y]->Aphi, a->key,
                                        a->value);
                          if (!isInTempList (bgNodeInfos[Y]->Aorig, a->key))
                            {
                              W[Y] = 1;
                              ++sz;
                            }
                        }
                    }

                  break;
                }
            }
        }
    }
}

bool
SSA_isPhi (LL_instr instr)
{
  return instr->kind == LL_OPER && strstr (instr->u.OPER.assem, "phi") != NULL
         && strstr (instr->u.OPER.assem, "[") != NULL;
}

void
SSA_rename (int n)
{
  STK_stack_t defStk = NULL;
  LL_block block = (LL_block)G_nodeInfo (bgNodeInfos[n]->bgNode);

  for (LL_instrList il = block->instrs; il; il = il->tail)
    {
      LL_instr S = il->head;
      if (!SSA_isPhi (S))
        {
          Temp_tempList useLst = NULL;
          switch (S->kind)
            {
            case LL_MOVE:
              useLst = S->u.MOVE.src;
              break;
            case LL_OPER:
              useLst = S->u.OPER.src;
              break;
            default:
              break;
            }
          while (useLst)
            {
              Temp_temp x = useLst->head;
              SSA_varInfo_t vmeta = SSA_tabLook (varInfoTab, x->num);
              Temp_temp i = (vmeta == NULL)          ? x
                            : (vmeta->stack == NULL) ? x
                                                     : STK_top (vmeta->stack);
              useLst->head = i;
              useLst = useLst->tail;
            }
        }
      Temp_tempList defLst = NULL;
      switch (S->kind)
        {
        case LL_OPER:
          defLst = S->u.OPER.dst;
          break;
        case LL_MOVE:
          defLst = S->u.MOVE.dst;
          break;
        default:
          break;
        }
      while (defLst)
        {
          Temp_temp x = defLst->head;
          Temp_temp i = Temp_newtemp (x->type);
          STK_push (&SSA_tabLook (varInfoTab, x->num)->stack, i);
          STK_push (&defStk, x);
          defLst->head = i;
          defLst = defLst->tail;
        }
    }

  for (G_nodeList succ = G_succ (bgNodeInfos[n]->bgNode); succ;
       succ = succ->tail)
    {
      G_node Y = succ->head;
      int j = 0;
      for (G_nodeList predGl = G_pred (Y); predGl != NULL;
           predGl = predGl->tail)
        {
          if (bgNodeInfos[n]->bgNode->mykey == predGl->head->mykey)
            break;
          ++j;
        }
      LL_block block = (LL_block)G_nodeInfo (Y);

      for (LL_instrList il = block->instrs; il; il = il->tail)
        {
          LL_instr instr = il->head;
          if (SSA_isPhi (instr))
            {
              // DBGPRT ("dddd\n");
              // DBGPRT ("instr at %p\n", instr);
              // DBGPRT ("instr content: %s\n", instr->u.OPER.assem);
              Temp_tempList used_list = instr->u.OPER.src;
              for (int i = 0; i < j; ++i)
                used_list = used_list->tail;
              STK_stack_t stk
                  = SSA_tabLook (varInfoTab, used_list->head->num)->stack;
              MYASRT (stk != NULL);
              used_list->head = STK_top (stk);
            }
        }
    }

  for (G_nodeList c = bgNodeInfos[n]->children; c; c = c->tail)
    SSA_rename (c->head->mykey);
  while (defStk)
    {
      Temp_temp x = STK_top (defStk);
      STK_pop (&defStk);
      STK_pop (&SSA_tabLook (varInfoTab, x->num)->stack);
    }
}

SSA_varInfo_t
SSA_constrVarInfo (int numNodes, Temp_temp t)
{
  SSA_varInfo_t ret = checked_malloc (sizeof (*ret));
  ret->defsites = SET_constrEmpty (numNodes);
  ret->stack = NULL;
  ret->tmp = t;
  return ret;
}

SSA_intVarInfoTab_t
SSA_constrEmptyTab ()
{
  SSA_intVarInfoTab_t ht = checked_malloc (sizeof (*ht));
  ht->size = VITABSZ;
  ht->table = checked_malloc (VITABSZ * sizeof (*ht->table));
  for (int i = 0; i < VITABSZ; ++i)
    ht->table[i] = NULL;
  return ht;
}

SSA_varInfo_t
SSA_tabLook (SSA_intVarInfoTab_t ht, int key)
{
  int idx = key % ht->size;
  SSA_tabEnt_t entry = ht->table[idx];
  while (entry)
    {
      if (entry->key == key)
        return entry->value;
      entry = entry->next;
    }
  return NULL;
}

void
SSA_tabEnter (SSA_intVarInfoTab_t tab, int key, SSA_varInfo_t value)
{
  int idx = key % tab->size;
  SSA_tabEnt_t entry = checked_malloc (sizeof (*entry));
  entry->key = key;
  entry->value = value;
  entry->next = tab->table[idx];
  tab->table[idx] = entry;
}

SSA_bgNodeInfo_t
SSA_constrBgNodeInfo (G_node node, int numNodes, LL_instrList bodyil,
                      G_nodeList lg)
{
  SSA_bgNodeInfo_t ret = checked_malloc (sizeof (*ret));
  ret->bgNode = node;
  ret->domSet = SET_constrEmpty (numNodes);
  ret->idom = NULL;
  ret->children = NULL;
  ret->df = NULL;

  LL_instr fstInstr = ((LL_block)G_nodeInfo (node))->instrs->head;
  for (; bodyil; bodyil = bodyil->tail, lg = lg->tail)
    {
      if (bodyil->head == fstInstr)
        break;
    }
  ret->in = LL_FG_In (lg->head);
  // DBGPRT ("node %d: in = ", node->mykey);
  // for (Temp_tempList inIter = ret->in; inIter; inIter = inIter->tail)
  //   DBGPRT ("%d ", inIter->head->num);
  // DBGPRT ("\n");
  ret->Aorig = NULL;
  LL_block block = (LL_block)G_nodeInfo (node);
  for (LL_instrList ilIter = block->instrs; ilIter; ilIter = ilIter->tail)
    {
      Temp_tempList orig = NULL;
      switch (ilIter->head->kind)
        {
        case LL_MOVE:
          orig = ilIter->head->u.MOVE.dst;
          break;
        case LL_OPER:
          orig = ilIter->head->u.OPER.dst;
          break;
        default:
          break;
        }
      for (; orig; orig = orig->tail)
        {
          Temp_temp t = (Temp_temp)checked_malloc (sizeof (*t));
          t->num = orig->head->num;
          t->type = orig->head->type;
          ret->Aorig = Temp_TempList (t, ret->Aorig);
        }
    }
  ret->Aphi = SSA_constrEmptyTab ();
  return ret;
}

static bool
isInTempList (Temp_tempList l, int key)
{
  for (; l; l = l->tail)
    {
      if (l->head->num == key)
        return 1;
    }
  return 0;
}
