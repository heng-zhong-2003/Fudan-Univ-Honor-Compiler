#include "llvmirbg.h"

#define __DEBUG
#undef __DEBUG

static S_table benv; // table of label -> block
static G_graph bg;   // graph of basic blocks

static void
LL_benv_empty ()
{
  benv = S_empty ();
}

static LL_block
LL_benv_look (Temp_label l)
{
  return (LL_block)S_look (benv, l);
}

static void
LL_benv_enter (Temp_label l, LL_block b)
{
  S_enter (benv, l, b);
}

static void
LL_bg_empty ()
{
  bg = G_Graph ();
}

static G_node
LL_bg_look (LL_block b)
{
  for (G_nodeList n = G_nodes (bg); n != NULL; n = n->tail)
    {
      if ((LL_block)G_nodeInfo (n->head) == b)
        {
          return n->head;
        }
    }
  return G_Node (bg, b);
}

static void
LL_bg_addNode (LL_block b)
{
  LL_bg_look (b);
}

static void
LL_bg_rmNode (LL_block b)
{
  for (G_nodeList n = G_nodes (bg); n != NULL; n = n->tail)
    {
      if ((LL_block)G_nodeInfo (n->head) == b)
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
LL_bg_addEdge (LL_block b1, LL_block b2)
{
  G_addEdge (LL_bg_look (b1), LL_bg_look (b2));
}

static void
LL_bg_rmEdge (LL_block b1, LL_block b2)
{
  G_rmEdge (LL_bg_look (b1), LL_bg_look (b2));
}

G_nodeList
LL_Create_bg (LL_blockList bl)
{
  LL_bg_empty ();   // reset bg
  LL_benv_empty (); // reset benv

  for (LL_blockList l = bl; l; l = l->tail)
    {
      LL_benv_enter (l->head->label, l->head); // enter label -> block pair
      LL_bg_addNode (l->head);                 // enter basic block node
    }

  for (LL_blockList l = bl; l; l = l->tail)
    {
      Temp_labelList tl = l->head->succs;
      while (tl)
        {
          LL_block succ = LL_benv_look (tl->head);
          // if the succ label doesn't have a block, assume it's the "exit
          // label", then this doesn't form an edge in the bg graph
          if (succ)
            LL_bg_addEdge (l->head, succ); // enter basic block edge
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
      LL_block beg = bl->head;
      bool del = TRUE;
      while (del)
        {
          del = FALSE;
          for (LL_blockList l = bl->tail; l; l = l->tail)
            {
              if (LL_bg_look (l->head)->preds)
                continue;
              // delete hanging block!
              del = TRUE;
              Temp_labelList tl = l->head->succs;
              while (tl)
                {
                  LL_block succ = LL_benv_look (tl->head);
                  if (succ)
                    LL_bg_rmEdge (l->head, succ); // enter basic block edge
                  tl = tl->tail;
                }
              LL_bg_rmNode (l->head);
              for (LL_blockList dd = bl; dd->tail; dd = dd->tail)
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
LL_Bg_graph ()
{
  return bg;
}

S_table
LL_Bg_env ()
{
  return benv;
}

static void
LL_show_LL_Block (LL_block b)
{
  fprintf (stdout, "%s", Temp_labelstring (b->label));
}

void
LL_Show_bg (FILE *out, G_nodeList l)
{
  G_show (out, l, (void *)LL_show_LL_Block);
}
