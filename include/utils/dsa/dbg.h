#pragma once

#include <stdio.h>
#include <stdlib.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#ifndef DEBUG
// comment the macro DEBUG when release
#define DEBUG 1
#endif

#ifdef DEBUG

/* Print debug message in cyan */
#define DBGPRT(...)                                                           \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "\033[0;36m");                                         \
      fprintf (stderr, __VA_ARGS__);                                          \
      fprintf (stderr, "\033[0m");                                            \
    }                                                                         \
  while (0)

/* Exit and print failure & position message in red when EXP is violated */
#define MYASRT(EXP)                                                           \
  do                                                                          \
    {                                                                         \
      if (!(EXP))                                                             \
        {                                                                     \
          fprintf (stderr,                                                    \
                   "\033[0;31m(%s, %d) Assertion violated: " #EXP             \
                   "\033[0m\n",                                               \
                   __FILE__, __LINE__);                                       \
          exit (1);                                                           \
        }                                                                     \
      else                                                                    \
        ;                                                                     \
    }                                                                         \
  while (0)

#else
#define DBGPRT(...)                                                           \
  do                                                                          \
    {                                                                         \
      ;                                                                       \
    }                                                                         \
  while (0)
#define MYASRT(EXP)                                                           \
  do                                                                          \
    {                                                                         \
      ;                                                                       \
    }                                                                         \
  while (0)
#endif
