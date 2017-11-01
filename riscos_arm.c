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

void prv_add_preamble(subtilis_arm_program_t *arm_p, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_SWI, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr->operands.swi.ccode = SUBTILIS_ARM_CCODE_AL;
	instr->operands.swi.code = 0x10;

	// TODO we need to check that the globals are not too big
	// for the amount of memory we have been allocated by the OS

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 12;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op1.num = 1;

	subtilis_arm_add_sub_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 arm_p->globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest.type = SUBTILIS_ARM_REG_FIXED;
	datai->dest.num = 13;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg.type = SUBTILIS_ARM_REG_FIXED;
	datai->op2.op.reg.num = 12;
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

	prv_add_preamble(arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return arm_p;

cleanup:

	subtilis_arm_program_delete(arm_p);
	free(parsed);

	return NULL;
}
