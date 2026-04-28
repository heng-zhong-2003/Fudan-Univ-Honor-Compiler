# RPiA mini (2024)

In this semester, we will only use a small subset of ARM instructions for our class. Since the full ARM instruction set is quite complex (and powerful), for simplicity, we are only dealing with RPiAmini in our class, and you are only allowed to use the instructions in RPiAmini in your submitted work.

In RPiAmini, we only use signed 32bit intergers and 32bit floating point numbers.

The other structures such as function declaration, external functions, data allocation etc may be discussed and used for assignments. See the "Rasberry Pi Assembler" document.

## Integers

We first describe ARM instructions for integers. Instructions for floating point numbers are described below.

##Arithmetic instructions


|  Instruction |   Meaning | 
|---|---|
|`add destReg, srcReg, op2` | destReg = srcReg + op2 |
|`sub destReg, srcReg, op2` | destReg = srcReg - op2 |
|`rsb destReg, srcReg, op2` | destReg = op2 - srcReg |
|`mul destReg, srcReg1, srcReg2` | destReg = srcReg1 * srcReg2 |

Note that ARM doesn’t have a division instruction.

* All arithmetic operations are done in 2’s complement (i.e., we are dealing with a signed int, and in LLVM IR, it's a signed `i32`)
* `rsb` (reverse subtract) is useful for negate a number (`rsb r0, r0, #0`, which can be written as `neg r0, r0` in assembly)
* `op2 can be:
	* a register or a <imm8m> constant (formed by 8bit right rotate: e.g., `0x0F000000`) 
	* allow different kinds of shift operations (e.g., `r0, ASL #1`, which is `r0*2`)
* A register can appear at multiple positions (except `mul`: `destReg` cannot be the same as `srcReg1`)：So `add r0, r0, r0` is OK, but `mul r0, r0, r0` is not.
* For division, there is an external function we may use (integer division): `bl    __aeabi_idiv`.

##Logical instructions
|  Instruction |   Meaning | 
|---|---|
|`and destReg, srcReg, op2` |destReg = srcReg AND op2|
|`orr destReg, srcReg, op2` |destReg = srcReg OR op2|

These are actually bit-wise logical operations

* `op2` is the same as for arithmetic instructions
* A register can appear at multiple positions, e.g., `and r0, r0, r1`

## Move and Memory Instructions
|  Instruction |   Meaning | 
|---|---|
| `mov destReg, op2` | destReg = op2 |
| `ldr destReg, [locationReg[, offset]]` |  destReg = content in mem location |
|`ldr destReg, =<label>` | destReg = address of the label |
|`str srcReg, [locationReg[, offset]]` | Mem location = srcReg |

Notes:

* Same `op2` as arithmetic instructions.
* offset can be an immediate, or a register, and is optional. Examples:
	* `ldr r0, [r1]`
	* `ldr r0, [r1, #12]`
	* `ldr r0, [r1, r2]`
* No such an instruction: `str destReg, =<label>`

## Compare and branch

|  Instruction |   Meaning | 
|---|---|
| `cmp Reg1, op2`| perform reg1-op2 and store the result status into `CPSR` |
| `b =label` | simple branch |
| `bl =label` | branch with next address stored in `lr` register |
| `bx register` | branch to the location in the register |
| `blx register` |  same as bx but with next address stored in lr |

Notes:

* All of the above may be followed by the suffix used as condition for branching (conditioned on the CPSR register, usually the result of cmp instruction):
	* `eq`, `ne`, `ge`, `gt`, `le`, `lt`
	* E.g., `blxeq r3`， `bgt =fun`

## No operation

|  Instruction |   Meaning | 
|---|---|
| `nop` | do nothing |

Do nothing. Sometimes useful to make the program easier to structure.

## Floating Point Numbers

###Regiters

There are **32** floating pointer registers **s0-s31** (these registers are also used for double precision floating point numbers, ie. 64 bits or double, and for vector operations, but these are beyond the scope of this class).

**Arithmetic operations:** There is a set of instructions specifically for these registers. They the same as those for the integer registers, except prefixed with "v" and suffixed with "f32". For example, `vadd.f32` for addition of two (32 bit) floats. Other instructions are: `vsub.f32`, `vmul.f32`, and `vdiv.f32`. 

**Move instruction:** Move (`vmov`) works for registers between float registers and between float registers and integer registers. Move doesn't do any conversion: 32 bits are copied without change!

**Conversion:** Conversion instruction (`vcvt`) works between float registers as follows: `vcvt.s32.f32` and `vcvt f32.s32`. Note s32 is the signed integer. (Once a number is converted into integer, it may be moved to an integer register to be processed further.)

**Example:** The following is an example code:

```
          vmov.f32 s2, #2.0
          vmov.f32 s3, #8.0
          vsub.f32 s1, s3, s2
          vcvt.s32.f32 s2, s1
          vmov r0, s2
          add r1, r0, #9
```

**Load and store:** Load and store may use `vstr.f32` and `vldr.f32` instructions. The memory address part is the same as for `str` and `ldr` instructions. For example:

```
          vstr.f32 s1, [r0, #8]
          vldr.f32 s2, [r0, #8]
```

**Compare:** Use `vcmp.f32`.

## Function Calling Convention

Registers `r0-r3` and `s0-s15` are caller-saved registers. All other registers are callee saved. Return value is in `r0` and `s0`. 


