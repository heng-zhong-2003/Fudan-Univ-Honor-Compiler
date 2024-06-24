#include "stack.h"

void
STK_push (STK_stack_t *stack, Temp_temp temp)
{
  STK_stackEntry_t entry = checked_malloc (sizeof (*entry));
  entry->temp = temp;
  entry->tail = *stack;
  *stack = entry;
}

void
STK_pop (STK_stack_t *stack)
{
  if (*stack == NULL)
    return;
  *stack = (*stack)->tail;
}

Temp_temp
STK_top (STK_stack_t stack)
{
  if (stack == NULL)
    return NULL;
  return stack->temp;
}
