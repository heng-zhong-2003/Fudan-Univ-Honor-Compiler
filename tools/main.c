#include "armgen.h"
#include "assem.h"
#include "assemflowgraph.h"
#include "assemig.h"
#include "assemliveness.h"
#include "canon.h"
#include "dbg.h"
#include "fdmjast.h"
#include "graph.h"
#include "llvm_print_ins.h"
#include "llvmgen.h"
#include "llvmir.h"
#include "llvmirbg.h"
#include "llvmirblock.h"
#include "llvmirflowgraph.h"
#include "llvmirliveness.h"
#include "lxml.h"
#include "parser.h"
#include "print_ast.h"
#include "print_irp.h"
#include "print_src.h"
#include "print_stm.h"
#include "regalloc.h"
#include "semant.h"
#include "ssa.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"
#include "xml2ins.h"
#include <stdio.h>
#include <stdlib.h>

#define FILENAMELEN 128

A_prog root;

extern int yyparse ();

static LL_blockList instrList2BL (LL_instrList il);
static void print_to_ssa_file (string file_ssa, LL_instrList il);

int
main (int argc, char *argv[])
{
  if (argc != 3)
    exit (1);
  int archSize = atoi (argv[1]);
  // DBGPRT ("arch size: %d\n", archSize);
  char *file = calloc (FILENAMELEN, sizeof (char));
  sprintf (file, "%s", argv[2]);
  char *fileFmj = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileFmj, "%s.fmd", file);
  char *fileSrc = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileSrc, "%s.1.src", file);
  char *fileAst = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileAst, "%s.2.ast", file);
  char *fileIrp = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileIrp, "%s.3.irp", file);
  char *fileStm = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileStm, "%s.4.stm", file);
  char *fileIns = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileIns, "%s.5.ins", file);
  char *fileInsXML = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileInsXML, "%s.6.ins", file);
  char *fileCfg = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileCfg, "%s.7.cfg", file);
  char *fileSSA = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileSSA, "%s.8.ssa", file);
  char *fileArm = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileArm, "%s.9.arm", file);
  char *fileRpi = calloc (FILENAMELEN, sizeof (char));
  sprintf (fileRpi, "%s.10.s", file);

  {
    yyparse ();
  }

  {
    T_funcDeclList fdl = transA_Prog (stderr, root, archSize);
    Temp_resetlabel ();
    freopen (fileInsXML, "a", stdout);
    fprintf (stdout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n");
    fflush (stdout);
    fclose (stdout);

    while (fdl)
      {
        freopen (fileInsXML, "a", stdout);
        fprintf (stdout, "<function name=\"%s\">\n", fdl->head->name);
        fflush (stdout);
        fclose (stdout);

        T_stm s = fdl->head->stm;
        freopen (fileStm, "a", stdout);
        fprintf (stdout, "------Original IR Tree------\n");
        printIRP_set (IRP_parentheses);
        printIRP_FuncDecl (stdout, fdl->head);
        fprintf (stdout, "\n\n");
        fflush (stdout);
        fclose (stdout);

        T_stmList sl = C_linearize (s);
        freopen (fileStm, "a", stdout);
        fprintf (stdout, "------Linearized IR Tree------\n");
        printStm_StmList (stdout, sl, 0);
        fprintf (stdout, "\n\n");
        fflush (stdout);
        fclose (stdout);

        struct C_block b = C_basicBlocks (sl);
        freopen (fileStm, "a", stdout);
        fprintf (stdout, "------Basic Blocks------\n");
        for (C_stmListList sll = b.stmLists; sll; sll = sll->tail)
          {
            fprintf (stdout, "For Label=%s\n",
                     S_name (sll->head->head->u.LABEL));
            printStm_StmList (stdout, sll->head, 0);
          }
        fprintf (stdout, "\n\n");
        fflush (stdout);
        fclose (stdout);

        sl = C_traceSchedule (b);
        freopen (fileStm, "a", stdout);
        fprintf (stdout, "------Canonical IR Tree------\n");
        printStm_StmList (stdout, sl, 0);
        fprintf (stdout, "\n\n");
        fflush (stdout);
        fclose (stdout);

        b = C_basicBlocks (sl);

        LL_instrList prologil = llvmprolog (fdl->head->name, fdl->head->args,
                                            fdl->head->ret_type);
        LL_blockList bodybl = NULL;
        for (C_stmListList sll = b.stmLists; sll; sll = sll->tail)
          {
            LL_instrList bil = llvmbody (sll->head);
            LL_blockList bbl = LL_BlockList (LL_Block (bil), NULL);
            bodybl = LL_BlockSplice (bodybl, bbl);
          }
        LL_instrList epilogil = llvmepilog (b.label);

        G_nodeList bg = LL_Create_bg (bodybl);
        freopen (fileIns, "a", stdout);
        fprintf (stdout, "\n------For function ----- %s\n\n", fdl->head->name);
        fprintf (stdout, "------Basic Block Graph---------\n");
        LL_Show_bg (stdout, bg);
        LL_instrList il = LL_traceSchedule (bodybl, prologil, epilogil, FALSE);
        printf ("------~Final traced AS instructions ---------\n");
        LL_printInstrList (stdout, il, Temp_name ());
        fflush (stdout);
        fclose (stdout);

        freopen (fileInsXML, "a", stdout);
        LL_print_xml (stdout, il, Temp_name ());
        fprintf (stdout, "</function>\n");
        fflush (stdout);
        fclose (stdout);

        fdl = fdl->tail;
      }
    freopen (fileInsXML, "a", stdout);
    fprintf (stdout, "</root>\n");
    fflush (stdout);
    fclose (stdout);
  }

  {
    XMLDocument doc;

    if (XMLDocument_load (&doc, fileInsXML))
      {
        if (!doc.root)
          {
            fprintf (stderr, "Error: Invalid ins XML file\n");
            return -1;
          }
        if (doc.root->children.size == 0)
          {
            fprintf (stderr, "Error: Nothing in the ins XML file\n");
            return -1;
          }
      }
    else
      MYASRT (0 && "XMLDocument_load fail");
    // DBGPRT ("load ins xml done\n");
    XMLNode *insroot = doc.root->children.data[0]; // this is the <root> node
    if (!insroot || insroot->children.size == 0)
      {
        fprintf (stderr, "Error: No function in the ins XML file\n");
        return -1;
      }

    LL_instrList inslist_func;

    for (int i = 0; i < insroot->children.size; i++)
      {
        XMLNode *fn = insroot->children.data[i];
        if (!fn || strcmp (fn->tag, "function"))
          {
            fprintf (stderr, "Error: Invalid function in the ins XML file\n");
            return -1;
          }
        // read from xml to an AS_instrList
        inslist_func = insxml2func (fn);

        // get the prologi and epilogi from the inslist_func
        // remove them from the inslist_func, and form the bodyil = instruction
        // body of the function
        LL_instr prologi = inslist_func->head;
        LL_instrList bodyil = inslist_func->tail;
        inslist_func->tail = NULL; // remove the prologi from the inslist_func
        LL_instrList til = bodyil;
        LL_instr epilogi;
        if (til->tail == NULL)
          {
            epilogi = til->head;
            bodyil = NULL;
          }
        else
          {
            while (til->tail->tail != NULL)
              {
                til = til->tail;
              }
            epilogi = til->tail->head;
            til->tail = NULL;
          }

        /* doing the control graph and print to *.8.cfg*/
        // get the control flow and print out the control flow graph to *.8.cfg
        G_graph fg = LL_FG_AssemFlowGraph (bodyil);
        freopen (fileCfg, "a", stdout);
        fprintf (stdout, "------Flow Graph------\n");
        fflush (stdout);
        G_show (stdout, G_nodes (fg), (void *)LL_FG_show);
        fflush (stdout);
        fclose (stdout);

        // data flow analysis
        freopen (fileCfg, "a", stdout);
        G_nodeList lg = LL_Liveness (G_nodes (fg));
        freopen (fileCfg, "a", stdout);
        fprintf (stdout, "/* ------Liveness Graph------*/\n");
        LL_Show_Liveness (stdout, lg);
        fflush (stdout);
        fclose (stdout);

        G_nodeList bg = LL_Create_bg (
            instrList2BL (bodyil)); // create a basic block graph
        freopen (fileCfg, "a", stdout);
        fprintf (stdout, "------Basic Block Graph------\n");
        LL_Show_bg (stdout, bg);
        fprintf (stdout, "\n\n");
        fflush (stdout);
        fclose (stdout);

        LL_instrList bodyil_in_SSA = LL_instrList_to_SSA (bodyil, lg, bg);

        // print the AS_instrList to the ssa file`
        LL_instrList finalssa
            = LL_splice (LL_InstrList (prologi, bodyil_in_SSA),
                         LL_InstrList (epilogi, NULL));
        print_to_ssa_file (fileSSA, finalssa);
        // DBGPRT ("ssa finish\n");

        AS_instrList prolog_in_ARM
            = ARM_armprolog (LL_InstrList (prologi, NULL));
        AS_instrList bodyil_in_ARM = ARM_armbody (bodyil_in_SSA);
        AS_instrList epilog_in_ARM
            = ARM_armepilog (LL_InstrList (epilogi, NULL));

        // print the AS_instrList to the arm file
        AS_instrList finalarm = AS_splice (
            AS_splice (prolog_in_ARM, bodyil_in_ARM), epilog_in_ARM);
        freopen (fileArm, "a", stdout);
        AS_printInstrList (stdout, finalarm, Temp_name ());
        fclose (stdout);
        // DBGPRT ("arm with temp finish\n");

        fg = AS_FG_AssemFlowGraph (finalarm);
        lg = AS_Liveness (G_nodes (fg));
        G_nodeList ig = AS_Create_ig (lg);

        // freopen (_file_liv, "a", stdout);
        // fprintf (stdout, "\n==========================\n");
        // fprintf (stdout, "\n--------flow graph--------\n");
        // G_show (stdout, G_nodes (fg), (void *)FG_show);
        // fprintf (stdout, "\n------liveness graph------\n");
        // Show_Liveness (stdout, lg);
        // fprintf (stdout, "\n----interference graph----\n");
        // Show_ig (stdout, ig);
        // fprintf (stdout, "\n==========================\n");
        // fclose (stdout);

        REG_allocRet_t alloc = REG_regalloc (finalarm, ig, NULL);
        freopen (fileRpi, "a", stdout);
        AS_printInstrList (stdout, alloc.il, alloc.map);
        fclose (stdout);
        // DBGPRT ("regalloced arm finish\n");
      }
  }

  freopen (fileSSA, "a", stdout);
  fprintf (stdout, "declare void @starttime()\n");
  fprintf (stdout, "declare void @stoptime()\n");
  fprintf (stdout, "declare i64* @malloc(i64)\n");
  fprintf (stdout, "declare void @putch(i64)\n");
  fprintf (stdout, "declare void @putint(i64)\n");
  fprintf (stdout, "declare void @putfloat(double)\n");
  fprintf (stdout, "declare i64 @getint()\n");
  fprintf (stdout, "declare float @getfloat()\n");
  fprintf (stdout, "declare i64* @getarray(i64)\n");
  fprintf (stdout, "declare i64* @getfarray(i64)\n");
  fprintf (stdout, "declare void @putarray(i64, i64*)\n");
  fprintf (stdout, "declare void @putfarray(i64, i64*)\n");
  fclose (stdout);

  freopen (fileArm, "a", stdout);
  fprintf (stdout, ".global malloc\n");
  fprintf (stdout, ".global getint\n");
  fprintf (stdout, ".global getch\n");
  fprintf (stdout, ".global getfloat\n");
  fprintf (stdout, ".global getarray\n");
  fprintf (stdout, ".global getfarray\n");
  fprintf (stdout, ".global putint\n");
  fprintf (stdout, ".global putch\n");
  fprintf (stdout, ".global putfloat\n");
  fprintf (stdout, ".global putarray\n");
  fprintf (stdout, ".global putfarray\n");
  fprintf (stdout, ".global starttime\n");
  fprintf (stdout, ".global stoptime\n");
  fclose (stdout);

  freopen (fileRpi, "a", stdout);
  fprintf (stdout, ".global malloc\n");
  fprintf (stdout, ".global getint\n");
  fprintf (stdout, ".global getch\n");
  fprintf (stdout, ".global getfloat\n");
  fprintf (stdout, ".global getarray\n");
  fprintf (stdout, ".global getfarray\n");
  fprintf (stdout, ".global putint\n");
  fprintf (stdout, ".global putch\n");
  fprintf (stdout, ".global putfloat\n");
  fprintf (stdout, ".global putarray\n");
  fprintf (stdout, ".global putfarray\n");
  fprintf (stdout, ".global starttime\n");
  fprintf (stdout, ".global stoptime\n");
  fclose (stdout);
  return 0;
}

static LL_blockList
instrList2BL (LL_instrList il)
{
  LL_instrList b = NULL;
  LL_blockList bl = NULL;
  LL_instrList til = il;

  while (til)
    {
      if (til->head->kind == LL_LABEL)
        {
          if (b)
            { // if we have a label but the current block is not empty, then we
              // have to stop the block
              Temp_label l = til->head->u.LABEL.label;
              b = LL_splice (
                  b, LL_InstrList (
                         LL_Oper (String ("br label %`j0"), NULL, NULL,
                                  LL_Targets (Temp_LabelList (l, NULL))),
                         NULL));
              // DBGPRT ("new br label: ");
              // LL_print (stderr,
              //           LL_Oper (String ("br label %`j0"), NULL, NULL,
              //                    LL_Targets (Temp_LabelList (l, NULL))),
              //           Temp_name ());
              // add a jump to the block to be stopped, only for LLVM IR
              bl = LL_BlockSplice (
                  bl, LL_BlockList (LL_Block (b),
                                    NULL)); // add the block to the block list
              b = NULL;                     // start a new block
            }
        }

      assert (b || til->head->kind == LL_LABEL); // if not a label to start a
                                                 // block, something's wrong!
      b = LL_splice (b, LL_InstrList (til->head, NULL));

      if (til->head->kind == LL_OPER
          && ((til->head->u.OPER.jumps && til->head->u.OPER.jumps->labels)
              || (!strcmp (til->head->u.OPER.assem, "ret i64 -1")
                  || !strcmp (til->head->u.OPER.assem, "ret double -1.0"))))
        {
          bl = LL_BlockSplice (
              bl,
              LL_BlockList (LL_Block (b), NULL)); // got a jump, stop a block
          b = NULL;                               // and start a new block
        }
      til = til->tail;
    }
  return bl;
}

static void
print_to_ssa_file (string file_ssa, LL_instrList il)
{
  freopen (file_ssa, "a", stdout);
  LL_printInstrList (stdout, il, Temp_name ());
  fclose (stdout);
}
