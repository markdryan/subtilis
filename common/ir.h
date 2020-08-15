/*
 * Copyright (c) 2017 Mark Ryan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SUBTILIS_IR_H
#define __SUBTILIS_IR_H

#include <stdint.h>

#include "buffer.h"
#include "builtins.h"
#include "constant_pool.h"
#include "error.h"
#include "settings.h"
#include "string_pool.h"
#include "type.h"

#define SUBTILIS_IR_MAX_ARGS_PER_TYPE 16

/* clang-format off */
enum {
	SUBTILIS_IR_REG_UNDEFINED,
	SUBTILIS_IR_REG_GLOBAL,
	SUBTILIS_IR_REG_LOCAL,
	SUBTILIS_IR_REG_STACK,
	SUBTILIS_IR_REG_TEMP_START,
};

/* clang-format on */

typedef enum {
	SUBTILIS_OP_INSTR,
	SUBTILIS_OP_LABEL,
	SUBTILIS_OP_CALL,
	SUBTILIS_OP_CALLI32,
	SUBTILIS_OP_CALLREAL,
	SUBTILIS_OP_PHI,
	SUBTILIS_OP_MAX,
} subtilis_op_type_t;

/* IMPORTANT.
 * Don't change the order of these enums.  There are static arrays
 * in ir.c and vm.c that rely on this ordering.
 */

typedef enum {
	/*
	 * addi32 r0, r1, r2
	 *
	 * Adds two 32 bit signed integers stored in registers and stores the
	 * result in another register.
	 *
	 * r0 = r1 + r2
	 */
	SUBTILIS_OP_INSTR_ADD_I32,

	/*
	 * addr fp0, fp1, fp2
	 *
	 * Adds two 64 bit doubles stored in floating point registers and stores
	 * the result in a third floating point register.
	 *
	 * fp0 = fp1 + fp2
	 */
	SUBTILIS_OP_INSTR_ADD_REAL,

	/*
	 * subi32 r0, r1, r2
	 *
	 * Subtracts two 32 bit integers stored in registers storing the result
	 * in a third register.
	 *
	 * r0 = r1 - r2
	 */

	SUBTILIS_OP_INSTR_SUB_I32,

	/*
	 * subr fp0, fp1, fp2
	 *
	 * Subtracts two 64 bit doubles stored in floating pointers registers
	 * storing the result in a third floating point register.
	 *
	 * fp0 = fp1 - fp2
	 */
	SUBTILIS_OP_INSTR_SUB_REAL,

	/*
	 * muli32 r0, r1, r2
	 *
	 * Multiples two 32 bit signed integers stored in registers storing the
	 * result in a third register.
	 *
	 * r0 = r1 * r2
	 */

	SUBTILIS_OP_INSTR_MUL_I32,

	/*
	 * mulr fp0, fp1, fp2
	 *
	 * Multiplies two 64 bit doubles stored in floating point registers
	 * storing the result in a third floating point register.
	 *
	 * fp0 = fp1 * fp2
	 */

	SUBTILIS_OP_INSTR_MUL_REAL,

	/*
	 * divi32 r0, r1, r2
	 *
	 * Divides two 32 bit signed integers stored in registers storing the
	 * result in a third register.  If a division by zero occurs the
	 * error flag is set and an error is written into the error offset for
	 * the current section.
	 *
	 * r0 = r1 / r2
	 */

	SUBTILIS_OP_INSTR_DIV_I32,

	/*
	 * modi32 r0, r1, r2
	 *
	 * Divides two 32 bit signed integers stored in registers storing the
	 * remainder in a third register.  If a division by zero occurs the
	 * error flag is set and an error is written into the error offset for
	 * the current section.
	 *
	 * r0 = r1 % r2
	 */

	SUBTILIS_OP_INSTR_MOD_I32,

	/*
	 * divr fp0, fp1, fp2
	 *
	 * Divides two 64 bit doubles stored in floating point registers.
	 * The result is stored in a third floating point register.
	 * If a division by zero occurs the error flag is set and an error is
	 * written into the error offset for the current section.
	 *
	 * fp0 = fp1 / fp2
	 */

	SUBTILIS_OP_INSTR_DIV_REAL,

	/*
	 * addii32 r0, r1, #i32
	 *
	 * Adds a 32 bit immediate constant to a 32 bit integer stored
	 * in a register.  The result is stored in a second register.
	 *
	 * r0 = r1 + #i32
	 */

	SUBTILIS_OP_INSTR_ADDI_I32,

	/*
	 * addir fp0, fp1, #r
	 *
	 * Adds a 64 bit double immediate constant to a 64 bit double
	 * stored in a floating point register.  The result is stored
	 * in a second floating point register.
	 *
	 * fp0 = fp1 + #r
	 */

	SUBTILIS_OP_INSTR_ADDI_REAL,

	/*
	 * subii32 r0, r1, #i32
	 *
	 * Subtracts a 32 bit integer immediate constant from a 32 bit
	 * integer stored in a register.  The result is stored in a second
	 * register.
	 *
	 * r0 = r1 - #i32
	 */

	SUBTILIS_OP_INSTR_SUBI_I32,

	/*
	 * subir fp0, fp1, #r
	 *
	 * Subtracts a 64 bit double immediate constant from a 64 bit
	 * double stored in a register.  The result is stored in a second
	 * register.
	 *
	 * fp0 = fp1 - #r
	 */

	SUBTILIS_OP_INSTR_SUBI_REAL,

	/*
	 * mulii32 r0, r1, #i32
	 *
	 * Multiplies a 32 bit immediate constant by a 32 bit integer
	 * stored in a register.  The resulting product is stored in
	 * a another register.
	 *
	 * r0 = r1 * #i32
	 */

	SUBTILIS_OP_INSTR_MULI_I32,

	/*
	 * mulir fp0, fp1, #r
	 *
	 * Multiplies a 64 bit double immediate constant by a 64 bit
	 * double stored in a register.  The result is placed in a
	 * second floating point register.
	 *
	 * fp0 = fp1 * r
	 */

	SUBTILIS_OP_INSTR_MULI_REAL,

	/*
	 * divii32 r0, r1, #i32
	 *
	 *- Divides a 32 bit integer stored in a register by a 32 bit
	 * integer immediate constant.  The result is stored in a second
	 * register.
	 *
	 * r0 = r1 / #i32
	 */

	SUBTILIS_OP_INSTR_DIVI_I32,

	/*
	 * divir fp0, fp1, #r
	 *
	 * Divides a 64 bit double stored in a register by 64 bit double
	 * immediate constant.  The result is stored in a second floating
	 * point register.
	 *
	 * fp0 = fp1 / #r
	 */

	SUBTILIS_OP_INSTR_DIVI_REAL,

	/*
	 * loadoi8 r0, r1, #off
	 *
	 * Loads an 8 bit integer from a memory location formed by
	 * the sum of the contents of a register and a constant into another
	 * register.
	 *
	 * r0 = [r1 + #off]
	 */

	SUBTILIS_OP_INSTR_LOADO_I8,

	/*
	 * loadoi32 r0, r1, #off
	 *
	 * Loads a 32 bit integer from a memory location formed by
	 * the sum of the contents of a register and a constant into another
	 * register.
	 *
	 * r0 = [r1 + #off]
	 */

	SUBTILIS_OP_INSTR_LOADO_I32,

	/*
	 * loador fp0, r0, #off
	 *
	 * Loads a 64 bit double from a memory location formed by
	 * the sum of the contents of a register and a constant.stored in a
	 * register into a floating pointer register.
	 *
	 * fp0 = [r0 + #off]
	 */

	SUBTILIS_OP_INSTR_LOADO_REAL,

	/*
	 * loadi32 r0, r1
	 *
	 * Loads a 32 bit integer from a memory location stored in a
	 * register into another register.
	 *
	 * r0 = [r1]
	 */

	SUBTILIS_OP_INSTR_LOAD_I32,

	/*
	 * loadr fp0, [r0]
	 *
	 * Loads a 64 bit double from a memory location stored in a
	 * register into a floating point register.
	 *
	 * fp0 = [r0]
	 */

	SUBTILIS_OP_INSTR_LOAD_REAL,

	/*
	 * storeoi8 r0, r1, #off
	 *
	 * Stores the least significant byte of register a into the
	 * memory location defined by the sum of a register and a constant.
	 *
	 * [r1 + #off] = r0 & 0xff
	 */

	SUBTILIS_OP_INSTR_STOREO_I8,

	/*
	 * storeoi32 r0, r1, #off
	 *
	 * Stores a 32 bit integer stored in a register into the
	 * memory location defined by the sum of a register and a constant.
	 *
	 * [r1 + #off] = r0
	 */

	SUBTILIS_OP_INSTR_STOREO_I32,

	/*
	 * storeor fp0, r1, #off
	 *
	 * Stores a 64 bit double stored in a floating point register into
	 * the  memory location defined by the sum of a register and a constant.
	 *
	 * [r1 + off] = fp0
	 */

	SUBTILIS_OP_INSTR_STOREO_REAL,

	/*
	 * storei32 r0, r1
	 *
	 * Stores a 32 bit integer contained in a register into a
	 * memory location contained within a second register.
	 *
	 * [r1] = r0
	 */

	SUBTILIS_OP_INSTR_STORE_I32,

	/*
	 * storer fp0, r1
	 *
	 * Stores a 64 bit double  contained in a register into a memory
	 * location contained within a register.
	 *
	 * [r1] = fp0
	 */

	SUBTILIS_OP_INSTR_STORE_REAL,

	/*
	 * movii32 r0, #i32
	 *
	 * Moves a 32 bit integer constant into a register.
	 *
	 * r0 = #i32
	 */

	SUBTILIS_OP_INSTR_MOVI_I32,

	/*
	 * movir fp0, #r
	 *
	 * Moves a 64 bit floating point constant into a floating point
	 * register.
	 *
	 * fp0 = r
	 */

	SUBTILIS_OP_INSTR_MOVI_REAL,

	/*
	 * mov r0, r1
	 *
	 * Moves the contents of one 32 bit integer register into another.
	 *
	 * r0 = r1
	 */

	SUBTILIS_OP_INSTR_MOV,

	/*
	 * movfp fp0, fp1
	 *
	 * Moves the contents of one floating point register into another.
	 *
	 * fp0 = fp1
	 */

	SUBTILIS_OP_INSTR_MOVFP,

	/*
	 * printstr r0, r1
	 *
	 * Prints r1 bytes from the string pointed to by r0.
	 */

	SUBTILIS_OP_INSTR_PRINT_STR,

	/*
	 * printnl
	 *
	 * Prints a newline character to the output stream.
	 */

	SUBTILIS_OP_INSTR_PRINT_NL,

	/*
	 * rsubii32 r0, r1, #i32
	 *
	 * Subtracts a 32 bit integer stored in a register from a
	 * 32 bit integer immediate constant.  The result is stored in a second
	 * register.
	 *
	 * r0 = #i32 - r1
	 */

	SUBTILIS_OP_INSTR_RSUBI_I32,

	/*
	 * rsubir fp0, fp1, #r
	 *
	 * Subtracts a 64 bit double stored in a register from a 64 bit double
	 * immediate constant.  The result is stored in a second register.
	 *
	 * fp0 = #r - fp1
	 */

	SUBTILIS_OP_INSTR_RSUBI_REAL,

	/*
	 * rdivir fp0, fp1, #r
	 *
	 * Divides a 64 bit double immediate constant by a 64 bit double
	 * stored in a register.  The result is stored in a second register.
	 * If a division by zero occurs the error flag is set and an error
	 * is written into the error offset for the current section.
	 *
	 * fp0 = #r / fp1
	 */

	SUBTILIS_OP_INSTR_RDIVI_REAL,

	/*
	 * andi32 r0, r1, r2
	 *
	 * Ands two 32 bit signed integers stored in registers and stores the
	 * result in another register.
	 *
	 * r0 = r1 & r2
	 */
	SUBTILIS_OP_INSTR_AND_I32,

	/*
	 * andii32 r0, r1, #i32
	 *
	 * Ands a 32 bit immediate constant with a 32 bit integer stored
	 * in a register.  The result is stored in a second register.
	 *
	 * r0 = r1 & #i32
	 */

	SUBTILIS_OP_INSTR_ANDI_I32,

	/*
	 * ori32 r0, r1, r2
	 *
	 * Ors two 32 bit signed integers stored in registers and stores the
	 * result in another register.
	 *
	 * r0 = r1 | r2
	 */
	SUBTILIS_OP_INSTR_OR_I32,

	/*
	 * orii32 r0, r1, #i32
	 *
	 * Ors a 32 bit immediate constant with a 32 bit integer stored
	 * in a register.  The result is stored in a second register.
	 *
	 * r0 = r1 | #i32
	 */

	SUBTILIS_OP_INSTR_ORI_I32,

	/*
	 * eori32 r0, r1, r2
	 *
	 * Eors two 32 bit signed integers stored in registers and stores the
	 * result in another register.
	 *
	 * r0 = r1 ^ r2
	 */
	SUBTILIS_OP_INSTR_EOR_I32,

	/*
	 * eorii32 r0, r1, #i32
	 *
	 * Eors a 32 bit immediate constant with a 32 bit integer stored
	 * in a register.  The result is stored in a second register.
	 *
	 * r0 = r1 ^ #i32
	 */

	SUBTILIS_OP_INSTR_EORI_I32,

	/*
	 * noti32 r0, r1
	 *
	 * Stores the complement of a 32 bit integer stored in a register
	 * in a second register.
	 *
	 * r0 = ~r1
	 */

	SUBTILIS_OP_INSTR_NOT_I32,

	/*
	 * eqi32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in R0
	 * if they are equal, and 0 otherwise.
	 *
	 * r0 = r1 == r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_EQ_I32,

	/*
	 * eqr r0, fp1, fp2
	 *
	 * Compares the 32 bit integers in fp1 and fp2, storing -1 in R0
	 * if they are equal, and 0 otherwise.
	 *
	 * r0 = fp1 == fp2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_EQ_REAL,

	/*
	 * eqii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in R0 if they are equal, and 0 otherwise.
	 *
	 * r0 = r1 == i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_EQI_I32,

	/*
	 * eqiir r0, fp1, #r
	 *
	 * Compares the 64 bit double in r1 with an immediate
	 * constant, storing -1 in R0 if they are equal, and 0 otherwise.
	 *
	 * r0 = fp1 == #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_EQI_REAL,

	/*
	 * neqi32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in R0
	 * if they are not equal, and 0 otherwise.
	 *
	 * r0 = r1 != r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_NEQ_I32,

	/*
	 * neqr r0, f1, f2
	 *
	 * Compares the 64 bit doubles in f1 and f2, storing -1 in R0
	 * if they are not equal, and 0 otherwise.
	 *
	 * r0 = f1 != f2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_NEQ_REAL,

	/*
	 * neqii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in R0 if they are not equal, and 0 otherwise.
	 *
	 * r0 = r1 != i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_NEQI_I32,

	/*
	 * neqir r0, f1, #r
	 *
	 * Compares the 64 bit integer in f1 with an immediate
	 * constant, storing -1 in R0 if they are not equal, and 0 otherwise.
	 *
	 * r0 = f1 != #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_NEQI_REAL,

	/*
	 * gti32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in
	 * r0 if r1 > r2 and 0 otherwise.
	 *
	 * r0 = r1 > r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GT_I32,

	/*
	 * gtr r0, fp1, fp2
	 *
	 * Compares the 64 bit doubles in fp1 and fp2, storing -1 in
	 * r0 if fp1 > fp2 and 0 otherwise.
	 *
	 * r0 = fp1 > fp2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GT_REAL,

	/*
	 * gtii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in r0 if the value in r1 is greater than
	 * the immediate constant and 0 otherwise.
	 *
	 * r0 = r1 > i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTI_I32,

	/*
	 * gtir r0, fp1, #r
	 *
	 * Compares the 64 bit double in fp1 with an immediate
	 * constant, storing -1 in r0 if the value in fp1 is greater than
	 * the immediate constant and 0 otherwise.
	 *
	 * r0 = fp > #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTI_REAL,

	/*
	 * ltei32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in
	 * r0 if r1 <= r2 and 0 otherwise.
	 *
	 * r0 = r1 <= r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTE_I32,

	/*
	 * lter32 r0, fp1, fp2
	 *
	 * Compares the 64 bit doubles in fp1 and fp2, storing -1 in
	 * r0 if fp1 <= fp2 and 0 otherwise.
	 *
	 * r0 = fp1 <= fp2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTE_REAL,

	/*
	 * lteii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in r0 if the value in r1 is less than
	 * or equal to the immediate constant and 0 otherwise.
	 *
	 * r0 = r1 <= i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTEI_I32,

	/*
	 * lteir r0, fp, #r
	 *
	 * Compares the 64 bit double in fp1 with an immediate
	 * constant, storing -1 in r0 if the value in fp1 is less than
	 * or equal to the immediate constant and 0 otherwise.
	 *
	 * r0 = fp1 <= #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTEI_REAL,

	/*
	 * lti32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in
	 * r0 if r1 < r2 and 0 otherwise.
	 *
	 * r0 = r1 < r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LT_I32,

	/*
	 * ltr r0, fp1, fp2
	 *
	 * Compares the 64 bit doubles in fp1 and fp2, storing -1 in
	 * r0 if fp1 < fp2 and 0 otherwise.
	 *
	 * r0 = fp1 < fp2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LT_REAL,

	/*
	 * ltii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in r0 if the value in r1 is less than
	 * the immediate constant and 0 otherwise.
	 *
	 * r0 = r1 < i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTI_I32,

	/*
	 * ltir r0, fp1, #r
	 *
	 * Compares the 64 bit doubles in fp1 with an immediate
	 * constant, storing -1 in r0 if the value in fp1 is less than
	 * the immediate constant and 0 otherwise.
	 *
	 * r0 = fp1 < #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_LTI_REAL,

	/*
	 * gtei32 r0, r1, r2
	 *
	 * Compares the 32 bit integers in r1 and r2, storing -1 in
	 * r0 if r1 >= r2 and 0 otherwise.
	 *
	 * r0 = r1 >= r2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTE_I32,

	/*
	 * gter r0, fp1, fp2
	 *
	 * Compares the 64 bit doubles in fp1 and fp2, storing -1 in
	 * r0 if fp1 >= fp2 and 0 otherwise.
	 *
	 * r0 = fp1 >= fp2 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTE_REAL,

	/*
	 * gteii32 r0, r1, i32
	 *
	 * Compares the 32 bit integers in r1 with a 32 bit immediate
	 * constant, storing -1 in r0 if the value in r1 is greater than
	 * or equal to the immediate constant and 0 otherwise.
	 *
	 * r0 = r1 >= i32 ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTEI_I32,

	/*
	 * gteir r0, fp1, #r
	 *
	 * Compares the 64 bit double in fp1 with an immediate
	 * constant, storing -1 in r0 if the value in fp1 is greater than
	 * or equal to the immediate constant and 0 otherwise.
	 *
	 * r0 = fp1 >= #r ? -1 : 0
	 */

	SUBTILIS_OP_INSTR_GTEI_REAL,

	/*
	 * JMPC r0, label1, label2
	 *
	 * Jumps to label1 if the value of r0 is non zero and jumps to label2
	 * otherwise.  This instruction made be fused with a preceding
	 * instruction by the backend, effectively discarding the value in r0.
	 * If the value is signficant, use SUBTILIS_OP_INSTR_JMPC_NF.
	 *
	 */

	SUBTILIS_OP_INSTR_JMPC,

	/*
	 * JMPC r0, label1, label2
	 *
	 * Jumps to label1 if the value of r0 is non zero and jumps to label2
	 * otherwise.  This instruction must never be fused by the backends with
	 * a preeeding comparison instruction.
	 *
	 */

	SUBTILIS_OP_INSTR_JMPC_NF,

	/*
	 * JMPC label1
	 *
	 * Unconditionally Jumps to label1.
	 *
	 */

	SUBTILIS_OP_INSTR_JMP,

	/*
	 * RET
	 *
	 * returns from a sub-routine call.
	 *
	 */

	SUBTILIS_OP_INSTR_RET,

	/*
	 * RET_I32
	 *
	 * returns from the value stored in a 32 bit register
	 * from a sub-routine call.
	 *
	 */

	SUBTILIS_OP_INSTR_RET_I32,

	/*
	 * RETI_I32
	 *
	 * returns a 32 bit value from a sub-routine call.
	 *
	 */

	SUBTILIS_OP_INSTR_RETI_I32,

	/*
	 * RET_REAL
	 *
	 * returns from the value stored in a 64 bit floating point register
	 * from a sub-routine call.
	 *
	 */

	SUBTILIS_OP_INSTR_RET_REAL,

	/*
	 * RETI_REAL
	 *
	 * returns a 64 bit floating point value from a sub-routine call.
	 *
	 */

	SUBTILIS_OP_INSTR_RETI_REAL,

	/*
	 * lsli32 r0, r1, r2
	 *
	 * Shifts r1 left by the contents of r2 and stores the resulting value
	 * in r0.
	 *
	 * r0 = r1 << r2
	 */
	SUBTILIS_OP_INSTR_LSL_I32,

	/*
	 * lslii32 r0, r1, #i32
	 *
	 * Shifts r1 left by the constant i32 and stored the resulting value
	 * in r0.
	 *
	 * r0 = r1 << #i32
	 */

	SUBTILIS_OP_INSTR_LSLI_I32,

	/*
	 * lsri32 r0, r1, r2
	 *
	 * Performs a logical right shift of r1 by the contents of r2 and stores
	 * the resulting value in r0.
	 *
	 * r0 = r1 >> r2
	 */

	SUBTILIS_OP_INSTR_LSR_I32,

	/*
	 * lsrii32 r0, r1, #i32
	 *
	 * Performs a logical right shift of r1 by #i32 and stores the resulting
	 * value in r0.
	 *
	 * r0 = r1 >> #i32
	 */

	SUBTILIS_OP_INSTR_LSRI_I32,

	/*
	 * asri32 r0, r1, r2
	 *
	 * Performs an arthimetic right shift of r1 by the contents of r2 andi32
	 * stores the resulting value in r0.
	 *
	 * r0 = r1 >>> r2
	 */

	SUBTILIS_OP_INSTR_ASR_I32,

	/*
	 * asrii32 r0, r1, #i32
	 *
	 * Performs an arthimetic right shift of r1 by #i32 and stores These
	 * resulting value in r0.
	 *
	 * r0 = r1 >>> #i32
	 */

	SUBTILIS_OP_INSTR_ASRI_I32,

	/*
	 * movi32fp f0, r0
	 *
	 * Copies the contents of a 32 bit integer register into a floating
	 * point register.
	 *
	 * f0 = r0
	 */

	SUBTILIS_OP_INSTR_MOV_I32_FP,

	/*
	 * movfpi32 r0, f0
	 *
	 * Copies the contents of a floating point register into a 32
	 * bit integer register.  The number is truncated by the copy.
	 *
	 * f0 = r0
	 */

	SUBTILIS_OP_INSTR_MOV_FP_I32,

	/*
	 * movfprdi32 r0, f0
	 *
	 * Copies the contents of a floating point register into a 32
	 * bit integer register.  The number is rounded down to - infinity.
	 *
	 * f0 = r0
	 */

	SUBTILIS_OP_INSTR_MOV_FPRD_I32,

	/*
	 * nop
	 *
	 * Does nothing.  This is used a placeholder for type promotion
	 * of parameters.
	 *
	 */

	SUBTILIS_OP_INSTR_NOP,

	/*
	 * mode_i32 r0
	 *
	 * Changes the screen mode to that of the number contained in the
	 * specified register.  If an error occurs the error flag is set
	 * and an error is written into the error offset for the current
	 * section.  Note errors are ignored unless handle_graphics_errors
	 * is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_MODE_I32,

	/*
	 * plot r0, r1, r2
	 *
	 * Platform specific drawing routine.  If an error occurs the error
	 * flag is set and an error is written into the error offset for
	 * the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_PLOT,

	/*
	 * gcol r0, r1
	 *
	 * Change the colour used for graphics operations.  If an error occurs
	 * the error flag is set and an error is written into the error offset
	 * for the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_GCOL,

	/*
	 * origin r0, r1
	 *
	 * Sets the graphics origin.  If an error occurs the error flag is set
	 * and an error is written into the error offset for the current
	 * section.  Note errors are ignored unless handle_graphics_errors
	 * is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_ORIGIN,

	/*
	 * gettime r0
	 *
	 * Stores the current value of the system timer in centi-seconds
	 * in r0.  If an error occurs the error flag is set and an error is
	 * written into the error offset for the current section.
	 *
	 */

	SUBTILIS_OP_INSTR_GETTIME,

	/*
	 * cls
	 *
	 * Clears the text viewport.  If an error occurs the error flag is set
	 * and an error is written into the error offset for the current
	 * section.  Note errors are ignored unless prv_handle_graphics_error
	 * is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_CLS,

	/*
	 * clg
	 *
	 * Clears the graphics viewport. If an error occurs the error flag
	 * is set and an error is written into the error offset for these
	 * current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_CLG,

	/*
	 * on
	 *
	 * Turns on the text cursor.  If an error occurs the error flag is set
	 * and an error is written into the error offset for the current
	 * section.  Note errors are ignored unless handle_graphics_errors is
	 * set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_ON,

	/*
	 * off
	 *
	 * Turns off the text cursor.  If an error occurs the error flag is set
	 * and an error is written into the error offset for the current
	 * section.  Note errors are ignored unless handle_graphics_errors is
	 * set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_OFF,

	/*
	 * wait
	 *
	 * Wait for end of current display frame.  If an error occurs the error
	 * flag is set and an error is written into the error offset for these
	 * current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 *
	 */

	SUBTILIS_OP_INSTR_WAIT,

	/*
	 * sin fp0, fp1
	 *
	 * fp0 = sin(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_SIN,

	/*
	 * cos fp0, fp1
	 *
	 * fp0 = cos(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_COS,

	/*
	 * tan fp0, fp1
	 *
	 * fp0 = tan(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_TAN,

	/*
	 * asn fp0, fp1
	 *
	 * fp0 = asn(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_ASN,

	/*
	 * acs fp0, fp1
	 *
	 * fp0 = acs(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_ACS,

	/*
	 * atn fp0, fp1
	 *
	 * fp0 = atn(fp1) where fp1 is an angle in radians.
	 *
	 */

	SUBTILIS_OP_INSTR_ATN,

	/*
	 * sqr fp0, fp1
	 *
	 * fp0 = squarerootof(fp1)
	 *
	 */

	SUBTILIS_OP_INSTR_SQR,

	/*
	 * log fp0, fp1
	 *
	 * If fp <=0 the error flag is set and an error is written into
	 * the error offset for the current section.
	 *
	 * fp0 = log10(fp1)
	 *
	 */

	SUBTILIS_OP_INSTR_LOG,

	/*
	 * ln fp0, fp1
	 *
	 * If fp <=0 the error flag is set and an error is written into
	 * the error offset for the current section.
	 *
	 * fp0 = loge(fp1)
	 *
	 */

	SUBTILIS_OP_INSTR_LN,

	/*
	 * absr fp0, fp1
	 *
	 * fp0 = absolute value of (fp1)
	 *
	 */

	SUBTILIS_OP_INSTR_ABSR,

	/*
	 * pow fp0, fp1, fp2
	 *
	 * fp0 = fp1 ^ fp2
	 *
	 */

	SUBTILIS_OP_INSTR_POWR,

	/*
	 * expr fp0, fp1
	 *
	 * fp0 = e ^ fp1
	 *
	 */

	SUBTILIS_OP_INSTR_EXPR,

	/*
	 * get r0
	 *
	 * Wait for a key press from the user and store the ASCII code
	 * in r0.  If an error occurs the error flag is set and an error
	 * is written into the error offset for the current section.
	 *
	 */

	SUBTILIS_OP_INSTR_GET,

	/*
	 * get_timeout r0, r1
	 *
	 * Wait for a key press from the user for r1 centi-seconds and store
	 * the ASCII code of the pressed key in r0. If no key was pressed
	 * within the time limit r0 will contain -1.  If an error occurs these
	 * error flag is set and an error is written into the error offset for
	 * the current section.
	 */

	SUBTILIS_OP_INSTR_GET_TO,

	/*
	 * inkey r0, r1
	 *
	 * r0 is set to 255 if the key specified in r1 is currently depressed,
	 * 0 otherwise.  If an error occurs the error flag is set and an error
	 * is written into the error offset for the current section.
	 */

	SUBTILIS_OP_INSTR_INKEY,

	/*
	 * osbyteid r0
	 *
	 * r0 is set to a single byte that identifies the operating system.
	 * Known values are:
	 * - &A0 for Arthur 1.20
	 * - &A1 for RISCOS 2.0
	 * - &A2 for RISCOS 2.01
	 * - &A3 for RISCOS 3
	 * - &A4 for RISCOS 3.10 and 3.11
	 * - &A7 for RISCOS 3.7.1
	 * - &01 for Subtilis VM
	 * - &02 for Subtilis ARM VM
	 *
	 * If an error occurs the error flag is set and an error is written
	 * into the error offset for the current section.
	 */

	SUBTILIS_OP_INSTR_OS_BYTE_ID,

	/*
	 *
	 * vdui i32
	 *
	 * Sends the lowest 8 bits of the integer constant to the output
	 * stream.  If an error occurs the error flag is set and an error
	 * is written into the error offset for the current section.
	 */

	SUBTILIS_OP_INSTR_VDUI,

	/*
	 *
	 * vdui r0
	 *
	 * Sends the lowest 8 bits of the integer stored in r0 to the
	 * output stream.  If an error occurs the error flag is set and
	 * an error is written into the error offset for the current section.
	 * Note errors are ignored unless handle_graphics_errors is set
	 * to false.
	 */

	SUBTILIS_OP_INSTR_VDU,

	/*
	 *
	 * point r0, r1, r2
	 *
	 * Stores the pixel colour at location r1, r2 in r0.  If an error occurs
	 * the error flag is set and an error is written into the error offsets
	 * for the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 */

	SUBTILIS_OP_INSTR_POINT,

	/*
	 *
	 * tint r0, r1, r2
	 *
	 * Stores the tint at location r1, r2 in r0.  If an error occurs
	 * the error flag is set and an error is written into the error offsets
	 * for the current section. Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 */

	SUBTILIS_OP_INSTR_TINT,

	/*
	 *
	 * end
	 *
	 * Causes the execution of the program to end successfully and
	 * immediately.
	 */

	SUBTILIS_OP_INSTR_END,

	/*
	 *
	 * testesc
	 *
	 * Checks the system escape condition is set.  If it is, the error
	 * flag is set and an error is written into the error offsets for the
	 * current section. This instruction also clears the system escape.
	 * condition.
	 */

	SUBTILIS_OP_INSTR_TESTESC,

	/*
	 *
	 * alloc r0, r1
	 *
	 * Attempts to allocate the number of bytes stored in r1 on the heap.
	 * A pointer to the new memory is stored returned in r0 on success.
	 * The reference count of the block is set to 1.
	 * If an error occurs the error flag is set and an error is written
	 * into the error offsets for the current section.
	 */

	SUBTILIS_OP_INSTR_ALLOC,

	/*
	 *
	 * realloc r0, r1, r2
	 *
	 * Attempts to increase the size of the block of memory pointed by r0
	 * so that it contains at least r1 bytes.  If r1 is less than the
	 * current size of the memory block this instruction does nothing.  If
	 * the memory block cannot be resized, a new memory block is allocated
	 * with at least r1 bytes, and the contents of the block pointed to r0
	 * are copied into the new memory location.  A deref is performed on
	 * the memory block pointed to by r0, and this memory will be freed
	 * if its reference count falls to zero.  The reference count of the
	 * new memory block are set to 1.
	 *
	 * Unless there is an error, r2 always points to the resized memory
	 * block on exit.  r2 and r0 may be equal.  They will be different if
	 * the block needs to be moved.
	 *
	 * If an error occurs the error flag is set and an error is written
	 * into the error offsets for the current section.
	 */

	SUBTILIS_OP_INSTR_REALLOC,

	/*
	 *
	 * ref r0
	 *
	 * Increases the reference count of the block of memory pointed to by
	 * r0.
	 */

	SUBTILIS_OP_INSTR_REF,

	/*
	 *
	 * deref r0
	 *
	 * Decreases the reference count of the block of memory pointed to by
	 * r0.  If the reference count drops to zero the memory is returned to
	 * the heap.
	 */

	SUBTILIS_OP_INSTR_DEREF,

	/*
	 *
	 * getref r0, r1
	 *
	 * Stores the reference count of the object pointed to by r1 in r0.
	 */

	SUBTILIS_OP_INSTR_GETREF,

	/*
	 *
	 * push r0
	 *
	 * Pushes the 32 bit value in r0 onto the stack.
	 */

	SUBTILIS_OP_INSTR_PUSH_I32,

	/*
	 *
	 * pop r0
	 *
	 * Pops a 32 bit value from the top of the stack into r0
	 */

	SUBTILIS_OP_INSTR_POP_I32,

	/*
	 *
	 * lca r0, id
	 *
	 * Loads the address of a constant identified by an int32 id,
	 * which is local to the source file, into r0.
	 */

	SUBTILIS_OP_INSTR_LCA,

	/*
	 *
	 * at r0, r1
	 *
	 * Moves the text cursor to the x and y coordinates specified by r0 and
	 * r1 which represent the x and y coordinates respectively.  If an error
	 * occurs the error flag is set and an error is written into the error
	 * offsets for the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 */

	SUBTILIS_OP_INSTR_AT,

	/*
	 *
	 * pos r0
	 *
	 * Writes the x coordinate of the text cursor to r0.  If an error
	 * occurs the error flag is set and an error is written into the error
	 * offsets for the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 */

	SUBTILIS_OP_INSTR_POS,

	/*
	 *
	 * vpos r0
	 *
	 * Writes the y coordinate of the text cursor to r0.  If an error
	 * occurs the error flag is set and an error is written into the error
	 * offsets for the current section.  Note errors are ignored unless
	 * handle_graphics_errors is set to false.
	 */

	SUBTILIS_OP_INSTR_VPOS,

	/*
	 *
	 * tcol r0
	 *
	 * Sets the current text colour to the value in r0.
	 */

	SUBTILIS_OP_INSTR_TCOL,

	/*
	 *
	 * palette r0, r1, r2, r3
	 *
	 * Sets the logical colour in r0, to the rgb values in r1, r2, and r3.
	 */

	SUBTILIS_OP_INSTR_PALETTE,

	/*
	 *
	 * i32todec r0, r1, r2
	 *
	 * Stores a string representation of r1 in the buffer pointed to by
	 * r2.  r2 must be large enough to contain a decimal representation of
	 * a 32 bit integer, including the sign, i.e., 11 bytes.  The number of
	 * bytes written are returned in r0.
	 */

	SUBTILIS_OP_INSTR_I32TODEC,

	/*
	 *
	 * realtodec r0, fp1, r2
	 *
	 * Stores a string representation of fp1 in the buffer pointed to by
	 * r2.  r2 must be at least 24 bytes.  The instruction is not allowed
	 * to write more than 24 bytes into the buffer pointed to by r2.  The
	 * number of bytes written are returned in r0.
	 */

	SUBTILIS_OP_INSTR_REALTODEC,

	/*
	 *
	 * i32tohex r0, r1, r2
	 *
	 * Stores a hexadecimal string representation of r1 in the buffer
	 * pointed to by r2.  r2 must be large enough to contain a decimal
	 * representation of a 32 bit integer, i.e., 8 bytes.  The number of
	 * bytes written are returned in r0.
	 */

	SUBTILIS_OP_INSTR_I32TOHEX,

	/*
	 *
	 * heapfree r0
	 *
	 * Returns the number of bytes of free space available in the heap in
	 * the register r0.
	 */

	SUBTILIS_OP_INSTR_HEAP_FREE,

	/*
	 *
	 * blockfree r0, r1
	 *
	 * Returns in r0 the number of bytes of free space available in the
	 * block pointed to by r1.  Not used if realloc is implemented by the
	 * backend.  Is used if it isn't.
	 */

	SUBTILIS_OP_INSTR_BLOCK_FREE,

	/*
	 *
	 * blockadjust r0, r1
	 *
	 * Reduces the number of bytes of free space available in the block
	 * pointed to by r0 by r1 bytes.  This instruction must not be used
	 * to reduce the amount of free space below 0.  The backend will not
	 * check for this, so don't do it!  Again, this instruction is only
	 * needed if the backend doesn't support realloc.
	 */

	SUBTILIS_OP_INSTR_BLOCK_ADJUST,

	/*
	 *
	 * cmovi32 r0, r1, r2, r3
	 *
	 * r0 = r2 if r1 != 0
	 * r0 = r3 if r1 == 0
	 *
	 * Assigns the value of either r2 or r3 to r0 depending on the
	 * value of r1.
	 */

	SUBTILIS_OP_INSTR_CMOV_I32,
} subtilis_op_instr_type_t;

typedef enum {
	SUBTILIS_OP_CLASS_REG_REG_REG_REG,
	SUBTILIS_OP_CLASS_REG_REG_REG,
	SUBTILIS_OP_CLASS_FREG_FREG_FREG,
	SUBTILIS_OP_CLASS_REG_REG_I32,
	SUBTILIS_OP_CLASS_FREG_REG_I32,
	SUBTILIS_OP_CLASS_FREG_FREG_REAL,
	SUBTILIS_OP_CLASS_REG_LABEL_LABEL,
	SUBTILIS_OP_CLASS_REG_FREG_FREG,
	SUBTILIS_OP_CLASS_REG_FREG_REAL,
	SUBTILIS_OP_CLASS_REG_FREG_REG,
	SUBTILIS_OP_CLASS_REG_I32,
	SUBTILIS_OP_CLASS_FREG_REAL,
	SUBTILIS_OP_CLASS_REG_REG,
	SUBTILIS_OP_CLASS_FREG_FREG,
	SUBTILIS_OP_CLASS_REG_FREG,
	SUBTILIS_OP_CLASS_FREG_REG,
	SUBTILIS_OP_CLASS_REG,
	SUBTILIS_OP_CLASS_FREG,
	SUBTILIS_OP_CLASS_I32,
	SUBTILIS_OP_CLASS_REAL,
	SUBTILIS_OP_CLASS_LABEL,
	SUBTILIS_OP_CLASS_NONE,
} subtilis_op_instr_class_t;

// TODO: Need a type for pointer offsets.  These may not always
// be 32bit integers.

union subtilis_ir_operand_t_ {
	int32_t integer;
	double real;
	size_t reg;
	size_t label;
};

typedef union subtilis_ir_operand_t_ subtilis_ir_operand_t;

#define SUBTILIS_IR_MAX_OP_ARGS 4

struct subtilis_ir_inst_t_ {
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t operands[SUBTILIS_IR_MAX_OP_ARGS];
};

typedef struct subtilis_ir_inst_t_ subtilis_ir_inst_t;

typedef enum {
	SUBTILIS_IR_REG_TYPE_INTEGER,
	SUBTILIS_IR_REG_TYPE_REAL,
} subtilis_ir_reg_type_t;

struct subtilis_ir_arg_t_ {
	subtilis_ir_reg_type_t type;
	size_t reg;
	size_t nop;
};

typedef struct subtilis_ir_arg_t_ subtilis_ir_arg_t;

struct subtilis_ir_call_t_ {
	size_t proc_id;
	size_t arg_count;
	size_t reg;
	subtilis_ir_arg_t *args;
};

typedef struct subtilis_ir_call_t_ subtilis_ir_call_t;

struct subtilis_ir_op_t_ {
	subtilis_op_type_t type;
	union {
		subtilis_ir_inst_t instr;
		size_t label;
		subtilis_ir_call_t call;
	} op;
};

typedef struct subtilis_ir_op_t_ subtilis_ir_op_t;

typedef struct subtilis_handler_list_t_ subtilis_handler_list_t;
struct subtilis_handler_list_t_ {
	size_t level;
	size_t label;
	subtilis_handler_list_t *next;
};

struct subtilis_ir_section_t_ {
	subtilis_type_section_t *type;
	size_t locals;
	size_t reg_counter;
	size_t freg_counter;
	size_t label_counter;
	size_t len;
	size_t max_len;
	subtilis_builtin_type_t ftype;
	subtilis_ir_op_t **ops;
	size_t error_len;
	size_t max_error_len;
	bool in_error_handler;
	subtilis_ir_op_t **error_ops;
	subtilis_handler_list_t *handler_list;
	size_t handler_offset;
	bool endproc;
	int32_t eflag_offset; // Again this is 32 bit specific.
	int32_t error_offset; // Again this is 32 bit specific.
	size_t end_label;
	size_t nofree_label; // Only used in the main function
	size_t ret_reg;
	size_t array_access;
	size_t cleanup_stack;
	size_t cleanup_stack_nop;
	size_t cleanup_stack_reg;
	bool destructor_needed;
};

typedef struct subtilis_ir_section_t_ subtilis_ir_section_t;

struct subtilis_ir_prog_t_ {
	subtilis_ir_section_t **sections;
	size_t num_sections;
	size_t max_sections;
	subtilis_string_pool_t *string_pool;
	const subtilis_settings_t *settings;
	subtilis_constant_pool_t *constant_pool;
};

typedef struct subtilis_ir_prog_t_ subtilis_ir_prog_t;

/* TODO: Matcher needs to move to its own file */

typedef enum {
	SUBTILIS_OP_MATCH_ANY,
	SUBTILIS_OP_MATCH_FIXED,
	SUBTILIS_OP_MATCH_FLOATING,
} subtilis_op_match_type_t;

struct subtilis_ir_inst_match_t_ {
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t operands[3];
	subtilis_op_match_type_t op_match[3];
};

typedef struct subtilis_ir_inst_match_t_ subtilis_ir_inst_match_t;

struct subtilis_ir_label_match_t_ {
	size_t label;
	subtilis_op_match_type_t op_match;
};

typedef struct subtilis_ir_label_match_t_ subtilis_ir_label_match_t;

struct subtilis_ir_op_match_t_ {
	subtilis_op_type_t type;
	union {
		subtilis_ir_inst_match_t instr;
		subtilis_ir_label_match_t label;
	} op;
};

typedef struct subtilis_ir_op_match_t_ subtilis_ir_op_match_t;

typedef void (*subtilis_ir_action_t)(subtilis_ir_section_t *s, size_t start,
				     void *user_data, subtilis_error_t *err);

#define SUBTILIS_IR_MAX_MATCHES 5

struct subtilis_ir_rule_t_ {
	subtilis_ir_op_match_t matches[SUBTILIS_IR_MAX_MATCHES];
	size_t matches_count;
	subtilis_ir_action_t action;
};

typedef struct subtilis_ir_rule_t_ subtilis_ir_rule_t;

struct subtilis_ir_rule_raw_t_ {
	const char *text;
	subtilis_ir_action_t action;
};

typedef struct subtilis_ir_rule_raw_t_ subtilis_ir_rule_raw_t;

void subtilis_handler_list_free(subtilis_handler_list_t *list);
subtilis_handler_list_t *
subtilis_handler_list_truncate(subtilis_handler_list_t *list, size_t level);
subtilis_handler_list_t *
subtilis_handler_list_update(subtilis_handler_list_t *list, size_t level,
			     size_t label, subtilis_error_t *err);

subtilis_ir_prog_t *subtilis_ir_prog_new(const subtilis_settings_t *settings,
					 subtilis_error_t *err);
/* clang-format off */
subtilis_ir_section_t *subtilis_ir_prog_section_new(
	subtilis_ir_prog_t *p, const char *name, size_t locals,
	subtilis_type_section_t *tp, subtilis_builtin_type_t ftype,
	const char *file, size_t line, int32_t eflag_offset,
	int32_t error_offset, subtilis_error_t *err);

/* clang-format on */
subtilis_ir_section_t *subtilis_ir_prog_find_section(subtilis_ir_prog_t *p,
						     const char *name);
void subtilis_ir_prog_dump(subtilis_ir_prog_t *p);
void subtilis_ir_prog_delete(subtilis_ir_prog_t *p);
/* Returns a private handle to the NOP */
size_t subtilis_ir_section_add_nop(subtilis_ir_section_t *s,
				   subtilis_error_t *err);
size_t subtilis_ir_section_promote_nop(subtilis_ir_section_t *s, size_t nop,
				       subtilis_op_instr_type_t type,
				       size_t op1, subtilis_error_t *err);
size_t subtilis_ir_section_add_instr(subtilis_ir_section_t *s,
				     subtilis_op_instr_type_t type,
				     subtilis_ir_operand_t op1,
				     subtilis_ir_operand_t op2,
				     subtilis_error_t *err);
size_t subtilis_ir_section_add_instr1(subtilis_ir_section_t *s,
				      subtilis_op_instr_type_t type,
				      subtilis_error_t *err);
size_t subtilis_ir_section_add_instr2(subtilis_ir_section_t *s,
				      subtilis_op_instr_type_t type,
				      subtilis_ir_operand_t op1,
				      subtilis_error_t *err);
void subtilis_ir_section_add_instr_no_arg(subtilis_ir_section_t *s,
					  subtilis_op_instr_type_t type,
					  subtilis_error_t *err);
void subtilis_ir_section_add_instr_no_reg(subtilis_ir_section_t *s,
					  subtilis_op_instr_type_t type,
					  subtilis_ir_operand_t op1,
					  subtilis_error_t *err);
void subtilis_ir_section_add_instr_no_reg2(subtilis_ir_section_t *s,
					   subtilis_op_instr_type_t type,
					   subtilis_ir_operand_t op0,
					   subtilis_ir_operand_t op1,
					   subtilis_error_t *err);
void subtilis_ir_section_add_instr_reg(subtilis_ir_section_t *s,
				       subtilis_op_instr_type_t type,
				       subtilis_ir_operand_t op0,
				       subtilis_ir_operand_t op1,
				       subtilis_ir_operand_t op2,
				       subtilis_error_t *err);
void subtilis_ir_section_add_instr4(subtilis_ir_section_t *s,
				    subtilis_op_instr_type_t type,
				    subtilis_ir_operand_t op0,
				    subtilis_ir_operand_t op1,
				    subtilis_ir_operand_t op2,
				    subtilis_ir_operand_t op3,
				    subtilis_error_t *err);
void subtilis_ir_section_dump(subtilis_ir_section_t *s);
size_t subtilis_ir_section_new_label(subtilis_ir_section_t *s);
void subtilis_ir_section_add_label(subtilis_ir_section_t *s, size_t l,
				   subtilis_error_t *err);
void subtilis_ir_merge_errors(subtilis_ir_section_t *s, subtilis_error_t *err);
/* Ownership of args passes to this function */
void subtilis_ir_section_add_call(subtilis_ir_section_t *s, size_t arg_count,
				  subtilis_ir_arg_t *args,
				  subtilis_error_t *err);
size_t subtilis_ir_section_add_i32_call(subtilis_ir_section_t *s,
					size_t arg_count,
					subtilis_ir_arg_t *args,
					subtilis_error_t *err);
size_t subtilis_ir_section_add_real_call(subtilis_ir_section_t *s,
					 size_t arg_count,
					 subtilis_ir_arg_t *args,
					 subtilis_error_t *err);
void subtilis_ir_parse_rules(const subtilis_ir_rule_raw_t *raw,
			     subtilis_ir_rule_t *parsed, size_t count,
			     subtilis_error_t *err);
void subtilis_ir_match(subtilis_ir_section_t *s, subtilis_ir_rule_t *rules,
		       size_t rule_count, void *user_data,
		       subtilis_error_t *err);

#endif
