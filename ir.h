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
#include "error.h"

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
	SUBTILIS_OP_PHI,
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
	 * result in a third register.
	 *
	 * r0 = r1 / r2
	 */

	SUBTILIS_OP_INSTR_DIV_I32,

	/*
	 * divr fp0, fp1, fp2
	 *
	 * Divides two 64 bit doubles stored in floating point registers.
	 * The result is stored in a third floating point register.
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

	SUBTILIS_OP_INSTR_MOV_REAL,

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
	 * printi32 r0
	 *
	 * Prints the 32 bit integer stored in r0 to the output stream.
	 */

	SUBTILIS_OP_INSTR_PRINT_I32,

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

} subtilis_op_instr_type_t;

// TODO: Need a type for pointer offsets.  These may not always
// be 32bit integers.

union subtilis_ir_operand_t_ {
	int32_t integer;
	double real;
	size_t reg;
};

typedef union subtilis_ir_operand_t_ subtilis_ir_operand_t;

struct subtilis_ir_inst_t_ {
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t operands[3];
};

typedef struct subtilis_ir_inst_t_ subtilis_ir_inst_t;

struct subtilis_ir_op_t_ {
	subtilis_op_type_t type;
	union {
		subtilis_ir_inst_t instr;
	} op;
};

typedef struct subtilis_ir_op_t_ subtilis_ir_op_t;

struct subtilis_ir_program_t_ {
	size_t reg_counter;
	size_t len;
	size_t max_len;
	subtilis_ir_op_t **ops;
};

typedef struct subtilis_ir_program_t_ subtilis_ir_program_t;

subtilis_ir_program_t *subtilis_ir_program_new(subtilis_error_t *err);
void subtilis_ir_program_delete(subtilis_ir_program_t *p);
size_t subtilis_ir_program_add_instr(subtilis_ir_program_t *p,
				     subtilis_op_instr_type_t type,
				     subtilis_ir_operand_t op1,
				     subtilis_ir_operand_t op2,
				     subtilis_error_t *err);
size_t subtilis_ir_program_add_instr2(subtilis_ir_program_t *p,
				      subtilis_op_instr_type_t type,
				      subtilis_ir_operand_t op1,
				      subtilis_error_t *err);
void subtilis_ir_program_add_instr_no_reg(subtilis_ir_program_t *p,
					  subtilis_op_instr_type_t type,
					  subtilis_ir_operand_t op1,
					  subtilis_error_t *err);
void subtilis_ir_program_add_instr_reg(subtilis_ir_program_t *p,
				       subtilis_op_instr_type_t type,
				       subtilis_ir_operand_t op0,
				       subtilis_ir_operand_t op1,
				       subtilis_ir_operand_t op2,
				       subtilis_error_t *err);
void subtilis_ir_program_dump(subtilis_ir_program_t *p);

#endif
