#include "set.h"
#include "temp.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

SET_set_t
SET_constrEmpty (int n)
{
  SET_set_t s = checked_malloc (n * sizeof (bool));
  memset (s, 0, n * sizeof (bool));
  return s;
}

SET_set_t
SET_union (SET_set_t s1, SET_set_t s2, int n)
{
  SET_set_t ret = SET_constrEmpty (n);
  for (int i = 0; i < n; ++i)
    ret[i] = s1[i] || s2[i];
  return ret;
}

SET_set_t
SET_intersect (SET_set_t s1, SET_set_t s2, int n)
{
  SET_set_t ret = SET_constrEmpty (n);
  for (int i = 0; i < n; ++i)
    ret[i] = s1[i] && s2[i];
  return ret;
}

bool
SET_eq (SET_set_t s1, SET_set_t s2, int n)
{
  for (int i = 0; i < n; ++i)
    {
      if (s1[i] != s2[i])
        return 0;
    }
  return 1;
}

/*
static void
SET_printSet (const string prompt, const SET_set_t s, int n)
{
  fprintf (stderr, "%s = {", prompt);
  for (int i = 0; i < n; i++)
    fprintf (stderr, " %d,", s[i]);
  fprintf (stderr, "}\n");
}
*/

int
SET_setSize (SET_set_t s, int n)
{
  int sz = 0;
  for (int i = 0; i < n; ++i)
    {
      if (s[i])
        ++sz;
    }
  return sz;
}
