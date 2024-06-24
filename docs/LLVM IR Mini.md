# LLVM IR Mini

2024年春季编译（H）班

[TOC]

# Convention

Please note that LLVM has a variety of types. Here we only use 64 bit types (except for comparison results which will be a 1-bit). All pointers are 64 bits with type i64\*. (When we compile to ARM (RPi), we will switch to 32 bits.)

# Notation

- `T` stands for i64, double, or i64*.
- `iBOP` stands for one of the binary operations: add, sub, mul, and sdiv (for integers)
- `fBoP` stands for one of the binary operations: fadd, fsub, fmul, fdiv (for floats)
- `iCND` stands for one of the comparison conditions: eq, ne, sgt, sge, slt, and sle (for integers)
- `fCND` stands for one of the comparison conditions: oeq, one, ogt, oge, olt, and ole (for floats)
- `%L` stands for any local (may be thought as a `temp` in Tiger IR+). 
- `OP` can be a constant or a local. (A constant is either an integer or a float, but must be correctly "typed" in an instruction, e.g., `double 1.0` is correct, while `double 1` is not.)


# Instruction

| Instruction                                         | Meaning                                                      |
| --------------------------------------------------- | ------------------------------------------------------------ |
| %L = iBOP i64 OP1, OP2                                 | perform BOP on OP1 and OP2 and store into %L, both OP1 and OP2 must be i64                 
| %L = fBOP double OP1, OP2                                 | perform BOP on OP1 and OP2 and store into %L, both OP1 and OP2 must be double
| %L = load T, i64* OP, align 8                               | load the value at the location OP, with return type T        |
| store T OP1, i64* OP2, align 8                               | store value OP1 into the location OP2                        |
| %L = icmp CND i64 OP1, OP2                          | compare two numbers and return the one-bit result to %L, the type of %L is i1  (which can only be used as a \<cond\> in br)     
| %L = fcmp CND double OP1, OP2                       | compare two numbers and return the one-bit result to %L, the type of %L is i1 (which can only be used as a \<cond\> in br)|
| br i1 \<cond\>, label \<iftrue\>, label \<iffalse\> | Conditional branch, where \<cond\> is a one-bit value         |
| br label \<dest\>                                   | Unconditional branch                                         |
| %L = call T1 func(T1 OP1, ..., Tn OPN)               | call the function "func" and return a value, where "func" can be a named function (e.g., `%1=call i64* @malloc(i64 16)`) or an address (usually stored in a local, e.g., `%2=call double %x(i64 %y)`)                 |
| call void func(T1 OP1, ... ,Tn OPN)                  | call the function but not returning anything                                            |
| ret T OP                                            | function return value                                        |
| ret void                                            | function return                                              |
| %L = getelementptr i64\*, i64\* OP1, i64 OP2                | Add offset OP2 to ptr OP1 and returns a new ptr to %L. Note that "offset" is in terms of 8 bytes (i64\*), e.g., offset=1 moves the pointer by 8 bytes. |      |
| %L = inttoptr i64 OP to i64*                        | convert an integer OP to a ptr and return to %L              |
| %L = ptrtoint i64* OP to i64                        | convert a ptr OP to an integer and return to %L    
| %L = fptosi double OP to i64			              | convert a double to an integer and return to %L
| %L = sitofp i64 OP to double			              | convert an integer to a double and return to %L
| %L = phi T [ \<val0\>, \<label0\>], ...           | The phi function (will be used later)                                            |




