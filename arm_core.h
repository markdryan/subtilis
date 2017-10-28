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

#ifndef __SUBTILIS_ARM_CORE_H
#define __SUBTILIS_ARM_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ir.h"

typedef enum {
	SUBTILIS_ARM_REG_FIXED,
	SUBTILIS_ARM_REG_FLOATING,
} subtilis_arm_reg_type_t;

struct subtilis_arm_reg_t_ {
	subtilis_arm_reg_type_t type;
	size_t num;
};

typedef struct subtilis_arm_reg_t_ subtilis_arm_reg_t;

typedef enum {
	SUBTILIS_ARM_OP2_REG,
	SUBTILIS_ARM_OP2_I32,
	SUBTILIS_ARM_OP2_SHIFTED,
} subtilis_arm_op2_type_t;

typedef enum {
	SUBTILIS_ARM_SHIFT_LSL,
	SUBTILIS_ARM_SHIFT_ASL,
	SUBTILIS_ARM_SHIFT_LSR,
	SUBTILIS_ARM_SHIFT_ASR,
	SUBTILIS_ARM_SHIFT_ROR,
	SUBTILIS_ARM_SHIFT_RRX,
} subtilis_arm_shift_type_t;

struct subtilis_arm_shift_t_ {
	subtilis_arm_reg_t reg;
	subtilis_arm_shift_type_t type;
	int32_t shift;
};

typedef struct subtilis_arm_shift_t_ subtilis_arm_shift_t;

struct subtilis_arm_op2_t_ {
	subtilis_arm_op2_type_t type;
	union {
		subtilis_arm_reg_t reg;
		uint32_t integer;
		subtilis_arm_shift_t shift;
	} op;
};

typedef struct subtilis_arm_op2_t_ subtilis_arm_op2_t;

typedef enum {
	SUBTILIS_ARM_CCODE_EQ = 0,
	SUBTILIS_ARM_CCODE_NE = 1,
	SUBTILIS_ARM_CCODE_CS = 2,
	SUBTILIS_ARM_CCODE_CC = 3,
	SUBTILIS_ARM_CCODE_MI = 4,
	SUBTILIS_ARM_CCODE_PL = 5,
	SUBTILIS_ARM_CCODE_VS = 6,
	SUBTILIS_ARM_CCODE_VC = 7,
	SUBTILIS_ARM_CCODE_HI = 8,
	SUBTILIS_ARM_CCODE_LS = 9,
	SUBTILIS_ARM_CCODE_GE = 10,
	SUBTILIS_ARM_CCODE_LT = 11,
	SUBTILIS_ARM_CCODE_GT = 12,
	SUBTILIS_ARM_CCODE_LE = 13,
	SUBTILIS_ARM_CCODE_AL = 14,
	SUBTILIS_ARM_CCODE_NV = 15
} subtilis_arm_ccode_type_t;

typedef enum {
	SUBTILIS_ARM_CCODE_AND = 0,
	SUBTILIS_ARM_CCODE_EOR = 1,
	SUBTILIS_ARM_CCODE_SUB = 2,
	SUBTILIS_ARM_CCODE_RSB = 3,
	SUBTILIS_ARM_INSTR_ADD = 4,
	SUBTILIS_ARM_CCODE_ADC = 5,
	SUBTILIS_ARM_CCODE_SBC = 6,
	SUBTILIS_ARM_CCODE_RSC = 7,
	SUBTILIS_ARM_CCODE_TST = 8,
	SUBTILIS_ARM_CCODE_TEQ = 9,
	SUBTILIS_ARM_CCODE_CMP = 10,
	SUBTILIS_ARM_CCODE_CMN = 11,
	SUBTILIS_ARM_CCODE_ORR = 12,
	SUBTILIS_ARM_CCODE_MOV = 13,
	SUBTILIS_ARM_CCODE_BIC = 14,
	SUBTILIS_ARM_CCODE_MVN = 15,
	SUBTILIS_ARM_CCODE_MUL,
	SUBTILIS_ARM_CCODE_MLA,
	SUBTILIS_ARM_CCODE_LDR,
	SUBTILIS_ARM_CCODE_STR,
	SUBTILIS_ARM_CCODE_LDM,
	SUBTILIS_ARM_CCODE_STM,
	SUBTILIS_ARM_CCODE_B,
	SUBTILIS_ARM_CCODE_BL,
	SUBTILIS_ARM_CCODE_SWI,
} subtilis_arm_instr_type_t;

struct subtilis_arm_data_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	bool status;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_op2_t op2;
};

typedef struct subtilis_arm_data_instr_t_ subtilis_arm_data_instr_t;

struct subtilis_arm_stran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	subtilis_arm_op2_t offset;
	bool pre_indexed;
	bool write_back;
};

typedef struct subtilis_arm_stran_instr_t_ subtilis_arm_stran_instr_t;

struct subtilis_arm_mtran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t op0;
	size_t reg_list; // bitmap
	bool pre_indexed;
	bool ascending;
	bool write_back;
};

typedef struct subtilis_arm_mtran_instr_t_ subtilis_arm_mtran_instr_t;

struct subtilis_arm_br_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	bool link;
	size_t label;
};

typedef struct subtilis_arm_br_instr_t_ subtilis_arm_br_instr_t;

struct subtilis_arm_swi_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	size_t code;
};

typedef struct subtilis_arm_swi_instr_t_ subtilis_arm_swi_instr_t;

struct subtilis_arm_instr_t_ {
	subtilis_arm_instr_type_t type;
	union {
		subtilis_arm_data_instr_t data;
		subtilis_arm_stran_instr_t stran;
		subtilis_arm_mtran_instr_t mtran;
		subtilis_arm_br_instr_t br;
		subtilis_arm_swi_instr_t swi;
	} operands;
};

typedef struct subtilis_arm_instr_t_ subtilis_arm_instr_t;

struct subtilis_arm_op_t_ {
	subtilis_op_type_t type;
	union {
		subtilis_arm_instr_t instr;
		size_t label;
	} op;
};

typedef struct subtilis_arm_op_t_ subtilis_arm_op_t;

struct subtilis_arm_program_t_ {
	size_t reg_counter;
	size_t label_counter;
	size_t len;
	size_t max_len;
	subtilis_arm_op_t *ops;
	size_t globals;
};

typedef struct subtilis_arm_program_t_ subtilis_arm_program_t;

subtilis_arm_program_t *subtilis_arm_program_new(size_t reg_counter,
						 size_t label_counter,
						 size_t globals,
						 subtilis_error_t *err);
void subtilis_arm_program_delete(subtilis_arm_program_t *p);

void subtilis_arm_program_add_label(subtilis_arm_program_t *p, size_t label,
				    subtilis_error_t *err);
subtilis_arm_instr_t *
subtilis_arm_program_add_instr(subtilis_arm_program_t *p,
			       subtilis_arm_instr_type_t type,
			       subtilis_error_t *err);

bool subtilis_arm_encode_imm(int32_t num, uint32_t *encoded);

#endif
