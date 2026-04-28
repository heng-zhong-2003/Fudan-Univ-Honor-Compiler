#pragma once

#include "util.h"

typedef bool *SET_set_t;
SET_set_t SET_constrEmpty (int n);
SET_set_t SET_union (SET_set_t s1, SET_set_t s2, int n);
SET_set_t SET_intersect (SET_set_t s1, SET_set_t s2, int n);
bool SET_eq (SET_set_t s1, SET_set_t s2, int n);
int SET_setSize (SET_set_t s, int n);
