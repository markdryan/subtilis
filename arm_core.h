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
	SUBTILIS_ARM_INSTR_AND = 0,
	SUBTILIS_ARM_INSTR_EOR = 1,
	SUBTILIS_ARM_INSTR_SUB = 2,
	SUBTILIS_ARM_INSTR_RSB = 3,
	SUBTILIS_ARM_INSTR_ADD = 4,
	SUBTILIS_ARM_INSTR_ADC = 5,
	SUBTILIS_ARM_INSTR_SBC = 6,
	SUBTILIS_ARM_INSTR_RSC = 7,
	SUBTILIS_ARM_INSTR_TST = 8,
	SUBTILIS_ARM_INSTR_TEQ = 9,
	SUBTILIS_ARM_INSTR_CMP = 10,
	SUBTILIS_ARM_INSTR_CMN = 11,
	SUBTILIS_ARM_INSTR_ORR = 12,
	SUBTILIS_ARM_INSTR_MOV = 13,
	SUBTILIS_ARM_INSTR_BIC = 14,
	SUBTILIS_ARM_INSTR_MVN = 15,
	SUBTILIS_ARM_INSTR_MUL,
	SUBTILIS_ARM_INSTR_MLA,
	SUBTILIS_ARM_INSTR_LDR,
	SUBTILIS_ARM_INSTR_STR,
	SUBTILIS_ARM_INSTR_LDM,
	SUBTILIS_ARM_INSTR_STM,
	SUBTILIS_ARM_INSTR_B,
	SUBTILIS_ARM_INSTR_BL,
	SUBTILIS_ARM_INSTR_SWI,
	SUBTILIS_ARM_INSTR_LDRC,
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
	uint32_t reg_mask;
};

typedef struct subtilis_arm_swi_instr_t_ subtilis_arm_swi_instr_t;

struct subtilis_arm_ldrc_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	size_t label;
};

typedef struct subtilis_arm_ldrc_instr_t_ subtilis_arm_ldrc_instr_t;

struct subtilis_arm_instr_t_ {
	subtilis_arm_instr_type_t type;
	union {
		subtilis_arm_data_instr_t data;
		subtilis_arm_stran_instr_t stran;
		subtilis_arm_mtran_instr_t mtran;
		subtilis_arm_br_instr_t br;
		subtilis_arm_swi_instr_t swi;
		subtilis_arm_ldrc_instr_t ldrc;
	} operands;
};

typedef struct subtilis_arm_instr_t_ subtilis_arm_instr_t;

struct subtilis_arm_op_t_ {
	subtilis_op_type_t type;
	union {
		subtilis_arm_instr_t instr;
		size_t label;
	} op;
	size_t next;
	size_t prev;
};

typedef struct subtilis_arm_op_t_ subtilis_arm_op_t;

struct subtilis_arm_op_pool_t_ {
	subtilis_arm_op_t *ops;
	size_t len;
	size_t max_len;
};

typedef struct subtilis_arm_op_pool_t_ subtilis_arm_op_pool_t;

struct subtilis_arm_constant_t_ {
	uint32_t integer;
	size_t label;
};

typedef struct subtilis_arm_constant_t_ subtilis_arm_constant_t;

struct subtilis_arm_program_t_ {
	size_t reg_counter;
	size_t label_counter;
	size_t len;
	size_t first_op;
	size_t last_op;
	size_t globals;
	subtilis_arm_constant_t *constants;
	size_t constant_count;
	size_t max_constants;
	subtilis_arm_op_pool_t *pool;
};

typedef struct subtilis_arm_program_t_ subtilis_arm_program_t;

subtilis_arm_op_pool_t *subtilis_arm_op_pool_new(subtilis_error_t *err);
size_t subtilis_arm_op_pool_alloc(subtilis_arm_op_pool_t *pool,
				  subtilis_error_t *err);
void subtilis_arm_op_pool_reset(subtilis_arm_op_pool_t *pool);
void subtilis_arm_op_pool_delete(subtilis_arm_op_pool_t *pool);

subtilis_arm_program_t *subtilis_arm_program_new(subtilis_arm_op_pool_t *pool,
						 size_t reg_counter,
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
/* clang-format off */
subtilis_arm_instr_t *
subtilis_arm_program_insert_instr(subtilis_arm_program_t *p,
				  subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_error_t *err);
/* clang-format on */
subtilis_arm_instr_t *subtilis_arm_program_dup_instr(subtilis_arm_program_t *p,
						     subtilis_error_t *err);
bool subtilis_arm_encode_imm(int32_t num, uint32_t *encoded);
bool subtilis_arm_encode_lvl2_imm(int32_t num, uint32_t *encoded1,
				  uint32_t *encoded2);
subtilis_arm_reg_t subtilis_arm_ir_to_arm_reg(size_t ir_reg);
size_t subtilis_add_data_imm_ldr_datai(subtilis_arm_program_t *p,
				       subtilis_arm_instr_type_t itype,
				       subtilis_arm_ccode_type_t ccode,
				       bool status, subtilis_arm_reg_t dest,
				       subtilis_arm_reg_t op1, int32_t op2,
				       subtilis_error_t *err);
void subtilis_arm_add_addsub_imm(subtilis_arm_program_t *p,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, int32_t op2,
				 subtilis_error_t *err);
void subtilis_arm_add_mul_imm(subtilis_arm_program_t *p,
			      subtilis_arm_ccode_type_t ccode, bool status,
			      subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			      int32_t op2, subtilis_error_t *err);
void subtilis_arm_add_mul(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			  subtilis_arm_reg_t op2, subtilis_error_t *err);
void subtilis_arm_add_data_imm(subtilis_arm_program_t *p,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err);
void subtilis_arm_mov_reg(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			  subtilis_error_t *err);
void subtilis_arm_mov_imm(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, int32_t op2,
			  subtilis_error_t *err);
void subtilis_arm_stran_imm(subtilis_arm_program_t *p,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			    int32_t offset, subtilis_error_t *err);
void subtilis_arm_insert_push(subtilis_arm_program_t *arm_p,
			      subtilis_arm_op_t *current,
			      subtilis_arm_ccode_type_t ccode, size_t reg_num,
			      subtilis_error_t *err);
void subtilis_arm_insert_pop(subtilis_arm_program_t *arm_p,
			     subtilis_arm_op_t *current,
			     subtilis_arm_ccode_type_t ccode, size_t reg_num,
			     subtilis_error_t *err);
/* clang-format off */
void subtilis_arm_insert_stran_spill_imm(subtilis_arm_program_t *p,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err);
/* clang-format on */
void subtilis_arm_insert_stran_imm(subtilis_arm_program_t *p,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err);
void subtilis_arm_cmp_imm(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t op1, int32_t op2,
			  subtilis_error_t *err);

#define subtilis_arm_add_add_imm(p, cc, s, dst, op1, op2, err)                 \
	subtilis_arm_add_addsub_imm(p, SUBTILIS_ARM_INSTR_ADD,                 \
				    SUBTILIS_ARM_INSTR_SUB, cc, s, dst, op1,   \
				    op2, err)
#define subtilis_arm_add_sub_imm(p, cc, s, dst, op1, op2, err)                 \
	subtilis_arm_add_addsub_imm(p, SUBTILIS_ARM_INSTR_SUB,                 \
				    SUBTILIS_ARM_INSTR_ADD, cc, s, dst, op1,   \
				    op2, err)

void subtilis_arm_program_dump(subtilis_arm_program_t *p);

#endif
