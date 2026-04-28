#pragma once

#include "llvmir.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>

void LL_print_xml (FILE *out, LL_instrList il, Temp_map m);
