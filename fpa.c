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

#include <stdlib.h>

#include "arm_core.h"

static void prv_add_real_constant(subtilis_arm_section_t *s, size_t label,
				  double num, subtilis_error_t *err)
{
	subtilis_arm_real_constant_t *c;
	subtilis_arm_real_constant_t *new_constants;
	size_t new_max;

	if (s->constants.real_count == s->constants.max_real) {
		new_max = s->constants.max_real + 64;
		new_constants =
		    realloc(s->constants.real,
			    new_max * sizeof(subtilis_arm_real_constant_t));
		if (!new_constants) {
			subtilis_error_set_oom(err);
			return;
		}
		s->constants.max_real = new_max;
		s->constants.real = new_constants;
	}
	c = &s->constants.real[s->constants.real_count++];
	c->real = num;
	c->label = label;
}

static size_t prv_add_real_ldr(subtilis_arm_section_t *s,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_arm_reg_t dest, double op2,
			       subtilis_error_t *err)
{
	subtilis_fpa_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label = s->label_counter++;

	prv_add_real_constant(s, label, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_FPA_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ldrc = &instr->operands.fpa_ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
	ldrc->size = 8;
	return label;
}

typedef struct subtilis_fpa_imm_enc_t_ subtilis_fpa_imm_enc_t;

struct subtilis_fpa_imm_enc_t_ {
	const double real;
	uint8_t encoded;
};

bool subtilis_fpa_encode_real(double real, uint8_t *encoded)
{
	/* clang-format off */

	subtilis_fpa_imm_enc_t encodings[] = {
		{0.0, 0x8}, {1.0, 0x9}, {2.0, 0xA}, {3.0, 0xB}, {4.0, 0xC},
		{5.0, 0xD}, {0.5, 0xE}, {10.0, 0xF}
	};

	/* clang-format on */

	size_t i;

	for (i = 0; i < sizeof(encodings) / sizeof(subtilis_fpa_imm_enc_t); i++)
		if (encodings[i].real == real)
			break;

	if (i == sizeof(encodings) / sizeof(subtilis_fpa_imm_enc_t))
		return false;

	*encoded = encodings[i].encoded;
	return true;
}

void subtilis_fpa_add_mvfmnf_imm(subtilis_arm_section_t *s,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_fpa_rounding_t rounding,
				 subtilis_arm_reg_t dest, double op2,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_data_instr_t *datai;
	double to_encode;
	subtilis_arm_instr_type_t itype;
	uint8_t encoded = 0;

	if (op2 < 0.0) {
		to_encode = -op2;
		itype = alt_type;
	} else {
		to_encode = op2;
		itype = type;
	}

	if (!subtilis_fpa_encode_real(to_encode, &encoded)) {
		(void)prv_add_real_ldr(s, ccode, dest, op2, err);
		return;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.fpa_data;
	datai->ccode = ccode;
	datai->rounding = rounding;
	datai->size = 8;
	datai->dest = dest;
	datai->immediate = true;
	datai->op2.imm = encoded;
}

void subtilis_fpa_add_mvfmnf(subtilis_arm_section_t *s,
			     subtilis_arm_ccode_type_t ccode,
			     subtilis_arm_instr_type_t type,
			     subtilis_fpa_rounding_t rounding,
			     subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			     subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_data_instr_t *datai;

	instr = subtilis_arm_section_add_instr(s, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.fpa_data;
	datai->ccode = ccode;
	datai->rounding = rounding;
	datai->size = 8;
	datai->dest = dest;
	datai->immediate = false;
	datai->op2.reg = op2;
}

void subtilis_fpa_add_data_imm(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_fpa_rounding_t rounding,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       double op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_data_instr_t *datai;
	uint8_t encoded;
	subtilis_arm_reg_t mov_dest;
	subtilis_fpa_op2_t data_opt2;
	bool immediate;

	immediate = subtilis_fpa_encode_real(op2, &encoded);
	if (!immediate) {
		mov_dest = subtilis_arm_acquire_new_freg(s);
		subtilis_fpa_add_mov_imm(s, ccode, rounding, mov_dest, op2,
					 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		data_opt2.reg = mov_dest;
	} else {
		data_opt2.imm = encoded;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.fpa_data;
	datai->ccode = ccode;
	datai->rounding = rounding;
	datai->size = 8;
	datai->dest = dest;
	datai->op1 = op1;
	datai->immediate = immediate;
	datai->op2 = data_opt2;
}

void subtilis_fpa_add_data(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_fpa_rounding_t rounding,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			   subtilis_arm_reg_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_data_instr_t *datai;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.fpa_data;
	datai->ccode = ccode;
	datai->rounding = rounding;
	datai->size = 8;
	datai->dest = dest;
	datai->op1 = op1;
	datai->immediate = false;
	datai->op2.reg = op2;
}

void subtilis_fpa_add_stran(subtilis_arm_section_t *s,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			    int32_t offset, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_stran_instr_t *stran;
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

	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = 8;
	stran->dest = dest;
	stran->base = base;
	stran->offset = (uint8_t)offset;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = subtract;
}

void subtilis_fpa_add_tran(subtilis_arm_section_t *s,
			   subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_fpa_rounding_t rounding,
			   subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_tran_instr_t *tran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran = &instr->operands.fpa_tran;
	tran->ccode = ccode;
	tran->rounding = rounding;
	tran->size = 8;
	tran->dest = dest;
	tran->op2.reg = op2;
	tran->immediate = false;
}

void subtilis_fpa_add_cmfcnf_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, double op2,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_cmp_instr_t *cmp;
	double to_encode;
	bool immediate;
	subtilis_arm_instr_type_t itype;
	subtilis_arm_reg_t mov_dest;
	subtilis_fpa_op2_t data_opt2;
	uint8_t encoded = 0;

	if (op2 < 0.0) {
		to_encode = -op2;
		itype = alt_type;
	} else {
		to_encode = op2;
		itype = type;
	}

	immediate = subtilis_fpa_encode_real(to_encode, &encoded);
	if (!immediate) {
		itype = type;
		mov_dest = subtilis_arm_acquire_new_freg(s);
		subtilis_fpa_add_mov_imm(s, ccode,
					 SUBTILIS_FPA_ROUNDING_NEAREST,
					 mov_dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		data_opt2.reg = mov_dest;
	} else {
		data_opt2.imm = encoded;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cmp = &instr->operands.fpa_cmp;
	cmp->ccode = ccode;
	cmp->dest = dest;
	cmp->immediate = immediate;
	cmp->op2 = data_opt2;
}

void subtilis_fpa_add_cmp(subtilis_arm_section_t *s,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_cmp_instr_t *tran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran = &instr->operands.fpa_cmp;
	tran->ccode = ccode;
	tran->dest = dest;
	tran->op2.reg = op2;
	tran->immediate = false;
}

/* clang-format off */
void subtilis_fpa_insert_stran_spill_imm(subtilis_arm_section_t *s,
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
	subtilis_fpa_stran_instr_t *stran;
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
	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = 8;
	stran->dest = dest;
	stran->base = spill_reg;
	stran->offset = 0;
	stran->pre_indexed = false;
	stran->write_back = false;
	stran->subtract = false;
}

void subtilis_fpa_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_stran_instr_t *stran;
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
	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = 8;
	stran->dest = dest;
	stran->base = base;
	stran->offset = (uint8_t)((uint32_t)offset / 4);
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = subtract;
}

void subtilis_fpa_push_reg(subtilis_arm_section_t *s,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_arm_reg_t dest, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_stran_instr_t *stran;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_FPA_INSTR_STF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = 8;
	stran->dest = dest;
	stran->base.type = SUBTILIS_ARM_REG_FIXED;
	stran->base.num = 13;
	stran->offset = 2;
	stran->pre_indexed = true;
	stran->write_back = true;
	stran->subtract = true;
}

void subtilis_fpa_pop_reg(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t dest, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_fpa_stran_instr_t *stran;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_FPA_INSTR_LDF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = 8;
	stran->dest = dest;
	stran->base.type = SUBTILIS_ARM_REG_FIXED;
	stran->base.num = 13;
	stran->offset = 2;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
}
