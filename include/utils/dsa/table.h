#pragma once

/*
 * table.h - generic hash table
 *
 * No algorithm should use these functions directly, because
 *  programming with void* is too error-prone.  Instead,
 *  each module should make "wrapper" functions that take
 *  well-typed arguments and call the TAB_ functions.
 */

#define TABSIZE 127

typedef struct TAB_table_ *TAB_table;
typedef struct binder_ *binder;

struct binder_
{
  void *key;
  void *value;
  binder next;
  void *prevtop;
};

struct TAB_table_
{
  binder table[TABSIZE];
  void *top;
};

/* Make a new table mapping "keys" to "values". */
TAB_table TAB_empty (void);

/* Enter the mapping "key"->"value" into table "t",
 *  shadowing but not destroying any previous binding for "key". */
void TAB_enter (TAB_table t, void *key, void *value);

/* Look up the most recent binding for "key" in table "t" */
void *TAB_look (TAB_table t, void *key);

/* Pop the most recent binding and return its key.
 * This may expose another binding for the same key, if there was one. */
void *TAB_pop (TAB_table t);

/* Call "show" on every "key"->"value" pair in the table,
 *  including shadowed bindings, in order from the most
 *  recent binding of any key to the oldest binding in the table */
void TAB_dump (TAB_table t, void (*show) (void *key, void *value));

binder TAB_getBinder (TAB_table t, void *key);
