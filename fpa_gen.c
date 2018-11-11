/*
 * Copyright (c) 2018 Mark Ryan
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

#include "fpa_gen.h"
#include "arm_core.h"
#include "arm_gen.h"

void subtilis_fpa_gen_movr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_freg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_freg(instr->operands[1].reg);

	subtilis_fpa_add_mov(arm_s, SUBTILIS_ARM_CCODE_AL,
			     SUBTILIS_FPA_ROUNDING_NEAREST, dest, op2, err);
}

void subtilis_fpa_gen_movir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	double op2 = instr->operands[1].real;

	dest = subtilis_arm_ir_to_freg(instr->operands[0].reg);

	subtilis_fpa_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL,
				 SUBTILIS_FPA_ROUNDING_NEAREST, dest, op2, err);
}

void subtilis_fpa_gen_movri32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_freg(instr->operands[1].reg);

	subtilis_fpa_add_tran(arm_s, SUBTILIS_FPA_INSTR_FIX,
			      SUBTILIS_ARM_CCODE_AL, SUBTILIS_FPA_ROUNDING_ZERO,
			      dest, op2, err);
}

void subtilis_fpa_gen_movi32r(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_freg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_fpa_add_tran(arm_s, SUBTILIS_FPA_INSTR_FLT,
			      SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_FPA_ROUNDING_NEAREST, dest, op2, err);
}

void subtilis_fpa_gen_callr(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;

	subtilis_arm_gen_call(s, start, user_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_freg(call->reg);
	op1.num = 0;
	op1.type = SUBTILIS_ARM_REG_FIXED;

	subtilis_fpa_add_mov(arm_s, SUBTILIS_ARM_CCODE_AL,
			     SUBTILIS_FPA_ROUNDING_NEAREST, dest, op1, err);
}

static void prv_data_simple(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;

	instr = subtilis_arm_section_add_instr(arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.fpa_data;
	datai->ccode = ccode;
	datai->size = 8;
	datai->rounding = SUBTILIS_FPA_ROUNDING_NEAREST;
	datai->dest = subtilis_arm_ir_to_freg(ir_op->operands[0].reg);
	datai->op1 = subtilis_arm_ir_to_freg(ir_op->operands[1].reg);
	datai->immediate = false;
	datai->op2.reg = subtilis_arm_ir_to_freg(ir_op->operands[2].reg);
}

static void prv_data_simple_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	double op2 = instr->operands[2].real;

	dest = subtilis_arm_ir_to_freg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_freg(instr->operands[1].reg);

	subtilis_fpa_add_data_imm(arm_s, itype, ccode,
				  SUBTILIS_FPA_ROUNDING_NEAREST, dest, op1, op2,
				  err);
}

void subtilis_fpa_gen_addr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_ADF,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_addir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_ADF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_subr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_SUF,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_subir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_SUF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_rsubir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_RSF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_mulr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_MUF,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_mulir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_MUF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_divr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_DVF,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_divir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_DVF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_rdivir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_RDF,
			    SUBTILIS_ARM_CCODE_AL, err);
}

static void prv_stran_instr(subtilis_arm_instr_type_t itype,
			    subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_freg(instr->operands[0].reg);
	base = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_fpa_add_stran(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest, base,
			       offset, err);
}

void subtilis_fpa_gen_storeor(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_FPA_INSTR_STF, s, start, user_data, err);
}

void subtilis_fpa_gen_loador(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_FPA_INSTR_LDF, s, start, user_data, err);
}

void subtilis_fpa_gen_retr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest.num = 0;
	dest.type = SUBTILIS_ARM_REG_FIXED;
	op2 = subtilis_arm_ir_to_freg(instr->operands[0].reg);

	subtilis_fpa_add_mov(arm_s, SUBTILIS_ARM_CCODE_AL,
			     SUBTILIS_FPA_ROUNDING_NEAREST, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_gen_ret(s, start, user_data, err);
}

void subtilis_fpa_gen_retir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	double op2 = instr->operands[0].real;

	dest.num = 0;
	dest.type = SUBTILIS_ARM_REG_FIXED;

	subtilis_fpa_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL,
				 SUBTILIS_FPA_ROUNDING_NEAREST, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_gen_ret(s, start, user_data, err);
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

	op1 = subtilis_arm_ir_to_freg(cmp->operands[1].reg);

	subtilis_fpa_add_cmf_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = false;
	br->target.label = jmp->operands[2].label;
}

void subtilis_fpa_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_fpa_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_fpa_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_fpa_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_fpa_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_fpa_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LT, err);
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

	op1 = subtilis_arm_ir_to_freg(ir_op->operands[1].reg);
	op2 = subtilis_arm_ir_to_freg(ir_op->operands[2].reg);

	subtilis_fpa_add_cmp(arm_s, itype, ccode, op1, op2, err);
}

static void prv_cmp_jmp(subtilis_ir_section_t *s, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ccode, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_FPA_INSTR_CMF,
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
	br->target.label = jmp->operands[2].label;
}

void subtilis_fpa_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_fpa_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_fpa_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_fpa_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_fpa_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_fpa_gen_if_gte(subtilis_ir_section_t *s, size_t start,
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

	op1 = subtilis_arm_ir_to_freg(cmp->operands[1].reg);
	subtilis_fpa_add_cmf_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_fpa_gen_gtir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		    SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_fpa_gen_ltir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LT,
		    SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_fpa_gen_eqir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		    SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_fpa_gen_neqir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		    SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_fpa_gen_gteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		    SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_fpa_gen_lteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LE,
		    SUBTILIS_ARM_CCODE_GT, err);
}

static void prv_cmp(subtilis_ir_section_t *s, size_t start, void *user_data,
		    subtilis_arm_ccode_type_t ok, subtilis_arm_ccode_type_t nok,
		    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_FPA_INSTR_CMF,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_fpa_gen_gtr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_fpa_gen_ltr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_LT,
		SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_fpa_gen_gter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_fpa_gen_lter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_LE,
		SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_fpa_gen_eqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_fpa_gen_neqr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		SUBTILIS_ARM_CCODE_EQ, err);
}