#include "assemblock.h"

/* Make a block list */
AS_blockList
AS_BlockList (AS_block head, AS_blockList tail)
{
  AS_blockList list = checked_malloc (sizeof *list);
  list->head = head;
  list->tail = tail;
  return list;
}

AS_blockList
AS_BlockSplice (AS_blockList a, AS_blockList b)
{
  AS_blockList p;
  if (a == NULL)
    return b;
  for (p = a; p->tail != NULL; p = p->tail)
    ;
  p->tail = b;
  return a;
}

/* given a list of instructions that's a basic block, i.e.,
   starts with a label and ends with a jump/ret
   make a AS_block, which consists of instrs, start_label, jump_to_labels
*/

AS_block
AS_Block (AS_instrList instrs)
{
  AS_block b = checked_malloc (sizeof *b);
  AS_instrList last;
  b->instrs = instrs;
  if (instrs->head->kind == I_LABEL)
    b->label = instrs->head->u.LABEL.label;
  else
    assert (0); // has to be a label!
  last = instrs;
  while (last->tail)
    last = last->tail;
  if (last->head->kind == I_OPER)
    {
      if (last->head->u.OPER.jumps)
        b->succs = last->head->u.OPER.jumps->labels;
      else
        b->succs = NULL;
    }
  else
    assert (0); // has to be an OPER (can't be a label or move)!
  return b;
}

static AS_blockList global_bl;
static S_table block_env;

static AS_instrList getNext (AS_instrList epilog, bool optimize);

static AS_instrList
getLast (AS_instrList list)
// list must leng>=2. sen'd to last!
{
  AS_instrList last = list;
  while (last->tail->tail)
    last = last->tail;
  return last;
}

static void
trace (AS_instrList list, AS_instrList epilog, bool optimize)
{                                     // this list is a basic block
  AS_instrList last = getLast (list); // second to the last before the br/ret
  AS_instr lab = list->head;          // this must be a label instruction
  AS_instr s = last->tail->head;      // the last (last) stm (jump/cjump)
  // or a return
  S_enter (block_env, lab->u.LABEL.label, NULL); // mark it already traced!

  if (!optimize)
    { // just pick up the next block and continue
      last->tail->tail = getNext (epilog, optimize);
      return;
    }
  /* now try an optimized trace, as follows:
     if br to one label, then try to get the block
     with the label to follow, then eliminate the br instruction.
     if br to more than one label, do nothing
  */
  if (s->kind != I_OPER)
    assert (-1); // can't be a label of move!

  if (!s->u.OPER.jumps || !s->u.OPER.jumps->labels)
    {
      // jump to nowhwere! assume it's a ret
      last->tail->tail = getNext (epilog, optimize);
      return;
    }

  /* now take a look a the target labels */
  Temp_labelList labellist = s->u.OPER.jumps->labels;
  if (labellist->tail)
    { // more than two labels! nothing to do again
      last->tail->tail = getNext (epilog, optimize);
      return;
    }

  Temp_label nn = labellist->head; // here to jump to

  AS_block target = (AS_block)S_look (block_env, nn);
  if (target)
    {
      last->tail = target->instrs;
      /* merge the 2 lists removing br instruction */
      trace (target->instrs, epilog, optimize);
    }
  else
    last->tail->tail = getNext (epilog, optimize);
  /* can't find the jump-to block!
     either it's a jumpt to nowhere (should be to the exit),
     or the block has already been traced! */
}

/* get the next block from the list of stmLists, using only those that have
 * not been traced yet */

static AS_instrList
getNext (AS_instrList epilog, bool optimize)
{
  if (!global_bl)
    return epilog;

  AS_block b = global_bl->head;
  if (S_look (block_env, b->label))
    {
      /* label exists in the table, i.e., not traced yet! */
      trace (b->instrs, epilog, optimize);
      return b->instrs;
    }
  else
    {
      global_bl = global_bl->tail;
      return getNext (epilog, optimize);
    }
}

/* build the entire method from blockList: 1) put the prolog in the beginning,
 then 2) trace the blocks in the blocklist, and 3) put the epilog in the end,
 4) If optimize is true, try to eliminate jumps, otherwise, keep all the jumps
 */
AS_instrList
AS_traceSchedule (AS_blockList bl, AS_instrList prolog, AS_instrList epilog,
                  bool optimize)
{
  AS_blockList blist;
  AS_instrList ll;

  block_env = S_empty (); /* (re)start block env */
  global_bl = bl;         /* give it to the global_bl */

  /* build a table: from label -> block */
  for (blist = bl; blist; blist = blist->tail)
    {
      S_enter (block_env, blist->head->label, blist->head);
    }

  if (!prolog)
    return getNext (epilog, optimize);
  ll = prolog;
  while (prolog->tail)
    prolog = prolog->tail;
  prolog->tail = getNext (epilog, optimize);
  return ll;
}
