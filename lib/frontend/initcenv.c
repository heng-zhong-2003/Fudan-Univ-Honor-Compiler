#include "initcenv.h"
#include "dbg.h"
#include "semant.h"
#include "symbol.h"
#include "table.h"
#include <stdlib.h>

int globaloff = 0;
extern int SEM_ARCH_SIZE;

Ty_ty
I_A_type2Ty_ty (A_type aty, A_pos defpos, string failvar, string failfun)
{
  Ty_ty type;
  switch (aty->t)
    {
    case A_intType:
      type = Ty_Int ();
      break;
    case A_floatType:
      type = Ty_Float ();
      break;
    case A_idType:
      type = Ty_Name (S_Symbol (aty->id));
      break;
    case A_intArrType:
      type = Ty_Array (Ty_Int ());
      break;
    case A_floatArrType:
      type = Ty_Array (Ty_Float ());
      break;
    default:
      DBGPRT ("DBGERR: %s fails to match %s at fmj source line %d, col %d\n",
              failfun, failvar, defpos->line, defpos->pos);
      exit (1);
    }
  return type;
}

int
I_tyEq (Ty_ty t1, Ty_ty t2)
{
  int ret = 0;
  switch (t1->kind)
    {
    case Ty_int:
      ret = t2->kind == Ty_int || t2->kind == Ty_float ? 1 : 0;
      break;
    case Ty_float:
      ret = t2->kind == Ty_float || t2->kind == Ty_int ? 1 : 0;
      break;
    case Ty_array:
      ret = (t2->kind == Ty_array && I_tyEq (t1->u.array, t2->u.array)) ? 1
                                                                        : 0;
      break;
    case Ty_name:
      ret = (t2->kind == Ty_name && t1->u.name == t2->u.name) ? 1 : 0;
      break;
    default:
      DBGPRT ("DBGERR: I_tyEq matching failed\n");
    }
  return ret;
}

int
I_strictTyEq (Ty_ty t1, Ty_ty t2)
{
  int ret = 0;
  switch (t1->kind)
    {
    case Ty_int:
      ret = t2->kind == Ty_int ? 1 : 0;
      break;
    case Ty_float:
      ret = t2->kind == Ty_float ? 1 : 0;
      break;
    case Ty_array:
      ret = (t2->kind == Ty_array && I_strictTyEq (t1->u.array, t2->u.array))
                ? 1
                : 0;
      break;
    case Ty_name:
      ret = (t2->kind == Ty_name && t1->u.name == t2->u.name) ? 1 : 0;
      break;
    default:
      DBGPRT ("DBGERR: I_tyEq matching failed\n");
    }
  return ret;
}

int
I_fieldListEq (Ty_fieldList fl1, Ty_fieldList fl2)
{
  if (fl1 == NULL && fl2 == NULL)
    return 1;
  else if ((fl1 == NULL && fl2 != NULL) || (fl1 != NULL && fl2 == NULL))
    return 0;
  else
    {
      MYASRT (fl1->head != NULL && fl2->head != NULL);
      return (I_strictTyEq (fl1->head->ty, fl2->head->ty)
              // && fl1->head->name == fl2->head->name
              && I_fieldListEq (fl1->tail, fl2->tail));
    }
}

void
I_recClassDeclList (FILE *out, A_classDeclList cdl, S_table clstbl)
{
  if (cdl == NULL)
    return;
  else
    {
      MYASRT (cdl->head != NULL);
      I_recClassDecl (out, cdl->head, clstbl);
      I_recClassDeclList (out, cdl->tail, clstbl);
    }
}

void
I_recClassDecl (FILE *out, A_classDecl cd, S_table clstbl)
{
  MYASRT (cd != NULL);
  E_enventry exists = S_look (clstbl, S_Symbol (cd->id));
  if (exists)
    {
      transError (out, cd->pos, "ERROR: multiple definition of classes\n");
      return;
    }
  S_table vtbl = S_empty ();
  S_table mtbl = S_empty ();
  I_recVarDeclList (out, cd->vdl, vtbl);
  I_recMethodDeclList (out, cd->mdl, cd->id, mtbl);
  S_enter (clstbl, S_Symbol (cd->id),
           E_ClassEntry (
               cd, S_Symbol (cd->parentID != NULL ? cd->parentID : mainClNm),
               E_transInit, vtbl, mtbl));
}

void
I_recVarDeclList (FILE *out, A_varDeclList vdl, S_table vtbl)
{
  if (vdl == NULL)
    return;
  else
    {
      MYASRT (vdl->head != NULL);
      I_recVarDecl (out, vdl->head, vtbl);
      I_recVarDeclList (out, vdl->tail, vtbl);
    }
}

void
I_recVarDecl (FILE *out, A_varDecl vd, S_table vtbl)
{
  MYASRT (vd != NULL);
  E_enventry exists = S_look (vtbl, S_Symbol (vd->v));
  if (exists)
    {
      transError (out, vd->pos,
                  "ERROR: multiple declarations of member variable\n");
      return;
    }
  Ty_ty type = I_A_type2Ty_ty (vd->t, vd->t->pos, "vd->t->t", "I_recVarDecl");
  S_enter (
      vtbl, S_Symbol (vd->v),
      E_VarEntry (
          vd, type,
          /* Temp_newtemp (type->kind == Ty_float ? T_float : T_int) */ NULL));
}

void
I_recMethodDeclList (FILE *out, A_methodDeclList mdl, string fromcls,
                     S_table mtbl)
{
  if (mdl == NULL)
    return;
  else
    {
      MYASRT (mdl->head != NULL);
      I_recMethodDecl (out, mdl->head, fromcls, mtbl);
      I_recMethodDeclList (out, mdl->tail, fromcls, mtbl);
    }
}

void
I_recMethodDecl (FILE *out, A_methodDecl md, string fromcls, S_table mtbl)
{
  MYASRT (md != NULL);
  E_enventry exists = S_look (mtbl, S_Symbol (md->id));
  if (exists)
    {
      transError (out, md->pos,
                  "ERROR: multiple definitions of method in a class\n");
      return;
    }
  S_table partbl = S_empty ();
  Ty_fieldList fldlst = I_recFormalList (out, md->fl, partbl);
  Ty_ty retty
      = I_A_type2Ty_ty (md->t, md->t->pos, "md->t->t", "I_recMethodDecl");
  S_enter (mtbl, S_Symbol (md->id),
           E_MethodEntry (md, S_Symbol (fromcls), retty, fldlst));
}

Ty_fieldList
I_recFormalList (FILE *out, A_formalList fl, S_table partbl)
{
  if (fl == NULL)
    return NULL;
  else
    {
      MYASRT (fl->head != NULL);
      Ty_field hd = I_recFormal (out, fl->head, partbl);
      Ty_fieldList tl = I_recFormalList (out, fl->tail, partbl);
      return Ty_FieldList (hd, tl);
    }
}

Ty_field
I_recFormal (FILE *out, A_formal f, S_table partbl)
{
  MYASRT (f != NULL);
  E_enventry exists = S_look (partbl, S_Symbol (f->id));
  if (exists)
    {
      transError (out, f->pos,
                  "ERROR: multiple occurences of the same parameter\n");
    }
  Ty_ty type = I_A_type2Ty_ty (f->t, f->t->pos, "f->t->t", "I_recFormal");
  S_enter (
      partbl, S_Symbol (f->id),
      E_VarEntry (
          A_VarDecl (f->pos, f->t, f->id, NULL), type,
          /* Temp_newtemp (type->kind == Ty_float ? T_float : T_int) */ NULL));
  return Ty_Field (S_Symbol (f->id), type);
}

void
I_chkClassDeclList (FILE *out, A_classDeclList cdl, S_table clstbl)
{
  if (cdl == NULL)
    return;
  else
    {
      MYASRT (cdl->head != NULL);
      I_chkClassDecl (out, cdl->head, clstbl);
      I_chkClassDeclList (out, cdl->tail, clstbl);
    }
}

void
I_chkClassDecl (FILE *out, A_classDecl cd, S_table clstbl)
{
  MYASRT (cd != NULL);
  E_enventry clsent = S_look (clstbl, S_Symbol (cd->id));
  E_enventry prnclsen = NULL;
  S_table newvtbl = NULL, newmtbl = NULL;
  switch (clsent->u.cls.status)
    {
    case E_transFill:
      return;
      break;
    case E_transFind:
      break;
    case E_transInit:
      if (clsent->u.cls.fa == S_Symbol (mainClNm))
        {
          clsent->u.cls.status = E_transFill;
          return;
        }
      else
        {
          // DBGPRT ("DBGMSG: not top class\n");
          prnclsen = S_look (clstbl, S_Symbol (cd->parentID));
          if (prnclsen == NULL)
            transError (out, cd->pos,
                        "ERROR: inheriting an undefined class\n");
          if (prnclsen->u.cls.status == E_transFind)
            {
              transError (out, prnclsen->u.cls.cd->pos,
                          "ERROR: cyclic inheritence\n");
              return;
            }
          clsent->u.cls.status = E_transFind;
          I_chkClassDecl (out, prnclsen->u.cls.cd, clstbl);
          prnclsen = S_look (clstbl, S_Symbol (cd->parentID));
          // DBGPRT ("DBGMSG: class var decl for loop begin\n");
          // for (binder curr = prnclsen->u.cls.vtbl->top; curr != NULL;
          //      curr = S_getBinder (prnclsen->u.cls.vtbl, curr->prevtop))
          for (S_symbol curr = prnclsen->u.cls.vtbl->top; curr != NULL;
               curr
               = ((binder)S_getBinder (prnclsen->u.cls.vtbl, curr))->prevtop)
            {
              // S_symbol vid = curr->key;
              // parent variable defined again in child
              // DBGPRT ("2\n");
              E_enventry definchd = S_look (clsent->u.cls.vtbl, curr);
              E_enventry definprn = S_look (prnclsen->u.cls.vtbl, curr);
              // DBGPRT ("3\n");
              if (definchd)
                {
                  transError (
                      out, definchd->u.var.vd->pos,
                      "ERROR: parent class variable redefined in child\n");
                  return;
                }
              else
                S_enter (clsent->u.cls.vtbl, curr, definprn);
              // S_enter (clsent->u.cls.vtbl, vid, curr->value);
              // DBGPRT ("4\n");
            }
          // DBGPRT ("DBGMSG: class mthd decl for loop\n");
          // for (binder curr = prnclsen->u.cls.mtbl->top; curr != NULL;
          //      curr = S_getBinder (prnclsen->u.cls.mtbl, curr->prevtop))
          for (S_symbol curr = prnclsen->u.cls.mtbl->top; curr != NULL;
               curr
               = ((binder)S_getBinder (prnclsen->u.cls.mtbl, curr))->prevtop)
            {
              // S_symbol mid = curr->key;
              E_enventry definchd = S_look (clsent->u.cls.mtbl, curr);
              E_enventry definprn = S_look (prnclsen->u.cls.mtbl, curr);
              if (definchd)
                {
                  if (I_strictTyEq (definchd->u.meth.ret, definprn->u.meth.ret)
                      && I_fieldListEq (definchd->u.meth.fl,
                                        definprn->u.meth.fl))
                    {
                      continue;
                    }
                  else
                    {
                      transError (out, definchd->u.meth.md->pos,
                                  "ERROR: parent and child method signature "
                                  "inconsistent\n");
                      return;
                    }
                }
              else
                S_enter (clsent->u.cls.mtbl, curr, definprn);
            }
          clsent->u.cls.status = E_transFill;
        }
      break;
    default:
      DBGPRT ("DBGERR: fmj source line %d col %d, fail to match "
              "clsent->u.cls.status in I_chkClassDecl\n",
              cd->pos->line, cd->pos->pos);
      exit (1);
      break;
    }
}

void
I_uorClassDeclList (FILE *out, A_classDeclList cdl, S_table voff, S_table moff)
{
  if (cdl == NULL)
    return;
  MYASRT (cdl->head);
  I_uorClassDecl (out, cdl->head, voff, moff);
  I_uorClassDeclList (out, cdl->tail, voff, moff);
  return;
}

void
I_uorClassDecl (FILE *out, A_classDecl cd, S_table voff, S_table moff)
{
  MYASRT (cd);
  I_uorVarDeclList (out, cd->vdl, voff);
  I_uorMethodDeclList (out, cd->mdl, moff);
}

void
I_uorVarDeclList (FILE *out, A_varDeclList vdl, S_table voff)
{
  if (vdl == NULL)
    return;
  MYASRT (vdl->head);
  I_uorVarDecl (out, vdl->head, voff);
  I_uorVarDeclList (out, vdl->tail, voff);
}

void
I_uorVarDecl (FILE *out, A_varDecl vd, S_table voff)
{
  MYASRT (vd);
  int *exists = S_look (voff, S_Symbol (vd->v));
  if (exists)
    return;
  int *newoff = checked_malloc (sizeof (int));
  *newoff = globaloff;
  globaloff += SEM_ARCH_SIZE;
  S_enter (voff, S_Symbol (vd->v), newoff);
}

void
I_uorMethodDeclList (FILE *out, A_methodDeclList mdl, S_table moff)
{
  if (mdl == NULL)
    return;
  MYASRT (mdl->head);
  I_uorMethodDecl (out, mdl->head, moff);
  I_uorMethodDeclList (out, mdl->tail, moff);
}

void
I_uorMethodDecl (FILE *out, A_methodDecl md, S_table moff)
{
  MYASRT (md);
  int *exists = S_look (moff, S_Symbol (md->id));
  if (exists)
    return;
  int *newoff = checked_malloc (sizeof (int));
  *newoff = globaloff;
  globaloff += SEM_ARCH_SIZE;
  S_enter (moff, S_Symbol (md->id), newoff);
}
