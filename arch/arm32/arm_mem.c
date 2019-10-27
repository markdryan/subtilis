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
				 stm_end_reg, end_reg, 8, err);
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