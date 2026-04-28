#pragma once

#include "env.h"
#include "fdmjast.h"
#include "tigerirp.h"
#include "translate.h"
// #include "types.h"
#include "util.h"
#include <stdio.h>

/* the name of the main class, base class for all classes
 * not extending some other classes
 */
static char mainClNm[] = "MAIN_CLASS";

void transError (FILE *out, A_pos pos, string msg);
T_funcDeclList transA_Prog (FILE *out, A_prog p, int arch_size);
