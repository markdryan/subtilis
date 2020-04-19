/*
 * Copyright (c) 2019 Mark Ryan
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

#include "arm_mem.h"

void subtilis_arm_mem_memseti32(subtilis_ir_section_t *s,
				subtilis_arm_section_t *arm_s,
				subtilis_error_t *err)
{
	const size_t base_reg = 0;
	const size_t size = 1;
	const size_t val = 2;
	const size_t stm_end_reg = 10;
	const size_t end_reg = 11;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_data_instr_t *datai;
	size_t i;
	size_t end_label = arm_s->label_counter++;
	size_t start_label = arm_s->label_counter++;
	size_t start_small_label = arm_s->label_counter++;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = end_reg;
	datai->op1 = base_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = size;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 stm_end_reg, end_reg, 32, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, base_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	for (i = 3; i <= 9; i++) {
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, i,
					 2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_section_add_label(arm_s, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, 0, 255 << 2,
			       SUBTILIS_ARM_MTRAN_IA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, base_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_LT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_label;

	subtilis_arm_section_add_label(arm_s, start_small_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, base_reg, end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = end_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = val;
	stran->base = base_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	subtilis_arm_section_add_label(arm_s, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
}

void subtilis_arm_mem_memcpy(subtilis_ir_section_t *s,
			     subtilis_arm_section_t *arm_s,
			     subtilis_error_t *err)
{
	const size_t dest_reg = 0;
	const size_t src_reg = 1;
	const size_t size = 2;
	const size_t val = 3;
	const size_t stm_end_reg = 10;
	const size_t end_reg = 11;
	const size_t words_reg = 9;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_data_instr_t *datai;
	size_t end_label = arm_s->label_counter++;
	size_t start_label = arm_s->label_counter++;
	size_t start_small_label = arm_s->label_counter++;
	size_t start_tiny_label = arm_s->label_counter++;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = end_reg;
	datai->op1 = dest_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = size;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 stm_end_reg, end_reg, 32, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, dest_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	subtilis_arm_section_add_label(arm_s, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, src_reg, 255 << 2,
			       SUBTILIS_ARM_MTRAN_IA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest_reg, 255 << 2,
			       SUBTILIS_ARM_MTRAN_IA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, dest_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_LT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_label;

	subtilis_arm_section_add_label(arm_s, start_small_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_BIC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = words_reg;
	datai->op1 = end_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = 3;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, dest_reg, words_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_tiny_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = val;
	stran->base = src_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = val;
	stran->base = dest_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	subtilis_arm_section_add_label(arm_s, start_tiny_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, dest_reg, end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = end_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = val;
	stran->base = src_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 1;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = true;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = val;
	stran->base = dest_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 1;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = true;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_tiny_label;

	subtilis_arm_section_add_label(arm_s, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
}

void subtilis_arm_mem_memcmp(subtilis_ir_section_t *s,
			     subtilis_arm_section_t *arm_s,
			     subtilis_error_t *err)
{
	size_t i;
	const size_t a1_reg = 0;
	const size_t a2_reg = 1;
	const size_t size = 2;
	const size_t stm_end_reg = 10;
	const size_t end_reg = 11;
	const size_t words_reg = 9;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_data_instr_t *datai;
	size_t eq_label = arm_s->label_counter++;
	size_t not_eq_label = arm_s->label_counter++;
	size_t start_label = arm_s->label_counter++;
	size_t start_small_label = arm_s->label_counter++;
	size_t start_tiny_label = arm_s->label_counter++;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = end_reg;
	datai->op1 = a1_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = size;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 stm_end_reg, end_reg, 16, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	subtilis_arm_section_add_label(arm_s, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, a1_reg, 0x3c,
			       SUBTILIS_ARM_MTRAN_IA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, a2_reg, 0x3c0,
			       SUBTILIS_ARM_MTRAN_IA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < 4; i++) {
		subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
				     SUBTILIS_ARM_CCODE_AL, 2 + i, 6 + i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_B, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		br = &instr->operands.br;
		br->ccode = SUBTILIS_ARM_CCODE_NE;
		br->link = false;
		br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
		br->target.label = not_eq_label;
	}

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_reg, stm_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_LT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_label;

	subtilis_arm_section_add_label(arm_s, start_small_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_BIC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = words_reg;
	datai->op1 = end_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = 3;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_reg, words_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_tiny_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = 2;
	stran->base = a1_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = 6;
	stran->base = a2_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, 2, 6, err);
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
	br->target.label = not_eq_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_small_label;

	subtilis_arm_section_add_label(arm_s, start_tiny_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_reg, end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = eq_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = 2;
	stran->base = a1_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 1;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = true;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = 6;
	stran->base = a2_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 1;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = true;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, 2, 6, err);
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
	br->target.label = not_eq_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_tiny_label;

	subtilis_arm_section_add_label(arm_s, eq_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mvn_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, not_eq_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
}

void subtilis_arm_mem_strcmp(subtilis_ir_section_t *s,
			     subtilis_arm_section_t *arm_s,
			     subtilis_error_t *err)
{
	size_t i;
	const size_t a1_reg = 7;
	const size_t a1_len = 1;
	const size_t a2_reg = 2;
	const size_t a2_len = 3;
	const size_t size = 4;
	const size_t a1_val = 5;
	const size_t a2_val = 6;
	const size_t block_end_reg = 8;
	const size_t a1_byte_val = 9;
	const size_t a2_byte_val = 11;
	const size_t ff = 4;
	const size_t end_reg = 10;
	const size_t res = 0;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_data_instr_t *datai;
	size_t block_eq_label = arm_s->label_counter++;
	size_t eq_label = arm_s->label_counter++;
	size_t start_label = arm_s->label_counter++;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, a1_reg, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_len, a2_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_LE, false, size,
				 a1_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_GT, false, size,
				 a2_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = end_reg;
	datai->op1 = a1_reg;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = size;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 block_end_reg, end_reg, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, ff, 0xff,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, a1_reg, block_end_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = block_eq_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = a1_val;
	stran->base = a1_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = a2_val;
	stran->base = a2_reg;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;

	for (i = 0; i < 4; i++) {
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_AND, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		datai = &instr->operands.data;
		datai->status = false;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->dest = a1_byte_val;
		datai->op1 = ff;
		datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
		datai->op2.op.shift.reg = a1_val;
		datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
		datai->op2.op.shift.shift_reg = false;
		datai->op2.op.shift.shift.integer = i * 8;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_AND, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		datai = &instr->operands.data;
		datai->status = false;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->dest = a2_byte_val;
		datai->op1 = ff;
		datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
		datai->op2.op.shift.reg = a2_val;
		datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
		datai->op2.op.shift.shift_reg = false;
		datai->op2.op.shift.shift.integer = i * 8;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_SUB, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		datai = &instr->operands.data;
		datai->status = true;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->dest = res;
		datai->op1 = a1_byte_val;
		datai->op2.type = SUBTILIS_ARM_OP2_REG;
		datai->op2.op.reg = a2_byte_val;

		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_NE, false,
					 15, 14, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = start_label;

	subtilis_arm_section_add_label(arm_s, block_eq_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < 3; i++) {
		subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
				     SUBTILIS_ARM_CCODE_AL, a1_reg, end_reg,
				     err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_B, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		br = &instr->operands.br;
		br->ccode = SUBTILIS_ARM_CCODE_GE;
		br->link = false;
		br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
		br->target.label = eq_label;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_LDR, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		stran = &instr->operands.stran;
		stran->ccode = SUBTILIS_ARM_CCODE_AL;
		stran->dest = a1_val;
		stran->base = a1_reg;
		stran->offset.type = SUBTILIS_ARM_OP2_I32;
		stran->offset.op.integer = 1;
		stran->pre_indexed = false;
		stran->write_back = true;
		stran->subtract = false;
		stran->byte = true;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_LDR, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		stran = &instr->operands.stran;
		stran->ccode = SUBTILIS_ARM_CCODE_AL;
		stran->dest = a2_val;
		stran->base = a2_reg;
		stran->offset.type = SUBTILIS_ARM_OP2_I32;
		stran->offset.op.integer = 1;
		stran->pre_indexed = false;
		stran->write_back = true;
		stran->subtract = false;
		stran->byte = true;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_SUB, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		datai = &instr->operands.data;
		datai->status = true;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->dest = res;
		datai->op1 = a1_val;
		datai->op2.type = SUBTILIS_ARM_OP2_REG;
		datai->op2.op.reg = a2_val;

		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_NE, false,
					 15, 14, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_section_add_label(arm_s, eq_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = res;
	datai->op1 = a1_len;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = a2_len;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
}
