#pragma once

#include "temp.h"

typedef struct STK_stackEntry_ *STK_stackEntry_t;
typedef STK_stackEntry_t STK_stack_t;

struct STK_stackEntry_
{
  Temp_temp temp;
  struct STK_stackEntry_ *tail;
};

void STK_push (STK_stack_t *stack, Temp_temp temp);
void STK_pop (STK_stack_t *stack);
Temp_temp STK_top (STK_stack_t stack);
