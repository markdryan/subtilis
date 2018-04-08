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
		mul->rn.type = SUBTILIS_ARM_REG_FIXED;
		mul->rn.num = (encoded >> 15) & 0x0f;
	} else {
		instr->type = SUBTILIS_ARM_INSTR_MUL;
	}

	mul->ccode = (encoded >> 28);
	mul->status = ((encoded >> 20) & 1);

	mul->dest.type = SUBTILIS_ARM_REG_FIXED;
	mul->dest.num = (encoded >> 16) & 0x0f;

	mul->rm.type = SUBTILIS_ARM_REG_FIXED;
	mul->rm.num = (encoded >> 8) & 0x0f;

	mul->rs.type = SUBTILIS_ARM_REG_FIXED;
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

	mtran->op0.type = SUBTILIS_ARM_REG_FIXED;
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
		op2->op.shift.reg.type = SUBTILIS_ARM_REG_FIXED;
		op2->op.shift.shift = (encoded >> 8) & 0x1f;
		op2->op.shift.type = shtype[((encoded >> 5) & 3)];
	} else {
		op2->type = SUBTILIS_ARM_OP2_REG;
		op2->op.reg.num = encoded & 0xf;
		op2->op.reg.type = SUBTILIS_ARM_REG_FIXED;
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
			op2->op.shift.reg.type = SUBTILIS_ARM_REG_FIXED;
			op2->op.shift.shift = (encoded >> 7) & 0x1f;
			op2->op.shift.type = shtype[((encoded >> 5) & 3)];
		} else {
			op2->type = SUBTILIS_ARM_OP2_REG;
			op2->op.reg.num = encoded & 0xf;
			op2->op.reg.type = SUBTILIS_ARM_REG_FIXED;
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

	/* Negative offsets not yet supported. */
	if ((encoded & (1 << 23)) == 0) {
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
	stran->dest.type = SUBTILIS_ARM_REG_FIXED;
	stran->dest.num = (encoded >> 12) & 0x0f;
	stran->base.type = SUBTILIS_ARM_REG_FIXED;
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

	datai->dest.type = SUBTILIS_ARM_REG_FIXED;
	datai->dest.num = (encoded >> 12) & 0x0f;

	datai->op1.type = SUBTILIS_ARM_REG_FIXED;
	datai->op1.num = (encoded >> 16) & 0x0f;
	prv_decode_op2(encoded, &datai->op2);
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

	subtilis_error_set_bad_instruction(err, encoded);
}
