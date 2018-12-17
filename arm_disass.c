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

#include "arm_disass.h"

static void prv_decode_mul(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_mul_instr_t *mul = &instr->operands.mul;

	if (encoded & (1 << 21)) {
		instr->type = SUBTILIS_ARM_INSTR_MLA;
		mul->rn.num = (encoded >> 15) & 0x0f;
	} else {
		instr->type = SUBTILIS_ARM_INSTR_MUL;
	}

	mul->ccode = (encoded >> 28);
	mul->status = ((encoded >> 20) & 1);

	mul->dest.num = (encoded >> 16) & 0x0f;

	mul->rm.num = (encoded >> 8) & 0x0f;
	mul->rs.num = encoded & 0x0f;
}

static void prv_decode_swi(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_swi_instr_t *swi = &instr->operands.swi;

	instr->type = SUBTILIS_ARM_INSTR_SWI;
	swi->ccode = (encoded >> 28);
	swi->code = encoded & 0xffffff;
}

static void prv_decode_branch(subtilis_arm_instr_t *instr, uint32_t encoded,
			      subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br = &instr->operands.br;

	instr->type = SUBTILIS_ARM_INSTR_B;
	br->link = (encoded & (1 << 24));
	br->ccode = (encoded >> 28);
	br->target.offset = encoded & 0xffffff;
	if (encoded & 0x800000)
		br->target.offset = encoded | 0xff000000;
}

static void prv_decode_mtran(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_mtran_instr_t *mtran = &instr->operands.mtran;

	/* Status bit not yet supported. */
	if (encoded & (1 << 22)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_ARM_INSTR_LDM;
	else
		instr->type = SUBTILIS_ARM_INSTR_STM;

	mtran->ccode = (encoded >> 28);

	mtran->op0.num = (encoded >> 16) & 0x0f;
	mtran->reg_list = encoded & 0xffff;
	mtran->write_back = (encoded & (1 << 21)) != 0;

	if (encoded & (1 << 23)) {
		if (encoded & (1 << 24))
			mtran->type = SUBTILIS_ARM_MTRAN_IB;
		else
			mtran->type = SUBTILIS_ARM_MTRAN_IA;
	} else {
		if (encoded & (1 << 24))
			mtran->type = SUBTILIS_ARM_MTRAN_DB;
		else
			mtran->type = SUBTILIS_ARM_MTRAN_DA;
	}
}

static void prv_decode_op2(uint32_t encoded, subtilis_arm_op2_t *op2)
{
	const subtilis_arm_shift_type_t shtype[] = {
	    SUBTILIS_ARM_SHIFT_LSL, SUBTILIS_ARM_SHIFT_LSR,
	    SUBTILIS_ARM_SHIFT_ASR, SUBTILIS_ARM_SHIFT_ROR,
	};

	if (encoded & (1 << 25)) {
		op2->type = SUBTILIS_ARM_OP2_I32;
		op2->op.integer = encoded & 0xfff;
	} else if (encoded & 0xff0) {
		op2->type = SUBTILIS_ARM_OP2_SHIFTED;
		op2->op.shift.reg.num = encoded & 0xf;
		op2->op.shift.shift_reg = (encoded & (1 << 4)) != 0;
		if (op2->op.shift.shift_reg) {
			op2->op.shift.shift.reg.num = (encoded >> 8) & 0xf;
		} else {
			op2->op.shift.shift.integer = (encoded >> 7) & 0x1f;
			if (op2->op.shift.shift.integer == 0)
				op2->op.shift.shift.integer = 32;
		}
		op2->op.shift.type = shtype[((encoded >> 5) & 3)];
	} else {
		op2->type = SUBTILIS_ARM_OP2_REG;
		op2->op.reg.num = encoded & 0xf;
	}
}

static void prv_decode_stran_op2(uint32_t encoded, subtilis_arm_op2_t *op2)
{
	const subtilis_arm_shift_type_t shtype[] = {
	    SUBTILIS_ARM_SHIFT_LSL, SUBTILIS_ARM_SHIFT_LSR,
	    SUBTILIS_ARM_SHIFT_ASR, SUBTILIS_ARM_SHIFT_ROR,
	};

	if (encoded & (1 << 25)) {
		if (encoded & 0xff0) {
			op2->type = SUBTILIS_ARM_OP2_SHIFTED;
			op2->op.shift.reg.num = encoded & 0xf;
			op2->op.shift.shift.integer = (encoded >> 7) & 0x1f;
			if (op2->op.shift.shift.integer == 0)
				op2->op.shift.shift.integer = 32;
			op2->op.shift.type = shtype[((encoded >> 5) & 3)];
		} else {
			op2->type = SUBTILIS_ARM_OP2_REG;
			op2->op.reg.num = encoded & 0xf;
		}
	} else {
		op2->type = SUBTILIS_ARM_OP2_I32;
		op2->op.integer = encoded & 0xfff;
	}
}

static void prv_decode_stran(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_stran_instr_t *stran = &instr->operands.stran;

	/* Byte access not yet supported. */
	if (encoded & (1 << 22)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_ARM_INSTR_LDR;
	else
		instr->type = SUBTILIS_ARM_INSTR_STR;

	stran->ccode = (encoded >> 28);
	stran->pre_indexed = (encoded & (1 << 24)) != 0;
	stran->write_back = (encoded & (1 << 21)) != 0;
	stran->subtract = (encoded & (1 << 23)) == 0;
	stran->dest.num = (encoded >> 12) & 0x0f;
	stran->base.num = (encoded >> 16) & 0x0f;
	prv_decode_stran_op2(encoded, &stran->offset);
}

static void prv_decode_datai(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_data_instr_t *datai = &instr->operands.data;

	instr->type = (subtilis_arm_instr_type_t)(encoded >> 21) & 0xf;
	datai->ccode = (encoded >> 28);
	datai->status = ((encoded >> 20) & 1);

	datai->dest.num = (encoded >> 12) & 0x0f;

	datai->op1.num = (encoded >> 16) & 0x0f;
	prv_decode_op2(encoded, &datai->op2);
}

static void prv_decode_fpa_stran(subtilis_arm_instr_t *instr, uint32_t encoded,
				 subtilis_error_t *err)
{
	uint32_t scratch;
	subtilis_fpa_stran_instr_t *stran = &instr->operands.fpa_stran;

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_FPA_INSTR_LDF;
	else
		instr->type = SUBTILIS_FPA_INSTR_STF;

	scratch = ((encoded >> 21) & 2) | ((encoded >> 15) & 1);
	if (scratch == 3) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	stran->size = 4 << scratch;

	stran->ccode = (encoded >> 28);
	stran->pre_indexed = (encoded & (1 << 24)) != 0;
	stran->write_back = (encoded & (1 << 21)) != 0;
	stran->subtract = (encoded & (1 << 23)) == 0;
	stran->dest.num = (encoded >> 12) & 0x7;
	stran->base.num = (encoded >> 16) & 0x0f;
	stran->offset = encoded & 0xff;
}

/* clang-format off */
static subtilis_arm_instr_type_t prv_fpa_data_opcodes[] = {
	SUBTILIS_FPA_INSTR_ADF,
	SUBTILIS_FPA_INSTR_MVF,
	SUBTILIS_FPA_INSTR_MUF,
	SUBTILIS_FPA_INSTR_MNF,
	SUBTILIS_FPA_INSTR_SUF,
	SUBTILIS_FPA_INSTR_ABS,
	SUBTILIS_FPA_INSTR_RSF,
	SUBTILIS_FPA_INSTR_RND,
	SUBTILIS_FPA_INSTR_DVF,
	SUBTILIS_FPA_INSTR_SQT,
	SUBTILIS_FPA_INSTR_RDF,
	SUBTILIS_FPA_INSTR_LOG,
	SUBTILIS_FPA_INSTR_POW,
	SUBTILIS_FPA_INSTR_LGN,
	SUBTILIS_FPA_INSTR_RPW,
	SUBTILIS_FPA_INSTR_EXP,
	SUBTILIS_FPA_INSTR_RMF,
	SUBTILIS_FPA_INSTR_SIN,
	SUBTILIS_FPA_INSTR_FML,
	SUBTILIS_FPA_INSTR_COS,
	SUBTILIS_FPA_INSTR_FDV,
	SUBTILIS_FPA_INSTR_TAN,
	SUBTILIS_FPA_INSTR_FRD,
	SUBTILIS_FPA_INSTR_ASN,
	SUBTILIS_FPA_INSTR_POL,
	SUBTILIS_FPA_INSTR_ACS,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_ATN,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_URD,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_NRM,
};

/* clang-format on */

static subtilis_fpa_op2_t prv_fpa_encode_op2(uint32_t encoded, bool *immediate)
{
	subtilis_fpa_op2_t op2;

	if (encoded & (1 << 3)) {
		*immediate = true;
		op2.imm = encoded & 0xf;
	} else {
		*immediate = false;
		op2.reg.num = encoded & 7;
	}

	return op2;
}

static void prv_decode_fpa_data(subtilis_arm_instr_t *instr, uint32_t encoded,
				subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	size_t scratch;
	subtilis_fpa_data_instr_t *data = &instr->operands.fpa_data;

	scratch = ((encoded >> 19) & 0x1E) | ((encoded >> 15) & 1);
	if (scratch >= sizeof(prv_fpa_data_opcodes) / sizeof(type)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	type = prv_fpa_data_opcodes[scratch];
	if (type == SUBTILIS_ARM_INSTR_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	data->ccode = (encoded >> 28);

	scratch = ((encoded >> 18) & 2) | ((encoded >> 7) & 1);
	if (scratch == 3) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	data->size = 4 << scratch;

	scratch = ((encoded >> 5) & 3);
	data->rounding = (subtilis_fpa_rounding_t)scratch;

	data->dest.num = (encoded >> 12) & 0x7;

	if (encoded & (1 << 15))
		data->op1.num = 6;
	else
		data->op1.num = (encoded >> 16) & 0x7;

	data->op2 = prv_fpa_encode_op2(encoded, &data->immediate);
}

static void prv_decode_fpa_cmp(subtilis_arm_instr_t *instr, uint32_t encoded,
			       subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	subtilis_fpa_cmp_instr_t *cmp = &instr->operands.fpa_cmp;

	switch ((encoded >> 21) & 0x7) {
	case 4:
		type = SUBTILIS_FPA_INSTR_CMF;
		break;
	case 5:
		type = SUBTILIS_FPA_INSTR_CNF;
		break;
	case 6:
		type = SUBTILIS_FPA_INSTR_CMFE;
		break;
	case 7:
		type = SUBTILIS_FPA_INSTR_CNFE;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	cmp->ccode = (encoded >> 28);

	cmp->dest.num = (encoded >> 16) & 0x7;
	cmp->op2 = prv_fpa_encode_op2(encoded, &cmp->immediate);
}

static void prv_decode_fpa_tran(subtilis_arm_instr_t *instr, uint32_t encoded,
				subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	size_t scratch;
	subtilis_fpa_tran_instr_t *tran = &instr->operands.fpa_tran;

	switch ((encoded >> 20) & 0xf) {
	case 0:
		type = SUBTILIS_FPA_INSTR_FLT;
		break;
	case 1:
		type = SUBTILIS_FPA_INSTR_FIX;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	tran->ccode = (encoded >> 28);
	scratch = ((encoded >> 5) & 3);
	tran->rounding = (subtilis_fpa_rounding_t)scratch;

	if (type == SUBTILIS_FPA_INSTR_FLT) {
		scratch = ((encoded >> 18) & 2) | ((encoded >> 7) & 1);
		if (scratch == 3) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		tran->size = 4 << scratch;
		tran->dest.num = (encoded >> 16) & 0x7;
		tran->immediate = false;
		tran->op2.reg.num = (encoded >> 12) & 0xf;
	} else {
		tran->dest.num = (encoded >> 12) & 0xf;
		tran->size = 0;
		tran->op2 = prv_fpa_encode_op2(encoded, &tran->immediate);
	}
}

void subtilis_arm_disass(subtilis_arm_instr_t *instr, uint32_t encoded,
			 subtilis_error_t *err)
{
	uint32_t mask;

	mask = encoded & (0x3f << 22);
	if ((mask == 0) && (((encoded >> 4) & 0xf) == 9)) {
		prv_decode_mul(instr, encoded, err);
		return;
	}

	mask = encoded & (0xf << 24);
	if (mask == 0x0f000000) {
		prv_decode_swi(instr, encoded, err);
		return;
	}

	mask = (encoded & (0x7 << 25)) >> 25;
	if (mask == 5) {
		prv_decode_branch(instr, encoded, err);
		return;
	}

	if (mask == 4) {
		prv_decode_mtran(instr, encoded, err);
		return;
	}

	mask = (encoded & (0x3 << 26)) >> 26;
	if (mask == 1) {
		prv_decode_stran(instr, encoded, err);
		return;
	}

	if (mask == 0) {
		prv_decode_datai(instr, encoded, err);
		return;
	}
	mask = (encoded >> 25) & 7;
	if (mask == 6) {
		prv_decode_fpa_stran(instr, encoded, err);
		return;
	}

	if (mask == 7) {
		if ((encoded & (1 << 4)) == 0) {
			prv_decode_fpa_data(instr, encoded, err);
			return;
		}

		if (((encoded >> 12) & 0xf) == 0xf) {
			prv_decode_fpa_cmp(instr, encoded, err);
			return;
		}

		prv_decode_fpa_tran(instr, encoded, err);
		return;
	}

	subtilis_error_set_bad_instruction(err, encoded);
}

void subtilis_arm_disass_dump(uint8_t *code, size_t len)
{
	size_t i;
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	size_t words = len / 4;

	for (i = 0; i < words; i++) {
		subtilis_error_init(&err);
		subtilis_arm_disass(&instr, ((uint32_t *)code)[i], &err);
		if (err.type == SUBTILIS_ERROR_OK)
			subtilis_arm_instr_dump(&instr);
		else
			printf("DCW &%x\n", ((uint32_t *)code)[i]);
	}

	for (i = words * 4; i < len; i++)
		printf("DCB &%x\n", code[i]);
}
