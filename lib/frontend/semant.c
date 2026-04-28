#include "semant.h"
#include "dbg.h"
#include "env.h"
#include "initcenv.h"
#include "symbol.h"
#include "table.h"
#include "translate.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int SEM_ARCH_SIZE; // to be set by arch_size in transA_Prog
extern int globaloff;
static S_table cenv = NULL, venv = NULL, varoff = NULL, methoff = NULL;
// label stacks for "while" statements
// push: S_enter (while[test/end]s, whl[Tst/End]Sb, xxx)
// pop: S_look (while[test/end]s, whl[Tst/End]Sb) (look if need value),
//   then TAB_pop (while[test/end]s)
static S_table whiletests = NULL;
static S_table whileends = NULL;
static char *cur_class_id = NULL;
static char *cur_method_id = NULL;
static S_symbol whlTstSb = NULL;
static S_symbol whlEndSb = NULL;

typedef struct Sem_tyCatTr_ *Sem_tyCatTr; // Ty_ty and value category pair
typedef enum
{
  Sem_L,
  Sem_R
} Sem_valCat; // value category (L/R value)

struct Sem_tyCatTr_
{
  Ty_ty type;
  Sem_valCat cat;
  Tr_exp tr;
};

Sem_tyCatTr
Sem_tyLval (Ty_ty ty, Tr_exp trex)
{
  Sem_tyCatTr ret = checked_malloc (sizeof (*ret));
  ret->type = ty;
  ret->cat = Sem_L;
  ret->tr = trex;
  return ret;
}

Sem_tyCatTr
Sem_tyRval (Ty_ty ty, Tr_exp trex)
{
  Sem_tyCatTr ret = checked_malloc (sizeof (*ret));
  ret->type = ty;
  ret->cat = Sem_R;
  ret->tr = trex;
  return ret;
}

static void
initGbls (int arch_size)
{
  SEM_ARCH_SIZE = arch_size;
  cenv = S_empty ();
  venv = S_empty ();
  varoff = S_empty ();
  methoff = S_empty ();
  whiletests = S_empty ();
  whileends = S_empty ();
  whlTstSb = S_Symbol ("whiletest");
  whlEndSb = S_Symbol ("whileend");
}

void
transError (FILE *out, A_pos pos, string msg)
{
  fprintf (out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush (out);
  exit (1);
}

T_funcDeclList transA_ClassDeclList (FILE *out, A_classDeclList cdl);
T_funcDeclList transA_ClassDecl (FILE *out, A_classDecl cd);
void transClassVarDeclList (FILE *out, A_varDeclList cvdl, S_table clsvtb);
void transClassVarDecl (FILE *out, A_varDecl cvd, S_table clsvtb);
T_funcDeclList transA_MethodDeclList (FILE *out, A_methodDeclList mdl);
T_funcDecl transA_MethodDecl (FILE *out, A_methodDecl md);
Temp_tempList transA_FormalList (FILE *out, A_formalList fmllst);
Temp_temp transA_Formal (FILE *out, A_formal fml);
T_funcDecl transA_MainMethod (FILE *out, A_mainMethod mmth);
Tr_exp transA_VarDeclList (FILE *out, A_varDeclList vdl);
// transA_VarDecl return value:
//   init int/float:   Tr_AssignStm (stm),
//   init array:       Tr_ArrayInit (stm),
//   not initializing: NULL
Tr_exp transA_VarDecl (FILE *out, A_varDecl vd);
Tr_exp transA_StmList (FILE *out, A_stmList sl);
Tr_exp transA_Stm (FILE *out, A_stm stm);
Tr_expList transA_ExpList (FILE *out, A_expList el);
Sem_tyCatTr transA_Exp (FILE *out, A_exp exp);
Tr_expList transCoerceArgList (Ty_fieldList fl, Tr_expList arglst);
static int elAllFI (FILE *out, A_expList el);
static int clsAsgCk (E_enventry entl, E_enventry entr);
static int cmpArgs (FILE *out, A_expList arglst, Ty_fieldList fldlst);

T_funcDeclList
transA_Prog (FILE *out, A_prog p, int arch_size)
{
  initGbls (arch_size);
  I_recClassDeclList (out, p->cdl, cenv);
  // DBGPRT ("#stg1 end\n");
  I_chkClassDeclList (out, p->cdl, cenv);
  // DBGPRT ("#stg2 end\n");
  I_uorClassDeclList (out, p->cdl, varoff, methoff);
  // DBGPRT ("#uor end\n");
  T_funcDeclList clsMthDL = transA_ClassDeclList (out, p->cdl);
  // DBGPRT ("#stg3 cls meth decl end\n");
  T_funcDecl mainDecl = transA_MainMethod (out, p->m);
  // DBGPRT ("#stg3 main meth decl end\n");
  T_funcDeclList progfdl = T_FuncDeclList (mainDecl, clsMthDL);
  // for (T_funcDeclList iter = clsMthDL; iter != NULL; iter = iter->tail)
  //   DBGPRT ("%s, clsMthDl %s\n", __func__, iter->head->name);
  // for (T_funcDeclList iter = progfdl; iter != NULL; iter = iter->tail)
  //   DBGPRT ("%s, fun %s\n", __func__, iter->head->name);
  return progfdl;
}

T_funcDeclList
transA_ClassDeclList (FILE *out, A_classDeclList cdl)
{
  // DBGPRT ("%s\n", __func__);
  static int i = 0;
  if (cdl == NULL)
    return NULL;
  MYASRT (cdl->head);
  // DBGPRT ("trans class %s\n", cdl->head->id);
  T_funcDeclList hdclsfdl = transA_ClassDecl (out, cdl->head);
  T_funcDeclList tlclsfdl = transA_ClassDeclList (out, cdl->tail);
  T_funcDeclList retfdl = Tr_ChainFuncDeclList (hdclsfdl, tlclsfdl);
  ++i;
  // for (T_funcDeclList iter = retfdl; iter != NULL; iter = iter->tail)
  //   DBGPRT ("%s, %d th rec, fun %s\n", __func__, i, iter->head->name);
  return retfdl;
}

T_funcDeclList
transA_ClassDecl (FILE *out, A_classDecl cd)
{
  MYASRT (cd);
  cur_class_id = cd->id;
  E_enventry clsent = S_look (cenv, S_Symbol (cd->id));
  transClassVarDeclList (out, cd->vdl, clsent->u.cls.vtbl);
  // DBGPRT ("transClassVarDeclList end\n");
  T_funcDeclList fdl = transA_MethodDeclList (out, cd->mdl);
  cur_class_id = NULL;
  return fdl;
}

void
transClassVarDeclList (FILE *out, A_varDeclList cvdl, S_table clsvtb)
{
  // DBGPRT ("%s\n", __func__);
  if (cvdl == NULL)
    return;
  transClassVarDecl (out, cvdl->head, clsvtb);
  transClassVarDeclList (out, cvdl->tail, clsvtb);
}

void
transClassVarDecl (FILE *out, A_varDecl cvd, S_table clsvtb)
{
  MYASRT (cvd != NULL);
  // DBGPRT ("%s\n", __func__);
  E_enventry vdentry = S_look (clsvtb, S_Symbol (cvd->v));
  Sem_tyCatTr tcl = NULL;
  // Var and type already inserted into class vtbl in stage 1 and 2,
  // should always be found in stage 3.
  MYASRT (vdentry != NULL);
  switch (cvd->t->t)
    {
    case A_intType:
    case A_floatType:
      if (cvd->elist == NULL || cvd->elist->head == NULL)
        return;
      else if (cvd->elist != NULL && cvd->elist->head != NULL
               && cvd->elist->tail == NULL)
        return;
      else
        transError (out, cvd->pos, "ERROR: assigning array to int / float\n");
      break;
    case A_intArrType:
    case A_floatArrType:
      break;
    case A_idType:
      if (cvd->elist == NULL || cvd->elist->head == NULL)
        {
          if (S_look (cenv, S_Symbol (cvd->t->id)) == NULL)
            transError (out, cvd->pos, "ERROR: class type undeclared\n");
        }
      else
        transError (
            out, cvd->pos,
            "ERROR: cannot assign to a class variable at declaration\n");
      break;
    }
}

T_funcDecl
transA_MainMethod (FILE *out, A_mainMethod mmth)
{
  S_beginScope (venv);
  // MYASRT (mmth && mmth->vdl && mmth->sl);
  // DBGPRT ("%s\n", __func__);
  Tr_exp vdltr = transA_VarDeclList (out, mmth->vdl);
  // DBGPRT ("temp1\n");
  Tr_exp sltr = transA_StmList (out, mmth->sl);
  // DBGPRT ("temp2\n");
  // MYASRT (vdltr && sltr);
  S_endScope (venv);
  T_funcDecl ret = Tr_MainMethod (vdltr, sltr);
  return ret;
}

T_funcDeclList
transA_MethodDeclList (FILE *out, A_methodDeclList mdl)
{
  if (mdl == NULL)
    return NULL;
  MYASRT (mdl->head);
  T_funcDecl hdmd = transA_MethodDecl (out, mdl->head);
  T_funcDeclList tlmdl = transA_MethodDeclList (out, mdl->tail);
  return T_FuncDeclList (hdmd, tlmdl);
}

T_funcDecl
transA_MethodDecl (FILE *out, A_methodDecl md)
{
  MYASRT (md);
  cur_method_id = md->id;
  S_beginScope (venv);
  if (md->t->t == A_idType)
    {
      if (S_look (cenv, S_Symbol (md->t->id)) == NULL)
        transError (out, md->pos, "ERROR: return type undefined\n");
    }
  Temp_tempList parTmpLs = transA_FormalList (out, md->fl);
  parTmpLs = Temp_TempList (Temp_namedtemp (99, T_int), parTmpLs);
  Tr_exp vdltr = transA_VarDeclList (out, md->vdl);
  Tr_exp sltr = transA_StmList (out, md->sl);
  char *cpltname = checked_malloc (128 * sizeof (char));
  strcpy (cpltname, cur_class_id);
  strcat (cpltname, "$");
  strcat (cpltname, md->id);
  S_endScope (venv);
  cur_method_id = NULL;
  // DBGPRT ("%s, method name %s\n", __func__, cpltname);
  return Tr_ClassMethod (
      cpltname, parTmpLs, vdltr, sltr,
      I_A_type2Ty_ty (md->t, md->t->pos, "md->t", "transA_MethodDecl"));
}

Temp_tempList
transA_FormalList (FILE *out, A_formalList fmllst)
{
  if (fmllst == NULL)
    return NULL;
  MYASRT (fmllst);
  Temp_temp hdtmp = transA_Formal (out, fmllst->head);
  Temp_tempList tltmplst = transA_FormalList (out, fmllst->tail);
  return Temp_TempList (hdtmp, tltmplst);
}

Temp_temp
transA_Formal (FILE *out, A_formal fml)
{
  MYASRT (fml);
  if (fml->t->t == A_idType)
    {
      if (S_look (cenv, S_Symbol (fml->t->id)) == NULL)
        transError (out, fml->pos, "ERROR: parameter type undefined\n");
    }
  Temp_temp thistmp
      = Temp_newtemp (fml->t->t == A_floatType ? T_float : T_int);
  S_enter (venv, S_Symbol (fml->id),
           E_VarEntry (A_VarDecl (fml->pos, fml->t, fml->id, NULL),
                       I_A_type2Ty_ty (fml->t, fml->pos, "", ""), thistmp));
  return thistmp;
}

Tr_exp
transA_VarDeclList (FILE *out, A_varDeclList vdl)
{
  // DBGPRT ("%s, enter\n", __func__);
  if (vdl == NULL)
    return NULL;
  Tr_exp hdtr = transA_VarDecl (out, vdl->head);
  // DBGPRT ("%s, temp1\n", __func__);
  Tr_exp tltr = transA_VarDeclList (out, vdl->tail);
  return Tr_StmList (hdtr, tltr);
}

Tr_exp
transA_VarDecl (FILE *out, A_varDecl vd)
{
  MYASRT (vd);
  // DBGPRT ("%s, enter\n", __func__);
  E_enventry existing = S_look (venv, S_Symbol (vd->v));
  if (existing != NULL)
    transError (out, vd->pos, "ERROR: re-declaration");
  switch (vd->t->t)
    {
    case A_intType:
      if (vd->elist == NULL || vd->elist->head == NULL
          || vd->elist->tail == NULL)
        {
          S_enter (venv, S_Symbol (vd->v),
                   E_VarEntry (vd, Ty_Int (), Temp_newtemp (T_int)));
          if (vd->elist == NULL || vd->elist->head == NULL)
            return NULL;
          else
            {
              E_enventry thisent = S_look (venv, S_Symbol (vd->v));
              Sem_tyCatTr rconst = transA_Exp (out, vd->elist->head);
              return Tr_VarDeclInt (thisent->u.var.tmp,
                                    (int)vd->elist->head->u.num);
            }
        }
      else
        transError (out, vd->pos, "ERROR: assigning an array to an int");
      break;
    case A_floatType:
      if (vd->elist == NULL || vd->elist->head == NULL
          || vd->elist->tail == NULL)
        {
          S_enter (venv, S_Symbol (vd->v),
                   E_VarEntry (vd, Ty_Float (), Temp_newtemp (T_float)));
          if (vd->elist == NULL || vd->elist->head == NULL)
            return NULL;
          else
            {
              E_enventry thisent = S_look (venv, S_Symbol (vd->v));
              Sem_tyCatTr rconst = transA_Exp (out, vd->elist->head);
              // return Tr_AssignStm (Tr_IdExp (thisent->u.var.tmp),
              //                      rconst->tr);
              return Tr_VarDeclFloat (thisent->u.var.tmp,
                                      vd->elist->head->u.num);
            }
        }
      else
        transError (out, vd->pos, "ERROR: assigning an array to a float");
      break;
    case A_intArrType:
      S_enter (venv, S_Symbol (vd->v),
               E_VarEntry (vd, Ty_Array (Ty_Int ()), Temp_newtemp (T_int)));
      if (vd->elist == NULL || vd->elist->head == NULL)
        return NULL;
      else
        {
          E_enventry thisent = S_look (venv, S_Symbol (vd->v));
          Tr_expList rconst = transA_ExpList (out, vd->elist);
          // DBGPRT ("%s, A_intArrType\n", __func__);
          return Tr_ArrayInit (Tr_IdExp (thisent->u.var.tmp), rconst, T_int);
        }
      break;
    case A_floatArrType:
      // DBGPRT ("%s, A_floatArrType\n", __func__);
      S_enter (venv, S_Symbol (vd->v),
               E_VarEntry (vd, Ty_Array (Ty_Float ()), Temp_newtemp (T_int)));
      if (vd->elist == NULL || vd->elist->head == NULL)
        return NULL;
      else
        {
          E_enventry thisent = S_look (venv, S_Symbol (vd->v));
          Tr_expList rconst = transA_ExpList (out, vd->elist);
          return Tr_ArrayInit (Tr_IdExp (thisent->u.var.tmp), rconst, T_float);
        }
      break;
    case A_idType:
      {
        // DBGPRT ("%s, A_idType enter\n", __func__);
        if (vd->elist != NULL
            || (vd->elist != NULL && vd->elist->head != NULL))
          transError (
              out, vd->pos,
              "ERROR: cannot assign to a class object at declaration\n");
        // DBGPRT ("aaaaaaaa\n");
        if (S_look (cenv, S_Symbol (vd->t->id)) == NULL)
          transError (out, vd->pos, "ERROR: class type undefined\n");
        Temp_temp newtmp = Temp_newtemp (T_int);
        // DBGPRT ("%s, A_idType, newtmp %d\n", __func__, newtmp->num);
        S_enter (venv, S_Symbol (vd->v),
                 E_VarEntry (vd, Ty_Name (S_Symbol (vd->t->id)), newtmp));
        return NULL;
        break;
      }
    }
}

Tr_exp
transA_StmList (FILE *out, A_stmList sl)
{
  if (sl == NULL)
    return NULL;
  MYASRT (sl->head != NULL);
  Tr_exp hdtr = transA_Stm (out, sl->head);
  Tr_exp tltr = transA_StmList (out, sl->tail);
  return Tr_StmList (hdtr, tltr);
}

Tr_exp
transA_Stm (FILE *out, A_stm stm)
{
  // MYASRT (stm);
  if (stm == NULL)
    return NULL;
  Sem_tyCatTr tct1 = NULL, tct2 = NULL;
  Ty_ty ty1 = NULL, ty2 = NULL;
  Tr_exp trexp1 = NULL, trexp2 = NULL;
  Temp_label lbl1 = NULL, lbl2 = NULL;
  Tr_expList initlst = NULL, parlst = NULL;
  E_enventry envent1 = NULL, envent2 = NULL;
  int *offptr = NULL;
  switch (stm->kind)
    {
    case A_ifStm:
      // DBGPRT ("transA_Stm : A_ifStm begin\n");
      tct1 = transA_Exp (out, stm->u.if_stat.e);
      ty1 = tct1->type;
      if (ty1->kind == Ty_int || ty1->kind == Ty_float)
        {
          trexp1 = transA_Stm (out, stm->u.if_stat.s1);
          trexp2 = transA_Stm (out, stm->u.if_stat.s2);
          // DBGPRT ("transA_Stm : A_ifStm last step\n");
          return Tr_IfStm (tct1->tr, trexp1, trexp2);
        }
      else
        transError (out, stm->pos,
                    "ERROR: condition expression for \'if\' statement not "
                    "int or float");
      break;
    case A_whileStm:
      tct1 = transA_Exp (out, stm->u.while_stat.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, stm->pos,
                    "ERROR: condition expression for \'while\' statement "
                    "not int or float");
      lbl1 = Temp_newlabel (); // while test label
      S_enter (whiletests, whlTstSb, lbl1);
      lbl2 = Temp_newlabel (); // while end label
      S_enter (whileends, whlEndSb, lbl2);
      trexp1 = transA_Stm (out, stm->u.while_stat.s);
      TAB_pop (whiletests);
      TAB_pop (whileends);
      return Tr_WhileStm (tct1->tr, trexp1, lbl1, lbl2);
      break;
    case A_assignStm:
      // DBGPRT ("transA_Stm : A_assignStm begin\n");
      tct1 = transA_Exp (out, stm->u.assign.arr);   // LHS of assign
      tct2 = transA_Exp (out, stm->u.assign.value); // RHS of assign
      // DBGPRT ("transA_Stm : A_assignStm finish trans exp\n");
      MYASRT (tct1 && tct2);
      if (tct1->cat != Sem_L)
        transError (out, stm->pos, "ERROR: assigning to an rvalue");
      if ((tct1->type->kind == Ty_int || tct1->type->kind == Ty_float)
          && (tct2->type->kind == Ty_int || tct2->type->kind == Ty_float))
        {
          return Tr_AssignStm (tct1->tr, tct2->tr);
        }
      else if (tct1->type->kind == Ty_array && tct2->type->kind == Ty_array)
        {
          // DBGPRT ("OK\n");
          // DBGPRT ("stm pos: %d\n", stm->pos->line);
          // MYASRT (0);
          if (tct1->type->u.array->kind == tct2->type->u.array->kind)
            return Tr_AssignStm (tct1->tr, tct2->tr);
          else
            transError (out, stm->pos, "ERROR: assignment type incorrect");
        }
      else if (tct1->type->kind == Ty_name && tct2->type->kind == Ty_name)
        {
          if (clsAsgCk (S_look (cenv, tct1->type->u.name),
                        S_look (cenv, tct2->type->u.name)))
            {
              // DBGPRT ("%s, A_assignStm, Ty_name, lvalue tmp id %d\n",
              // __func__,
              //         ((E_enventry)S_look (venv,
              //                              S_Symbol
              //                              (stm->u.assign.arr->u.v)))
              //             ->u.var.tmp->num);
              return Tr_AssignStm (tct1->tr, tct2->tr);
            }
          else
            transError (out, stm->pos,
                        "ERROR: assignment type inconsistent\n");
        }
      else
        transError (out, stm->pos, "ERROR: assignment type incorrect");
      break;
    case A_arrayInit:
      tct1 = transA_Exp (out, stm->u.array_init.arr);
      if (tct1->cat != Sem_L)
        transError (out, stm->pos,
                    "ERROR: array being initialized is not an lvalue\n");
      if (tct1->type->kind != Ty_array)
        transError (out, stm->pos,
                    "ERROR: assigning an initialization list to a non-array "
                    "value\n");
      initlst = transA_ExpList (out, stm->u.array_init.init_values);
      if (!elAllFI (out, stm->u.array_init.init_values))
        transError (out, stm->pos,
                    "ERROR: array initialization list type inconsistent\n");
      // DBGPRT ("%s, A_arrayInit, %s\n", __func__,
      //         tct1->type->u.array->kind == Ty_float ? "T_float" : "T_int");
      return Tr_ArrayInit (tct1->tr, initlst,
                           tct1->type->u.array->kind == Ty_int ? T_int
                                                               : T_float);
      break;
    case A_callStm:
      tct1 = transA_Exp (out, stm->u.call_stat.obj); // caller object
      envent1 = S_look (cenv, tct1->type->u.name);   // caller class entry
      if (envent1 == NULL)
        transError (out, stm->pos,
                    "ERROR: caller object not a class instance\n");
      // called method entry
      envent2 = S_look (envent1->u.cls.mtbl, S_Symbol (stm->u.call_stat.fun));
      if (envent2 == NULL)
        transError (out, stm->pos, "ERROR: method undefined\n");
      if (!cmpArgs (out, stm->u.call_stat.el, envent2->u.meth.fl))
        transError (out, stm->pos, "ERROR: argument number/type incorrect\n");
      parlst = transA_ExpList (out, stm->u.call_stat.el);
      offptr = S_look (methoff, S_Symbol (stm->u.call_stat.fun));
      parlst = transCoerceArgList (envent2->u.meth.fl, parlst);
      return Tr_CallStm (
          String (stm->u.call_stat.fun), tct1->tr, *offptr, parlst,
          envent2->u.meth.ret->kind == Ty_float ? T_float : T_int);
      break;
    case A_continue:
      // most recent (innermost) while test label
      lbl1 = S_look (whiletests, whlTstSb);
      if (lbl1 == NULL)
        transError (out, stm->pos, "ERROR: continue outside of any while");
      return Tr_Continue (lbl1);
      break;
    case A_break:
      // most recent (innermost) while end label
      lbl1 = S_look (whileends, whlEndSb);
      if (lbl1 == NULL)
        transError (out, stm->pos, "ERROR: break outside of any while");
      return Tr_Break (lbl1);
      break;
    case A_return:
      tct1 = transA_Exp (out, stm->u.e);
      if (cur_class_id == NULL || cur_method_id == NULL)
        {
          if (tct1->type->kind == Ty_int || tct1->type->kind == Ty_float)
            return Tr_Return (Tr_Cast (tct1->tr, T_int));
          else
            transError (out, stm->pos,
                        "ERROR: main function should return int or float\n");
        }
      else
        {
          // current class entry
          envent1 = S_look (cenv, S_Symbol (cur_class_id));
          // current method entry in current class' mtbl
          envent2 = S_look (envent1->u.cls.mtbl, S_Symbol (cur_method_id));
          if ((envent2->u.meth.ret->kind == Ty_int
               || envent2->u.meth.ret->kind == Ty_float
               || envent2->u.meth.ret->kind == Ty_array)
              && I_tyEq (envent2->u.meth.ret, tct1->type))
            {
              Tr_exp retExp
                  = envent2->u.meth.ret->kind != Ty_array ? Tr_Cast (
                        tct1->tr, envent2->u.meth.ret->kind == Ty_float
                                      ? T_float
                                      : T_int)
                                                          : tct1->tr;
              // DBGPRT ("%s, return type %s\n", __func__,
              //         envent2->u.meth.ret->kind == Ty_float ? "float" :
              //         "int");
              return Tr_Return (/* tct1->tr */ retExp);
            }
          else if (envent2->u.meth.ret->kind == Ty_name)
            {
              if (tct1->type->kind != Ty_name)
                transError (out, stm->pos,
                            "ERROR: returning exp value differ from method "
                            "signature\n");
              E_enventry clsentr = S_look (cenv, envent2->u.meth.ret->u.name);
              E_enventry clsentl = S_look (cenv, tct1->type->u.name);
              if (clsAsgCk (clsentr, clsentl))
                return Tr_Return (tct1->tr);
              else
                transError (out, stm->pos,
                            "ERROR: returning exp value differ from method "
                            "signature\n");
            }
          else
            transError (
                out, stm->pos,
                "ERROR: returning exp value differ from method signature\n");
        }
      break;
    case A_putch:
      tct1 = transA_Exp (out, stm->u.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, stm->pos, "ERROR: putch argument not int/float");
      return Tr_Putch (tct1->tr);
      break;
    case A_putnum:
      // DBGPRT ("transA_Stm : A_putnum\n");
      tct1 = transA_Exp (out, stm->u.e);
      switch (tct1->type->kind)
        {
        case Ty_int:
          // DBGPRT ("temp\n");
          return Tr_Putint (tct1->tr);
          break;
        case Ty_float:
          return Tr_Putfloat (tct1->tr);
          break;
        default:
          transError (out, stm->pos, "ERROR: putnum argument not int/float");
          break;
        }
      break;
    case A_putarray:
      tct1 = transA_Exp (out, stm->u.putarray.e1);
      tct2 = transA_Exp (out, stm->u.putarray.e2);
      if ((tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
          || tct2->type->kind != Ty_array)
        transError (out, stm->pos, "ERROR: putarray argument type incorrect");
      return tct2->type->u.array->kind == Ty_int
                 ? Tr_Putarray (tct1->tr, tct2->tr)
                 : Tr_Putfarray (tct1->tr, tct2->tr);
      break;
    case A_starttime:
      return Tr_Starttime ();
      break;
    case A_stoptime:
      return Tr_Stoptime ();
      break;
    case A_nestedStm:
      return transA_StmList (out, stm->u.ns);
      break;
    default:
      MYASRT (0);
      break;
    }
}

Tr_expList
transA_ExpList (FILE *out, A_expList el)
{
  if (el == NULL)
    return NULL;
  MYASRT (el->head != NULL);
  Tr_exp hdtrex = transA_Exp (out, el->head)->tr;
  Tr_expList tltrexls = transA_ExpList (out, el->tail);
  return Tr_ExpList (hdtrex, tltrexls);
}

Sem_tyCatTr
transA_Exp (FILE *out, A_exp exp)
{
  MYASRT (exp != NULL);
  Sem_tyCatTr tct1 = NULL, tct2 = NULL, ret = NULL;
  Tr_exp trexp = NULL;
  Tr_expList parlst = NULL;
  E_enventry varent = NULL, clsent = NULL, methent = NULL;
  int isInt = 0;
  int *offptr = NULL;
  switch (exp->kind)
    {
    case A_numConst:
      isInt = fabs (exp->u.num - (int)exp->u.num) < 1e-7;
      return Sem_tyRval (isInt ? Ty_Int () : Ty_Float (),
                         Tr_NumConst (exp->u.num, isInt ? T_int : T_float));
      break;
    case A_boolConst:
      return Sem_tyRval (Ty_Int (), Tr_BoolConst (exp->u.b));
      break;
    case A_lengthExp:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_array)
        transError (out, exp->pos, "ERROR: argument of length not an array");
      return Sem_tyRval (Ty_Int (), Tr_LengthExp (tct1->tr));
      break;
    case A_getnum:
      return Sem_tyRval (Ty_Float (), Tr_Getfloat ());
      break;
    case A_getch:
      return Sem_tyRval (Ty_Int (), Tr_Getch ());
      break;
    case A_getarray:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_array)
        transError (out, exp->pos, "ERROR: argument of getarray not an array");
      return Sem_tyRval (Ty_Int (), tct1->type->u.array->kind == Ty_int
                                        ? Tr_Getarray (tct1->tr)
                                        : Tr_Getfarray (tct1->tr));
      break;
    case A_idExp:
      varent = S_look (venv, S_Symbol (exp->u.v));
      if (varent == NULL)
        transError (out, exp->pos, "variable undefined");
      return Sem_tyLval (varent->u.var.ty, Tr_IdExp (varent->u.var.tmp));
      break;
    case A_thisExp:
      if (cur_class_id == NULL)
        transError (out, exp->pos,
                    "ERROR: using \'this\' expression outside a class\n");
      clsent = S_look (cenv, S_Symbol (cur_class_id));
      MYASRT (clsent);
      return Sem_tyRval (Ty_Name (S_Symbol (clsent->u.cls.cd->id)),
                         Tr_ThisExp (Temp_namedtemp (99, T_int)));
      break;
    case A_newIntArrExp:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, exp->pos, "ERROR: new array size not int or float");
      return Sem_tyRval (Ty_Array (Ty_Int ()), Tr_NewArrExp (tct1->tr));
      break;
    case A_newFloatArrExp:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, exp->pos, "ERROR: new array size not int or float");
      return Sem_tyRval (Ty_Array (Ty_Float ()), Tr_NewArrExp (tct1->tr));
      break;
    case A_newObjExp:
      clsent = S_look (cenv, S_Symbol (exp->u.v));
      if (clsent == NULL)
        transError (out, exp->pos,
                    "ERROR: undefined class name used as constructor\n");
      return Sem_tyRval (Ty_Name (S_Symbol (exp->u.v)),
                         Tr_NewObjExp (out, clsent, varoff, methoff,
                                       Tr_NumConst (globaloff, T_int)));
      break;
    case A_opExp:
      tct1 = transA_Exp (out, exp->u.op.left); // left operand exp
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, exp->pos,
                    "ERROR: binary operator operand type incorrect");
      tct2 = transA_Exp (out, exp->u.op.right); // right operand exp
      if (tct2->type->kind != Ty_int && tct2->type->kind != Ty_float)
        transError (out, exp->pos,
                    "ERROR: binary operator operand type incorrect");
      if (tct1->type->kind == Ty_int && tct2->type->kind == Ty_int)
        {
          return Sem_tyRval (Ty_Int (),
                             Tr_OpExp (exp->u.op.oper, tct1->tr, tct2->tr));
        }
      else
        return Sem_tyRval (Ty_Float (),
                           Tr_OpExp (exp->u.op.oper, tct1->tr, tct2->tr));
      break;
    case A_notExp:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, exp->pos, "ERROR: \'!\' operand type inappropriate");
      return Sem_tyRval (Ty_Int (), Tr_NotExp (tct1->tr));
      break;
    case A_minusExp:
      tct1 = transA_Exp (out, exp->u.e);
      if (tct1->type->kind != Ty_int && tct1->type->kind != Ty_float)
        transError (out, exp->pos,
                    "ERROR: unary \'-\' operand type inappropriate");
      return Sem_tyRval (tct1->type, Tr_MinusExp (tct1->tr));
      break;
    case A_escExp:
      // escape statement list
      // DBGPRT ("%s, A_escExp\n", __func__);
      trexp = transA_StmList (out, exp->u.escExp.ns);
      tct1 = transA_Exp (out, exp->u.escExp.exp);
      MYASRT (trexp && tct1->tr);
      // Sem_tyRval (tct1->type, Tr_EscExp (trexp, tct1->tr));
      // DBGPRT ("temp\n");
      return Sem_tyRval (tct1->type, Tr_EscExp (trexp, tct1->tr));
    case A_callExp:
      tct1 = transA_Exp (out, exp->u.call.obj);
      if (tct1->type->kind != Ty_name)
        transError (out, exp->pos,
                    "ERROR: caller object not a class instance");
      clsent = S_look (cenv, tct1->type->u.name);
      if (clsent == NULL)
        transError (out, exp->pos,
                    "ERROR: caller object not a class instance");
      methent = S_look (clsent->u.cls.mtbl, S_Symbol (exp->u.call.fun));
      if (methent == NULL)
        transError (out, exp->pos, "ERROR: method undefined");
      if (!cmpArgs (out, exp->u.call.el, methent->u.meth.fl))
        transError (out, exp->pos, "ERROR: argument number incorrect");
      offptr = S_look (methoff, S_Symbol (exp->u.call.fun));
      if (offptr == NULL)
        transError (out, exp->pos, "ERROR: method undefined");
      parlst = transA_ExpList (out, exp->u.call.el);
      parlst = transCoerceArgList (methent->u.meth.fl, parlst);
      return Sem_tyRval (
          methent->u.meth.ret,
          Tr_CallExp (String (exp->u.call.fun), tct1->tr, *offptr, parlst,
                      methent->u.meth.ret->kind == Ty_float ? T_float
                                                            : T_int));
      break;
    case A_classVarExp:
      tct1 = transA_Exp (out, exp->u.classvar.obj);
      if (tct1->type->kind != Ty_name)
        transError (out, exp->pos,
                    "ERROR: referencing object not a class instance");
      clsent = S_look (cenv, tct1->type->u.name);
      if (clsent == NULL)
        transError (out, exp->pos,
                    "ERROR: referencing object not a class instance");
      varent = S_look (clsent->u.cls.vtbl, S_Symbol (exp->u.classvar.var));
      if (varent == NULL)
        transError (out, exp->pos, "ERROR: member variable undefined\n");
      offptr = S_look (varoff, S_Symbol (exp->u.classvar.var));
      return Sem_tyLval (varent->u.var.ty,
                         Tr_ClassVarExp (tct1->tr, *offptr,
                                         varent->u.var.ty->kind == Ty_float
                                             ? T_float
                                             : T_int));
      break;
    case A_arrayExp:
      tct1 = transA_Exp (out, exp->u.array_pos.arr);     // array
      tct2 = transA_Exp (out, exp->u.array_pos.arr_pos); // index
      if (tct1->type->kind != Ty_array)
        transError (out, exp->pos, "ERROR: indexing a non-array");
      if (tct2->type->kind != Ty_int && tct2->type->kind != Ty_float)
        transError (out, exp->pos, "ERROR: array index not int or float");
      // DBGPRT ("array elem type: %s\n",
      //         tct1->type->u.array->kind == Ty_int ? "int" : "float");
      return Sem_tyLval (
          tct1->type->u.array,
          Tr_ArrayExp (tct1->tr, tct2->tr,
                       tct1->type->u.array->kind == Ty_int ? T_int : T_float));
      break;
    default:
      MYASRT (0);
      break;
    }
}

Tr_expList
transCoerceArgList (Ty_fieldList fl, Tr_expList arglst)
{
  if (fl == NULL || arglst == NULL)
    return NULL;
  return Tr_ExpList (
      Tr_Cast (arglst->head, fl->head->ty->kind == Ty_float ? T_float : T_int),
      transCoerceArgList (fl->tail, arglst->tail));
}

static int
elAllFI (FILE *out, A_expList el)
{
  if (el == NULL)
    return 1;
  Ty_ty hdty = transA_Exp (out, el->head)->type;
  int hdFI = (int)(hdty->kind == Ty_int || hdty->kind == Ty_float);
  return hdFI && elAllFI (out, el->tail);
}

static int
clsAsgCk (E_enventry entl, E_enventry entr)
{
  if (S_Symbol (entl->u.cls.cd->id) == S_Symbol (entr->u.cls.cd->id))
    return 1;
  else if (entr->u.cls.fa != S_Symbol (mainClNm))
    return clsAsgCk (entl, S_look (cenv, entr->u.cls.fa));
  else
    return 0;
}

static int
cmpArgs (FILE *out, A_expList arglst, Ty_fieldList fldlst)
{
  if (arglst == NULL && fldlst == NULL)
    return 1;
  else if ((arglst == NULL && fldlst != NULL)
           || (arglst != NULL && fldlst == NULL))
    return 0;
  else
    {
      MYASRT (arglst->head != NULL && fldlst->head != NULL);
      Sem_tyCatTr argtc = transA_Exp (out, arglst->head);
      int thisrslt = I_tyEq (argtc->type, fldlst->head->ty);
      if (fldlst->head->ty->kind == Ty_name)
        thisrslt = clsAsgCk (S_look (cenv, fldlst->head->ty->u.name),
                             S_look (cenv, argtc->type->u.name));
      if (!thisrslt)
        transError (out, arglst->head->pos,
                    "ERROR: argument type incompatible\n");
      return (thisrslt && cmpArgs (out, arglst->tail, fldlst->tail));
    }
}
