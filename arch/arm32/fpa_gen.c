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

#include <limits.h>

#include "../../arch/arm32/arm_fpa_dist.h"
#include "../../common/error_codes.h"
#include "arm_core.h"
#include "arm_gen.h"
#include "fpa_alloc.h"
#include "fpa_gen.h"

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

void subtilis_fpa_gen_movrrdi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_freg(instr->operands[1].reg);

	subtilis_fpa_add_tran(
	    arm_s, SUBTILIS_FPA_INSTR_FIX, SUBTILIS_ARM_CCODE_AL,
	    SUBTILIS_FPA_ROUNDING_MINUS_INFINITY, dest, op2, err);
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

	subtilis_arm_gen_call_gen(s, start, user_data,
				  SUBTILIS_ARM_BR_LINK_REAL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_freg(call->reg);
	op1 = 0;

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

static void prv_data_monadic_simple(subtilis_ir_section_t *s, size_t start,
				    void *user_data,
				    subtilis_arm_instr_type_t itype,
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
	datai->op1 = 0;
	datai->immediate = false;
	datai->op2.reg = subtilis_arm_ir_to_freg(ir_op->operands[1].reg);
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

static void prv_check_divbyzero(subtilis_arm_section_t *arm_s,
				subtilis_ir_section_t *s, int32_t error_code,
				int32_t mask, subtilis_error_t *err)
{
	subtilis_arm_reg_t status;
	subtilis_arm_reg_t dest;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_br_instr_t *br;
	size_t label = arm_s->label_counter++;

	status = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	dest = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_fpa_add_cptran(arm_s, SUBTILIS_FPA_INSTR_RFS,
				SUBTILIS_ARM_CCODE_AL, status, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TST,
				 SUBTILIS_ARM_CCODE_AL, status, mask, err);
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
	br->target.label = label;

	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_AL, dest, error_code,
			      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_BIC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = status;
	datai->op1 = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = mask;

	subtilis_fpa_add_cptran(arm_s, SUBTILIS_FPA_INSTR_WFS,
				SUBTILIS_ARM_CCODE_AL, status, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

void subtilis_fpa_gen_divr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_DVF,
			SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, SUBTILIS_ERROR_CODE_DIV_BY_ZERO, 2, err);
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
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_simple_imm(s, start, user_data, SUBTILIS_FPA_INSTR_RDF,
			    SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, SUBTILIS_ERROR_CODE_DIV_BY_ZERO, 2, err);
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

	dest = 0;
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

	dest = 0;

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
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
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
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
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

void subtilis_fpa_gen_sin(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_SIN,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_cos(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_COS,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_tan(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_TAN,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_asn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_ASN,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_acs(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_ACS,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_atn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_ATN,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_sqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_SQT,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_log(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_LOG,
				SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, SUBTILIS_ERROR_CODE_LOG_RANGE, 3, err);
}

void subtilis_fpa_gen_ln(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_LGN,
				SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, SUBTILIS_ERROR_CODE_LOG_RANGE, 3, err);
}

void subtilis_fpa_gen_absr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_ABS,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_pow(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, start, user_data, SUBTILIS_FPA_INSTR_POW,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_exp(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_data_monadic_simple(s, start, user_data, SUBTILIS_FPA_INSTR_EXP,
				SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_fpa_gen_preamble(subtilis_arm_section_t *arm_s,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t mask;
	subtilis_arm_reg_t status;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	mask = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	status = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, mask,
				 31 << 16, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mvn_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, mask,
				 mask, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_fpa_add_cptran(arm_s, SUBTILIS_FPA_INSTR_RFS,
				SUBTILIS_ARM_CCODE_AL, status, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_AND, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = status;
	datai->op1 = status;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = mask;

	subtilis_fpa_add_cptran(arm_s, SUBTILIS_FPA_INSTR_WFS,
				SUBTILIS_ARM_CCODE_AL, status, err);
}

void subtilis_fpa_gen_movi8tofp(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_reg_t tmp;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *signx = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_freg(signx->operands[0].reg);
	src = subtilis_arm_ir_to_arm_reg(signx->operands[1].reg);
	tmp = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_gen_signx8to32_helper(arm_s, tmp, src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_fpa_add_tran(arm_s, SUBTILIS_FPA_INSTR_FLT,
			      SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_FPA_ROUNDING_NEAREST, dest, tmp, err);
}

void subtilis_fpa_gen_movfptoi32i32(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest1;
	subtilis_arm_reg_t dest2;
	subtilis_arm_reg_t src;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *mov = &s->ops[start]->op.instr;

	dest1 = subtilis_arm_ir_to_arm_reg(mov->operands[0].reg);
	dest2 = subtilis_arm_ir_to_arm_reg(mov->operands[1].reg);
	src = subtilis_arm_ir_to_freg(mov->operands[2].reg);

	/*
	 * TOOD: Need to check for stack overflow.
	 */

	subtilis_fpa_add_stran(arm_s, SUBTILIS_FPA_INSTR_STF,
			       SUBTILIS_ARM_CCODE_AL, src, 13, -8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, dest2, 13, -4, false,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, dest1, 13, -8, false,
				   err);
}

size_t subtilis_fpa_preserve_regs(subtilis_arm_section_t *arm_s,
				  int save_real_start, subtilis_error_t *err)
{
	int i;
	size_t stf_site = INT_MAX;

	for (i = save_real_start; i < SUBTILIS_ARM_REG_MAX_FPA_REGS; i++) {
		subtilis_fpa_push_reg(arm_s, SUBTILIS_ARM_CCODE_NV, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return INT_MAX;
		if (stf_site == INT_MAX)
			stf_site = arm_s->last_op;
	}

	return stf_site;
}

size_t subtilis_fpa_restore_regs(subtilis_arm_section_t *arm_s,
				 int save_real_start, subtilis_error_t *err)
{
	int i;
	size_t ldf_site = INT_MAX;

	for (i = SUBTILIS_ARM_REG_MAX_FPA_REGS - 1; i >= save_real_start; i--) {
		subtilis_fpa_pop_reg(arm_s, SUBTILIS_ARM_CCODE_NV, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return INT_MAX;
		if (ldf_site == INT_MAX)
			ldf_site = arm_s->last_op;
	}

	return ldf_site;
}

void subtilis_fpa_preserve_update(subtilis_arm_section_t *arm_s,
				  subtilis_arm_call_site_t *call_site,
				  size_t real_regs_saved, size_t real_regs_used,
				  subtilis_error_t *err)
{
	subtilis_arm_op_t *op;
	size_t i;
	size_t fpa_reg_count;

	if (real_regs_saved > SUBTILIS_ARM_REG_MAX_ARGS)
		real_regs_saved =
		    SUBTILIS_ARM_REG_MAX_FPA_REGS - SUBTILIS_ARM_REG_MAX_ARGS;
	else
		real_regs_saved =
		    SUBTILIS_ARM_REG_MAX_FPA_REGS - real_regs_saved;

	fpa_reg_count = SUBTILIS_ARM_REG_MAX_FPA_REGS - real_regs_saved;
	op = &arm_s->op_pool->ops[call_site->stf_site];

	for (; fpa_reg_count < SUBTILIS_ARM_REG_MAX_FPA_REGS; fpa_reg_count++) {
		if (real_regs_used & (1 << fpa_reg_count)) {
			op->op.instr.operands.fpa_stran.ccode =
			    SUBTILIS_ARM_CCODE_AL;
		}
		op = &arm_s->op_pool->ops[op->next];
	}

	fpa_reg_count = SUBTILIS_ARM_REG_MAX_FPA_REGS - 1;
	op = &arm_s->op_pool->ops[call_site->ldf_site];
	for (i = 0; i < real_regs_saved; i++) {
		if (real_regs_used & (1 << fpa_reg_count))
			op->op.instr.operands.fpa_stran.ccode =
			    SUBTILIS_ARM_CCODE_AL;
		op = &arm_s->op_pool->ops[op->next];
		fpa_reg_count--;
	}
}

void subtilis_fpa_update_offsets(subtilis_arm_section_t *arm_s,
				 subtilis_arm_call_site_t *call_site,
				 size_t bytes_saved, subtilis_error_t *err)
{
	size_t i;
	size_t ptr;
	subtilis_arm_op_t *op;
	subtilis_fpa_stran_instr_t *ft;

	for (i = 0; i < call_site->real_args - 4; i++) {
		ptr = call_site->real_arg_ops[i];
		op = &arm_s->op_pool->ops[ptr];
		ft = &op->op.instr.operands.fpa_stran;
		ft->offset += bytes_saved / 4;
	}
}

void subtilis_fpa_store_double(subtilis_arm_section_t *arm_s,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			       size_t offset, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_stran_instr_t *fstran;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_FPA_INSTR_STF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	fstran = &instr->operands.fpa_stran;
	fstran->ccode = SUBTILIS_ARM_CCODE_AL;
	fstran->dest = dest;
	fstran->base = base;
	fstran->offset = offset / 4;
	fstran->size = 8;
	fstran->pre_indexed = true;
	fstran->write_back = false;
	fstran->subtract = true;
}

void subtilis_fpa_mov_reg(subtilis_arm_section_t *arm_s,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			  subtilis_error_t *err)
{
	subtilis_fpa_add_mov(arm_s, SUBTILIS_ARM_CCODE_AL,
			     SUBTILIS_FPA_ROUNDING_NEAREST, dest, src, err);
}

void subtilis_arm_fpa_if_init(subtilis_arm_fp_if_t *fp_if)
{
	double dummy_float = 1.0;
	uint32_t *lower_word = (uint32_t *)((void *)&dummy_float);

	fp_if->max_regs = SUBTILIS_ARM_REG_MAX_FPA_REGS;
	fp_if->max_offset = 1023;
	fp_if->store_type = SUBTILIS_FPA_INSTR_STF;
	fp_if->load_type = SUBTILIS_FPA_INSTR_LDF;

	/* Slightly weird but on ARM FPA the words of a double are big endian */
	fp_if->reverse_fpa_consts = (*lower_word) == 0;
	fp_if->preamble_fn = subtilis_fpa_gen_preamble;
	fp_if->preserve_regs_fn = subtilis_fpa_preserve_regs;
	fp_if->restore_regs_fn = subtilis_fpa_restore_regs;
	fp_if->update_regs_fn = subtilis_fpa_preserve_update;
	fp_if->update_offs_fn = subtilis_fpa_update_offsets;
	fp_if->store_dbl_fn = subtilis_fpa_store_double;
	fp_if->mov_reg_fn = subtilis_fpa_mov_reg;
	fp_if->spill_imm_fn = subtilis_fpa_insert_stran_spill_imm;
	fp_if->stran_imm_fn = subtilis_fpa_insert_stran_imm;
	fp_if->is_fixed_fn = subtilis_fpa_is_fixed;
	fp_if->init_dist_walker_fn = subtilis_init_fpa_dist_walker;
	fp_if->init_used_walker_fn = subtilis_init_fpa_used_walker;
	fp_if->init_real_alloc_fn = subtilis_fpa_alloc_init_walker;
}
