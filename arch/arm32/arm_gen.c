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

#include <limits.h>

#include "arm_gen.h"

#include "arm_core.h"
#include "arm_reg_alloc.h"

static void prv_data_simple(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;

	instr = subtilis_arm_section_add_instr(arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	datai->op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = subtilis_arm_ir_to_arm_reg(ir_op->operands[2].reg);
}

static void prv_cmp_simple(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_error_t *err)
{
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);
	op2 = subtilis_arm_ir_to_arm_reg(ir_op->operands[2].reg);

	subtilis_arm_add_cmp(arm_s, itype, ccode, op1, op2, err);
}

void subtilis_arm_gen_mov(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

void subtilis_arm_gen_movii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[1].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

void subtilis_arm_gen_addii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

void subtilis_arm_gen_subii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

void subtilis_arm_gen_rsubii32(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_rsub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest,
				  op1, op2, err);
}

void subtilis_arm_gen_mulii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_mul_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

void subtilis_arm_gen_muli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[2].reg);

	subtilis_arm_add_mul(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
			     op2, err);
}

void subtilis_arm_gen_addi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_subi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_ARM_INSTR_SUB,
			SUBTILIS_ARM_CCODE_AL, err);
}

static void prv_stran_instr(subtilis_arm_instr_type_t itype,
			    subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	base = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);
	subtilis_arm_add_stran_imm(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest,
				   base, offset, err);
}

void subtilis_arm_gen_storeoi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_ARM_INSTR_STR, s, start, user_data, err);
}

void subtilis_arm_gen_loadoi32(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_ARM_INSTR_LDR, s, start, user_data, err);
}

void subtilis_arm_gen_label(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	size_t label = s->ops[start]->op.label;

	subtilis_arm_section_add_label(arm_s, label, err);
}

static void prv_cmp_jmp_imm(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(cmp->operands[1].reg);

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[2].label;
}

void subtilis_arm_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_arm_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LT, err);
}

static void prv_data_imm(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_arm_instr_type_t itype,
			 subtilis_arm_ccode_type_t ccode, bool status,
			 subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;

	dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);

	subtilis_arm_add_data_imm(arm_s, itype, ccode, status, dest, op1,
				  ir_op->operands[2].integer, err);
}

static void prv_cmp_jmp(subtilis_ir_section_t *s, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ccode, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_ARM_INSTR_CMP,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[2].label;
}

void subtilis_arm_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_arm_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_LT, err);
}

static void prv_cmp_imm(subtilis_ir_section_t *s, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ok,
			subtilis_arm_ccode_type_t nok, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(cmp->operands[1].reg);
	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_arm_gen_gtii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		    SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_ltii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LT,
		    SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_gteii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		    SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_arm_gen_lteii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LE,
		    SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_eqii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		    SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_neqii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		    SUBTILIS_ARM_CCODE_EQ, err);
}

static void prv_cmp(subtilis_ir_section_t *s, size_t start, void *user_data,
		    subtilis_arm_ccode_type_t ok, subtilis_arm_ccode_type_t nok,
		    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_ARM_INSTR_CMP,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_arm_gen_gti32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_lti32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_LT,
		SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_gtei32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_arm_gen_ltei32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_LE,
		SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_eqi32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_neqi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_arm_gen_jump(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[0].label;
}

void subtilis_arm_gen_jmpc(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(jmp->operands[0].reg);

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, op1, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[2].label;
}

void subtilis_arm_gen_jmpc_rev(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(jmp->operands[0].reg);

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, op1, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_NE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[1].label;
}

void subtilis_arm_gen_jmpc_no_label(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(jmp->operands[0].reg);

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, op1, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_NE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[1].label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = jmp->operands[2].label;
}

void subtilis_arm_gen_eorii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, start, user_data, SUBTILIS_ARM_INSTR_EOR,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_orii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, start, user_data, SUBTILIS_ARM_INSTR_ORR,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_andii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, start, user_data, SUBTILIS_ARM_INSTR_AND,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_eori32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_ARM_INSTR_EOR,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_ori32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_ARM_INSTR_ORR,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_mvni32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_mvn_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

void subtilis_arm_gen_andi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_ARM_INSTR_AND,
			SUBTILIS_ARM_CCODE_AL, err);
}

static void prv_stack_args(subtilis_arm_section_t *arm_s,
			   subtilis_ir_call_t *call, size_t int_args_left,
			   size_t real_args_left, size_t *int_arg_ops,
			   size_t *real_arg_ops, subtilis_error_t *err)
{
	size_t offset;
	subtilis_arm_reg_t op0;
	subtilis_arm_reg_t arg_dest;
	subtilis_arm_reg_t arg_src;
	size_t i;
	size_t reg_num;
	size_t arg;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_fpa_stran_instr_t *fstran;
	size_t stack_ops;

	op0 = 13;

	/* TODO: There should be an argument limit here. */

	/*
	 * We shove the remaining arguments on the stack.  Then we just need
	 * to prime the register allocator to think it has previously spilled
	 * these values to the stack.
	 */

	offset = 0;
	stack_ops = 0;
	for (i = call->arg_count; i > 0 && int_args_left > 4; i--) {
		if (call->args[i - 1].type == SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		offset += 4;
		reg_num = call->args[i - 1].reg;
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_STR, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		int_arg_ops[stack_ops++] = arm_s->last_op;
		stran = &instr->operands.stran;
		stran->ccode = SUBTILIS_ARM_CCODE_AL;
		stran->dest = subtilis_arm_ir_to_arm_reg(reg_num);
		stran->base = op0;
		stran->offset.type = SUBTILIS_ARM_OP2_I32;
		stran->offset.op.integer = offset;
		stran->pre_indexed = true;
		stran->write_back = false;
		stran->subtract = true;
		int_args_left--;
	}

	/* TODO.  Would be better not to have FPA specific code in here */

	stack_ops = 0;
	for (i = call->arg_count; i > 0 && real_args_left > 4; i--) {
		if (call->args[i - 1].type != SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		offset += 8;
		reg_num = call->args[i - 1].reg;
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_FPA_INSTR_STF, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		real_arg_ops[stack_ops++] = arm_s->last_op;
		fstran = &instr->operands.fpa_stran;
		fstran->ccode = SUBTILIS_ARM_CCODE_AL;
		fstran->dest = subtilis_arm_ir_to_freg(reg_num);
		fstran->base = op0;
		fstran->offset = offset / 4;
		fstran->size = 8;
		fstran->pre_indexed = true;
		fstran->write_back = false;
		fstran->subtract = true;
		real_args_left--;
	}

	for (i = 0, arg = 0; i < call->arg_count && arg < int_args_left; i++) {
		if (call->args[i].type != SUBTILIS_IR_REG_TYPE_INTEGER)
			continue;
		arg_dest = arg;
		arg_src = subtilis_arm_ir_to_arm_reg(call->args[i].reg);
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 arg_dest, arg_src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		arg++;
	}

	for (i = 0, arg = 0; i < call->arg_count && arg < real_args_left; i++) {
		if (call->args[i].type != SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		arg_dest = arg;
		arg_src = subtilis_arm_ir_to_freg(call->args[i].reg);
		subtilis_fpa_add_mov(arm_s, SUBTILIS_ARM_CCODE_AL,
				     SUBTILIS_FPA_ROUNDING_NEAREST, arg_dest,
				     arg_src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		arg++;
	}
}

void subtilis_arm_gen_call_gen(subtilis_ir_section_t *s, size_t start,
			       void *user_data,
			       subtilis_arm_br_link_type_t link_type,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t op0;
	subtilis_arm_reg_t fpa_reg;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	size_t stm_site;
	size_t ldm_site;
	size_t call_site;
	int i;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;
	subtilis_arm_section_t *arm_s = user_data;
	size_t stf_site = INT_MAX;
	size_t ldf_site = INT_MAX;
	size_t int_args = 0;
	size_t real_args = 0;
	int save_real_start;
	size_t int_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];
	size_t real_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];

	for (i = 0; i < call->arg_count; i++) {
		if (call->args[i].type == SUBTILIS_IR_REG_TYPE_REAL)
			real_args++;
		else
			int_args++;
	}

	if (real_args > SUBTILIS_IR_MAX_ARGS_PER_TYPE ||
	    int_args > SUBTILIS_IR_MAX_ARGS_PER_TYPE) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prv_stack_args(arm_s, call, int_args, real_args, int_arg_ops,
		       real_arg_ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0 = 13;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, op0, 1 << 14 | 1 << 11,
			       SUBTILIS_ARM_MTRAN_FD, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stm_site = arm_s->last_op;

	/* TODO. We shouldn't have fpa stuff in here.  It should be abstracted*/

	if (s->freg_counter > 0) {
		save_real_start = real_args > 4 ? 4 : real_args;
		for (i = save_real_start; i < SUBTILIS_ARM_REG_MAX_FPA_REGS;
		     i++) {
			fpa_reg = i;
			subtilis_fpa_push_reg(arm_s, SUBTILIS_ARM_CCODE_NV,
					      fpa_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (stf_site == INT_MAX)
				stf_site = arm_s->last_op;
		}
	}

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = true;
	br->link_type = link_type;
	br->target.label = call->proc_id;

	call_site = arm_s->last_op;

	if (s->freg_counter > 0) {
		for (i = SUBTILIS_ARM_REG_MAX_FPA_REGS - 1;
		     i >= save_real_start; i--) {
			fpa_reg = i;
			subtilis_fpa_pop_reg(arm_s, SUBTILIS_ARM_CCODE_NV,
					     fpa_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (ldf_site == INT_MAX)
				ldf_site = arm_s->last_op;
		}
	}

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, op0, 1 << 14 | 1 << 11,
			       SUBTILIS_ARM_MTRAN_FD, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ldm_site = arm_s->last_op;
	subtilis_arm_section_add_call_site(
	    arm_s, stm_site, ldm_site, stf_site, ldf_site, int_args, real_args,
	    call_site, int_arg_ops, real_arg_ops, err);
}

void subtilis_arm_gen_call(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_gen_call_gen(s, start, user_data,
				  SUBTILIS_ARM_BR_LINK_VOID, err);
}

void subtilis_arm_gen_calli32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;

	subtilis_arm_gen_call_gen(s, start, user_data, SUBTILIS_ARM_BR_LINK_INT,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(call->reg);
	op1 = 0;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 err);
}

void subtilis_arm_gen_ret(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_instr_t *stack_add;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;

	stack_add =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &stack_add->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = 13;
	datai->op1 = datai->dest;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = 0;

	subtilis_arm_section_add_ret_site(arm_s, arm_s->last_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 15;
	op2 = 14;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

void subtilis_arm_gen_reti32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_gen_ret(s, start, user_data, err);
}

void subtilis_arm_gen_retii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[0].integer;

	dest = 0;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_gen_ret(s, start, user_data, err);
}

static void prv_gen_shift(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_arm_shift_type_t type,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *mov;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);
	op2 = subtilis_arm_ir_to_arm_reg(ir_op->operands[2].reg);

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	mov = &instr->operands.data;

	mov->status = false;
	mov->ccode = SUBTILIS_ARM_CCODE_AL;
	mov->dest = dest;
	mov->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	mov->op2.op.shift.type = type;
	mov->op2.op.shift.reg = op1;
	mov->op2.op.shift.shift_reg = true;
	mov->op2.op.shift.shift.reg = op2;
}

static void prv_gen_shift_i32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_arm_shift_type_t type,
			      subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *mov;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	mov = &instr->operands.data;

	mov->status = false;
	mov->ccode = SUBTILIS_ARM_CCODE_AL;
	mov->dest = dest;
	mov->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	mov->op2.op.shift.type = type;
	mov->op2.op.shift.reg = op1;
	mov->op2.op.shift.shift_reg = false;
	mov->op2.op.shift.shift.integer = ir_op->operands[2].integer;
}

void subtilis_arm_gen_lsli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_gen_shift(s, start, user_data, SUBTILIS_ARM_SHIFT_LSL, err);
}

void subtilis_arm_gen_lslii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_gen_shift_i32(s, start, user_data, SUBTILIS_ARM_SHIFT_LSL, err);
}

void subtilis_arm_gen_lsri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_gen_shift(s, start, user_data, SUBTILIS_ARM_SHIFT_LSR, err);
}

void subtilis_arm_gen_lsrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_gen_shift_i32(s, start, user_data, SUBTILIS_ARM_SHIFT_LSR, err);
}

void subtilis_arm_gen_asri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_gen_shift(s, start, user_data, SUBTILIS_ARM_SHIFT_ASR, err);
}

void subtilis_arm_gen_asrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_gen_shift_i32(s, start, user_data, SUBTILIS_ARM_SHIFT_ASR, err);
}
