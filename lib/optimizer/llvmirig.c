#include "llvmirig.h"

static G_graph LL_RA_ig;

void
LL_Ig_empty ()
{
  LL_RA_ig = G_Graph ();
}

G_graph
LL_Ig_graph ()
{
  return LL_RA_ig;
}

G_node
LL_Look_ig (Temp_temp t)
{
  G_node n1 = NULL;
  for (G_nodeList n = G_nodes (LL_RA_ig); n != NULL; n = n->tail)
    {
      if ((Temp_temp)G_nodeInfo (n->head) == t)
        {
          n1 = n->head;
          break;
        }
    }
  if (n1 == NULL)
    return (G_Node (LL_RA_ig, t));
  else
    return n1;
}

void
LL_Enter_ig (Temp_temp t1, Temp_temp t2)
{
  G_node n1 = LL_Look_ig (t1);
  G_node n2 = LL_Look_ig (t2);
  G_addEdge (n1, n2);
  return;
}

// input flowgraph after liveness analysis (so FG_In and FG_Out are available)

G_nodeList
LL_Create_ig (G_nodeList flowgraph)
{
  G_nodeList flg = flowgraph;
  G_node n, ig_n;
  Temp_tempList use;
  bool is_m;

  LL_RA_ig = G_Graph ();

  while (flg != NULL)
    {
      n = flg->head;
      // prepare to handle the move case
      if ((is_m = LL_FG_isMove (n)))
        {
          use = LL_FG_use (n);
          assert (use);
        }
      else
        use = NULL;

      // for each instruction, find the conflict from def to liveout
      for (Temp_tempList tl1 = LL_FG_def (n); tl1 != NULL; tl1 = tl1->tail)
        {
          LL_Look_ig (tl1->head);
          for (Temp_tempList tl2 = LL_FG_Out (n); tl2 != NULL; tl2 = tl2->tail)
            {
              if ((!(is_m && tl2->head == use->head))
                  && (tl2->head != tl1->head))
                {
                  LL_Enter_ig (tl1->head, tl2->head);
                }
            }
        }
      flg = flg->tail;
    }
  return G_nodes (LL_RA_ig);
}

static void
LL_show_temp (Temp_temp t)
{
  fprintf (stdout, "%s, ", Temp_look (Temp_name (), t));
}

void
LL_Show_ig (FILE *out, G_nodeList l)
{
  // while ( l!=NULL ) {
  // G_node n = l->head;
  //   fprintf(out, "----------------------\n");
  G_show (out, l, (void *)LL_show_temp);
  // l=l->tail;
  // }
}

// The following procedure prints out the ig create code from a given ig data
// structure so this code may be used for testing register allocation (from ig
// generated from liveness analysis etc)

void
LL_Create_ig_Code (FILE *out, G_nodeList ig)
{
  G_nodeList s;

  fprintf (out, "G_nodeList Create_ig1() {\n");

  for (s = ig; s; s = s->tail)
    {
      Temp_temp t = G_nodeInfo (s->head);
      string s = Temp_look (Temp_name (), t);
      if (atoi (s) >= 100)
        fprintf (out, "\tTemp_temp t%s=Temp_newtemp();\n", s);
      else
        {
          fprintf (out, "\tTemp_temp t%s=F_Ri(%s);\n", s, &s[1]);
        }
    }

  fprintf (out, "\n\tIg_empty();\n\n");

  G_nodeList succ;
  for (s = ig; s; s = s->tail)
    {
      G_node n = s->head;
      Temp_temp t_fr = (Temp_temp)G_nodeInfo (n);
      string fr = Temp_look (Temp_name (), G_nodeInfo (n));
      if ((succ = G_succ (n)))
        {
          for (; succ; succ = succ->tail)
            {
              string to = Temp_look (Temp_name (),
                                     (Temp_temp)G_nodeInfo (succ->head));
              fprintf (out, "\tEnter_ig(t%s, t%s);\n", fr, to);
            }
        }
    }

  fprintf (out, "\n\treturn G_nodes(Ig_graph());\n}\n");
  return;
}
