#include "regalloc.h"
#include "assem.h"
#include "assemig.h"
#include "dbg.h"
#include "graph.h"
#include "temp.h"
#include <stdio.h>
#include <stdlib.h>

#define TEMPNUM 1024
#define IGNDNUM 1024
#define INSTRLEN 128
#define ARCHSZ 4
#define REGLEN 16
#define INTK 7    /* r1-r7 */
#define FLOATK 28 /* s1-s28 */

typedef struct REG_igNodeInfo REG_igNodeInfo_t;
struct REG_igNodeInfo
{
  int simpl; /* is simplified. boolean */
  int *intrf;
  int deg;
  int reg; /* -1: not yet colored, -2: spilt */
  int off; /* -1: not yet assigned */
};

static int igNodeNum = 0;
static REG_igNodeInfo_t *igNodeInfoArr = NULL;
static Temp_map regMap = NULL;
static Temp_tempList spiltTempLst = NULL;
static Temp_temp intRegs[16] = { NULL }, floatRegs[32] = { NULL };

REG_igNodeInfo_t REG_CnstrIgNodeInfo (G_node nd);
Temp_temp REG_rmOneVertex (G_nodeList ig);
Temp_tempList REG_rmVertices (G_nodeList ig, Temp_tempList stack);
Temp_temp REG_spill (G_nodeList ig);
void REG_checkRegValid (G_nodeList nl, Temp_temp tmp, int valid[]);
void REG_color (Temp_tempList stack);
void REG_assignRegs (G_nodeList ig);
AS_instrList REG_allocStack (AS_instrList il, int sz);
void REG_insertMemop (AS_instrList il);

static void initGlobs (G_nodeList ig);
static Temp_map cnstrRegMap ();
static void decDeg (G_nodeList nl, G_node nd);
static char *temp2Str (Temp_temp tmp);
static void prtTempList (Temp_tempList tl);

REG_allocRet_t
REG_regalloc (AS_instrList il, G_nodeList ig_local, G_nodeList igCopy)
{
  if (il == NULL)
    {
      REG_allocRet_t reg = { .il = NULL, .map = NULL };
    }
  initGlobs (ig_local);
  // DBGPRT ("initGlobs finish\n");
  REG_assignRegs (ig_local);
  // DBGPRT ("REG_assignRegs finish\n");
  // DBGPRT ("spiltTempList: ");
  // prtTempList (spiltTempLst);
  REG_insertMemop (il);
  // DBGPRT ("REG_insertMemop finish\n");
  // AS_printInstrList (stderr, il, regMap);
  // Temp_dumpMap (stderr, regMap);
  REG_allocRet_t ret = { .il = il, .map = regMap };
  return ret;
}

REG_igNodeInfo_t
REG_CnstrIgNodeInfo (G_node nd)
{
  REG_igNodeInfo_t ret = {
    .simpl = 0,
    .intrf = calloc (igNodeNum, sizeof (int)),
    .deg = 0,
    .reg = -1,
    .off = -1,
  };

  Temp_temp tmp = (Temp_temp)G_nodeInfo (nd);
  for (G_nodeList pditer = G_pred (nd); pditer != NULL; pditer = pditer->tail)
    {
      Temp_temp pdtmp = (Temp_temp)G_nodeInfo (pditer->head);
      if (pdtmp->type == tmp->type)
        ret.intrf[pditer->head->mykey] = 1;
    }
  for (G_nodeList sciter = G_succ (nd); sciter != NULL; sciter = sciter->tail)
    {
      Temp_temp sctmp = (Temp_temp)G_nodeInfo (sciter->head);
      if (sctmp->type == tmp->type)
        ret.intrf[sciter->head->mykey] = 1;
    }
  for (int i = 0; i < igNodeNum; ++i)
    {
      if (ret.intrf[i])
        ++ret.deg;
    }

  if (tmp->num <= 98)
    ret.reg = tmp->num;

  return ret;
}

Temp_temp
REG_rmOneVertex (G_nodeList ig)
{
  for (G_nodeList igiter = ig; igiter != NULL; igiter = igiter->tail)
    {
      G_node nd = igiter->head;
      Temp_temp tmp = G_nodeInfo (nd);
      if (igNodeInfoArr[nd->mykey].simpl)
        continue;
      if ((tmp->type == T_int && igNodeInfoArr[nd->mykey].deg < INTK
           && igNodeInfoArr[nd->mykey].reg == -1)
          || (tmp->type == T_float && igNodeInfoArr[nd->mykey].deg < FLOATK
              && igNodeInfoArr[nd->mykey].reg == -1))
        {
          igNodeInfoArr[nd->mykey].simpl = 1;
          decDeg (G_pred (nd), nd);
          decDeg (G_succ (nd), nd);
          return G_nodeInfo (nd);
        }
    }
  return NULL;
}

Temp_tempList
REG_rmVertices (G_nodeList ig, Temp_tempList stack)
{
  for (;;)
    {
      Temp_temp tmp = REG_rmOneVertex (ig);
      if (tmp == NULL)
        break;
      else
        stack = Temp_TempList (tmp, stack);
    }
  return stack;
}

Temp_temp
REG_spill (G_nodeList ig)
{
  int maxDeg = 0;
  G_node spilt = NULL;
  for (G_nodeList igiter = ig; igiter != NULL; igiter = igiter->tail)
    {
      G_node nd = igiter->head;
      if (igNodeInfoArr[nd->mykey].simpl == 0
          && igNodeInfoArr[nd->mykey].reg == -1
          && igNodeInfoArr[nd->mykey].deg > maxDeg)
        {
          maxDeg = igNodeInfoArr[nd->mykey].deg;
          spilt = nd;
        }
    }
  if (spilt == NULL)
    return NULL;
  igNodeInfoArr[spilt->mykey].simpl = 1;
  igNodeInfoArr[spilt->mykey].reg = -2;
  decDeg (G_pred (spilt), spilt);
  decDeg (G_succ (spilt), spilt);
  return G_nodeInfo (spilt);
}

void
REG_checkRegValid (G_nodeList nl, Temp_temp tmp, int valid[])
{
  for (G_nodeList nliter = nl; nliter != NULL; nliter = nliter->tail)
    {
      G_node nd = nliter->head;
      Temp_temp ndtmp = G_nodeInfo (nd);
      if (ndtmp->type == tmp->type && igNodeInfoArr[nd->mykey].reg >= 0)
        valid[igNodeInfoArr[nd->mykey].reg] = 0;
    }
}

void
REG_color (Temp_tempList stack)
{
  for (Temp_tempList stkiter = stack; stkiter != NULL; stkiter = stkiter->tail)
    {
      Temp_temp tmp = stkiter->head;
      // DBGPRT ("coloring temp: %s\n", temp2Str (tmp));
      int *intValid = calloc (INTK, sizeof (int)),
          *floatValid = calloc (FLOATK, sizeof (int));
      for (int i = 0; i < INTK; ++i)
        intValid[i] = 1;
      for (int i = 0; i < FLOATK; ++i)
        floatValid[i] = 1;
      G_node nd = AS_Look_ig (tmp);
      if (igNodeInfoArr[nd->mykey].reg != -1)
        continue;
      int *valid = tmp->type == T_int ? intValid : floatValid;
      REG_checkRegValid (G_pred (nd), G_nodeInfo (nd), valid);
      REG_checkRegValid (G_succ (nd), G_nodeInfo (nd), valid);
      int currReg = 0;
      if (tmp->type == T_int)
        {
          while (currReg < INTK && !valid[currReg])
            ++currReg;
          MYASRT (currReg < INTK);
        }
      else
        {
          while (currReg < FLOATK && !valid[currReg])
            ++currReg;
          MYASRT (currReg < FLOATK);
        }
      igNodeInfoArr[nd->mykey].reg = currReg;
      char *s = calloc (REGLEN, sizeof (char));
      if (tmp->type == T_int)
        sprintf (s, "r%d", currReg);
      else
        sprintf (s, "s%d", currReg);
      // DBGPRT ("Enter %s -> %s%d to regMap\n", temp2Str (tmp),
      //         tmp->type == T_int ? "r" : "s", currReg);
      Temp_enter (regMap, tmp, s);
    }
}

void
REG_assignRegs (G_nodeList ig)
{
  Temp_tempList stack = NULL;
  for (;;)
    {
      stack = REG_rmVertices (ig, stack);
      Temp_temp spilt = REG_spill (ig);
      if (spilt == NULL)
        ;
      else
        spiltTempLst = Temp_TempList (spilt, spiltTempLst);
      if (spilt == NULL)
        break;
    }
  REG_color (stack);
}

AS_instrList
REG_allocStack (AS_instrList il, int sz)
{
  char *subsp = calloc (INSTRLEN, sizeof (char));
  sprintf (subsp, "sub sp, sp, #%d", sz);
  il->tail = AS_InstrList (AS_Oper (subsp, NULL, NULL, NULL), il->tail);
  return il->tail;
}

void
REG_insertMemop (AS_instrList il)
{
  int stackSize = 0;
  for (Temp_tempList spliter = spiltTempLst; spliter != NULL;
       spliter = spliter->tail)
    {
      G_node nd = AS_Look_ig (spliter->head);
      igNodeInfoArr[nd->mykey].off = stackSize;
      ++stackSize;
    }
  stackSize *= ARCHSZ;
  int firstUse = 1;
  AS_instrList prev = il;
  il = il->tail;
  for (Temp_tempList spliter = spiltTempLst; spliter != NULL;
       spliter = spliter->tail)
    {
      G_node nd = AS_Look_ig (spliter->head);
      // DBGPRT ("Spilt temp %s get offset %d\n", temp2Str (spliter->head),
      //         igNodeInfoArr[nd->mykey].off * ARCHSZ - 96 - stackSize);
    }
  while (il)
    {
      AS_instr ins = il->head;
      // AS_print (stderr, ins, Temp_name ());
      int intSpillReg = 8, floatSpillReg = 29;
      for (Temp_tempList srciter = ins->u.OPER.src; srciter != NULL;
           srciter = srciter->tail)
        {
          if (Temp_TempInTempList (srciter->head, spiltTempLst))
            {
              if (firstUse)
                {
                  prev = REG_allocStack (prev, stackSize);
                  firstUse = 0;
                }
              G_node nd = AS_Look_ig (srciter->head);
              char *ldstr = calloc (INSTRLEN, sizeof (char));
              if (srciter->head->type == T_int)
                {
                  sprintf (ldstr, "ldr r%d, [fp, #%d]", intSpillReg,
                           igNodeInfoArr[nd->mykey].off * ARCHSZ - 96
                               - stackSize);
                  srciter->head = intRegs[intSpillReg];
                  ++intSpillReg;
                }
              else
                {
                  sprintf (ldstr, "vldr.f32 s%d, [fp, #%d]", floatSpillReg,
                           igNodeInfoArr[nd->mykey].off * ARCHSZ - 96
                               - stackSize);
                  srciter->head = floatRegs[floatSpillReg];
                  ++floatSpillReg;
                }
              AS_instr ldins = AS_Oper (ldstr, NULL, NULL, NULL);
              prev->tail = AS_InstrList (ldins, il);
              // AS_printInstrList (stderr, prev, Temp_name ());
              prev = prev->tail;
            }
        }
      for (Temp_tempList dstiter = ins->u.OPER.dst; dstiter != NULL;
           dstiter = dstiter->tail)
        {
          // AS_print (stderr, ins, Temp_name ());
          // DBGPRT ("dst: %s\n", temp2Str (dstiter->head));
          if (Temp_TempInTempList (dstiter->head, spiltTempLst))
            {
              if (firstUse)
                {
                  prev = REG_allocStack (prev, stackSize);
                  firstUse = 0;
                }
              G_node nd = AS_Look_ig (dstiter->head);
              char *ststr = calloc (INSTRLEN, sizeof (char));
              if (dstiter->head->type == T_int)
                {
                  sprintf (ststr, "str r%d, [fp, #%d]", intSpillReg,
                           igNodeInfoArr[nd->mykey].off * ARCHSZ - 96
                               - stackSize);
                  dstiter->head = intRegs[intSpillReg];
                  ++intSpillReg;
                }
              else
                {
                  sprintf (ststr, "vstr.f32 s%d, [fp, #%d]", floatSpillReg,
                           igNodeInfoArr[nd->mykey].off * ARCHSZ - 96
                               - stackSize);
                  dstiter->head = floatRegs[floatSpillReg];
                  ++floatSpillReg;
                }
              AS_instr stins = AS_Oper (ststr, NULL, NULL, NULL);
              il->tail = AS_InstrList (stins, il->tail);
            }
        }
      prev = prev->tail;
      il = il->tail;
    }
}

static void
initGlobs (G_nodeList ig)
{
  igNodeNum = ig == NULL ? 0 : ig->head->mygraph->nodecount;
  igNodeInfoArr = calloc (igNodeNum, sizeof (REG_igNodeInfo_t));
  for (G_nodeList igiter = ig; igiter != NULL; igiter = igiter->tail)
    igNodeInfoArr[igiter->head->mykey] = REG_CnstrIgNodeInfo (igiter->head);
  regMap = cnstrRegMap ();
  spiltTempLst = NULL;
}

static Temp_map
cnstrRegMap ()
{
  Temp_map ret = Temp_empty ();
  for (int i = 0; i < 16; ++i)
    {
      Temp_temp t = Temp_reg (i, T_int);
      intRegs[i] = t;
      char *s = calloc (REGLEN, sizeof (char));
      sprintf (s, "r%d", i);
      Temp_enter (ret, t, s);
    }
  for (int i = 0; i < 32; ++i)
    {
      Temp_temp t = Temp_reg (i, T_float);
      floatRegs[i] = t;
      char *s = calloc (REGLEN, sizeof (char));
      sprintf (s, "s%d", i);
      Temp_enter (ret, t, s);
    }
  return ret;
}

static void
decDeg (G_nodeList nl, G_node nd)
{
  Temp_temp ndtmp = G_nodeInfo (nd);
  for (G_nodeList nliter = nl; nliter != NULL; nliter = nliter->tail)
    {
      Temp_temp tmp = G_nodeInfo (nliter->head);
      if (ndtmp->type == tmp->type)
        {
          if (igNodeInfoArr[nliter->head->mykey].intrf[nd->mykey])
            {
              igNodeInfoArr[nliter->head->mykey].intrf[nd->mykey] = 0;
              --igNodeInfoArr[nliter->head->mykey].deg;
            }
          if (igNodeInfoArr[nliter->head->mykey].deg < 0)
            MYASRT (0);
        }
    }
}

static char *
temp2Str (Temp_temp tmp)
{
  char *ret = calloc (32, sizeof (char));
  sprintf (ret, "TMP%d#%s", tmp->num, tmp->type == T_int ? "INT" : "FLOAT");
  return ret;
}

static void
prtTempList (Temp_tempList tl)
{
  fprintf (stderr, "%s", KYEL);
  for (Temp_tempList iter = tl; iter != NULL; iter = iter->tail)
    fprintf (stderr, "%s, ", temp2Str (iter->head));
  fprintf (stderr, "%s\n", KNRM);
}
