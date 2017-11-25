/*
 * Copyright (c) 2017 Mark Ryan
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

#include "riscos_arm.h"

size_t prv_add_preamble(subtilis_arm_program_t *arm_p, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	size_t label;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_SWI, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	instr->operands.swi.ccode = SUBTILIS_ARM_CCODE_AL;
	instr->operands.swi.code = 0x10;

	// TODO we need to check that the globals are not too big
	// for the amount of memory we have been allocated by the OS

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 12;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op1.num = 1;

	label = subtilis_add_data_imm_ldr_datai(arm_p, SUBTILIS_ARM_INSTR_SUB,
						SUBTILIS_ARM_CCODE_AL, false,
						dest, op1, arm_p->globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	dest.num = 13;
	op2.type = SUBTILIS_ARM_REG_FIXED;
	op2.num = 12;
	subtilis_arm_mov_reg(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
			     err);
	return label;
}

void prv_add_coda(subtilis_arm_program_t *arm_p, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t dest;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 0;
	subtilis_arm_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 1;
	subtilis_arm_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest,
			     0x58454241, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 2;
	subtilis_arm_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_SWI, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr->operands.swi.ccode = SUBTILIS_ARM_CCODE_AL;
	instr->operands.swi.code = 0x11;
}

/* clang-format off */
subtilis_arm_program_t *
subtilis_riscos_generate(
	subtilis_ir_program_t *p, const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_ir_rule_t *parsed;
	subtilis_arm_program_t *arm_p = NULL;
	//	size_t stack_frame_label;

	parsed = malloc(sizeof(*parsed) * rule_count);
	if (!parsed) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	subtilis_ir_parse_rules(rules_raw, parsed, rule_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_arm_program_new(p->reg_counter, p->label_counter,
					 globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	stack_frame_label = prv_add_preamble(arm_p, err);
	(void)prv_add_preamble(arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_match(p, parsed, rule_count, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_add_coda(arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return arm_p;

cleanup:

	//	printf("\n\n");
	///	subtilis_arm_program_dump(arm_p);

	subtilis_arm_program_delete(arm_p);
	free(parsed);

	return NULL;
}

void subtilis_riscos_arm_printi(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *printi = &p->ops[start]->op.instr;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 0;
	op2 = subtilis_arm_ir_to_arm_reg(printi->operands[0].reg);

	subtilis_arm_mov_reg(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.type = SUBTILIS_ARM_REG_FIXED;
	op1.num = 13;
	dest.num = 1;

	subtilis_arm_add_sub_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 2;
	subtilis_arm_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest,
			     SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_SWI, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr->operands.swi.ccode = SUBTILIS_ARM_CCODE_AL;
	instr->operands.swi.code = 0xdc;

	instr = subtilis_arm_program_dup_instr(arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.swi.code = 0x2;

	instr = subtilis_arm_program_dup_instr(arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.swi.code = 0x3;
}
