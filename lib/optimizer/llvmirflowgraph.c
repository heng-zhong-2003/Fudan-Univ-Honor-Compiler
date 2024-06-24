/*
 * CS5161 Assignment 6
 *
 * TIGER's Generation of Control Flow Graph
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */

#include "llvmirflowgraph.h"

#define __DEBUG
#undef __DEBUG

/* Interfaces */
struct _UDinfo
{
  Temp_tempList uses;
  Temp_tempList defs;
  bool isMove;
};
typedef struct _UDinfo *UDinfo;

// Create a table mapping between node and UDinfo
static void LL_UD_init ();
// Enter a new node with UDinfo into the table
static void LL_UD_enter (G_node n, UDinfo info);
// Retrieve info from the table
static UDinfo LL_UD_lookup (G_node n);
// Create a UDInfo with given information
static UDinfo LL_UDInfo (Temp_tempList uses, Temp_tempList defs, bool isMove);

// Get the Lable-Node table
static TAB_table LL_LNTable ();
// Enter a new entry into the label-node table
static void LL_LT_enter (Temp_label l, G_node n);
// Look up a node using lable as key
static G_node LL_LT_lookup (Temp_label l);

/* Implementation */
static G_table LL_UDTable = NULL;

static void
LL_UD_init ()
{
  if (LL_UDTable == NULL)
    {
      LL_UDTable = G_empty ();
    }
}

static void
LL_UD_enter (G_node n, UDinfo info)
{
  G_enter (LL_UDTable, n, info);
}

static UDinfo
LL_UD_lookup (G_node n)
{
  return G_look (LL_UDTable, n);
}

static UDinfo
LL_UDInfo (Temp_tempList uses, Temp_tempList defs, bool isMove)
{
  UDinfo info = (UDinfo)checked_malloc (sizeof (struct _UDinfo));
  info->uses = uses;
  info->defs = defs;
  info->isMove = isMove;
  return info;
}

static TAB_table _lntable = NULL;

static TAB_table
LL_LNTable ()
{
  if (_lntable == NULL)
    {
      _lntable = TAB_empty ();
    }
  return _lntable;
}

static G_node
LL_LT_lookup (Temp_label l)
{
  return (G_node)TAB_look (LL_LNTable (), l);
}

static void
LL_LT_enter (Temp_label l, G_node n)
{
  TAB_enter (LL_LNTable (), l, n);
}

Temp_tempList
LL_FG_def (G_node n)
{
  UDinfo i = LL_UD_lookup (n);
  if (!i)
    return NULL;
  else
    return i->defs;
}

Temp_tempList
LL_FG_use (G_node n)
{
  UDinfo i = LL_UD_lookup (n);
  if (!i)
    return NULL;
  else
    return i->uses;
}

bool
LL_FG_isMove (G_node n)
{
  UDinfo i = LL_UD_lookup (n);
  if (!i)
    return FALSE;
  else
    return i->isMove;
}

void
LL_FG_Showinfo (FILE *out, LL_instr instr, Temp_map map)
{
  char *cs;
  char *lb;
  char r[200]; /* result */
  switch (instr->kind)
    {
    case LL_OPER:
      cs = instr->u.OPER.assem;
      lb = strrchr (cs, '\n');
      if (lb != NULL)
        {
          *lb = '\0';
        }
      LL_format (r, cs, instr->u.OPER.dst, instr->u.OPER.src,
                 instr->u.OPER.jumps, map);
      fprintf (out, "[%20s] ", r); // instr->u.OPER.assem);
      break;
    case LL_LABEL:
      cs = instr->u.LABEL.assem;
      lb = strrchr (cs, '\n');
      if (lb != NULL)
        {
          *lb = '\0';
        }
      LL_format (r, cs, NULL, NULL, NULL, map);
      fprintf (out, "[%20s] ", r);
      break;
    case LL_MOVE:
      cs = instr->u.MOVE.assem;
      lb = strrchr (cs, '\n');
      if (lb != NULL)
        {
          *lb = '\0';
        }
      LL_format (r, cs, instr->u.MOVE.dst, instr->u.MOVE.src, NULL, map);
      fprintf (out, "[%20s] ", r);
      break;
    }
}

void
LL_FG_show (LL_instr ins)
{
  LL_FG_Showinfo (stdout, ins, Temp_name ());
}

#define IT_COMMON 0
#define IT_JUMP 1
#define IT_MOVE 2
G_graph
LL_FG_AssemFlowGraph (LL_instrList il)
{
  LL_UD_init ();

  //(I) Iterate over the entire instruction list
  LL_instr instr = NULL;
  G_node prev = NULL;
  G_node curr = NULL;
  G_graph graph = G_Graph ();
  G_nodeList jumpList = NULL;
  G_nodeList jumpListHead = NULL;
  for (; il != NULL; il = il->tail)
    {
#ifdef __DEBUG
      fprintf (stdout, "Processing: il->head: %s\n", il->head->u.OPER.assem);
      fflush (stdout);
#endif
      instr = il->head;
      if (instr != NULL)
        {
          // 1) create a node (and put it into the graph), using the
          //    instruction as the associated info.
          curr = G_Node (graph, instr);

          // 2) special handling
          int type = IT_COMMON;
          Temp_tempList defs = NULL;
          Temp_tempList uses = NULL;

          switch (instr->kind)
            {
            case LL_OPER:
#ifdef __DEBUG
              fprintf (stderr, "instru->kind=%d, %d, %s\n", instr->kind,
                       I_OPER, instr->u.OPER.assem);
              fflush (stderr);
              if (instr->u.OPER.jumps != NULL
                  && instr->u.OPER.jumps->labels != NULL)
                {
                  fprintf (stderr, "1. jump detected\n");
                  fflush (stderr);
                }
#endif
              // Check if it's a JUMP instruction
              // We do this check here by looking if LL_target is null,
              // instead of inspecting the assembly op (j, beq, ...)
              if (instr->u.OPER.jumps != NULL
                  && instr->u.OPER.jumps->labels != NULL)
                {
                  type = IT_JUMP;
                  // put this instruction into a separate list
                  if (jumpList == NULL)
                    {
                      jumpList = G_NodeList (curr, NULL);
                      jumpListHead = jumpList;
                    }
                  else
                    {
                      jumpList->tail = G_NodeList (curr, NULL);
                      jumpList = jumpList->tail;
                    }
                }
              defs = instr->u.OPER.dst;
              uses = instr->u.OPER.src;
              break;
            case LL_LABEL:
#ifdef __DEBUG
              fprintf (stderr, "instru->kind=%d, %d, %s\n", instr->kind,
                       I_LABEL, instr->u.LABEL.assem);
              fflush (stderr);
#endif
              // 2.2) label should be also saved in the label-node list for
              // (II)
              LL_LT_enter (instr->u.LABEL.label, curr);
              break;
            case LL_MOVE:
#ifdef __DEBUG
              fprintf (stderr, "instru->kind=%d, %d, %s\n", instr->kind,
                       I_MOVE, instr->u.MOVE.assem);
              fflush (stderr);
#endif
              // 2.3) it's a move instruction
              type = IT_MOVE;
              defs = instr->u.MOVE.dst;
              uses = instr->u.MOVE.src;
              break;
            }

          // 3) put information into table
          LL_UD_enter (curr, LL_UDInfo (uses, defs, type == IT_MOVE));

          // 4) link with the previous node for falling through, if possible.
          //    Note that prev is NULL if the previous instruction is a JUMP.
          if (prev != NULL)
            {
              G_addEdge (prev, curr);
            }

          // 5) set as previous node for next time of iteration
          prev = (type != IT_JUMP) ? curr : NULL;
        }
    }

#ifdef __DEBUG
  fprintf (stderr, "Done with proceesing all instr\n");
#endif
  //(II) Iterate over the list that has all the JUMP instruction collected.
  Temp_labelList labels;
  for (; jumpListHead != NULL; jumpListHead = jumpListHead->tail)
    {
      curr = jumpListHead->head;
      labels
          = ((LL_instr)G_nodeInfo (curr))
                ->u.OPER.jumps->labels; // no need to check its nullity again
#ifdef __DEBUG
      fprintf (stderr, "Processing jump of instruction: %s\n",
               ((LL_instr)G_nodeInfo (curr))->u.OPER.assem);
      fflush (stderr);
#endif
      Temp_label label;
      G_node dest;
      // for each target it may jump to, add a corresponding edge in the graph
      for (; labels != NULL; labels = labels->tail)
        {
          label = labels->head;
          if (label != NULL)
            {
              // quickly retieve the target node using the label-node table
              dest = LL_LT_lookup (label);
              // establish edge between this node and its jump target (if
              // target exists)
              if (dest != NULL)
                G_addEdge (curr, dest);
              else
                {
#ifdef __DEBUG
                  fprintf (stderr,
                           "Warning: jump to a non-existing label: %s\n",
                           Temp_labelstring (label));
                  fflush (stderr);
#endif
                }
            }
        }
    }

  return graph;
}
#undef IT_COMMON
#undef IT_JUMP
#undef IT_MOVE
