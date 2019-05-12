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

/*
 * Two register namespaces overlap for each register type, int or real.
 * For integers register numbers < 16 are fixed and will not be altered
 * by the register allocator.  FPA register numbers < 8 are fixed.
 * All other register numbers are floating and will need to be allocated
 * to a fixed register during register allocation.
 */

#define SUBTILIS_ARM_INT_VIRT_REG_START 16
#define SUBTILIS_ARM_FPA_VIRT_REG_START 8

typedef size_t subtilis_arm_reg_t;

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
	union {
		int32_t integer;
		subtilis_arm_reg_t reg;
	} shift;
	bool shift_reg;
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
	SUBTILIS_ARM_INSTR_SWI,
	SUBTILIS_ARM_INSTR_LDRC,

	SUBTILIS_FPA_INSTR_LDF,
	SUBTILIS_FPA_INSTR_STF,
	SUBTILIS_FPA_INSTR_LDRC,
	SUBTILIS_FPA_INSTR_MVF,
	SUBTILIS_FPA_INSTR_MNF,
	SUBTILIS_FPA_INSTR_ADF,
	SUBTILIS_FPA_INSTR_MUF,
	SUBTILIS_FPA_INSTR_SUF,
	SUBTILIS_FPA_INSTR_RSF,
	SUBTILIS_FPA_INSTR_DVF,
	SUBTILIS_FPA_INSTR_RDF,
	SUBTILIS_FPA_INSTR_POW,
	SUBTILIS_FPA_INSTR_RPW,
	SUBTILIS_FPA_INSTR_RMF,
	SUBTILIS_FPA_INSTR_FML,
	SUBTILIS_FPA_INSTR_FDV,
	SUBTILIS_FPA_INSTR_FRD,
	SUBTILIS_FPA_INSTR_POL,
	SUBTILIS_FPA_INSTR_ABS,
	SUBTILIS_FPA_INSTR_RND,
	SUBTILIS_FPA_INSTR_SQT,
	SUBTILIS_FPA_INSTR_LOG,
	SUBTILIS_FPA_INSTR_LGN,
	SUBTILIS_FPA_INSTR_EXP,
	SUBTILIS_FPA_INSTR_SIN,
	SUBTILIS_FPA_INSTR_COS,
	SUBTILIS_FPA_INSTR_TAN,
	SUBTILIS_FPA_INSTR_ASN,
	SUBTILIS_FPA_INSTR_ACS,
	SUBTILIS_FPA_INSTR_ATN,
	SUBTILIS_FPA_INSTR_URD,
	SUBTILIS_FPA_INSTR_NRM,
	SUBTILIS_FPA_INSTR_FLT,
	SUBTILIS_FPA_INSTR_FIX,
	SUBTILIS_FPA_INSTR_CMF,
	SUBTILIS_FPA_INSTR_CNF,
	SUBTILIS_FPA_INSTR_CMFE,
	SUBTILIS_FPA_INSTR_CNFE,
	SUBTILIS_FPA_INSTR_WFS,
	SUBTILIS_FPA_INSTR_RFS,
	SUBTILIS_ARM_INSTR_MAX,
} subtilis_arm_instr_type_t;

/*
 * TODO: Maybe we should get rid of the stack versions of these
 * instructions as we'll lose the stack information when we
 * dissasemlble.
 */

typedef enum {
	SUBTILIS_ARM_MTRAN_IA,
	SUBTILIS_ARM_MTRAN_IB,
	SUBTILIS_ARM_MTRAN_DA,
	SUBTILIS_ARM_MTRAN_DB,
	SUBTILIS_ARM_MTRAN_FA,
	SUBTILIS_ARM_MTRAN_FD,
	SUBTILIS_ARM_MTRAN_EA,
	SUBTILIS_ARM_MTRAN_ED
} subtilis_arm_mtran_type_t;

struct subtilis_arm_data_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	bool status;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_op2_t op2;
};

typedef struct subtilis_arm_data_instr_t_ subtilis_arm_data_instr_t;

struct subtilis_arm_mul_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	bool status;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t rm;
	subtilis_arm_reg_t rs;
	subtilis_arm_reg_t rn;
};

typedef struct subtilis_arm_mul_instr_t_ subtilis_arm_mul_instr_t;

struct subtilis_arm_stran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	subtilis_arm_op2_t offset;
	bool pre_indexed;
	bool write_back;
	bool subtract;
};

typedef struct subtilis_arm_stran_instr_t_ subtilis_arm_stran_instr_t;

struct subtilis_arm_mtran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t op0;
	size_t reg_list; // bitmap
	subtilis_arm_mtran_type_t type;
	bool write_back;
};

typedef struct subtilis_arm_mtran_instr_t_ subtilis_arm_mtran_instr_t;

typedef enum {
	SUBTILIS_ARM_BR_LINK_VOID,
	SUBTILIS_ARM_BR_LINK_INT,
	SUBTILIS_ARM_BR_LINK_REAL,
} subtilis_arm_br_link_type_t;

struct subtilis_arm_br_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	bool link;
	subtilis_arm_br_link_type_t link_type;
	union {
		size_t label;
		int32_t offset;
	} target;
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

typedef enum {
	SUBTILIS_FPA_ROUNDING_NEAREST,
	SUBTILIS_FPA_ROUNDING_PLUS_INFINITY,
	SUBTILIS_FPA_ROUNDING_MINUS_INFINITY,
	SUBTILIS_FPA_ROUNDING_ZERO,
} subtilis_fpa_rounding_t;

union subtilis_fpa_op2_t_ {
	uint8_t imm;
	subtilis_arm_reg_t reg;
};

typedef union subtilis_fpa_op2_t_ subtilis_fpa_op2_t;

struct subtilis_fpa_data_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_fpa_rounding_t rounding;
	size_t size;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	bool immediate;
	subtilis_fpa_op2_t op2;
};

typedef struct subtilis_fpa_data_instr_t_ subtilis_fpa_data_instr_t;

struct subtilis_fpa_stran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	size_t size; /* Currently always set to 8 */
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	uint8_t offset;
	bool pre_indexed;
	bool write_back;
	bool subtract;
};

typedef struct subtilis_fpa_stran_instr_t_ subtilis_fpa_stran_instr_t;

struct subtilis_fpa_tran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	bool immediate; /* Currently always set to false */
	subtilis_fpa_op2_t op2;
	size_t size;
	subtilis_fpa_rounding_t rounding;
};

typedef struct subtilis_fpa_tran_instr_t_ subtilis_fpa_tran_instr_t;

struct subtilis_fpa_cptran_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
};

typedef struct subtilis_fpa_cptran_instr_t_ subtilis_fpa_cptran_instr_t;

struct subtilis_fpa_cmp_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	bool immediate;
	subtilis_fpa_op2_t op2;
};

typedef struct subtilis_fpa_cmp_instr_t_ subtilis_fpa_cmp_instr_t;

struct subtilis_fpa_ldrc_instr_t_ {
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_reg_t dest;
	size_t size;
	size_t label;
};

typedef struct subtilis_fpa_ldrc_instr_t_ subtilis_fpa_ldrc_instr_t;

struct subtilis_arm_instr_t_ {
	subtilis_arm_instr_type_t type;
	union {
		subtilis_arm_data_instr_t data;
		subtilis_arm_mul_instr_t mul;
		subtilis_arm_stran_instr_t stran;
		subtilis_arm_mtran_instr_t mtran;
		subtilis_arm_br_instr_t br;
		subtilis_arm_swi_instr_t swi;
		subtilis_arm_ldrc_instr_t ldrc;
		subtilis_fpa_data_instr_t fpa_data;
		subtilis_fpa_stran_instr_t fpa_stran;
		subtilis_fpa_tran_instr_t fpa_tran;
		subtilis_fpa_cmp_instr_t fpa_cmp;
		subtilis_fpa_ldrc_instr_t fpa_ldrc;
		subtilis_fpa_cptran_instr_t fpa_cptran;
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

struct subtilis_arm_ui32_constant_t_ {
	uint32_t integer;
	size_t label;
};

typedef struct subtilis_arm_ui32_constant_t_ subtilis_arm_ui32_constant_t;

struct subtilis_arm_real_constant_t_ {
	double real;
	size_t label;
};

typedef struct subtilis_arm_real_constant_t_ subtilis_arm_real_constant_t;

struct subtilis_arm_call_site_t_ {
	size_t ldm_site;
	size_t stm_site;
	size_t ldf_site;
	size_t stf_site;
	size_t int_args;
	size_t real_args;
	size_t call_site;
	size_t int_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];
	size_t real_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];
};

typedef struct subtilis_arm_call_site_t_ subtilis_arm_call_site_t;

typedef struct subtilis_arm_constants_t_ subtilis_arm_constants_t;

struct subtilis_arm_constants_t_ {
	subtilis_arm_ui32_constant_t *ui32;
	size_t ui32_count;
	size_t max_ui32;
	subtilis_arm_real_constant_t *real;
	size_t real_count;
	size_t max_real;
};

struct subtilis_arm_section_t_ {
	size_t reg_counter;
	size_t freg_counter;
	size_t label_counter;
	size_t len;
	size_t first_op;
	size_t last_op;
	size_t locals;
	subtilis_arm_op_pool_t *op_pool;
	size_t call_site_count;
	size_t max_call_site_count;
	subtilis_arm_call_site_t *call_sites;
	subtilis_arm_constants_t constants;
	size_t ret_site_count;
	size_t max_ret_site_count;
	size_t *ret_sites;
	subtilis_type_section_t *stype;
};

typedef struct subtilis_arm_section_t_ subtilis_arm_section_t;

struct subtilis_arm_prog_t_ {
	subtilis_arm_section_t **sections;
	size_t num_sections;
	size_t max_sections;
	subtilis_string_pool_t *string_pool;
	subtilis_arm_op_pool_t *op_pool;
	bool reverse_fpa_consts;
};

typedef struct subtilis_arm_prog_t_ subtilis_arm_prog_t;

subtilis_arm_op_pool_t *subtilis_arm_op_pool_new(subtilis_error_t *err);
size_t subtilis_arm_op_pool_alloc(subtilis_arm_op_pool_t *pool,
				  subtilis_error_t *err);
void subtilis_arm_op_pool_reset(subtilis_arm_op_pool_t *pool);
void subtilis_arm_op_pool_delete(subtilis_arm_op_pool_t *pool);

subtilis_arm_reg_t subtilis_arm_acquire_new_reg(subtilis_arm_section_t *s);
subtilis_arm_reg_t subtilis_arm_acquire_new_freg(subtilis_arm_section_t *s);

subtilis_arm_section_t *
subtilis_arm_section_new(subtilis_arm_op_pool_t *pool,
			 subtilis_type_section_t *stype, size_t reg_counter,
			 size_t freg_counter, size_t label_counter,
			 size_t locals, subtilis_error_t *err);
void subtilis_arm_section_delete(subtilis_arm_section_t *s);

subtilis_arm_prog_t *subtilis_arm_prog_new(size_t max_sections,
					   subtilis_arm_op_pool_t *op_pool,
					   subtilis_string_pool_t *string_pool,
					   subtilis_error_t *err);
/* clang-format off */
subtilis_arm_section_t *
subtilis_arm_prog_section_new(subtilis_arm_prog_t *prog,
			      subtilis_type_section_t *stype,
			      size_t reg_counter, size_t freg_counter,
			      size_t label_counter,
			      size_t locals, subtilis_error_t *err);
/* clang-format on */

void subtilis_arm_section_max_regs(subtilis_arm_section_t *s, size_t *int_regs,
				   size_t *real_regs);

void subtilis_arm_prog_delete(subtilis_arm_prog_t *prog);
void subtilis_arm_section_add_call_site(subtilis_arm_section_t *s,
					size_t stm_site, size_t ldm_site,
					size_t stf_site, size_t ldf_site,
					size_t int_args, size_t real_args,
					size_t op, size_t *int_arg_ops,
					size_t *real_arg_ops,
					subtilis_error_t *err);
void subtilis_arm_section_add_ret_site(subtilis_arm_section_t *s, size_t op,
				       subtilis_error_t *err);
void subtilis_arm_section_add_label(subtilis_arm_section_t *s, size_t label,
				    subtilis_error_t *err);
void subtilis_arm_section_insert_label(subtilis_arm_section_t *s, size_t label,
				       subtilis_arm_op_t *pos,
				       subtilis_error_t *err);
subtilis_arm_instr_t *
subtilis_arm_section_add_instr(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t type,
			       subtilis_error_t *err);
size_t subtilis_arm_insert_data_imm_ldr(subtilis_arm_section_t *s,
					subtilis_arm_op_t *current,
					subtilis_arm_ccode_type_t ccode,
					subtilis_arm_reg_t dest, int32_t op2,
					subtilis_error_t *err);
/* clang-format off */
subtilis_arm_instr_t *
subtilis_arm_section_insert_instr(subtilis_arm_section_t *s,
				  subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_error_t *err);
/* clang-format on */
subtilis_arm_instr_t *subtilis_arm_section_dup_instr(subtilis_arm_section_t *s,
						     subtilis_error_t *err);
bool subtilis_arm_encode_imm(int32_t num, uint32_t *encoded);
uint32_t subtilis_arm_encode_nearest(int32_t num, subtilis_error_t *err);
bool subtilis_arm_encode_lvl2_imm(int32_t num, uint32_t *encoded1,
				  uint32_t *encoded2);
subtilis_arm_reg_t subtilis_arm_ir_to_arm_reg(size_t ir_reg);
subtilis_arm_reg_t subtilis_arm_ir_to_freg(size_t ir_reg);
size_t subtilis_add_data_imm_ldr_datai(subtilis_arm_section_t *s,
				       subtilis_arm_instr_type_t itype,
				       subtilis_arm_ccode_type_t ccode,
				       bool status, subtilis_arm_reg_t dest,
				       subtilis_arm_reg_t op1, int32_t op2,
				       subtilis_error_t *err);
void subtilis_arm_add_addsub_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, int32_t op2,
				 subtilis_error_t *err);
void subtilis_arm_add_rsub_imm(subtilis_arm_section_t *s,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err);
void subtilis_arm_add_mul_imm(subtilis_arm_section_t *s,
			      subtilis_arm_ccode_type_t ccode, bool status,
			      subtilis_arm_reg_t dest, subtilis_arm_reg_t rm,
			      int32_t rs, subtilis_error_t *err);
void subtilis_arm_add_mul(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t rm,
			  subtilis_arm_reg_t rs, subtilis_error_t *err);
void subtilis_arm_add_data_imm(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err);
void subtilis_arm_add_swi(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode, size_t code,
			  uint32_t reg_mask, subtilis_error_t *err);
void subtilis_arm_add_movmvn_reg(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, subtilis_error_t *err);
void subtilis_arm_add_movmvn_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest, int32_t op2,
				 subtilis_error_t *err);
void subtilis_arm_add_stran_imm(subtilis_arm_section_t *s,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				subtilis_arm_reg_t base, int32_t offset,
				subtilis_error_t *err);
void subtilis_arm_insert_push(subtilis_arm_section_t *arm_s,
			      subtilis_arm_op_t *current,
			      subtilis_arm_ccode_type_t ccode, size_t reg_num,
			      subtilis_error_t *err);
void subtilis_arm_insert_pop(subtilis_arm_section_t *arm_s,
			     subtilis_arm_op_t *current,
			     subtilis_arm_ccode_type_t ccode, size_t reg_num,
			     subtilis_error_t *err);
/* clang-format off */
void subtilis_arm_insert_stran_spill_imm(subtilis_arm_section_t *s,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err);
/* clang-format on */
void subtilis_arm_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err);
void subtilis_arm_add_cmp_imm(subtilis_arm_section_t *s,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_arm_reg_t op1, int32_t op2,
			      subtilis_error_t *err);
void subtilis_arm_add_cmp(subtilis_arm_section_t *s,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t op1, subtilis_arm_reg_t op2,
			  subtilis_error_t *err);

void subtilis_arm_add_mtran(subtilis_arm_section_t *s,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t op0, size_t reg_list,
			    subtilis_arm_mtran_type_t type, bool write_back,
			    subtilis_error_t *err);

bool subtilis_arm_is_fixed(subtilis_arm_reg_t reg);

#define subtilis_arm_add_add_imm(s, cc, st, dst, op1, op2, err)                \
	subtilis_arm_add_addsub_imm(s, SUBTILIS_ARM_INSTR_ADD,                 \
				    SUBTILIS_ARM_INSTR_SUB, cc, st, dst, op1,  \
				    op2, err)
#define subtilis_arm_add_sub_imm(s, cc, st, dst, op1, op2, err)                \
	subtilis_arm_add_addsub_imm(s, SUBTILIS_ARM_INSTR_SUB,                 \
				    SUBTILIS_ARM_INSTR_ADD, cc, st, dst, op1,  \
				    op2, err)
#define subtilis_arm_add_mov_imm(s, cc, st, dst, op2, err)                     \
	subtilis_arm_add_movmvn_imm(s, SUBTILIS_ARM_INSTR_MOV,                 \
				    SUBTILIS_ARM_INSTR_MVN, cc, st, dst, op2,  \
				    err)
#define subtilis_arm_add_mvn_imm(s, cc, st, dst, op2, err)                     \
	subtilis_arm_add_movmvn_imm(s, SUBTILIS_ARM_INSTR_MVN,                 \
				    SUBTILIS_ARM_INSTR_MOV, cc, st, dst, op2,  \
				    err)
#define subtilis_arm_add_mov_reg(s, cc, st, dst, op2, err)                     \
	subtilis_arm_add_movmvn_reg(s, SUBTILIS_ARM_INSTR_MOV, cc, st, dst,    \
				    op2, err)
#define subtilis_arm_add_mvn_reg(s, cc, st, dst, op2, err)                     \
	subtilis_arm_add_movmvn_reg(s, SUBTILIS_ARM_INSTR_MVN, cc, st, dst,    \
				    op2, err)

void subtilis_arm_section_dump(subtilis_arm_prog_t *p,
			       subtilis_arm_section_t *s);
void subtilis_arm_prog_dump(subtilis_arm_prog_t *p);
void subtilis_arm_instr_dump(subtilis_arm_instr_t *instr);

void subtilis_arm_restore_stack(subtilis_arm_section_t *arm_s,
				size_t stack_space, subtilis_error_t *err);

/* FPA functions implemented in fpa.c */

bool subtilis_fpa_encode_real(double real, uint8_t *encoded);
void subtilis_fpa_add_mvfmnf_imm(subtilis_arm_section_t *s,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_fpa_rounding_t rounding,
				 subtilis_arm_reg_t dest, double op2,
				 subtilis_error_t *err);
void subtilis_fpa_add_data_imm(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_fpa_rounding_t rounding,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       double op2, subtilis_error_t *err);
void subtilis_fpa_add_mvfmnf(subtilis_arm_section_t *s,
			     subtilis_arm_ccode_type_t ccode,
			     subtilis_arm_instr_type_t type,
			     subtilis_fpa_rounding_t rounding,
			     subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			     subtilis_error_t *err);
void subtilis_fpa_add_stran(subtilis_arm_section_t *s,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			    int32_t offset, subtilis_error_t *err);
void subtilis_fpa_add_tran(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_fpa_rounding_t rounding,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			   subtilis_error_t *err);
void subtilis_fpa_add_cmfcnf_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_instr_type_t alttype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest, double op2,
				 subtilis_error_t *err);
void subtilis_fpa_add_cmp(subtilis_arm_section_t *s,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			  subtilis_error_t *err);
void subtilis_fpa_add_cptran(subtilis_arm_section_t *s,
			     subtilis_arm_instr_type_t itype,
			     subtilis_arm_ccode_type_t ccode,
			     subtilis_arm_reg_t dest, subtilis_error_t *err);

/* clang-format off */
void subtilis_fpa_insert_stran_spill_imm(subtilis_arm_section_t *s,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err);
/* clang-format on */
void subtilis_fpa_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err);

void subtilis_fpa_push_reg(subtilis_arm_section_t *s,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_reg_t dest, subtilis_error_t *err);
void subtilis_fpa_pop_reg(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t dest, subtilis_error_t *err);
double subtilis_fpa_extract_imm(subtilis_fpa_op2_t op2, subtilis_error_t *err);

bool subtilis_fpa_is_fixed(subtilis_arm_reg_t reg);

#define subtilis_fpa_add_mov_imm(s, cc, round, dst, op2, err)                  \
	subtilis_fpa_add_mvfmnf_imm(s, cc, SUBTILIS_FPA_INSTR_MVF,             \
				    SUBTILIS_FPA_INSTR_MNF, round, dst, op2,   \
				    err)

#define subtilis_fpa_add_mnf_imm(s, cc, round, dst, op2, err)                  \
	subtilis_fpa_add_mvfmnf_imm(s, cc, SUBTILIS_FPA_INSTR_MVF,             \
				    SUBTILIS_FPA_INSTR_MNF, round, dst, op2,   \
				    err)
#define subtilis_fpa_add_mov(s, cc, round, dst, op2, err)                      \
	subtilis_fpa_add_mvfmnf(s, cc, SUBTILIS_FPA_INSTR_MVF, round, dst,     \
				op2, err)

#define subtilis_fpa_add_mnf(s, cc, round, dst, op2, err)                      \
	subtilis_fpa_add_mvfmnf(s, cc, SUBTILIS_FPA_INSTR_MNF, round, dst,     \
				op2, err)

#define subtilis_fpa_add_cmf_imm(s, cc, dst, op2, err)                         \
	subtilis_fpa_add_cmfcnf_imm(s, SUBTILIS_FPA_INSTR_CMF,                 \
				    SUBTILIS_FPA_INSTR_CNF, cc, dst, op2, err)

#define subtilis_fpa_add_cnf_imm(s, cc, dst, op2, err)                         \
	subtilis_fpa_add_cmfcnf_imm(s, SUBTILIS_FPA_INSTR_CNF,                 \
				    SUBTILIS_FPA_INSTR_CMF, cc, dst, op2, err)

#define subtilis_fpa_add_cmfe_imm(s, cc, round, dst, op2, err)                 \
	subtilis_fpa_add_cmfcnf_imm(s, cc, SUBTILIS_FPA_INSTR_CMFE,            \
				    SUBTILIS_FPA_INSTR_CNFE, round, dst, op2,  \
				    err)

#define subtilis_fpa_add_cnfe_imm(s, cc, round, dst, op2, err)                 \
	subtilis_fpa_add_cmfcnf_imm(s, cc, SUBTILIS_FPA_INSTR_CNFE,            \
				    SUBTILIS_FPA_INSTR_CMFE, round, dst, op2,  \
				    err)

#endif
