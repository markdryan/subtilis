/*
 * Copyright (c) 2020 Mark Ryan
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

#include "../../common/error_codes.h"
#include "arm_core.h"
#include "arm_gen.h"
#include "arm_vfp_dist.h"
#include "vfp_alloc.h"
#include "vfp_gen.h"

static void prv_stran_instr(subtilis_arm_instr_type_t itype,
			    subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	base = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_vfp_add_stran(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest, base,
			       offset, err);
}

void subtilis_vfp_gen_movr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_dreg(instr->operands[1].reg);

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, src, err);
}

void subtilis_vfp_gen_movir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	double src;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	src = instr->operands[1].real;

	subtilis_vfp_add_copy_imm(arm_s, SUBTILIS_ARM_CCODE_AL, dest, src, err);
}

void subtilis_vfp_gen_movri32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_reg_t tmp;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_dreg(instr->operands[1].reg);
	tmp = subtilis_arm_ir_to_dreg(s->freg_counter++);

	subtilis_vfp_add_tran(arm_s, SUBTILIS_VFP_INSTR_FTOSIZD,
			      SUBTILIS_ARM_CCODE_AL, true, tmp, src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_cptran(arm_s, SUBTILIS_VFP_INSTR_FMRS,
				SUBTILIS_ARM_CCODE_AL, true, dest, tmp, err);
}

void subtilis_vfp_gen_movrrdi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_reg_t fpscr;
	subtilis_arm_reg_t tmp;
	subtilis_arm_reg_t fpscr_mod;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_dreg(instr->operands[1].reg);
	tmp = subtilis_arm_ir_to_dreg(s->freg_counter++);
	fpscr = subtilis_arm_ir_to_dreg(s->reg_counter++);
	fpscr_mod = subtilis_arm_ir_to_dreg(s->reg_counter++);

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMRX,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, fpscr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_BIC,
				  SUBTILIS_ARM_CCODE_AL, false, fpscr_mod,
				  fpscr, 3 << 22, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
				  SUBTILIS_ARM_CCODE_AL, false, fpscr_mod,
				  fpscr_mod, 2 << 22, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMXR,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, fpscr_mod, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_tran(arm_s, SUBTILIS_VFP_INSTR_FTOSID,
			      SUBTILIS_ARM_CCODE_AL, true, tmp, src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMXR,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, fpscr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_vfp_add_cptran(arm_s, SUBTILIS_VFP_INSTR_FMRS,
				SUBTILIS_ARM_CCODE_AL, true, dest, tmp, err);
}

void subtilis_vfp_gen_movi32r(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_reg_t tmp;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);
	tmp = subtilis_arm_ir_to_dreg(s->freg_counter++);

	subtilis_vfp_add_cptran(arm_s, SUBTILIS_VFP_INSTR_FMSR,
				SUBTILIS_ARM_CCODE_AL, true, tmp, src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_tran(arm_s, SUBTILIS_VFP_INSTR_FSITOD,
			      SUBTILIS_ARM_CCODE_AL, true, dest, tmp, err);
}

static void prv_gen_callr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, bool indirect, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;

	subtilis_arm_gen_call_gen(s, start, user_data,
				  SUBTILIS_ARM_BR_LINK_REAL, indirect, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_dreg(call->reg);
	op1 = 0;

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, op1, err);
}

void subtilis_vfp_gen_callr(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_gen_callr(s, start, user_data, false, err);
}

void subtilis_vfp_gen_callr_ptr(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_gen_callr(s, start, user_data, true, err);
}

static void prv_check_divbyzero(subtilis_arm_section_t *arm_s,
				subtilis_ir_section_t *s,
				subtilis_arm_reg_t res, int32_t error_code,
				subtilis_error_t *err)
{
	subtilis_arm_reg_t status;
	subtilis_arm_reg_t dest;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_br_instr_t *br;
	size_t label = arm_s->label_counter++;

	dest = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	status = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMRX,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, status, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TST,
				 SUBTILIS_ARM_CCODE_AL, status, 2, err);
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
	datai->op2.op.integer = 2;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMXR,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, status, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

static void prv_data_simple(subtilis_ir_section_t *s,
			    subtilis_arm_instr_type_t itype, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	subtilis_arm_reg_t op1 =
	    subtilis_arm_ir_to_dreg(instr->operands[1].reg);
	subtilis_arm_reg_t op2 =
	    subtilis_arm_ir_to_dreg(instr->operands[2].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_data(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest, op1,
			      op2, err);
}

static void prv_data_imm(subtilis_ir_section_t *s,
			 subtilis_arm_instr_type_t itype, size_t start,
			 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t op2 = subtilis_arm_ir_to_dreg(s->freg_counter++);
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	subtilis_arm_reg_t op1 =
	    subtilis_arm_ir_to_dreg(instr->operands[1].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_copy_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op2,
				  instr->operands[2].real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest, op1,
			      op2, err);
}

static void prv_data_imm_reversed(subtilis_ir_section_t *s,
				  subtilis_arm_instr_type_t itype, size_t start,
				  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t op2 = subtilis_arm_ir_to_dreg(s->freg_counter++);
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	subtilis_arm_reg_t op1 =
	    subtilis_arm_ir_to_dreg(instr->operands[1].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_copy_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op2,
				  instr->operands[2].real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(arm_s, itype, SUBTILIS_ARM_CCODE_AL, dest, op2,
			      op1, err);
}

void subtilis_vfp_gen_addr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, SUBTILIS_VFP_INSTR_FADDD, start, user_data, err);
}

void subtilis_vfp_gen_addir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, SUBTILIS_VFP_INSTR_FADDD, start, user_data, err);
}

void subtilis_vfp_gen_subr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, SUBTILIS_VFP_INSTR_FSUBD, start, user_data, err);
}

void subtilis_vfp_gen_subir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, SUBTILIS_VFP_INSTR_FSUBD, start, user_data, err);
}

void subtilis_vfp_gen_rsubir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_imm_reversed(s, SUBTILIS_VFP_INSTR_FSUBD, start, user_data,
			      err);
}

void subtilis_vfp_gen_fma_right(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *mul = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *add = &s->ops[start + 1]->op.instr;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_dreg(add->operands[0].reg);
	subtilis_arm_reg_t op1 = subtilis_arm_ir_to_dreg(mul->operands[1].reg);
	subtilis_arm_reg_t op2 = subtilis_arm_ir_to_dreg(mul->operands[2].reg);
	subtilis_arm_reg_t op3 = subtilis_arm_ir_to_dreg(add->operands[1].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, op3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(arm_s, SUBTILIS_VFP_INSTR_FMACD,
			      SUBTILIS_ARM_CCODE_AL, dest, op1, op2, err);
}

void subtilis_vfp_gen_fma_left(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *mul = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *add = &s->ops[start + 1]->op.instr;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_dreg(add->operands[0].reg);
	subtilis_arm_reg_t op1 = subtilis_arm_ir_to_dreg(mul->operands[1].reg);
	subtilis_arm_reg_t op2 = subtilis_arm_ir_to_dreg(mul->operands[2].reg);
	subtilis_arm_reg_t op3 = subtilis_arm_ir_to_dreg(add->operands[2].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, op3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(arm_s, SUBTILIS_VFP_INSTR_FMACD,
			      SUBTILIS_ARM_CCODE_AL, dest, op1, op2, err);
}

void subtilis_vfp_gen_nfma_right(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *mul = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *add = &s->ops[start + 1]->op.instr;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_dreg(add->operands[0].reg);
	subtilis_arm_reg_t op1 = subtilis_arm_ir_to_dreg(mul->operands[1].reg);
	subtilis_arm_reg_t op2 = subtilis_arm_ir_to_dreg(mul->operands[2].reg);
	subtilis_arm_reg_t op3 = subtilis_arm_ir_to_dreg(add->operands[1].reg);
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, op3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(arm_s, SUBTILIS_VFP_INSTR_FNMACD,
			      SUBTILIS_ARM_CCODE_AL, dest, op1, op2, err);
}

void subtilis_vfp_gen_mulr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_data_simple(s, SUBTILIS_VFP_INSTR_FMULD, start, user_data, err);
}

void subtilis_vfp_gen_mulir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, SUBTILIS_VFP_INSTR_FMULD, start, user_data, err);
}

void subtilis_vfp_gen_divr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_simple(s, SUBTILIS_VFP_INSTR_FDIVD, start, user_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, dest, SUBTILIS_ERROR_CODE_DIV_BY_ZERO,
			    err);
}

void subtilis_vfp_gen_divir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_imm(s, SUBTILIS_VFP_INSTR_FDIVD, start, user_data, err);
}

void subtilis_vfp_gen_rdivir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	subtilis_arm_section_t *arm_s = user_data;

	prv_data_imm_reversed(s, SUBTILIS_VFP_INSTR_FDIVD, start, user_data,
			      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_check_divbyzero(arm_s, s, dest, SUBTILIS_ERROR_CODE_DIV_BY_ZERO,
			    err);
}

void subtilis_vfp_gen_storeor(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_VFP_INSTR_FSTD, s, start, user_data, err);
}

void subtilis_vfp_gen_loador(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_VFP_INSTR_FLDD, s, start, user_data, err);
}

void subtilis_vfp_gen_retr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest = 0;
	op2 = subtilis_arm_ir_to_dreg(instr->operands[0].reg);

	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_gen_ret(s, start, user_data, err);
}

void subtilis_vfp_gen_retir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_cmp_jmp_imm(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	op1 = subtilis_arm_ir_to_dreg(cmp->operands[1].reg);

	if (cmp->operands[2].real == 0.0) {
		subtilis_vfp_add_cmpz(arm_s, SUBTILIS_VFP_INSTR_FCMPZD,
				      SUBTILIS_ARM_CCODE_AL, op1, err);
	} else {
		op2 = subtilis_arm_ir_to_dreg(s->freg_counter++);
		subtilis_vfp_add_copy_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op2,
					  cmp->operands[2].real, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmp(arm_s, SUBTILIS_VFP_INSTR_FCMPD,
				     SUBTILIS_ARM_CCODE_AL, op1, op2, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMRX,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, 15, err);
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

void subtilis_vfp_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_vfp_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_vfp_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_vfp_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_vfp_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LS, err);
}

void subtilis_vfp_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_MI, err);
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

	op1 = subtilis_arm_ir_to_dreg(ir_op->operands[1].reg);
	op2 = subtilis_arm_ir_to_dreg(ir_op->operands[2].reg);

	subtilis_vfp_add_cmp(arm_s, itype, SUBTILIS_ARM_CCODE_AL, op1, op2,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMRX,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, 15, err);
}

static void prv_cmp_jmp(subtilis_ir_section_t *s, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ccode, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_VFP_INSTR_FCMPD,
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

void subtilis_vfp_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_vfp_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_vfp_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_vfp_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_vfp_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_LS, err);
}

void subtilis_vfp_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_ARM_CCODE_MI, err);
}

static void prv_cmp_imm(subtilis_ir_section_t *s, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ok,
			subtilis_arm_ccode_type_t nok, subtilis_error_t *err)
{
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_dreg(cmp->operands[1].reg);

	if (cmp->operands[2].real == 0.0) {
		subtilis_vfp_add_cmpz(arm_s, SUBTILIS_VFP_INSTR_FCMPZD,
				      SUBTILIS_ARM_CCODE_AL, op1, err);
	} else {
		op2 = subtilis_arm_ir_to_dreg(s->freg_counter++);
		subtilis_vfp_add_copy_imm(arm_s, SUBTILIS_ARM_CCODE_AL, op2,
					  cmp->operands[2].real, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmp(arm_s, SUBTILIS_VFP_INSTR_FCMPD,
				     SUBTILIS_ARM_CCODE_AL, op1, op2, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMRX,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, 15, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_vfp_gen_gtir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		    SUBTILIS_ARM_CCODE_LS, err);
}

void subtilis_vfp_gen_ltir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_MI,
		    SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_vfp_gen_eqir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		    SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_vfp_gen_neqir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		    SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_vfp_gen_gteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		    SUBTILIS_ARM_CCODE_MI, err);
}

void subtilis_vfp_gen_lteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(s, start, user_data, SUBTILIS_ARM_CCODE_LS,
		    SUBTILIS_ARM_CCODE_GT, err);
}

static void prv_cmp(subtilis_ir_section_t *s, size_t start, void *user_data,
		    subtilis_arm_ccode_type_t ok, subtilis_arm_ccode_type_t nok,
		    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;

	prv_cmp_simple(s, start, user_data, SUBTILIS_VFP_INSTR_FCMPD,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_s, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_s, nok, false, dest, 0, err);
}

void subtilis_vfp_gen_gtr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GT,
		SUBTILIS_ARM_CCODE_LS, err);
}

void subtilis_vfp_gen_ltr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_MI,
		SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_vfp_gen_gter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_GE,
		SUBTILIS_ARM_CCODE_MI, err);
}

void subtilis_vfp_gen_lter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_LS,
		SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_vfp_gen_eqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_vfp_gen_neqr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	prv_cmp(s, start, user_data, SUBTILIS_ARM_CCODE_NE,
		SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_vfp_gen_sin(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_cos(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_tan(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_asn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_acs(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_atn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_sqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_dreg(instr->operands[1].reg);

	subtilis_vfp_add_sqrt(arm_s, SUBTILIS_VFP_INSTR_FSQRTD,
			      SUBTILIS_ARM_CCODE_AL, dest, src, err);
}

void subtilis_vfp_gen_log(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_ln(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_absr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(instr->operands[0].reg);
	src = subtilis_arm_ir_to_dreg(instr->operands[1].reg);

	subtilis_vfp_add_tran(arm_s, SUBTILIS_VFP_INSTR_FABSD,
			      SUBTILIS_ARM_CCODE_AL, true, dest, src, err);
}

void subtilis_vfp_gen_pow(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_exp(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_preamble(subtilis_arm_section_t *arm_s,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t status;

	status = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, status, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(arm_s, SUBTILIS_VFP_INSTR_FMXR,
				SUBTILIS_ARM_CCODE_AL,
				SUBTILIS_VFP_SYSREG_FPSCR, status, err);
}

void subtilis_vfp_gen_movi8tofp(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	subtilis_arm_reg_t tmp;
	subtilis_arm_reg_t tmp1;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *signx = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_dreg(signx->operands[0].reg);
	src = subtilis_arm_ir_to_arm_reg(signx->operands[1].reg);
	tmp = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	tmp1 = subtilis_arm_ir_to_dreg(arm_s->freg_counter++);

	subtilis_arm_add_signx(arm_s, SUBTILIS_ARM_INSTR_SXTB,
			       SUBTILIS_ARM_CCODE_AL, tmp, src,
			       SUBTILIS_ARM_SIGNX_ROR_NONE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_cptran(arm_s, SUBTILIS_VFP_INSTR_FMSR,
				SUBTILIS_ARM_CCODE_AL, true, tmp1, tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_tran(arm_s, SUBTILIS_VFP_INSTR_FSITOD,
			      SUBTILIS_ARM_CCODE_AL, true, dest, tmp1, err);
}

void subtilis_vfp_gen_movfptoi32i32(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest1;
	subtilis_arm_reg_t dest2;
	subtilis_arm_reg_t src;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *mov = &s->ops[start]->op.instr;

	dest1 = subtilis_arm_ir_to_arm_reg(mov->operands[0].reg);
	dest2 = subtilis_arm_ir_to_arm_reg(mov->operands[1].reg);
	src = subtilis_arm_ir_to_dreg(mov->operands[2].reg);

	subtilis_vfp_add_tran_dbl(arm_s, SUBTILIS_VFP_INSTR_FMRRD,
				  SUBTILIS_ARM_CCODE_AL, dest1, dest2, src, 0,
				  err);
}

size_t subtilis_vfp_preserve_regs(subtilis_arm_section_t *arm_s,
				  int save_real_start, subtilis_error_t *err)
{
	int i;
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *stran;
	size_t stf_site = INT_MAX;

	for (i = save_real_start; i < SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS; i++) {
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_VFP_INSTR_FSTD, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return INT_MAX;

		stran = &instr->operands.vfp_stran;
		stran->ccode = SUBTILIS_ARM_CCODE_NV;
		stran->dest = i;
		stran->base = 13;
		stran->offset = 2;
		stran->pre_indexed = true;
		stran->write_back = true;
		stran->subtract = true;
		if (stf_site == INT_MAX)
			stf_site = arm_s->last_op;
	}

	return stf_site;
}

size_t subtilis_vfp_restore_regs(subtilis_arm_section_t *arm_s,
				 int save_real_start, subtilis_error_t *err)
{
	int i;
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *stran;
	size_t ldf_site = INT_MAX;

	for (i = SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS - 1; i >= save_real_start;
	     i--) {
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_VFP_INSTR_FLDD, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return INT_MAX;
		stran = &instr->operands.vfp_stran;
		stran->ccode = SUBTILIS_ARM_CCODE_NV;
		stran->dest = i;
		stran->base = 13;
		stran->offset = 2;
		stran->pre_indexed = false;
		stran->write_back = true;
		stran->subtract = false;

		if (ldf_site == INT_MAX)
			ldf_site = arm_s->last_op;
	}

	return ldf_site;
}

void subtilis_vfp_preserve_update(subtilis_arm_section_t *arm_s,
				  subtilis_arm_call_site_t *call_site,
				  size_t real_regs_saved, size_t real_regs_used,
				  subtilis_error_t *err)
{
	subtilis_arm_op_t *op;
	size_t i;
	size_t vfp_reg_count;

	if (real_regs_saved > SUBTILIS_ARM_REG_MAX_ARGS)
		real_regs_saved = SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS -
				  SUBTILIS_ARM_REG_MAX_ARGS;
	else
		real_regs_saved =
		    SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS - real_regs_saved;

	vfp_reg_count = SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS - real_regs_saved;
	op = &arm_s->op_pool->ops[call_site->stf_site];

	for (; vfp_reg_count < SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS;
	     vfp_reg_count++) {
		if (real_regs_used & (1 << vfp_reg_count)) {
			op->op.instr.operands.vfp_stran.ccode =
			    SUBTILIS_ARM_CCODE_AL;
		}
		op = &arm_s->op_pool->ops[op->next];
	}

	vfp_reg_count = SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS - 1;
	op = &arm_s->op_pool->ops[call_site->ldf_site];
	for (i = 0; i < real_regs_saved; i++) {
		if (real_regs_used & (1 << vfp_reg_count))
			op->op.instr.operands.vfp_stran.ccode =
			    SUBTILIS_ARM_CCODE_AL;
		op = &arm_s->op_pool->ops[op->next];
		vfp_reg_count--;
	}
}

void subtilis_vfp_update_offsets(subtilis_arm_section_t *arm_s,
				 subtilis_arm_call_site_t *call_site,
				 size_t bytes_saved, subtilis_error_t *err)
{
	size_t i;
	size_t ptr;
	subtilis_arm_op_t *op;
	subtilis_vfp_stran_instr_t *ft;

	for (i = 0; i < call_site->real_args - 4; i++) {
		ptr = call_site->real_arg_ops[i];
		op = &arm_s->op_pool->ops[ptr];
		ft = &op->op.instr.operands.vfp_stran;
		ft->offset += bytes_saved / 4;
	}
}

void subtilis_vfp_store_double(subtilis_arm_section_t *arm_s,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			       size_t offset, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *fstran;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_VFP_INSTR_FSTD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	fstran = &instr->operands.vfp_stran;
	fstran->ccode = SUBTILIS_ARM_CCODE_AL;
	fstran->dest = dest;
	fstran->base = base;
	fstran->offset = offset / 4;
	fstran->pre_indexed = true;
	fstran->write_back = false;
	fstran->subtract = true;
}

void subtilis_vfp_mov_reg(subtilis_arm_section_t *arm_s,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			  subtilis_error_t *err)
{
	subtilis_vfp_add_copy(arm_s, SUBTILIS_ARM_CCODE_AL,
			      SUBTILIS_VFP_INSTR_FCPYD, dest, src, err);
}

bool subtilis_vfp_is_fixed(size_t reg)
{
	return reg < SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS;
}

void subtilis_arm_vfp_if_init(subtilis_arm_fp_if_t *fp_if)
{
	fp_if->max_regs =
	    SUBTILIS_ARM_REG_MAX_VFP_DBL_REGS; /* TODO: needs constant */

	fp_if->max_offset = 1023;
	fp_if->store_type = SUBTILIS_VFP_INSTR_FSTD;
	fp_if->load_type = SUBTILIS_VFP_INSTR_FLDD;
	fp_if->reverse_fpa_consts = false;
	fp_if->preamble_fn = subtilis_vfp_gen_preamble;
	fp_if->preserve_regs_fn = subtilis_vfp_preserve_regs;
	fp_if->restore_regs_fn = subtilis_vfp_restore_regs;
	fp_if->update_regs_fn = subtilis_vfp_preserve_update;
	fp_if->update_offs_fn = subtilis_vfp_update_offsets;
	fp_if->store_dbl_fn = subtilis_vfp_store_double;
	fp_if->mov_reg_fn = subtilis_vfp_mov_reg;
	fp_if->spill_imm_fn = subtilis_vfp_insert_stran_spill_imm;
	fp_if->stran_imm_fn = subtilis_vfp_insert_stran_imm;
	fp_if->is_fixed_fn = subtilis_vfp_is_fixed;
	fp_if->init_dist_walker_fn = subtilis_init_vfp_dist_walker;
	fp_if->init_used_walker_fn = subtilis_init_vfp_used_walker;
	fp_if->init_real_alloc_fn = subtilis_vfp_alloc_init_walker;
}
