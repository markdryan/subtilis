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

#include "arm_core.h"

void subtilis_vfp_add_copy(subtilis_arm_section_t *s,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_copy_instr_t *copy;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	copy = &instr->operands.vfp_copy;
	copy->ccode = ccode;
	copy->dest = dest;
	copy->src = src;
}

static size_t prv_add_real_ldr(subtilis_arm_section_t *s,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_arm_reg_t dest, double op2,
			       subtilis_error_t *err)
{
	subtilis_fpa_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label = subtilis_arm_get_real_label(s, op2, err);

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_VFP_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ldrc = &instr->operands.vfp_ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
	return label;
}

void subtilis_vfp_add_copy_imm(subtilis_arm_section_t *s,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_arm_reg_t dest, double src,
			       subtilis_error_t *err)
{
	/*
	 * TODO: If the immediate has no fractional part and it can be
	 * directly encoded, it might be better to mov it into an
	 * integer register and translate.  It's the same number of
	 * instructions and fewer bytes.
	 */

	(void)prv_add_real_ldr(s, ccode, dest, src, err);
}

void subtilis_vfp_add_stran(subtilis_arm_section_t *s,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			    int32_t offset, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *stran;
	subtilis_arm_reg_t mov_dest;
	bool subtract = false;

	if (offset > 1023 || offset < -1023) {
		mov_dest = subtilis_arm_acquire_new_reg(s);
		subtilis_arm_add_data_imm(s, SUBTILIS_ARM_INSTR_ADD, ccode,
					  false, mov_dest, base, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		offset = 0;
		base = mov_dest;
	} else {
		if (offset < 0) {
			offset -= offset;
			subtract = true;
		}
		offset /= 4;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.vfp_stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = (uint8_t)offset;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = subtract;
}

/* clang-format off */
void subtilis_vfp_insert_stran_spill_imm(subtilis_arm_section_t *s,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *stran;
	subtilis_arm_data_instr_t *datai;

	(void)subtilis_arm_insert_data_imm_ldr(s, current, ccode, spill_reg,
					       offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_section_insert_instr(s, current,
						  SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->status = false;
	datai->dest = spill_reg;
	datai->op1 = spill_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = base;

	instr = subtilis_arm_section_insert_instr(s, current, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.vfp_stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = spill_reg;
	stran->offset = 0;
	stran->pre_indexed = false;
	stran->write_back = false;
	stran->subtract = false;
}

void subtilis_vfp_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_stran_instr_t *stran;
	bool subtract = false;

	if (offset < 0) {
		offset = -offset;
		subtract = true;
	}

	if (offset > 1023) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	instr = subtilis_arm_section_insert_instr(s, current, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.vfp_stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = (uint8_t)((uint32_t)offset / 4);
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = subtract;
}

void subtilis_vfp_add_tran(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode, bool use_dregs,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_tran_instr_t *tran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran = &instr->operands.vfp_tran;
	tran->ccode = ccode;
	tran->use_dregs = use_dregs;
	tran->dest = dest;
	tran->src = src;
}

void subtilis_vfp_add_cptran(subtilis_arm_section_t *s,
			     subtilis_arm_instr_type_t itype,
			     subtilis_arm_ccode_type_t ccode, bool use_dregs,
			     subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			     subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_cptran_instr_t *tran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran = &instr->operands.vfp_cptran;
	tran->ccode = ccode;
	tran->use_dregs = use_dregs;
	tran->dest = dest;
	tran->src = src;
}

void subtilis_vfp_add_data(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			   subtilis_arm_reg_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_data_instr_t *data;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	data = &instr->operands.vfp_data;
	data->ccode = ccode;
	data->dest = dest;
	data->op1 = op1;
	data->op2 = op2;
}

void subtilis_vfp_add_cmp(subtilis_arm_section_t *s,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t op1, subtilis_arm_reg_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_cmp_instr_t *cmp;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cmp = &instr->operands.vfp_cmp;
	cmp->ccode = ccode;
	cmp->op1 = op1;
	cmp->op2 = op2;
}

void subtilis_vfp_add_cmpz(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_reg_t op1, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_cmp_instr_t *cmp;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cmp = &instr->operands.vfp_cmp;
	cmp->ccode = ccode;
	cmp->op1 = op1;
}

void subtilis_vfp_add_sqrt(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_sqrt_instr_t *sqrt;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sqrt = &instr->operands.vfp_sqrt;
	sqrt->ccode = ccode;
	sqrt->dest = dest;
	sqrt->op1 = op1;
}

void subtilis_vfp_add_sysreg(subtilis_arm_section_t *s,
			     subtilis_arm_instr_type_t itype,
			     subtilis_arm_ccode_type_t ccode,
			     subtilis_vfp_sysreg_t sysreg,
			     subtilis_arm_reg_t reg, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_sysreg_instr_t *sysregi;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sysregi = &instr->operands.vfp_sysreg;
	sysregi->ccode = ccode;
	sysregi->sysreg = sysreg;
	sysregi->arm_reg = reg;
}

void subtilis_vfp_add_tran_dbl(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_arm_reg_t dest1,
			       subtilis_arm_reg_t dest2,
			       subtilis_arm_reg_t src1, subtilis_arm_reg_t src2,
			       subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_vfp_tran_dbl_instr_t *tran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran = &instr->operands.vfp_tran_dbl;
	tran->ccode = ccode;
	tran->dest1 = dest1;
	tran->dest2 = dest2;
	tran->src1 = src1;
	tran->src2 = src2;
}
