#include "armgen.h"
#include "assem.h"
#include "dbg.h"
#include "graph.h"
#include "llvmir.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARMGEN_DEBUG
#undef ARMGEN_DEBUG

#define FOR_IN_S_TABLE(ptr, tbl)                                              \
  for (S_symbol ptr = (S_symbol)tbl->top; ptr != NULL;                        \
       ptr = ((binder)S_getBinder (tbl, ptr))->prevtop)

#define ARCH_SIZE 4
#define TYPELEN 10
#define GLOBLEN 32
#define ARGLEN 32
#define INSTRLEN 128
#define LLARGNUM 8
#define FUNNAMELEN 64
#define IMMNUM 2

enum AS_type_
{
  NONE,
  BR,
  RET,
  ADD,
  SUB,
  MUL,
  DIV,
  FADD,
  FSUB,
  FMUL,
  FDIV,
  F2I,
  I2F,
  I2P,
  P2I,
  LOAD,
  STORE,
  CALL,
  EXTCALL,
  ICMP,
  FCMP,
  LABEL,
  CJUMP,
  PHI,
};

enum AS_cmpCnd_
{
  AS_eq,
  AS_ne,
  AS_gt,
  AS_ge,
  AS_lt,
  AS_le,
};

/* instrNxt is for translation of ICMP and FCMP.
 * It is added to the params of other functions
 *   for uniformity, and thus simplicity.
 * For functions other than ICMP and FCMP, instrNxt is ignored */
void ARM_transNone (LL_instr instr, LL_instr instrNxt);
void ARM_transADD (LL_instr instr, LL_instr instrNxt);
void ARM_transSUB (LL_instr instr, LL_instr instrNxt);
void ARM_transMUL (LL_instr instr, LL_instr instrNxt);
void ARM_transDIV (LL_instr instr, LL_instr instrNxt);
void ARM_transFADD (LL_instr instr, LL_instr instrNxt);
void ARM_transFSUB (LL_instr instr, LL_instr instrNxt);
void ARM_transFMUL (LL_instr instr, LL_instr instrNxt);
void ARM_transFDIV (LL_instr instr, LL_instr instrNxt);
void ARM_transF2I (LL_instr instr, LL_instr instrNxt);
void ARM_transI2F (LL_instr instr, LL_instr instrNxt);
void ARM_transI2P (LL_instr instr, LL_instr instrNxt);
void ARM_transP2I (LL_instr instr, LL_instr instrNxt);
void ARM_transLOAD (LL_instr instr, LL_instr instrNxt);
void ARM_transSTORE (LL_instr instr, LL_instr instrNxt);
void ARM_transCALL (LL_instr instr, LL_instr instrNxt);
/* void ARM_transEXTCALL (AS_instr instr); */
void ARM_transBR (LL_instr instr, LL_instr instrNxt);
void ARM_transRET (LL_instr instr, LL_instr instrNxt);
void ARM_transICMP (LL_instr instr, LL_instr instrNxt);
void ARM_transFCMP (LL_instr instr, LL_instr instrNxt);
void ARM_transLABEL (LL_instr instr, LL_instr instrNxt);
void ARM_transCJUMP (LL_instr instr, LL_instr instrNxt);

typedef void (*ARM_transBodyFun_t) (LL_instr, LL_instr);
ARM_transBodyFun_t armbodyVtbl[] = {
  [BR] = ARM_transBR,       [RET] = ARM_transRET,   [ADD] = ARM_transADD,
  [SUB] = ARM_transSUB,     [MUL] = ARM_transMUL,   [DIV] = ARM_transDIV,
  [FADD] = ARM_transFADD,   [FSUB] = ARM_transFSUB, [FMUL] = ARM_transFMUL,
  [FDIV] = ARM_transFDIV,   [F2I] = ARM_transF2I,   [I2F] = ARM_transI2F,
  [I2P] = ARM_transI2P,     [P2I] = ARM_transP2I,   [LOAD] = ARM_transLOAD,
  [STORE] = ARM_transSTORE, [CALL] = ARM_transCALL, [EXTCALL] = NULL,
  [ICMP] = ARM_transICMP,   [FCMP] = ARM_transFCMP, [LABEL] = ARM_transLABEL,
  [CJUMP] = ARM_transCJUMP, [NONE] = ARM_transNone, [PHI] = ARM_transNone,
};

typedef union ARM_immfmt ARM_immfmt_t;
union ARM_immfmt
{
  uint32_t i;
  float f;
  struct
  {
    uint16_t lo, hi;
  } parts;
};

ARM_immfmt_t int2fmt (int i);
ARM_immfmt_t float2fmt (float f);

typedef struct intarrNumPair intarrNumPair_t;
struct intarrNumPair
{
  int *immarr;
  int num;
};

typedef struct floatarrNumPair floatarrNumPair_t;
struct floatarrNumPair
{
  float *immarr;
  int num;
};

/* Intrusive linked list */

#define offsetof(type, member) ((size_t) & ((type *)0)->member)
#define container_of(ptr, type, member)                                       \
  ((type *)((char *)(ptr)-offsetof (type, member)))

typedef struct listNode listNode_t;
struct listNode
{
  struct listNode *prev;
  struct listNode *next;
};

void listInit (listNode_t *nd);
void listAdd (listNode_t *nd, listNode_t *prev, listNode_t *next);

typedef struct phiEntry *phiEntry_t;
struct phiEntry
{
  Temp_temp dst, src;
  listNode_t nd;
};

phiEntry_t CnstrPhiEntry (Temp_temp dst, Temp_temp src);

int ARM_isPhi (LL_instr instr);
void ARM_dePhi (LL_instrList il);

static AS_instrList iList = NULL, last = NULL;
static void emit (AS_instr inst);
static void resetILs ();
static Temp_temp intImmIntoTemp (int i);
/* This stores the binary rep of a float into a T_int temp */
static Temp_temp floatImmIntoTemp (float f);
static inline Temp_tempList TL (Temp_temp tmp, Temp_tempList tl);
static inline Temp_temp TNWT (T_type ty);
static inline Temp_temp TRG (int num, T_type ty);
static char *parseFunName (char asmstr[]);
static char *parseFunName4Ptr (char asmstr[]);
static int parseFunArgs (char asmstr[], Temp_tempList args);
static intarrNumPair_t parseIntImms (char asmstr[]);
static floatarrNumPair_t parseFloatImms (char asmstr[]);
static char *parseCond (char asmstr[]);

AS_instrList
ARM_armprolog (LL_instrList il)
{
  LL_instr prolog = il->head;
  char *funname = parseFunName (prolog->u.OPER.assem);
  resetILs ();
  Temp_tempList dst = NULL, src = NULL;
  emit (AS_Oper (".text", NULL, NULL, NULL));
  emit (AS_Oper (".align 1", NULL, NULL, NULL));
  char *globalstr = calloc (INSTRLEN, sizeof (char));
  sprintf (globalstr, ".global %s", funname);
  emit (AS_Oper (globalstr, NULL, NULL, NULL));
  char *funlabelstr = calloc (INSTRLEN, sizeof (char));
  sprintf (funlabelstr, "%s:", funname);
  emit (AS_Oper (funlabelstr, NULL, NULL, NULL));
  emit (AS_Oper ("push {fp}", NULL, NULL, NULL));
  emit (AS_Oper ("push {fp}", NULL, NULL, NULL));
  emit (AS_Oper ("mov fp, sp", NULL, NULL, NULL));

  dst = NULL;
  src = NULL;
  for (int i = 10; i >= 4; --i)
    src = TL (TRG (i, T_int), src);
  emit (AS_Oper ("push {`s0, `s1, `s2, `s3, `s4, `s5, `s6, lr}", dst, src,
                 NULL));

  dst = NULL;
  src = NULL;
  for (int i = 23; i >= 16; --i)
    src = TL (TRG (i, T_float), src);
  emit (AS_Oper ("vpush {`s0, `s1, `s2, `s3, `s4, `s5, `s6, `s7}", dst, src,
                 NULL));

  dst = NULL;
  src = NULL;
  for (int i = 31; i >= 24; --i)
    src = TL (TRG (i, T_float), src);
  emit (AS_Oper ("vpush {`s0, `s1, `s2, `s3, `s4, `s5, `s6, `s7}", dst, src,
                 NULL));

  char *argstrs[] = { "r0", "r1", "r2", "r3" };
  int i = 0;
  Temp_tempList iter = prolog->u.OPER.src;
  for (; iter != NULL; iter = iter->tail)
    {
      dst = TL (iter->head, NULL);
      src = TL (TRG (i, iter->head->type), NULL);
      char *mvstr = calloc (INSTRLEN, sizeof (char));
      if (iter->head->type == T_int)
        {
          sprintf (mvstr, "mov `d0, `s0");
          emit (AS_Oper (mvstr, dst, src, NULL));
        }
      else
        {
          sprintf (mvstr, "vmov.f32 `d0, `s0");
          emit (AS_Oper (mvstr, dst, src, NULL));
        }
      ++i;
    }
  return iList;
}

AS_instrList
ARM_armbody (LL_instrList il)
{
  // DBGPRT ("dephi begin\n");
  ARM_dePhi (il);
  // DBGPRT ("dePhi'ed ssa il\n");
  // AS_printInstrList (stderr, il, Temp_name ());
  // DBGPRT ("dephi end\n");
  resetILs ();
  for (; il->tail != NULL; il = il->tail)
    {
      LL_instr ins = il->head;
      // DBGPRT ("current ins: ");
      // LL_print (stderr, ins, Temp_name ());
      LL_instr insNxt = il->tail->head;
      AS_type insty = ARM_gettype (ins);
      // DBGPRT ("armbody, insty: %d\n", insty);
      armbodyVtbl[insty](ins, insNxt);
    }
  return iList;
}

AS_instrList
ARM_armepilog (LL_instrList il)
{
  resetILs ();
  return NULL;
}

int
ARM_isPhi (LL_instr instr)
{
  return instr->kind == LL_OPER
         && (strstr (instr->u.OPER.assem, "phi") != NULL)
         && (strstr (instr->u.OPER.assem, "[") != NULL);
}

static char *
_format (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  string buf = (string)checked_malloc (IR_MAXLEN);
  vsprintf (buf, fmt, args);
  va_end (args);
  return buf;
}

void
ARM_dePhi (LL_instrList il)
{
  S_table phiTab = S_empty ();
  /* Find all phi's and insert into the table phiTab */
  for (LL_instrList iliter = il; iliter != NULL; iliter = iliter->tail)
    {
      LL_instr ins = iliter->head;
      if (ARM_isPhi (ins))
        {
          Temp_tempList dst = ins->u.OPER.dst;
          Temp_tempList src = ins->u.OPER.src;
          Temp_labelList labels = ins->u.OPER.jumps->labels;
          while (src != NULL)
            {
              char *labelstr = Temp_labelstring (labels->head);
              phiEntry_t phient = CnstrPhiEntry (dst->head, src->head);
              phiEntry_t existing = S_look (phiTab, S_Symbol (labelstr));
              if (existing == NULL)
                {
                  phiEntry_t hd = CnstrPhiEntry (NULL, NULL);
                  S_enter (phiTab, S_Symbol (labelstr), hd);
                  existing = S_look (phiTab, S_Symbol (labelstr));
                }
              listAdd (&phient->nd, &existing->nd, existing->nd.next);
              src = src->tail;
              labels = labels->tail;
            }
        }
    }

  // DBGPRT ("\nphiTab\n");
  // FOR_IN_S_TABLE (label, phiTab)
  // {
  //   phiEntry_t entry = S_look (phiTab, label);
  //   DBGPRT ("label: \"%s\":\n", S_name (label));
  //   for (listNode_t *node = entry->nd.next; node != &entry->nd;
  //        node = node->next)
  //     {
  //       phiEntry_t pe = container_of (node, struct phiEntry, nd);
  //       DBGPRT ("\t%d <- %d\n", pe->dst->num, pe->src->num);
  //     }
  // }
  // DBGPRT ("\n");

  /* Remove phi's */
  for (LL_instrList iliter = il; iliter != NULL && iliter->tail != NULL;)
    {
      char *blockLabelstr = Temp_labelstring (iliter->head->u.LABEL.label);
      // DBGPRT ("iliter: at %p\n", iliter);
      // LL_printInstrList (stderr, iliter, Temp_name ());
      LL_instrList blockBeforeJmp = NULL;
      while (iliter != NULL)
        {
          LL_instr ins = iliter->tail->head;
          // DBGPRT ("ins: ");
          // LL_print (stderr, ins, Temp_name ());
          if (ARM_gettype (ins) == BR || ARM_gettype (ins) == CJUMP
              || ARM_gettype (ins) == RET)
            {
              blockBeforeJmp = iliter;
              break;
            }
          iliter = iliter->tail;
        }
      iliter = iliter->tail->tail;
      MYASRT (blockBeforeJmp);
      phiEntry_t phient = S_look (phiTab, S_Symbol (blockLabelstr));

      if (phient == NULL)
        continue;
      for (listNode_t *node = phient->nd.next; node != &phient->nd;
           node = node->next)
        {
          phiEntry_t pe = container_of (node, struct phiEntry, nd);
          char *opstr = pe->dst->type == T_int ? "add i64" : "fadd double";
          char *insstr = calloc (INSTRLEN, sizeof (char));
          sprintf (insstr, "%%`d0 = %s `s0, 0", opstr);
          LL_instr pushedupIns
              = LL_Oper (insstr, TL (pe->dst, NULL), TL (pe->src, NULL), NULL);
          blockBeforeJmp->tail
              = LL_InstrList (pushedupIns, blockBeforeJmp->tail);
        }
    }
}

void
ARM_transNone (LL_instr instr, LL_instr instrNxt)
{
  return;
}

void
ARM_transBR (LL_instr instr, LL_instr instrNxt)
{
  char *armstr = calloc (INSTRLEN, sizeof (char));
  sprintf (armstr, "b `j0");
  emit (
      AS_Oper (armstr, NULL, NULL, AS_Targets (instr->u.OPER.jumps->labels)));
}

void
ARM_transRET (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList src = instr->u.OPER.src;
  // DBGPRT ("ARM_transRET instr: ");
  // LL_print (stderr, instr, Temp_name ());
  // MYASRT (src);
  if (src == NULL)
    return;
  if (src->head->type == T_int)
    {
      Temp_tempList armdst = TL (TRG (0, T_int), NULL);
      Temp_tempList armsrc = TL (src->head, NULL);
      char *mvto0str = calloc (INSTRLEN, sizeof (char));
      sprintf (mvto0str, "mov `d0, `s0");
      emit (AS_Oper (mvto0str, armdst, armsrc, NULL));
    }
  else
    {
      Temp_tempList armdst = TL (TRG (0, T_float), NULL);
      Temp_tempList armsrc = TL (src->head, NULL);
      char *mvto0str = calloc (INSTRLEN, sizeof (char));
      sprintf (mvto0str, "vmov.f32 `d0, `s0");
      emit (AS_Oper (mvto0str, armdst, armsrc, NULL));
    }
  /* Recover callee-saved regs and calling-related regs */
  {
    char *recvspstr = calloc (INSTRLEN, sizeof (char));
    sprintf (recvspstr, "sub sp, fp, #96");
    emit (AS_Oper (recvspstr, NULL, NULL, NULL));
  }
  {
    Temp_tempList armdst = NULL, armsrc = NULL;
    for (int i = 31; i >= 24; --i)
      armdst = TL (TRG (i, T_float), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "vpop {`d0, `d1, `d2, `d3, `d4, `d5, `d6, `d7}");
    emit (AS_Oper (popstr, armdst, armsrc, NULL));
  }
  {
    Temp_tempList armdst = NULL, armsrc = NULL;
    for (int i = 23; i >= 16; --i)
      armdst = TL (TRG (i, T_float), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "vpop {`d0, `d1, `d2, `d3, `d4, `d5, `d6, `d7}");
    emit (AS_Oper (popstr, armdst, armsrc, NULL));
  }
  {
    Temp_tempList armdst = NULL, armsrc = NULL;
    for (int i = 10; i >= 4; --i)
      armdst = TL (TRG (i, T_int), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "pop {`d0, `d1, `d2, `d3, `d4, `d5, `d6, lr}");
    emit (AS_Oper (popstr, armdst, armsrc, NULL));
  }
  {
    char *popfpstr = calloc (INSTRLEN, sizeof (char));
    char *bxlrstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popfpstr, "pop {fp}");
    sprintf (popfpstr, "pop {fp}");
    sprintf (bxlrstr, "bx lr");
    emit (AS_Oper (popfpstr, NULL, NULL, NULL));
    emit (AS_Oper (bxlrstr, NULL, NULL, NULL));
  }
}

void
ARM_transADD (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0] + imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "mov `d0, `s0");
    }
  else if (imms.num == 1)
    {
      if (imms.immarr[0] == 0)
        {
          armsrc = TL (src->head, NULL);
          sprintf (armstr, "mov `d0, `s0");
        }
      else
        {
          Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
          armsrc = TL (src->head, TL (immtmp, NULL));
          sprintf (armstr, "add `d0, `s0, `s1");
        }
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "add `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transSUB (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0] - imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "mov `d0, `s0");
    }
  else if (imms.num == 1)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
      armsrc = TL (src->head, TL (immtmp, NULL));
      if (strncmp (instr->u.OPER.assem, "%`d0 = sub i64 %`s0", 19) == 0)
        sprintf (armstr, "sub `d0, `s0, `s1");
      else
        sprintf (armstr, "rsb `d0, `s0, `s1");
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "sub `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transMUL (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0] * imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "mov `d0, `s0");
    }
  else if (imms.num == 1)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
      armsrc = TL (src->head, TL (immtmp, NULL));
      sprintf (armstr, "mul `d0, `s0, `s1");
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "mul `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transDIV (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0] / imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "mov `d0, `s0");
      emit (AS_Oper (armstr, armdst, armsrc, NULL));
    }
  else if (imms.num == 1)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
      char *mvdividendstr = calloc (INSTRLEN, sizeof (char));
      char *mvdivisorstr = calloc (INSTRLEN, sizeof (char));
      sprintf (mvdividendstr, "mov `d0, `s0");
      sprintf (mvdivisorstr, "mov `d0, `s0");
      Temp_tempList dividendsrc = NULL, divisorsrc = NULL;
      Temp_tempList dividenddst = TL (TRG (0, T_int), NULL);
      Temp_tempList divisordst = TL (TRG (1, T_int), NULL);
      if (strncmp (instr->u.OPER.assem, "%`d0 = sdiv i64 %`s0", 20) == 0)
        {
          dividendsrc = TL (src->head, NULL);
          divisorsrc = TL (immtmp, NULL);
        }
      else
        {
          dividendsrc = TL (immtmp, NULL);
          divisorsrc = TL (src->head, NULL);
        }
      emit (AS_Oper (mvdividendstr, dividenddst, dividendsrc, NULL));
      emit (AS_Oper (mvdivisorstr, divisordst, divisorsrc, NULL));
      sprintf (armstr, "blx __aeabi_idiv");
      emit (AS_Oper (armstr, NULL, NULL, NULL));
      char *mvresultstr = calloc (INSTRLEN, sizeof (char));
      sprintf (mvresultstr, "mov `d0, `s0");
      Temp_tempList resultdst = TL (dst->head, NULL);
      Temp_tempList resultsrc = TL (TRG (0, T_int), NULL);
      emit (AS_Oper (mvresultstr, resultdst, resultsrc, NULL));
    }
  else
    {
      MYASRT (imms.num == 0);
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
      char *mvdividendstr = calloc (INSTRLEN, sizeof (char));
      char *mvdivisorstr = calloc (INSTRLEN, sizeof (char));
      sprintf (mvdividendstr, "mov `d0, `s0");
      sprintf (mvdivisorstr, "mov `d0, `s0");
      Temp_tempList dividendsrc = TL (src->head, NULL);
      Temp_tempList divisorsrc = TL (src->tail->head, NULL);
      Temp_tempList dividenddst = TL (TRG (0, T_int), NULL);
      Temp_tempList divisordst = TL (TRG (1, T_int), NULL);
      emit (AS_Oper (mvdividendstr, dividenddst, dividendsrc, NULL));
      emit (AS_Oper (mvdivisorstr, divisordst, divisorsrc, NULL));
      sprintf (armstr, "blx __aeabi_idiv");
      emit (AS_Oper (armstr, NULL, NULL, NULL));
      char *mvresultstr = calloc (INSTRLEN, sizeof (char));
      sprintf (mvresultstr, "mov `d0, `s0");
      Temp_tempList resultdst = TL (dst->head, NULL);
      Temp_tempList resultsrc = TL (TRG (0, T_int), NULL);
      emit (AS_Oper (mvresultstr, resultdst, resultsrc, NULL));
    }
}

void
ARM_transFADD (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = floatImmIntoTemp (imms.immarr[0] + imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "vmov.f32 `d0, `s0");
    }
  else if (imms.num == 1)
    {
      if (imms.immarr[0] == 0)
        {
          armsrc = TL (src->head, NULL);
          sprintf (armstr, "vmov.f32 `d0, `s0");
        }
      else
        {
          Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
          Temp_temp floattmp = TNWT (T_float);
          char *cvtstr = calloc (INSTRLEN, sizeof (char));
          sprintf (cvtstr, "vmov.f32 `d0, `s0");
          emit (
              AS_Oper (cvtstr, TL (floattmp, NULL), TL (inttmp, NULL), NULL));
          armsrc = TL (src->head, TL (floattmp, NULL));
          sprintf (armstr, "vadd.f32 `d0, `s0, `s1");
        }
    }
  else
    {
      // AS_print (stderr, instr, Temp_name ());
      // DBGPRT ("imms.num: %d\n", imms.num);
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "vadd.f32 `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transFSUB (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = floatImmIntoTemp (imms.immarr[0] - imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "vmov.f32 `d0, `s0");
    }
  else if (imms.num == 1)
    {
      Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
      Temp_temp floattmp = TNWT (T_float);
      char *cvtstr = calloc (INSTRLEN, sizeof (char));
      sprintf (cvtstr, "vmov.f32 `d0, `s0");
      emit (AS_Oper (cvtstr, TL (floattmp, NULL), TL (inttmp, NULL), NULL));
      if (strncmp (instr->u.OPER.assem, "`d0 = sub double `s0", 25) == 0)
        armsrc = TL (src->head, TL (floattmp, NULL));
      else
        armsrc = TL (floattmp, TL (src->head, NULL));
      sprintf (armstr, "vsub.f32 `d0, `s0, `s1");
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "vsub.f32 `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transFMUL (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = floatImmIntoTemp (imms.immarr[0] * imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "vmov.f32 `d0, `s0");
    }
  else if (imms.num == 1)
    {
      if (imms.immarr[0] == 0)
        {
          armsrc = TL (src->head, NULL);
          sprintf (armstr, "vmov.f32 `d0, `s0");
        }
      else
        {
          Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
          Temp_temp floattmp = TNWT (T_float);
          char *cvtstr = calloc (INSTRLEN, sizeof (char));
          sprintf (cvtstr, "vmov.f32 `d0, `s0");
          emit (
              AS_Oper (cvtstr, TL (floattmp, NULL), TL (inttmp, NULL), NULL));
          armsrc = TL (src->head, TL (floattmp, NULL));
          sprintf (armstr, "vmul.f32 `d0, `s0, `s1");
        }
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "vmul.f32 `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transFDIV (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (imms.num == 2)
    {
      Temp_temp immtmp = floatImmIntoTemp (imms.immarr[0] / imms.immarr[1]);
      armsrc = TL (immtmp, NULL);
      sprintf (armstr, "vmov.f32 `d0, `s0");
    }
  else if (imms.num == 1)
    {
      Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
      Temp_temp floattmp = TNWT (T_float);
      char *cvtstr = calloc (INSTRLEN, sizeof (char));
      sprintf (cvtstr, "vmov.f32 `d0, `s0");
      emit (AS_Oper (cvtstr, TL (floattmp, NULL), TL (inttmp, NULL), NULL));
      if (strncmp (instr->u.OPER.assem, "`d0 = sub double `s0", 25) == 0)
        armsrc = TL (src->head, TL (floattmp, NULL));
      else
        armsrc = TL (floattmp, TL (src->head, NULL));
      sprintf (armstr, "vdiv.f32 `d0, `s0, `s1");
    }
  else
    {
      MYASRT (imms.num == 0);
      armsrc = TL (src->head, TL (src->tail->head, NULL));
      sprintf (armstr, "vdiv.f32 `d0, `s0, `s1");
    }
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transF2I (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_temp transfer = TNWT (T_float);
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  if (instr->u.OPER.src != NULL || imms.num == 0)
    {
      {
        Temp_tempList armdst = TL (transfer, NULL);
        Temp_tempList armsrc = TL (src->head, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vcvt.s32.f32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
      {
        Temp_tempList armdst = TL (dst->head, NULL);
        Temp_tempList armsrc = TL (transfer, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vmov `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
    }
  else
    {
      Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
      Temp_temp floattmp = TNWT (T_float);
      char *cvtstr = calloc (INSTRLEN, sizeof (char));
      sprintf (cvtstr, "vmov.f32 `d0, `s0");
      emit (AS_Oper (cvtstr, TL (floattmp, NULL), TL (inttmp, NULL), NULL));
      {
        Temp_tempList armdst = TL (transfer, NULL);
        Temp_tempList armsrc = TL (floattmp, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vcvt.s32.f32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
      {
        Temp_tempList armdst = TL (dst->head, NULL);
        Temp_tempList armsrc = TL (transfer, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vmov `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
    }
}

void
ARM_transI2F (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_temp transfer = TNWT (T_float);
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  if (instr->u.OPER.src != NULL || imms.num == 0)
    {
      {
        Temp_tempList armdst = TL (transfer, NULL);
        Temp_tempList armsrc = TL (src->head, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vmov.f32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
      {
        Temp_tempList armdst = TL (dst->head, NULL);
        Temp_tempList armsrc = TL (transfer, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vcvt.f32.s32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
    }
  else
    {
      Temp_temp inttmp = floatImmIntoTemp (imms.immarr[0]);
      {
        Temp_tempList armdst = TL (transfer, NULL);
        Temp_tempList armsrc = TL (inttmp, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vmov.f32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
      {
        Temp_tempList armdst = TL (dst->head, NULL);
        Temp_tempList armsrc = TL (transfer, NULL);
        char *armstr = calloc (INSTRLEN, sizeof (char));
        sprintf (armstr, "vcvt.f32.s32 `d0, `s0");
        emit (AS_Oper (armstr, armdst, armsrc, NULL));
      }
    }
}

void
ARM_transI2P (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = TL (src->head, NULL);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  sprintf (armstr, "mov `d0, `s0");
  emit (AS_Oper (armstr, armdst, armsrc, NULL));
}

void
ARM_transP2I (LL_instr instr, LL_instr instrNxt)
{
  string llasm = instr->u.OPER.assem;
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = NULL;
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (src)
    {
      armsrc = TL (src->head, NULL);
      sprintf (armstr, "mov `d0, `s0");
      emit (AS_Oper (armstr, armdst, armsrc, NULL));
    }
  else
    {
      char *funname = parseFunName4Ptr (llasm);
      sprintf (armstr, "ldr `d0, = %s", funname);
      emit (AS_Oper (armstr, armdst, NULL, NULL));
    }
}

void
ARM_transLOAD (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armdst = TL (dst->head, NULL);
  Temp_tempList armsrc = TL (src->head, NULL);
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (dst->head->type == T_int)
    {
      sprintf (armstr, "ldr `d0, [`s0]");
      emit (AS_Oper (armstr, armdst, armsrc, NULL));
    }
  else
    {
      sprintf (armstr, "vldr.f32 `d0, [`s0]");
      emit (AS_Oper (armstr, armdst, armsrc, NULL));
    }
}

void
ARM_transSTORE (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList src = instr->u.OPER.src;
  Temp_tempList armsrc = TL (src->head, TL (src->tail->head, NULL));
  char *armstr = calloc (INSTRLEN, sizeof (char));
  if (src->head->type == T_int)
    {
      sprintf (armstr, "str `s0, [`s1]");
      emit (AS_Oper (armstr, NULL, armsrc, NULL));
    }
  else
    {
      sprintf (armstr, "vstr.f32 `s0, [`s1]");
      emit (AS_Oper (armstr, NULL, armsrc, NULL));
    }
}

void
ARM_transCALL (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src = instr->u.OPER.src;
  char *funname = parseFunName (instr->u.OPER.assem);

  /* Push caller-saved regs */
  {
    Temp_tempList armsrc = NULL;
    for (int i = 3; i >= 0; --i)
      armsrc = TL (TRG (i, T_int), armsrc);
    char *pushstr = calloc (INSTRLEN, sizeof (char));
    sprintf (pushstr, "push {`s0, `s1, `s2, `s3}");
    emit (AS_Oper (pushstr, NULL, armsrc, NULL));
    // emit (AS_Oper (pushstr, NULL, NULL, NULL));
  }
  {
    Temp_tempList armsrc = NULL;
    for (int i = 7; i >= 0; --i)
      armsrc = TL (TRG (i, T_float), armsrc);
    char *pushstr = calloc (INSTRLEN, sizeof (char));
    sprintf (pushstr, "vpush {`s0, `s1, `s2, `s3, `s4, `s5, `s6, `s7}");
    emit (AS_Oper (pushstr, NULL, armsrc, NULL));
    // emit (AS_Oper (pushstr, NULL, NULL, NULL));
  }
  {
    Temp_tempList armsrc = NULL;
    for (int i = 15; i >= 8; --i)
      armsrc = TL (TRG (i, T_float), armsrc);
    char *pushstr = calloc (INSTRLEN, sizeof (char));
    sprintf (pushstr, "vpush {`s0, `s1, `s2, `s3, `s4, `s5, `s6, `s7}");
    emit (AS_Oper (pushstr, NULL, armsrc, NULL));
    // emit (AS_Oper (pushstr, NULL, NULL, NULL));
  }

  /* Call the function */
  if (funname == NULL)
    {
      Temp_tempList args = src->tail;
      int argsnum = parseFunArgs (instr->u.OPER.assem, args);
      Temp_tempList armsrc = NULL;
      Temp_tempList calldst = TL (TRG (0, T_int), TL (TRG (0, T_float), NULL));
      Temp_tempList callsrc = NULL;
      for (int i = argsnum - 1; i >= 0; --i)
        {
          callsrc = TL (TRG (i, T_int), callsrc);
          callsrc = TL (TRG (i, T_float), callsrc);
        }
      armsrc = TL (src->head, callsrc);
      char *blxstr = calloc (INSTRLEN, sizeof (char));
      sprintf (blxstr, "blx `s0");
      emit (AS_Oper (blxstr, calldst, armsrc, NULL));
    }
  else
    {
      int argsnum = parseFunArgs (instr->u.OPER.assem, src);
      Temp_tempList calldst = TL (TRG (0, T_int), TL (TRG (0, T_float), NULL));
      Temp_tempList callsrc = NULL;
      for (int i = argsnum - 1; i >= 0; --i)
        {
          callsrc = TL (TRG (i, T_int), callsrc);
          callsrc = TL (TRG (i, T_float), callsrc);
        }
      char *blxstr = calloc (INSTRLEN, sizeof (char));
      sprintf (blxstr, "blx %s", funname);
      emit (AS_Oper (blxstr, calldst, callsrc, NULL));
    }

  if (dst != NULL)
    {
      if (dst->head->type == T_int)
        {
          Temp_tempList armdst = TL (dst->head, NULL);
          Temp_tempList armsrc = TL (TRG (0, T_int), NULL);
          char *mvrsltstr = calloc (INSTRLEN, sizeof (char));
          sprintf (mvrsltstr, "mov `d0, `s0");
          emit (AS_Oper (mvrsltstr, armdst, armsrc, NULL));
        }
      else
        {
          Temp_tempList armdst = TL (dst->head, NULL);
          Temp_tempList armsrc = TL (TRG (0, T_float), NULL);
          char *mvrsltstr = calloc (INSTRLEN, sizeof (char));
          sprintf (mvrsltstr, "vmov.f32 `d0, `s0");
          emit (AS_Oper (mvrsltstr, armdst, armsrc, NULL));
        }
    }

  /* Pop back caller-saved regs */
  {
    Temp_tempList armdst = NULL;
    for (int i = 15; i >= 8; --i)
      armdst = TL (TRG (i, T_float), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "vpop {`d0, `d1, `d2, `d3, `d4, `d5, `d6, `d7}");
    emit (AS_Oper (popstr, armdst, NULL, NULL));
  }
  {
    Temp_tempList armdst = NULL;
    for (int i = 7; i >= 0; --i)
      armdst = TL (TRG (i, T_float), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "vpop {`d0, `d1, `d2, `d3, `d4, `d5, `d6, `d7}");
    emit (AS_Oper (popstr, armdst, NULL, NULL));
  }
  {
    Temp_tempList armdst = NULL;
    for (int i = 3; i >= 0; --i)
      armdst = TL (TRG (i, T_int), armdst);
    char *popstr = calloc (INSTRLEN, sizeof (char));
    sprintf (popstr, "pop {`d0, `d1, `d2, `d3}");
    emit (AS_Oper (popstr, armdst, NULL, NULL));
  }
}

void
oldICMP (LL_instr instr, LL_instr instrNxt)
{
  // DBGPRT ("oldICMP, instr: ");
  // LL_print (stderr, instr, Temp_name ());
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src;
  intarrNumPair_t imms = parseIntImms (instr->u.OPER.assem);
  char *condstr = parseCond (instr->u.OPER.assem);
  if (imms.num == 2)
    {
      Temp_temp immtmp1 = intImmIntoTemp (imms.immarr[0]);
      Temp_temp immtmp2 = intImmIntoTemp (imms.immarr[1]);
      src = TL (immtmp1, TL (immtmp2, NULL));
    }
  else if (imms.num == 1)
    {
      Temp_temp immtmp = intImmIntoTemp (imms.immarr[0]);
      /* %`d0 = icmp CND i64 %`s0, 123
       *                     ^
       *                     12+len(CND)+5 */
      if (strncmp (&instr->u.OPER.assem[12 + strlen (condstr) + 1 + 5], "%`s0",
                   4)
          == 0)
        {
          src = TL (instr->u.OPER.src->head, TL (immtmp, NULL));
        }
      else
        {
          src = TL (immtmp, TL (instr->u.OPER.src->head, NULL));
        }
    }
  else
    {
      MYASRT (imms.num == 0);
      src = instr->u.OPER.src;
    }
  AS_targets jmps = AS_Targets (instrNxt->u.OPER.jumps->labels);

  Temp_tempList cmpsrc = TL (src->head, TL (src->tail->head, NULL));
  char *cmpstr = calloc (INSTRLEN, sizeof (char));
  sprintf (cmpstr, "cmp `s0, `s1");
  emit (AS_Oper (cmpstr, NULL, cmpsrc, NULL));

  char *brstr = calloc (INSTRLEN, sizeof (char));
  sprintf (brstr, "b%s `j0", condstr);
  emit (AS_Oper (brstr, NULL, NULL, jmps));

  AS_targets elsejmps
      = AS_Targets (Temp_LabelList (jmps->labels->tail->head, NULL));
  char *bstr = calloc (INSTRLEN, sizeof (char));
  sprintf (bstr, "b `j0");
  emit (AS_Oper (bstr, NULL, NULL, elsejmps));
}

static LL_instr prevCmpIns = NULL;

void
oldFCMP (LL_instr instr, LL_instr instrNxt)
{
  Temp_tempList dst = instr->u.OPER.dst;
  Temp_tempList src;
  AS_targets jmps = AS_Targets (instrNxt->u.OPER.jumps->labels);
  char *condstr = parseCond (instr->u.OPER.assem);
  floatarrNumPair_t imms = parseFloatImms (instr->u.OPER.assem);
  if (imms.num == 2)
    {
      Temp_temp immtmp1 = floatImmIntoTemp (imms.immarr[0]);
      Temp_temp immtmp2 = floatImmIntoTemp (imms.immarr[1]);
      src = TL (immtmp1, TL (immtmp2, NULL));
    }
  else if (imms.num == 1)
    {
      Temp_temp immtmp = floatImmIntoTemp (imms.immarr[0]);
      /* %`d0 = fcmp CND double %`s0, 123
       *                        ^
       *                        12+len(CND)+8 */
      if (strncmp (&instr->u.OPER.assem[12 + strlen (condstr) + 1 + 8], "%`s0",
                   4)
          == 0)
        src = TL (instr->u.OPER.src->head, TL (immtmp, NULL));
      else
        src = TL (immtmp, TL (instr->u.OPER.src->head, NULL));
    }
  else
    {
      MYASRT (imms.num == 0);
      src = instr->u.OPER.src;
    }

  Temp_tempList cmpsrc = TL (src->head, TL (src->tail->head, NULL));
  char *cmpstr = calloc (INSTRLEN, sizeof (char));
  sprintf (cmpstr, "vcmp.f32 `s0, `s1");
  emit (AS_Oper (cmpstr, NULL, cmpsrc, NULL));

  emit (AS_Oper ("vmrs APSR_nzcv, FPSCR", NULL, NULL, NULL));

  char *brstr = calloc (INSTRLEN, sizeof (char));
  sprintf (brstr, "b%s `j0", condstr);
  emit (AS_Oper (brstr, NULL, NULL, jmps));

  AS_targets elsejmps
      = AS_Targets (Temp_LabelList (jmps->labels->tail->head, NULL));
  char *bstr = calloc (INSTRLEN, sizeof (char));
  sprintf (bstr, "b `j0");
  emit (AS_Oper (bstr, NULL, NULL, elsejmps));
}

void
ARM_transICMP (LL_instr instr, LL_instr instrNxt)
{
  if (prevCmpIns != NULL)
    MYASRT (0 && "continuous cmp's without br in between");
  prevCmpIns = instr;
}

void
ARM_transFCMP (LL_instr instr, LL_instr instrNxt)
{
  if (prevCmpIns != NULL)
    MYASRT (0 && "continuous cmp's without br in between");
  prevCmpIns = instr;
}

void
ARM_transLABEL (LL_instr instr, LL_instr instrNxt)
{
  emit (AS_Label (instr->u.LABEL.assem, instr->u.LABEL.label));
}

void
ARM_transCJUMP (LL_instr instr, LL_instr instrNxt)
{
  if (prevCmpIns == NULL)
    MYASRT (0 && "cjump with previous cmp missing");
  LL_instr prvins = prevCmpIns;
  prevCmpIns = NULL;
  LL_instr insnxt = instr;
  AS_type t = ARM_gettype (prvins);
  if (t == ICMP)
    oldICMP (prvins, insnxt);
  else if (t == FCMP)
    oldFCMP (prvins, insnxt);
  else
    MYASRT (0 && "previous cmp not ICMP/FCMP");
}

ARM_immfmt_t
int2fmt (int i)
{
  ARM_immfmt_t ret = { .i = i };
  return ret;
}

ARM_immfmt_t
float2fmt (float f)
{
  ARM_immfmt_t ret = { .f = f };
  return ret;
}

void
listInit (listNode_t *nd)
{
  nd->prev = nd;
  nd->next = nd;
}

void
listAdd (listNode_t *nd, listNode_t *prev, listNode_t *next)
{
  prev->next = nd;
  nd->prev = prev;
  nd->next = next;
  next->prev = nd;
}

phiEntry_t
CnstrPhiEntry (Temp_temp dst, Temp_temp src)
{
  phiEntry_t ret = calloc (1, sizeof (struct phiEntry));
  ret->dst = dst;
  ret->src = src;
  listInit (&ret->nd);
  return ret;
}

static void
emit (AS_instr inst)
{
  if (last)
    last = last->tail = AS_InstrList (inst, NULL);
  else
    last = iList = AS_InstrList (inst, NULL);
}

static void
resetILs ()
{
  iList = NULL;
  last = NULL;
}

static Temp_temp
intImmIntoTemp (int i)
{
  Temp_temp tmp = TNWT (T_int);
  ARM_immfmt_t fmt = int2fmt (i);
  char *mvlostr = calloc (INSTRLEN, sizeof (char));
  sprintf (mvlostr, "movw `d0, #%d", fmt.parts.lo);
  emit (AS_Oper (mvlostr, TL (tmp, NULL), NULL, NULL));
  if (fmt.parts.hi != 0)
    {
      char *mvhistr = calloc (INSTRLEN, sizeof (char));
      sprintf (mvhistr, "movt `d0, #%d", fmt.parts.hi);
      emit (AS_Oper (mvhistr, TL (tmp, NULL), NULL, NULL));
    }
  return tmp;
}

static Temp_temp
floatImmIntoTemp (float f)
{
  Temp_temp tmp = TNWT (T_int);
  ARM_immfmt_t fmt = float2fmt (f);
  char *mvlostr = calloc (INSTRLEN, sizeof (char));
  char *mvhistr = calloc (INSTRLEN, sizeof (char));
  sprintf (mvlostr, "movw `d0, #%d", fmt.parts.lo);
  sprintf (mvhistr, "movt `d0, #%d", fmt.parts.hi);
  emit (AS_Oper (mvlostr, TL (tmp, NULL), NULL, NULL));
  emit (AS_Oper (mvhistr, TL (tmp, NULL), NULL, NULL));
  return tmp;
}

static inline Temp_tempList
TL (Temp_temp tmp, Temp_tempList tl)
{
  return Temp_TempList (tmp, tl);
}

static inline Temp_temp
TNWT (T_type ty)
{
  return Temp_newtemp (ty);
}

static inline Temp_temp
TRG (int num, T_type ty)
{
  return Temp_reg (num, ty);
}

static char *
parseFunName (char asmstr[])
{
  char *namebeg = strchr (asmstr, '@');
  char *nameend = strchr (asmstr, '(');
  if (namebeg == NULL)
    return NULL;
  MYASRT (nameend);
  char *ret = calloc (nameend - namebeg + 16, sizeof (char));
  strncpy (ret, namebeg + 1, nameend - namebeg - 1);
  return ret;
}

static char *
parseFunName4Ptr (char asmstr[])
{
  char *cpy = calloc (strlen (asmstr) + 16, sizeof (char));
  strcpy (cpy, asmstr);
  char *tok = strtok (cpy, " ");
  while (tok != NULL)
    {
      if (tok[0] == '@')
        break;
      tok = strtok (NULL, " ");
    }
  ++tok;
  char *ret = calloc (strlen (tok) + 16, sizeof (char));
  strcpy (ret, tok);
  return ret;
}

static int
parseFunArgs (char asmstr[], Temp_tempList args)
{
  char *argbeg = strchr (asmstr, '(');
  char *argend = strchr (asmstr, ')');
  char *cpy = calloc (argend - argbeg + 16, sizeof (char));
  strncpy (cpy, argbeg + 1, argend - argbeg - 1);
  char *tok = strtok (cpy, ", ");
  int idx = 0;
  while (tok != NULL)
    {
      char *tystr = tok;
      tok = strtok (NULL, ", ");
      char *argstr = tok;
      tok = strtok (NULL, ", ");
      if (argstr[0] == '%')
        {
          Temp_tempList dst = TL (TRG (idx, args->head->type), NULL);
          Temp_tempList src = TL (args->head, NULL);
          char *mvstr = calloc (INSTRLEN, sizeof (char));
          if (args->head->type == T_int)
            {
              sprintf (mvstr, "mov `d0, `s0");
              emit (AS_Oper (mvstr, dst, src, NULL));
            }
          else
            {
              sprintf (mvstr, "vmov.f32 `d0, `s0");
              emit (AS_Oper (mvstr, dst, src, NULL));
            }
          args = args->tail;
        }
      else if (strcmp (tystr, "i64") == 0)
        {
          Temp_tempList dst = TL (TRG (idx, T_int), NULL);
          Temp_temp imm = intImmIntoTemp (atoi (argstr));
          Temp_tempList src = TL (imm, NULL);
          char *mvstr = calloc (INSTRLEN, sizeof (char));
          sprintf (mvstr, "mov `d0, `s0");
          emit (AS_Oper (mvstr, dst, src, NULL));
        }
      else if (strcmp (tystr, "double") == 0)
        {
          Temp_tempList dst = TL (TRG (idx, T_float), NULL);
          Temp_temp imm = intImmIntoTemp (atoi (argstr));
          Temp_tempList src = TL (imm, NULL);
          char *mvstr = calloc (INSTRLEN, sizeof (char));
          sprintf (mvstr, "vmov.f32 `d0, `s0");
          emit (AS_Oper (mvstr, dst, src, NULL));
        }
      else
        MYASRT (0);
      ++idx;
    }
  return idx;
}

static intarrNumPair_t
parseIntImms (char asmstr[])
{
  intarrNumPair_t ret = {
    .immarr = calloc (IMMNUM, sizeof (int)),
    .num = 0,
  };
  char *cpy = calloc (strlen (asmstr) + 1, sizeof (char));
  strcpy (cpy, asmstr);
  char *tok = strtok (cpy, " ");
  while (tok != NULL)
    {
      if (isdigit (tok[0]) || tok[0] == '-')
        {
          ret.immarr[ret.num] = atoi (tok);
          ++ret.num;
          MYASRT (ret.num <= IMMNUM);
        }
      tok = strtok (NULL, " ");
    }
  return ret;
}

static floatarrNumPair_t
parseFloatImms (char asmstr[])
{
  floatarrNumPair_t ret = {
    .immarr = calloc (IMMNUM, sizeof (float)),
    .num = 0,
  };
  char *cpy = calloc (strlen (asmstr) + 1, sizeof (char));
  strcpy (cpy, asmstr);
  char *tok = strtok (cpy, " ");
  while (tok != NULL)
    {
      /* All float imms we could meet are of the form X.Y, X nonempty */
      if (isdigit (tok[0]) || tok[0] == '-')
        {
          ret.immarr[ret.num] = atof (tok);
          // DBGPRT ("floattok: %s\n", tok);
          ++ret.num;
          MYASRT (ret.num <= IMMNUM);
        }
      tok = strtok (NULL, " ");
    }
  return ret;
}

static char *
parseCond (char asmstr[])
{
  // DBGPRT ("parseCond asmstr: %s\n", asmstr);
  char *cpy = calloc (strlen (asmstr) + 1, sizeof (char));
  strcpy (cpy, asmstr);
  char *tok = strtok (cpy, " ");
  // DBGPRT ("cond tok: %s\n", tok);
  while (tok != NULL)
    {
      // DBGPRT ("cond tok: %s\n", tok);
      if (strcmp (tok, "icmp") == 0 || strcmp (tok, "fcmp") == 0)
        break;
      tok = strtok (NULL, " ");
    }
  tok = strtok (NULL, " ");
  char *ret = calloc (4, sizeof (char));
  int toklen = strlen (tok);
  ret[0] = tok[toklen - 2];
  ret[1] = tok[toklen - 1];
  ret[2] = '\0';
  return ret;
}

AS_type
ARM_gettype (LL_instr ins)
{
  AS_type ret = NONE;
  string assem = ins->u.OPER.assem;
  if (ins->kind == I_MOVE)
    {
      assem = ins->u.MOVE.assem;
    }
  else if (ins->kind == I_LABEL)
    {
      ret = LABEL;
      return ret;
    }
  if (!strncmp (assem, "%`d0 = phi", 10))
    {
      ret = PHI;
      return ret;
    }
  else if (!strncmp (assem, "br i1", 5))
    {
      ret = CJUMP;
      return ret;
    }
  else if (!strncmp (assem, "br", 2))
    {
      ret = BR;
      return ret;
    }
  else if (!strncmp (assem, "ret", 3))
    {
      ret = RET;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fadd", 11))
    {
      ret = FADD;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = add", 10))
    {
      ret = ADD;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fsub", 11))
    {
      ret = FSUB;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = sub", 10))
    {
      ret = SUB;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fmul", 11))
    {
      ret = FMUL;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = mul", 10))
    {
      ret = MUL;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fdiv", 11))
    {
      ret = FDIV;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = sdiv", 11))
    {
      ret = DIV;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fptosi", 13))
    {
      ret = F2I;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = sitofp", 13))
    {
      ret = I2F;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = inttoptr", 15))
    {
      ret = I2P;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = load", 11))
    {
      ret = LOAD;
      return ret;
    }
  else if (!strncmp (assem, "store", 5))
    {
      ret = STORE;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = ptrtoint", 15))
    {
      ret = P2I;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = call", 11))
    {
      ret = CALL;
      return ret;
    }
  else if (!strncmp (assem, "call", 4))
    {
      ret = CALL;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = icmp", 11))
    {
      ret = ICMP;
      return ret;
    }
  else if (!strncmp (assem, "%`d0 = fcmp", 11))
    {
      ret = FCMP;
      return ret;
    }
  return ret;
}
