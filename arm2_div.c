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

#include "arm2_div.h"

static const size_t r;	/* R0 */
static const size_t d = 1;    /* R1 */
static const size_t t = 2;    /* R2 */
static const size_t q = 3;    /* R3 */
static const size_t sign = 4; /* R4 */

static void prv_add_rsa_group(subtilis_ir_section_t *s,
			      subtilis_arm_section_t *arm_s, size_t i,
			      subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *rsb;
	subtilis_arm_data_instr_t *sub;
	subtilis_arm_data_instr_t *adc;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_op2_t op2_reg;
	subtilis_arm_op2_t op2_reg_shift;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op2_reg.type = SUBTILIS_ARM_OP2_REG;
	op2_reg.op.reg.type = SUBTILIS_ARM_REG_FIXED;
	op2_reg_shift.type = SUBTILIS_ARM_OP2_SHIFTED;
	op2_reg_shift.op.shift.reg.type = SUBTILIS_ARM_REG_FIXED;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dest.num = t;
	op1.num = d;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	op2_reg_shift.op.shift.reg.num = r;
	op2_reg_shift.op.shift.shift = i;
	rsb = &instr->operands.data;
	rsb->status = true;
	rsb->ccode = SUBTILIS_ARM_CCODE_AL;
	rsb->dest = dest;
	rsb->op1 = op1;
	rsb->op2 = op2_reg_shift;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = r;
	op2_reg_shift.op.shift.reg.num = d;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	op2_reg_shift.op.shift.shift = i;

	sub = &instr->operands.data;
	sub->status = false;
	sub->ccode = SUBTILIS_ARM_CCODE_CS;
	sub->dest = dest;
	sub->op1 = dest;
	sub->op2 = op2_reg_shift;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = q;
	op2_reg.op.reg = dest;
	adc = &instr->operands.data;
	adc->status = false;
	adc->ccode = SUBTILIS_ARM_CCODE_AL;
	adc->dest = dest;
	adc->op1 = dest;
	adc->op2 = op2_reg;
}

void subtilis_arm2_idiv_add(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_op2_t op2_reg;
	subtilis_arm_op2_t op2_reg_shift;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *shiftd;
	subtilis_arm_data_instr_t *rsb;
	subtilis_arm_data_instr_t *eor;
	subtilis_arm_data_instr_t *sub;
	subtilis_arm_data_instr_t *adc;
	subtilis_arm_data_instr_t *mov;
	subtilis_arm_br_instr_t *br;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op2_reg.type = SUBTILIS_ARM_OP2_REG;
	op2_reg.op.reg.type = SUBTILIS_ARM_REG_FIXED;
	op2_reg_shift.type = SUBTILIS_ARM_OP2_SHIFTED;
	op2_reg_shift.op.shift.reg.type = SUBTILIS_ARM_REG_FIXED;

	dest.num = sign;
	op1.num = d;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_AND,
				  SUBTILIS_ARM_CCODE_AL, true, dest, op1,
				  1 << 31, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.num = d;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_MI, false, op1, op1, 0,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_EOR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2_reg_shift.op.shift.reg.num = r;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_ASR;
	op2_reg_shift.op.shift.shift = 32;

	eor = &instr->operands.data;
	eor->status = true;
	eor->ccode = SUBTILIS_ARM_CCODE_AL;
	eor->dest = dest;
	eor->op1 = dest;
	eor->op2 = op2_reg_shift;

	op1.num = r;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_CS, false, op1, op1, 0,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = q;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = t;
	op1.num = d;
	op2_reg_shift.op.shift.reg.num = r;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	op2_reg_shift.op.shift.shift = 3;

	rsb = &instr->operands.data;
	rsb->status = true;
	rsb->ccode = SUBTILIS_ARM_CCODE_AL;
	rsb->dest = dest;
	rsb->op1 = op1;
	rsb->op2 = op2_reg_shift;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CC;
	br->link = false;
	br->target.label = 3;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.op.shift.shift = 8;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CC;
	br->link = false;
	br->target.label = 1;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = d;
	op2_reg_shift.op.shift.reg.num = d;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	op2_reg_shift.op.shift.shift = 8;

	shiftd = &instr->operands.data;
	shiftd->status = false;
	shiftd->ccode = SUBTILIS_ARM_CCODE_AL;
	shiftd->dest = dest;
	shiftd->op2 = op2_reg_shift;

	dest.num = q;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
				  SUBTILIS_ARM_CCODE_AL, false, dest, dest,
				  0xFF000000, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.op.shift.shift = 4;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CC;
	br->link = false;
	br->target.label = 2;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.op.shift.shift = 8;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CC;
	br->link = false;
	br->target.label = 1;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *shiftd;

	dest.num = q;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
				  SUBTILIS_ARM_CCODE_AL, false, dest, dest,
				  0x00FF0000, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.op.shift.shift = 8;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *shiftd;
	instr->operands.data.ccode = SUBTILIS_ARM_CCODE_CS;

	dest.num = q;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
				  SUBTILIS_ARM_CCODE_CS, false, dest, dest,
				  0x0000FF00, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.op.shift.shift = 4;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CC;
	br->link = false;
	br->target.label = 2;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *rsb;
	instr->operands.data.op2.type = SUBTILIS_ARM_OP2_I32;
	instr->operands.data.op2.op.integer = 0;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CS;
	br->link = false;
	br->target.label = 5;

	subtilis_arm_section_add_label(arm_s, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.data = *shiftd;
	instr->operands.data.ccode = SUBTILIS_ARM_CCODE_CS;
	instr->operands.data.op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;

	subtilis_arm_section_add_label(arm_s, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 7; i >= 4; i--) {
		prv_add_rsa_group(s, arm_s, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_section_add_label(arm_s, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_rsa_group(s, arm_s, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 2; i > 0; i--) {
		prv_add_rsa_group(s, arm_s, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op2_reg.op.reg.num = r;
	instr->operands.data = *rsb;
	instr->operands.data.op2 = op2_reg;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = r;
	op2_reg.op.reg.num = d;
	sub = &instr->operands.data;
	sub->status = false;
	sub->ccode = SUBTILIS_ARM_CCODE_CS;
	sub->dest = dest;
	sub->op1 = dest;
	sub->op2 = op2_reg;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = q;
	op2_reg.op.reg.num = q;
	adc = &instr->operands.data;
	adc->status = true;
	adc->ccode = SUBTILIS_ARM_CCODE_AL;
	adc->dest = dest;
	adc->op1 = dest;
	adc->op2 = op2_reg;

	subtilis_arm_section_add_label(arm_s, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_CS;
	br->link = false;
	br->target.label = 0;

	dest.num = 1;
	op1.num = r;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 0;
	op1.num = q;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = sign;
	op2_reg_shift.op.shift.reg.num = sign;
	op2_reg_shift.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	op2_reg_shift.op.shift.shift = 1;

	mov = &instr->operands.data;
	mov->status = true;
	mov->ccode = SUBTILIS_ARM_CCODE_AL;
	mov->dest = dest;
	mov->op2 = op2_reg_shift;

	op1.num = r;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_CS, false, op1, op1, 0,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.num = d;
	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_MI, false, op1, op1, 0,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 15;
	op1.num = 14;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 err);

	subtilis_arm_section_add_label(arm_s, 5, err); /* div_by_zero */
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 0;
	subtilis_arm_add_mvn_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 1;
	subtilis_arm_add_mvn_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 15;
	op1.num = 14;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 err);
}
