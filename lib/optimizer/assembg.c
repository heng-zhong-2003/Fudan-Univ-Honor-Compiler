#include "assembg.h"

#define __DEBUG
#undef __DEBUG

static S_table benv; // table of label -> block
static G_graph bg;   // graph of basic blocks

static void
AS_benv_empty ()
{
  benv = S_empty ();
}

static AS_block
AS_benv_look (Temp_label l)
{
  return (AS_block)S_look (benv, l);
}

static void
AS_benv_enter (Temp_label l, AS_block b)
{
  S_enter (benv, l, b);
}

static void
AS_bg_empty ()
{
  bg = G_Graph ();
}

static G_node
AS_bg_look (AS_block b)
{
  for (G_nodeList n = G_nodes (bg); n != NULL; n = n->tail)
    {
      if ((AS_block)G_nodeInfo (n->head) == b)
        {
          return n->head;
        }
    }
  return G_Node (bg, b);
}

static void
AS_bg_addNode (AS_block b)
{
  AS_bg_look (b);
}

static void
AS_bg_rmNode (AS_block b)
{
  for (G_nodeList n = G_nodes (bg); n != NULL; n = n->tail)
    {
      if ((AS_block)G_nodeInfo (n->head) == b)
        {
          int delkey = n->head->mykey;
          for (G_nodeList m = G_nodes (bg); m != NULL; m = m->tail)
            {
              if (m->head->mykey > delkey)
                {
                  m->head->mykey -= 1;
                }
            }
          n->head->mygraph->nodecount -= 1;
          G_nodeList fa = n->head->mygraph->mynodes;
          if (fa->head == n->head)
            n->head->mygraph->mynodes = n->head->mygraph->mynodes->tail;
          for (G_nodeList dd = n->head->mygraph->mynodes->tail; dd;
               dd = dd->tail)
            {
              if (dd->head == n->head)
                {
                  fa->tail = dd->tail;
                  if (n->head->mygraph->mylast->head == n->head)
                    {
                      n->head->mygraph->mylast->head = fa->head;
                    }
                  break;
                }
              fa = dd;
            }
          return;
        }
    }
}

static void
AS_bg_addEdge (AS_block b1, AS_block b2)
{
  G_addEdge (AS_bg_look (b1), AS_bg_look (b2));
}

static void
AS_bg_rmEdge (AS_block b1, AS_block b2)
{
  G_rmEdge (AS_bg_look (b1), AS_bg_look (b2));
}

G_nodeList
AS_Create_bg (AS_blockList bl)
{
  AS_bg_empty ();   // reset bg
  AS_benv_empty (); // reset benv

  for (AS_blockList l = bl; l; l = l->tail)
    {
      AS_benv_enter (l->head->label, l->head); // enter label -> block pair
      AS_bg_addNode (l->head);                 // enter basic block node
    }

  for (AS_blockList l = bl; l; l = l->tail)
    {
      Temp_labelList tl = l->head->succs;
      while (tl)
        {
          AS_block succ = AS_benv_look (tl->head);
          // if the succ label doesn't have a block, assume it's the "exit
          // label", then this doesn't form an edge in the bg graph
          if (succ)
            AS_bg_addEdge (l->head, succ); // enter basic block edge
#ifdef __DEBUG
          if (succ)
            {
              fprintf (stderr, "block %s found its successor blocks %s\n",
                       Temp_labelstring (l->head->label),
                       Temp_labelstring (tl->head));
              fflush (stderr);
            }
          else
            {
              fprintf (stderr, "block %s doesn't find any successor\n",
                       Temp_labelstring (l->head->label));
              fflush (stderr);
            }
#endif
          tl = tl->tail;
        }
    }

  // remove dangling nodes
  if (bl && bl->head)
    {
      AS_block beg = bl->head;
      bool del = TRUE;
      while (del)
        {
          del = FALSE;
          for (AS_blockList l = bl->tail; l; l = l->tail)
            {
              if (AS_bg_look (l->head)->preds)
                continue;
              // delete hanging block!
              del = TRUE;
              Temp_labelList tl = l->head->succs;
              while (tl)
                {
                  AS_block succ = AS_benv_look (tl->head);
                  if (succ)
                    AS_bg_rmEdge (l->head, succ); // enter basic block edge
                  tl = tl->tail;
                }
              AS_bg_rmNode (l->head);
              for (AS_blockList dd = bl; dd->tail; dd = dd->tail)
                {
                  if (dd->tail->head == l->head)
                    {
                      dd->tail = dd->tail->tail;
                      l = dd;
                      break;
                    }
                }
            }
        }
    }

  return G_nodes (bg);
}

G_graph
AS_Bg_graph ()
{
  return bg;
}

S_table
AS_Bg_env ()
{
  return benv;
}

static void
AS_show_AS_Block (AS_block b)
{
  fprintf (stdout, "%s", Temp_labelstring (b->label));
}

void
AS_Show_bg (FILE *out, G_nodeList l)
{
  G_show (out, l, (void *)AS_show_AS_Block);
}
