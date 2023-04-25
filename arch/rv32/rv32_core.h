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

#define SUBTILIS_RV_REG_LINK 1
#define SUBTILIS_RV_REG_STACK 2
#define SUBTILIS_RV_REG_GLOBAL 3
#define SUBTILIS_RV_REG_HEAP 4
#define SUBTILIS_RV_REG_T0 5
#define SUBTILIS_RV_REG_T1 6
#define SUBTILIS_RV_REG_T2 7
#define SUBTILIS_RV_REG_LOCAL 8
#define SUBTILIS_RV_REG_A0 10
#define SUBTILIS_RV_REG_A1 11
#define SUBTILIS_RV_REG_A2 12
#define SUBTILIS_RV_REG_A3 13
#define SUBTILIS_RV_REG_A4 14
#define SUBTILIS_RV_REG_A5 15
#define SUBTILIS_RV_REG_A6 16
#define SUBTILIS_RV_REG_A7 17

#define SUBTILIS_RV_REG_T3 28
#define SUBTILIS_RV_REG_T4 29
#define SUBTILIS_RV_REG_T5 30
#define SUBTILIS_RV_REG_T6 31
#define SUBTILIS_RV_REG_T7 32

#define SUBTILIS_RV_REG_FA0 10
#define SUBTILIS_RV_REG_FA1 11
#define SUBTILIS_RV_REG_FA2 12
#define SUBTILIS_RV_REG_FA3 13
#define SUBTILIS_RV_REG_FA4 14
#define SUBTILIS_RV_REG_FA5 15
#define SUBTILIS_RV_REG_FA6 16
#define SUBTILIS_RV_REG_FA7 17

#define SUBTILIS_RV_MAX_OFFSET 2047
#define SUBTILIS_RV_MIN_OFFSET -2048


/*
 * Two register namespaces overlap for each register type, int or real.
 * For integers register numbers < 32 are fixed and will not be altered
 * by the register allocator.
 */

#define SUBTILIS_RV_INT_FIRST_FREE 5
#define SUBTILIS_RV_REAL_FIRST_FREE 0
#define SUBTILIS_RV_REG_MAX_INT_REGS 32
#define SUBTILIS_RV_REG_MAX_REAL_REGS 32

#define RV_MAX_REG_ARGS 8

typedef enum {
	SUBTILIS_RV_JAL_LINK_VOID,
	SUBTILIS_RV_JAL_LINK_INT,
	SUBTILIS_RV_JAL_LINK_REAL,
} subtilis_rv_jal_link_type_t;

/*
 * These match the actual values to be encoded statically in instructions
 * for convenience.
 */

typedef enum {
	SUBTILIS_RV_FRM_RTE,
	SUBTILIS_RV_FRM_RTZ,
	SUBTILIS_RV_FRM_RDN,
	SUBTILIS_RV_FRM_RUP,
	SUBTILIS_RV_FRM_RMM,
	SUBTILIS_RV_FRM_RES1,
	SUBTILIS_RV_FRM_RES2,
	SUBTILIS_RV_FRM_DYN,
} subtilis_rv_frm_t;

#define SUBTILIS_RV_DEFAULT_FRM SUBTILIS_RV_FRM_RTE

typedef size_t subtilis_rv_reg_t;

struct rv_rtype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
};
typedef struct rv_rtype_t_ rv_rtype_t;

struct rv_rrtype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_frm_t frm;
};
typedef struct rv_rrtype_t_ rv_rrtype_t;

struct rv_r4type_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_reg_t rs3;
	subtilis_rv_frm_t frm;
};
typedef struct rv_r4type_t_ rv_r4type_t;

struct rv_itype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rs1;
	int32_t imm;
	subtilis_rv_jal_link_type_t link_type;
};
typedef struct rv_itype_t_ rv_itype_t;

struct rv_sbtype_t_ {
	bool is_label;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	union {
		int32_t imm;
		size_t label;
	} op;
};
typedef struct rv_sbtype_t_ rv_sbtype_t;

struct rv_ujtype_t_ {
	bool is_label;
	subtilis_rv_reg_t rd;
	union {
		int32_t imm; // offset in bytes not words
		size_t label;
	} op;
	subtilis_rv_jal_link_type_t link_type;
};
typedef struct rv_ujtype_t_ rv_ujtype_t;

struct rv_ldrctype_t_ {
	subtilis_rv_reg_t rd;
	subtilis_rv_reg_t rd2; /* Xreg used to hold the pointer */
	size_t label;
};

typedef struct rv_ldrctype_t_ rv_ldrctype_t;

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
	SUBTILIS_RV_MUL,
	SUBTILIS_RV_MULH,
	SUBTILIS_RV_MULHSU,
	SUBTILIS_RV_MULHU,
	SUBTILIS_RV_DIV,
	SUBTILIS_RV_DIVU,
	SUBTILIS_RV_REM,
	SUBTILIS_RV_REMU,

	SUBTILIS_RV_FLW,
	SUBTILIS_RV_FSW,
	SUBTILIS_RV_FMADD_S,
	SUBTILIS_RV_FMSUB_S,
	SUBTILIS_RV_FNMSUB_S,
	SUBTILIS_RV_FNMADD_S,
	SUBTILIS_RV_FADD_S,
	SUBTILIS_RV_FSUB_S,
	SUBTILIS_RV_FMUL_S,
	SUBTILIS_RV_FDIV_S,
	SUBTILIS_RV_FSQRT_S,
	SUBTILIS_RV_FSGNJ_S,
	SUBTILIS_RV_FSGNJN_S,
	SUBTILIS_RV_FSGNJX_S,
	SUBTILIS_RV_FMIN_S,
	SUBTILIS_RV_FMAX_S,
	SUBTILIS_RV_FCVT_W_S,
	SUBTILIS_RV_FCVT_WU_S,
	SUBTILIS_RV_FMV_X_W,
	SUBTILIS_RV_FEQ_S,
	SUBTILIS_RV_FLT_S,
	SUBTILIS_RV_FLE_S,
	SUBTILIS_RV_FCLASS_S,
	SUBTILIS_RV_FCVT_S_W,
	SUBTILIS_RV_FCVT_S_WU,
	SUBTILIS_RV_FMV_W_X,

	SUBTILIS_RV_FLD,
	SUBTILIS_RV_FSD,
	SUBTILIS_RV_FMADD_D,
	SUBTILIS_RV_FMSUB_D,
	SUBTILIS_RV_FNMSUB_D,
	SUBTILIS_RV_FNMADD_D,
	SUBTILIS_RV_FADD_D,
	SUBTILIS_RV_FSUB_D,
	SUBTILIS_RV_FMUL_D,
	SUBTILIS_RV_FDIV_D,
	SUBTILIS_RV_FSQRT_D,
	SUBTILIS_RV_FSGNJ_D,
	SUBTILIS_RV_FSGNJN_D,
	SUBTILIS_RV_FSGNJX_D,
	SUBTILIS_RV_FMIN_D,
	SUBTILIS_RV_FMAX_D,
	SUBTILIS_RV_FCVT_S_D,
	SUBTILIS_RV_FCVT_D_S,
	SUBTILIS_RV_FEQ_D,
	SUBTILIS_RV_FLT_D,
	SUBTILIS_RV_FLE_D,
	SUBTILIS_RV_FCLASS_D,
	SUBTILIS_RV_FCVT_W_D,
	SUBTILIS_RV_FCVT_WU_D,
	SUBTILIS_RV_FCVT_D_W,
	SUBTILIS_RV_FCVT_D_WU,

	/*
	 * not real instructions
	 * and only generated by the compiler.
	 */

	SUBTILIS_RV_LC,   /* Load address of a constant */
	SUBTILIS_RV_LP,   /* Load address of a procedure */
	SUBTILIS_RV_LDRCF /* Load a 64 bit constant float */
} subtilis_rv_instr_type_t;

typedef enum {
	SUBTILIS_RV_R_TYPE,
	SUBTILIS_RV_I_TYPE,
	SUBTILIS_RV_S_TYPE,
	SUBTILIS_RV_B_TYPE,
	SUBTILIS_RV_U_TYPE,
	SUBTILIS_RV_J_TYPE,
	SUBTILIS_RV_FENCE_TYPE,

	/*
	 * Even though the floating point instructions use the integer
	 * encodings its more convenient for the backend (mainly the
	 * register allocator), if we treat them separately.  There's
	 * no need to have separate encodings for floats and doubles.
	 */

	SUBTILIS_RV_REAL_R_TYPE,
	SUBTILIS_RV_REAL_R4_TYPE,
	SUBTILIS_RV_REAL_I_TYPE,
	SUBTILIS_RV_REAL_S_TYPE,
	SUBTILIS_RV_LDRC_F_TYPE,
} subtilis_rv_instr_encoding_t;

struct subtilis_rv_instr_t_ {
	subtilis_rv_instr_type_t itype;
	subtilis_rv_instr_encoding_t etype;
	union {
		rv_rtype_t r;
		rv_rrtype_t rr;
		rv_r4type_t r4;
		rv_itype_t i;
		rv_sbtype_t sb;
		rv_ujtype_t uj;
		rv_ldrctype_t ldrc;
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

subtilis_rv_instr_t *
subtilis_rv_section_add_instr(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_instr_encoding_t etype,
			      subtilis_error_t *err);

subtilis_rv_instr_t *
subtilis_rv_section_insert_instr(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_instr_encoding_t etype,
				 subtilis_error_t *err);

void
subtilis_rv_section_add_itype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      int32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_real_itype(subtilis_rv_section_t *s,
				   subtilis_rv_instr_type_t itype,
				   subtilis_rv_reg_t rd,
				   subtilis_rv_reg_t rs1,
				   int32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_itype_link(subtilis_rv_section_t *s,
				   subtilis_rv_instr_type_t itype,
				   subtilis_rv_reg_t rd,
				   subtilis_rv_reg_t rs1,
				   int32_t imm,
				   subtilis_rv_jal_link_type_t link_type,
				   subtilis_error_t *err);

void
subtilis_rv_section_insert_itype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 subtilis_rv_reg_t rs1,
				 int32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_utype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      int32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_utype_const(subtilis_rv_section_t *s,
				    subtilis_rv_instr_type_t itype,
				    subtilis_rv_reg_t rd,
				    int32_t id,  subtilis_error_t *err);

void
subtilis_rv_section_insert_utype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 int32_t imm,  subtilis_error_t *err);

void
subtilis_rv_section_add_stype_gen(subtilis_rv_section_t *s,
				  subtilis_rv_instr_type_t itype,
				  subtilis_rv_instr_encoding_t etype,
				  subtilis_rv_reg_t rs1,
				  subtilis_rv_reg_t rs2,
				  int32_t imm,  subtilis_error_t *err);

#define subtilis_rv_section_add_stype(s, itype, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype_gen(s, itype, SUBTILIS_RV_S_TYPE, rs1,\
					  rs2, imm, err)

#define subtilis_rv_section_add_real_stype(s, itype, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype_gen(s, itype, SUBTILIS_RV_REAL_S_TYPE, \
					  rs1, rs2, imm, err)

void
subtilis_rv_section_add_btype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      size_t label, subtilis_error_t *err);

void
subtilis_rv_section_insert_sbtype(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_instr_type_t itype,
				  subtilis_rv_reg_t rs1,
				  subtilis_rv_reg_t rs2,
				  int32_t imm, subtilis_error_t *err);

void
subtilis_rv_section_add_rtype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      subtilis_error_t *err);

void
subtilis_rv_section_add_rrtype(subtilis_rv_section_t *s,
			       subtilis_rv_instr_type_t itype,
			       subtilis_rv_reg_t rd,
			       subtilis_rv_reg_t rs1,
			       subtilis_rv_reg_t rs2,
			       subtilis_rv_frm_t frm,
			       subtilis_error_t *err);

void
subtilis_rv_section_add_r4type(subtilis_rv_section_t *s,
			       subtilis_rv_instr_type_t itype,
			       subtilis_rv_reg_t rd,
			       subtilis_rv_reg_t rs1,
			       subtilis_rv_reg_t rs2,
			       subtilis_rv_reg_t rs3,
			       subtilis_rv_frm_t frm,
			       subtilis_error_t *err);

void
subtilis_rv_section_insert_rtype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 subtilis_rv_reg_t rs1,
				 subtilis_rv_reg_t rs2,
				 subtilis_error_t *err);

void
subtilis_rv_section_add_li(subtilis_rv_section_t *s,
			   subtilis_rv_reg_t rd,
			   int32_t imm,  subtilis_error_t *err);
void
subtilis_rv_section_insert_li(subtilis_rv_section_t *s,
			      subtilis_rv_op_t *pos,
			      subtilis_rv_reg_t rd,
			      int32_t imm,  subtilis_error_t *err);

#define subtilis_rv_section_add_lc(s, rd, id, err) \
	subtilis_rv_section_add_utype_const(s, SUBTILIS_RV_LC, rd, id, err)

#define subtilis_rv_section_add_lp(s, rd, id, err) \
	subtilis_rv_section_add_utype_const(s, SUBTILIS_RV_LP, rd, id, err)

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
#define subtilis_rv_section_add_sltiu(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_SLTIU, rd, rs1, imm, err)
#define subtilis_rv_section_add_not(s, rd, rs1, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_XORI, rd, rs1, -1, err)

#define subtilis_rv_section_insert_addi(s, pos, rd, rs1, imm, err) \
	subtilis_rv_section_insert_itype(s, pos, SUBTILIS_RV_ADDI, rd, rs1, \
					 imm, err)

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

#define subtilis_rv_section_insert_add(s, pos, rd, rs1, rs2, err) \
	subtilis_rv_section_insert_itype(s, pos, SUBTILIS_RV_ADD, rd, rs1, \
					 rs2, err)

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

void subtilis_rv_section_insert_lw(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t dest,
				   subtilis_rv_reg_t base,
				   int32_t offset, subtilis_error_t *err);
void subtilis_rv_section_insert_sw(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t rs1,
				   subtilis_rv_reg_t rs2,
				   int32_t offset, subtilis_error_t *err);

#define subtilis_rv_section_add_sb(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SB, rs1, rs2, imm, err)
#define subtilis_rv_section_add_sh(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SH, rs1, rs2, imm, err)
#define subtilis_rv_section_add_sw(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_stype(s, SUBTILIS_RV_SW, rs1, rs2, imm, err)

#define subtilis_rv_section_add_lui(s, rd, imm, err) \
	subtilis_rv_section_add_utype(s, SUBTILIS_RV_LUI, rd, imm, err)

#define subtilis_rv_section_add_auipc(s, rd, imm, err) \
	subtilis_rv_section_add_utype(s, SUBTILIS_RV_AUIPC, rd, imm, err)

#define subtilis_rv_section_add_nop(s, err) \
	subtilis_rv_section_add_addi(s, 0, 0, 0, err)

#define subtilis_rv_section_add_ecall(s, err) \
	subtilis_rv_section_add_itype(s, SUBTILIS_RV_ECALL, 0, 0, 0, err)

#define subtilis_rv_section_add_beq(s, rs1, rs2, label, err) \
	subtilis_rv_section_add_btype(s, SUBTILIS_RV_BEQ, rs1, rs2, label, err)

#define subtilis_rv_section_add_bne(s, rs1, rs2, label, err) \
	subtilis_rv_section_add_btype(s, SUBTILIS_RV_BNE, rs1, rs2, label, err)

#define subtilis_rv_section_add_bge(s, rs1, rs2, label, err) \
	subtilis_rv_section_add_btype(s, SUBTILIS_RV_BGE, rs1, rs2, label, err)

#define subtilis_rv_section_add_blt(s, rs1, rs2, label, err) \
	subtilis_rv_section_add_btype(s, SUBTILIS_RV_BLT, rs1, rs2, label, err)


void
subtilis_rv_section_add_known_jal(subtilis_rv_section_t *s,
				  subtilis_rv_reg_t rd,
				  int32_t offset, subtilis_error_t *err);

void
subtilis_rv_section_add_jal(subtilis_rv_section_t *s,
			    subtilis_rv_reg_t rd,
			    size_t label, subtilis_rv_jal_link_type_t link_type,
			    subtilis_error_t *err);

void
subtilis_rv_section_add_jalr(subtilis_rv_section_t *s, subtilis_rv_reg_t rd,
			     subtilis_rv_reg_t rs1, int32_t offset,
			     subtilis_rv_jal_link_type_t link_type,
			     subtilis_error_t *err);

/*
 * Floating point macros
 */

#define subtilis_rv_section_add_flw(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_real_itype(s, SUBTILIS_RV_FLW, rd, rs1, imm,\
					   err)
#define subtilis_rv_section_add_fsw(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_real_stype(s, SUBTILIS_RV_FSW, rs1, rs2, imm,\
					   err)
#define subtilis_rv_section_add_fmadd_s(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FMADD_S, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fmsub_s(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FMSUB_S, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fnmsub_s(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FNMSUB_S, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fnmadd_s(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FNMADD_S, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fadd_s(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FADD_S, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fsub_s(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSUB_S, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fmul_s(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMUL_S, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fdiv_s(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FDIV_S, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fsqrt_s(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSQRT_S, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fsgnj_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJ_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fsgnjn_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJN_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fsgnjx_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJX_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RDN, err)
#define subtilis_rv_section_add_fmin_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMIN_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fmax_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMAX_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fcvt_w_s(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_W_S, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_wu_s(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_WU_S, rd, rs1, 1,\
				       frm, err)
#define subtilis_rv_section_add_fmv_x_w(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMV_X_W, rd, rs1, 0, \
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_feq_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FEQ_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RDN, err)
#define subtilis_rv_section_add_flt_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FLT_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fle_s(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FLE_S, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fclass_s(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCLASS_S, rd, rs1, 0,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fcvt_s_w(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_S_W, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_s_wu(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_S_WU, rd, rs1, 1,\
				       frm, err)
#define subtilis_rv_section_add_fmv_w_x(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMX_W_X, rd, rs1, 0,\
				       SUBTILIS_RV_FRM_RTE, err)

#define subtilis_rv_section_add_fmv_s(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJ_S, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fneg_s(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJN_S, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fabs_s(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJX_S, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RDN, err)

#define subtilis_rv_section_add_fld(s, rd, rs1, imm, err) \
	subtilis_rv_section_add_real_itype(s, SUBTILIS_RV_FLD, rd, rs1, imm,\
					   err)
#define subtilis_rv_section_add_fsd(s, rs1, rs2, imm, err) \
	subtilis_rv_section_add_real_stype(s, SUBTILIS_RV_FSD, rs1, rs2, imm,\
					   err)
#define subtilis_rv_section_add_fmadd_d(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FMADD_D, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fmsub_d(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FMSUB_D, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fnmsub_d(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FNMSUB_D, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fnmadd_d(s, rd, rs1, rs2, r3, frm, err) \
	subtilis_rv_section_add_r4type(s, SUBTILIS_RV_FNMADD_D, rd, rs1, rs2,\
				       rs3, frm, err)
#define subtilis_rv_section_add_fadd_d(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FADD_D, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fsub_d(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSUB_D, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fmul_d(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMUL_D, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fdiv_d(s, rd, rs1, rs2, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FDIV_D, rd, rs1, rs2,\
				       frm, err)
#define subtilis_rv_section_add_fsqrt_d(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSQRT_D, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fsgnj_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJ_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fsgnjn_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJN_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fsgnjx_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJX_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RDN, err)
#define subtilis_rv_section_add_fmin_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMIN_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fmax_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FMAX_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fcvt_s_d(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_S_D, rd, rs1, 1,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_d_s(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_D_S, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_feq_d(s, rd, rs1, rs2, err)		\
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FEQ_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RDN, err)
#define subtilis_rv_section_add_flt_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FLT_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fle_d(s, rd, rs1, rs2, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FLE_D, rd, rs1, rs2,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fclass_d(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCLASS_D, rd, rs1, 0,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fcvt_w_d(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_W_D, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_wu_d(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_WU_D, rd, rs1, 1,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_d_w(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_D_W, rd, rs1, 0,\
				       frm, err)
#define subtilis_rv_section_add_fcvt_d_wu(s, rd, rs1, frm, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FCVT_D_WU, rd, rs1, 1,\
				       frm, err)
#define subtilis_rv_section_add_fmv_d(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJ_D, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RTE, err)
#define subtilis_rv_section_add_fneg_d(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJN_D, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RTZ, err)
#define subtilis_rv_section_add_fabs_d(s, rd, rs1, err) \
	subtilis_rv_section_add_rrtype(s, SUBTILIS_RV_FSGNJX_D, rd, rs1, rs1,\
				       SUBTILIS_RV_FRM_RDN, err)

void subtilis_rv_add_real_constant(subtilis_rv_section_t *s, size_t label,
				   double num, subtilis_error_t *err);
void subtilis_rv_add_copy_immd(subtilis_rv_section_t *s,
			       subtilis_rv_reg_t dest, subtilis_rv_reg_t dest2,
			       double src, subtilis_error_t *err);

void
subtilis_rv_section_insert_real_itype(subtilis_rv_section_t *s,
				      subtilis_rv_op_t *pos,
				      subtilis_rv_instr_type_t itype,
				      subtilis_rv_reg_t rd,
				      subtilis_rv_reg_t rs1,
				      int32_t imm,  subtilis_error_t *err);
void
subtilis_rv_section_insert_real_stype(subtilis_rv_section_t *s,
				      subtilis_rv_op_t *pos,
				      subtilis_rv_instr_type_t itype,
				      subtilis_rv_reg_t rs1,
				      subtilis_rv_reg_t rs2,
				      int32_t imm, subtilis_error_t *err);
void subtilis_rv_section_insert_ld(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t dest,
				   subtilis_rv_reg_t base,
				   int32_t offset, subtilis_error_t *err);
void subtilis_rv_section_insert_sd(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t rs1,
				   subtilis_rv_reg_t rs2,
				   int32_t offset, subtilis_error_t *err);

void subtilis_rv_prog_dump(subtilis_rv_prog_t *p);
void subtilis_rv_instr_dump(subtilis_rv_instr_t *instr);

subtilis_rv_reg_t subtilis_rv_ir_to_rv_reg(size_t ir_reg);
subtilis_rv_reg_t subtilis_rv_ir_to_real_reg(size_t ir_reg);

/*
 * Updates base by inserting instructions at pos that add offset
 * to base using the tmp register tmp.
 */

void subtilis_rv_insert_offset_helper(subtilis_rv_section_t *s,
				      subtilis_rv_op_t *pos,
				      subtilis_rv_reg_t base,
				      subtilis_rv_reg_t tmp,
				      int32_t offset, subtilis_error_t *err);

/*
 * Inserts an lw statement into the code stream at the location
 * indicated by current.  If the offset cannot fit in 12 bits
 * we li the constant into the destination register, add the
 * base and then do a load from dest to dest with a zero offset.
 */

void subtilis_rv_insert_lw_helper(subtilis_rv_section_t *rv_s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t dest,
				  subtilis_rv_reg_t base,
				  int32_t offset, subtilis_error_t *err);

/*
 * Inserts an sw statement into the code stream at the location
 * indicated by current.  If the offset cannot fit in 12 bits
 * we li the constant into tmp, add the
 * rs1 (base)  and then do a store from tmp to dest with a zero offset.
 */

void subtilis_rv_insert_sw_helper(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t rs1,
				  subtilis_rv_reg_t rs2,
				  subtilis_rv_reg_t tmp,
				  int32_t offset, subtilis_error_t *err);

/*
 * Inserts an fld statement into the code stream at the location
 * indicated by current.  If the offset cannot fit in 12 bits
 * we li the constant into the destination register, add the
 * base and then do a load from dest to dest with a zero offset.
 */

void subtilis_rv_insert_ld_helper(subtilis_rv_section_t *rv_s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t dest,
				  subtilis_rv_reg_t base,
				  int32_t offset, subtilis_error_t *err);

/*
 * Inserts an fsd statement into the code stream at the location
 * indicated by current.  If the offset cannot fit in 12 bits
 * we li the constant into tmp, add the
 * rs1 (base)  and then do a store from tmp to dest with a zero offset.
 */

void subtilis_rv_insert_sd_helper(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t rs1,
				  subtilis_rv_reg_t rs2,
				  subtilis_rv_reg_t tmp,
				  int32_t offset, subtilis_error_t *err);


void subtilis_rv_section_add_ret_site(subtilis_rv_section_t *s, size_t op,
				      subtilis_error_t *err);


void subtilis_rv_section_nopify_instr(subtilis_rv_instr_t *instr);
bool subtilis_rv_section_is_nop(subtilis_rv_op_t *op);

#endif
