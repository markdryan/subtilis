/*
 * Copyright (c) 2023 Mark Ryan
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

#ifndef SUBTILIS_RV32_CORE_H
#define SUBTILIS_RV32_CORE_H

#include <stdint.h>

#include "../../common/ir.h"
#include "../../common/sizet_vector.h"

#define SUBTILIS_RV_REG_STACK 2
#define SUBTILIS_RV_REG_GLOBAL 3
#define SUBTILIS_RV_REG_HEAP 4
#define SUBTILIS_RV_REG_LOCAL 8
#define SUBTILIS_RV_REG_A0 10
#define SUBTILIS_RV_REG_A1 11
#define SUBTILIS_RV_REG_A2 12
#define SUBTILIS_RV_REG_A3 13
#define SUBTILIS_RV_REG_A4 14
#define SUBTILIS_RV_REG_A5 15
#define SUBTILIS_RV_REG_A6 16
#define SUBTILIS_RV_REG_A7 17

/*
 * Two register namespaces overlap for each register type, int or real.
 * For integers register numbers < 32 are fixed and will not be altered
 * by the register allocator.
 */

#define SUBTILIS_RV_INT_VIRT_REG_START 32
#define SUBTILIS_RV_REAL_VIRT_REG_START 32


#define RV_MAX_REG_ARGS 8

typedef size_t subtilis_rv_reg_t;

struct rv_rtype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
};
typedef struct rv_rtype_t_ rv_rtype_t;

struct rv_itype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	uint32_t imm;
};
typedef struct rv_itype_t_ rv_itype_t;

struct rv_labeltype_t_ {
	subtilis_rv_reg_t rd;
	size_t label;
};
typedef struct rv_labeltype_t_ rv_labeltype_t;

struct rv_sbtype_t_ {
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	uint32_t imm;
};
typedef struct rv_sbtype_t_ rv_sbtype_t;

struct rv_ujtype_t_ {
	subtilis_rv_reg_t rd;
	uint32_t imm; // offset in bytes not words
};
typedef struct rv_ujtype_t_ rv_ujtype_t;

typedef enum {
	SUBTILIS_RV_ADDI,
	SUBTILIS_RV_SLTI,
	SUBTILIS_RV_SLTIU,
	SUBTILIS_RV_ANDI,
	SUBTILIS_RV_ORI,
	SUBTILIS_RV_XORI,
	SUBTILIS_RV_SLLI,
	SUBTILIS_RV_SRLI,
	SUBTILIS_RV_SRAI,
	SUBTILIS_RV_LUI,
	SUBTILIS_RV_AUIPC,
	SUBTILIS_RV_ADD,
	SUBTILIS_RV_SLT,
	SUBTILIS_RV_SLTU,
	SUBTILIS_RV_AND,
	SUBTILIS_RV_OR,
	SUBTILIS_RV_XOR,
	SUBTILIS_RV_SLL,
	SUBTILIS_RV_SRL,
	SUBTILIS_RV_SUB,
	SUBTILIS_RV_SRA,
	SUBTILIS_RV_NOP,
	SUBTILIS_RV_JAL,
	SUBTILIS_RV_JALR,
	SUBTILIS_RV_BEQ,
	SUBTILIS_RV_BNE,
	SUBTILIS_RV_BLT,
	SUBTILIS_RV_BLTU,
	SUBTILIS_RV_BGE,
	SUBTILIS_RV_BGEU,
	SUBTILIS_RV_LW,
	SUBTILIS_RV_LH,
	SUBTILIS_RV_LHU,
	SUBTILIS_RV_LB,
	SUBTILIS_RV_LBU,
	SUBTILIS_RV_SW,
	SUBTILIS_RV_SH,
	SUBTILIS_RV_SB,
	SUBTILIS_RV_FENCE,
	SUBTILIS_RV_ECALL,
	SUBTILIS_RV_EBREAK,
	SUBTILIS_RV_HINT,
} subtilis_rv_instr_type_t;

typedef enum {
	SUBTILIS_RV_R_TYPE,
	SUBTILIS_RV_I_TYPE,
	SUBTILIS_RV_LABEL_TYPE,
	SUBTILIS_RV_S_TYPE,
	SUBTILIS_RV_B_TYPE,
	SUBTILIS_RV_U_TYPE,
	SUBTILIS_RV_J_TYPE,
	SUBTILIS_RV_FENCE_TYPE,
} subtilis_rv_instr_encoding_t;

struct subtilis_rv_instr_t_ {
	subtilis_rv_instr_type_t itype;
	subtilis_rv_instr_encoding_t etype;
	union {
		rv_rtype_t r;
		rv_itype_t i;
		rv_sbtype_t sb;
		rv_ujtype_t uj;
		rv_labeltype_t label;
	} operands;
};

typedef struct subtilis_rv_instr_t_ subtilis_rv_instr_t;

typedef enum {
	SUBTILIS_RV_OP_INSTR,
	SUBTILIS_RV_OP_LABEL,
	SUBTILIS_RV_OP_BYTE,
	SUBTILIS_RV_OP_TWO_BYTE,
	SUBTILIS_RV_OP_FOUR_BYTE,
	SUBTILIS_RV_OP_DOUBLE,
	SUBTILIS_RV_OP_FLOAT,
	SUBTILIS_RV_OP_STRING,
	SUBTILIS_RV_OP_ALIGN,
	SUBTILIS_RV_OP_PHI,
	SUBTILIS_RV_OP_MAX,
} subtilis_rv_op_type_t;

struct subtilis_rv_op_t_ {
	subtilis_rv_op_type_t type;
	union {
		uint8_t byte;
		uint16_t two_bytes;
		uint32_t four_bytes;
		uint32_t alignment;
		double dbl;
		float flt;
		subtilis_rv_instr_t instr;
		size_t label;
		char *str;
	} op;
	size_t next;
	size_t prev;
};
typedef struct subtilis_rv_op_t_ subtilis_rv_op_t;


struct subtilis_rv_op_pool_t_ {
	subtilis_rv_op_t *ops;
	size_t len;
	size_t max_len;
};

typedef struct subtilis_rv_op_pool_t_ subtilis_rv_op_pool_t;

struct subtilis_rv_call_site_t_ {
	size_t ldi_site;
	size_t sti_site;
	size_t ldf_site;
	size_t stf_site;
	size_t int_args;
	size_t real_args;
	size_t call_site;
	size_t int_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - RV_MAX_REG_ARGS];
	size_t real_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - RV_MAX_REG_ARGS];
};

typedef struct subtilis_rv_call_site_t_ subtilis_rv_call_site_t;

struct subtilis_rv_ui32_constant_t_ {
	uint32_t integer;
	size_t label;
	bool link_time;
};

typedef struct subtilis_rv_ui32_constant_t_ subtilis_rv_ui32_constant_t;

struct subtilis_rv_real_constant_t_ {
	double real;
	size_t label;
};

typedef struct subtilis_rv_real_constant_t_ subtilis_rv_real_constant_t;

struct subtilis_rv_constants_t_ {
	subtilis_rv_ui32_constant_t *ui32;
	size_t ui32_count;
	size_t max_ui32;
	subtilis_rv_real_constant_t *real;
	size_t real_count;
	size_t max_real;
};

typedef struct subtilis_rv_constants_t_ subtilis_rv_constants_t;


struct subtilis_rv_section_t_ {
	size_t reg_counter;
	size_t freg_counter;
	size_t label_counter;
	size_t len;
	size_t first_op;
	size_t last_op;
	size_t locals;
	subtilis_rv_op_pool_t *op_pool;
	size_t call_site_count;
	size_t max_call_site_count;
	subtilis_rv_call_site_t *call_sites;
	subtilis_rv_constants_t constants;

	subtilis_sizet_vector_t ret_sites;
	subtilis_type_section_t *stype;
	const subtilis_settings_t *settings;
	size_t no_cleanup_label;
	int32_t start_address;
};

typedef struct subtilis_rv_section_t_ subtilis_rv_section_t;

struct subtilis_rv_prog_t_ {
	subtilis_rv_section_t **sections;
	size_t num_sections;
	size_t max_sections;
	subtilis_string_pool_t *string_pool;
	subtilis_constant_pool_t *constant_pool;
	subtilis_rv_op_pool_t *op_pool;
	const subtilis_settings_t *settings;
	int32_t start_address;
};

typedef struct subtilis_rv_prog_t_ subtilis_rv_prog_t;


subtilis_rv_op_pool_t *subtilis_rv_op_pool_new(subtilis_error_t *err);
size_t subtilis_rv_op_pool_alloc(subtilis_rv_op_pool_t *pool,
				  subtilis_error_t *err);
void subtilis_rv_op_pool_reset(subtilis_rv_op_pool_t *pool);
void subtilis_rv_op_pool_delete(subtilis_rv_op_pool_t *pool);

subtilis_rv_reg_t subtilis_rv_acquire_new_reg(subtilis_rv_section_t *s);
subtilis_rv_reg_t subtilis_rv_acquire_new_freg(subtilis_rv_section_t *s);

/* clang-format off */
subtilis_rv_section_t *subtilis_rv_section_new(subtilis_rv_op_pool_t *pool,
					       subtilis_type_section_t *stype,
					       size_t reg_counter,
					       size_t freg_counter,
					       size_t label_counter,
					       size_t locals,
					       const subtilis_settings_t *set,
					       int32_t start_address,
					       subtilis_error_t *err);

subtilis_rv_prog_t *subtilis_rv_prog_new(size_t max_sections,
					  subtilis_rv_op_pool_t *op_pool,
					  subtilis_string_pool_t *string_pool,
					  subtilis_constant_pool_t *cnst_pool,
					  const subtilis_settings_t *settings,
					  int32_t start_address,
					  subtilis_error_t *err);
/* clang-format off */
subtilis_rv_section_t *
subtilis_rv_prog_section_new(subtilis_rv_prog_t *prog,
			     subtilis_type_section_t *stype,
			     size_t reg_counter, size_t freg_counter,
			     size_t label_counter,
			     size_t locals, subtilis_error_t *err);
/* clang-format on */
void subtilis_rv_section_delete(subtilis_rv_section_t *s);

void subtilis_rv_prog_append_section(subtilis_rv_prog_t *prog,
				     subtilis_rv_section_t *rv_s,
				     subtilis_error_t *err);

void subtilis_rv_section_max_regs(subtilis_rv_section_t *s, size_t *int_regs,
				   size_t *real_regs);
void subtilis_rv_prog_delete(subtilis_rv_prog_t *prog);

void
subtilis_rv_section_add_itype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      uint32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_utype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      uint32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_stype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      uint32_t imm, subtilis_error_t *err);

void
subtilis_rv_section_add_rtype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      subtilis_error_t *err);


void
subtilis_rv_section_add_li(subtilis_rv_section_t *s,
			   subtilis_rv_reg_t rd,
			   int32_t imm,  subtilis_error_t *err);

void subtilis_rv_section_add_label(subtilis_rv_section_t *s, size_t label,
				   subtilis_error_t *err);
void subtilis_rv_section_insert_label(subtilis_rv_section_t *s, size_t label,
				      subtilis_rv_op_t *pos,
				      subtilis_error_t *err);

#define subtilis_rv_section_add_addi(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ADDI, rd, rs1, imm, err)
#define subtilis_rv_section_add_mv(s, rd, rs, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ADDI, rd, rs, 0, err)
#define subtilis_rv_section_add_ori(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ORI, rd, rs1, imm, err)
#define subtilis_rv_section_add_andi(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ANDI, rd, rs1, imm, err)
#define subtilis_rv_section_add_xori(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_XORI, rd, rs1, imm, err)
#define subtilis_rv_section_add_slti(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_SLTI, rd, rs1, imm, err)
#define subtilis_rv_section_add_sltui(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_SLTUI, rd, rs1, imm, err)
#define subtilis_rv_section_add_not(s, rd, rs1, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_XORI, rd, rs1, -1, err)

#define subtilis_rv_section_add_add(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rtype(s, SUBTILIS_RV_ADD, rd, rs1, rs2, err)
#define subtilis_rv_section_add_sub(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rtype(s, SUBTILIS_RV_SUB, rd, rs1, rs2, err)
#define subtilis_rv_section_add_and(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rtype(s, SUBTILIS_RV_AND, rd, rs1, rs2, err)
#define subtilis_rv_section_add_or(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rtype(s, SUBTILIS_RV_OR, rd, rs1, rs2, err)
#define subtilis_rv_section_add_xor(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rtype(s, SUBTILIS_RV_XOR, rd, rs1, rs2, err)


#define subtilis_rv_section_add_lb(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_LB, rd, rs1, imm, err)
#define subtilis_rv_section_add_lh(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_LH, rd, rs1, imm, err)
#define subtilis_rv_section_add_lw(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_LW, rd, rs1, imm, err)
#define subtilis_rv_section_add_lbu(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_LBU, rd, rs1, imm, err)
#define subtilis_rv_section_add_lhu(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_LHU, rd, rs1, imm, err)

#define subtilis_rv_section_add_sb(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SB, rs1, rs2, imm, err)
#define subtilis_rv_section_add_sh(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SH, rs1, rs2, imm, err)
#define subtilis_rv_section_add_sw(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SW, rs1, rs2, imm, err)

#define subtilis_rv_section_add_lui(s, rd, imm, err) \
	subtilis_rv_section_add_utype(s, SUBTILIS_RV_LUI, rd, imm, err)

#define subtilis_rv_section_add_nop(s, err) \
	subtilis_rv_section_add_addi(s, 0, 0, 0, err)

#define subtilis_rv_section_add_ecall(s, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ECALL, 0, 0, 0, err)

void
subtilis_rv_section_add_known_jal(subtilis_rv_section_t *s,
				  subtilis_rv_reg_t rd,
				  uint32_t offset,
				  subtilis_error_t *err);

void subtilis_rv_prog_dump(subtilis_rv_prog_t *p);
void subtilis_rv_instr_dump(subtilis_rv_instr_t *instr);

subtilis_rv_reg_t subtilis_rv_ir_to_rv_reg(size_t ir_reg);
subtilis_rv_reg_t subtilis_rv_ir_to_real_reg(size_t ir_reg);

#endif
