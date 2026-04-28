#include "llvmir.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"

LL_instrList llvmbody (T_stmList stmList);
LL_instrList llvmbodybeg (Temp_label lbeg);
LL_instrList llvmprolog (string methodname, Temp_tempList args,
                         T_type rettype);
LL_instrList llvmepilog (Temp_label lend);
