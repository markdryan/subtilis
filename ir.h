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

typedef enum {
	SUBTILIS_OP_INSTR_ADD_I32,
	SUBTILIS_OP_INSTR_ADD_REAL,
	SUBTILIS_OP_INSTR_ADD_STR,
	SUBTILIS_OP_INSTR_SUB_I32,
	SUBTILIS_OP_INSTR_SUB_REAL,
	SUBTILIS_OP_INSTR_MUL_I32,
	SUBTILIS_OP_INSTR_MUL_REAL,
	SUBTILIS_OP_INSTR_DIV_I32,
	SUBTILIS_OP_INSTR_DIV_REAL,
	SUBTILIS_OP_INSTR_ADDI_I32,
	SUBTILIS_OP_INSTR_ADDI_REAL,
	SUBTILIS_OP_INSTR_SUBI_I32,
	SUBTILIS_OP_INSTR_SUBI_REAL,
	SUBTILIS_OP_INSTR_MULI_I32,
	SUBTILIS_OP_INSTR_MULI_REAL,
	SUBTILIS_OP_INSTR_DIVI_I32,
	SUBTILIS_OP_INSTR_DIVI_REAL,
	SUBTILIS_OP_INSTR_LOADI_I32,
	SUBTILIS_OP_INSTR_LOADI_REAL,
	SUBTILIS_OP_INSTR_LOADI_STR,
	SUBTILIS_OP_INSTR_LOAD_I32,
	SUBTILIS_OP_INSTR_LOAD_REAL,
	SUBTILIS_OP_INSTR_LOAD_STR,
	SUBTILIS_OP_INSTR_STOREI_I32,
	SUBTILIS_OP_INSTR_STOREI_REAL,
	SUBTILIS_OP_INSTR_STOREI_STR,
	SUBTILIS_OP_INSTR_STORE_I32,
	SUBTILIS_OP_INSTR_STORE_REAL,
	SUBTILIS_OP_INSTR_STORE_STR,
} subtilis_op_instr_type_t;

union subtilis_ir_operand_t_ {
	int32_t integer;
	double real;
	subtilis_buffer_t str;
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
void subtilis_ir_program_add_instr_reg(subtilis_ir_program_t *p,
				       subtilis_op_instr_type_t type,
				       subtilis_ir_operand_t op0,
				       subtilis_ir_operand_t op1,
				       subtilis_ir_operand_t op2,
				       subtilis_error_t *err);
void subtilis_ir_program_dump(subtilis_ir_program_t *p);

#endif
