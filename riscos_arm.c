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

#include "arm2_div.h"
#include "arm_reg_alloc.h"
#include "riscos_arm.h"

static void prv_add_preamble(subtilis_arm_section_t *arm_s, size_t globals,
			     subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;

	/* 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x10, 0x7, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// TODO we need to check that the globals are not too big
	// for the amount of memory we have been allocated by the OS

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 12;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op1.num = 1;

	/*
	 * TODO:  This can be a simple sub.  Avoid label if possible.  We'd
	 * only need the label if we allow globals in multiple files.
	 */

	(void)subtilis_add_data_imm_ldr_datai(arm_s, SUBTILIS_ARM_INSTR_SUB,
					      SUBTILIS_ARM_CCODE_AL, false,
					      dest, op1, globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 13;
	op2.type = SUBTILIS_ARM_REG_FIXED;
	op2.num = 12;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

static void prv_add_coda(subtilis_arm_section_t *arm_s, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 0;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 1;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest,
				 0x58454241, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 2;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x11, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_clear_locals(subtilis_arm_section_t *arm_s,
			     subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t counter;
	subtilis_arm_reg_t base;
	size_t i;
	size_t label;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;

	dest = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	base.num = 11;
	base.type = SUBTILIS_ARM_REG_FIXED;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (arm_s->locals <= 16) {
		/* Loop would be overkill */

		for (i = 0; i < arm_s->locals; i += 4) {
			subtilis_arm_add_stran_imm(
			    arm_s, SUBTILIS_ARM_INSTR_STR,
			    SUBTILIS_ARM_CCODE_AL, dest, base, (int32_t)i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		return;
	}

	counter = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, counter,
				 base, arm_s->locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	label = arm_s->label_counter++;
	subtilis_arm_section_add_label(arm_s, label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = dest;
	stran->base = counter;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = true;
	stran->write_back = true;
	stran->subtract = true;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, counter, base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->target.label = label;
}

static void prv_add_builtin(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;

	/*
	 * TODO:  Just a place holder for now to test builtin functions.  ABS
	 * will be inlined soon.
	 */

	switch (s->ftype) {
	case SUBTILIS_BUILTINS_ABS:
		dest.type = SUBTILIS_ARM_REG_FIXED;
		op2.type = SUBTILIS_ARM_REG_FIXED;
		dest.num = 0;

		subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_CCODE_AL, dest, 0,
					 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op2.num = 0;

		subtilis_arm_add_mvn_reg(arm_s, SUBTILIS_ARM_CCODE_LT, false,
					 dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_LT, false,
					 dest, dest, 1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		dest.num = 15;
		op2.num = 14;
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 dest, op2, err);
		break;
	case SUBTILIS_BUILTINS_IDIV:
		subtilis_arm2_idiv_add(s, arm_s, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_add_section(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_ir_rule_t *parsed, size_t rule_count,
			    subtilis_error_t *err)
{
	size_t spill_regs;
	size_t stack_space;
	subtilis_arm_instr_t *stack_sub;
	subtilis_arm_data_instr_t *datai;
	uint32_t encoded;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;

	stack_sub =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &stack_sub->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest.type = SUBTILIS_ARM_REG_FIXED;
	datai->dest.num = 13;
	datai->op1 = datai->dest;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = 0;

	dest.num = 11;
	dest.type = SUBTILIS_ARM_REG_FIXED;
	op2.type = SUBTILIS_ARM_REG_FIXED;
	op2.num = 13;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);

	if (arm_s->locals > 0) {
		prv_clear_locals(arm_s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_ir_match(s, parsed, rule_count, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	spill_regs = subtilis_arm_reg_alloc(arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stack_space = spill_regs + arm_s->locals;

	encoded = subtilis_arm_encode_nearest(stack_space, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai->op2.op.integer = encoded;

	subtilis_arm_save_regs(arm_s, err);
	subtilis_arm_restore_stack(arm_s, encoded, err);
}

/* clang-format off */
subtilis_arm_prog_t *
subtilis_riscos_generate(
	subtilis_arm_op_pool_t *op_pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_ir_rule_t *parsed;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_section_t *arm_s;
	subtilis_ir_section_t *s;
	size_t i;

	parsed = malloc(sizeof(*parsed) * rule_count);
	if (!parsed) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	subtilis_ir_parse_rules(rules_raw, parsed, rule_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_arm_prog_new(p->num_sections + 2, op_pool,
				      p->string_pool, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = p->sections[0];
	arm_s = subtilis_arm_prog_section_new(arm_p, s->type, s->reg_counter,
					      s->freg_counter, s->label_counter,
					      s->locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_preamble(arm_s, globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_section(s, arm_s, parsed, rule_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_coda(arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 1; i < p->num_sections; i++) {
		s = p->sections[i];
		arm_s = subtilis_arm_prog_section_new(
		    arm_p, s->type, s->reg_counter, s->freg_counter,
		    s->label_counter, s->locals, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (s->ftype != SUBTILIS_BUILTINS_MAX)
			prv_add_builtin(s, arm_s, err);
		else
			prv_add_section(s, arm_s, parsed, rule_count, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	free(parsed);
	parsed = NULL;

	//		printf("\n\n");
	//		subtilis_arm_prog_dump(arm_p);

	return arm_p;

cleanup:

	printf("\n\n");
	if (arm_p)
		subtilis_arm_prog_dump(arm_p);

	subtilis_arm_prog_delete(arm_p);
	free(parsed);

	return NULL;
}

void subtilis_riscos_arm_printi(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *printi = &s->ops[start]->op.instr;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 0;
	op2 = subtilis_arm_ir_to_arm_reg(printi->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.type = SUBTILIS_ARM_REG_FIXED;
	op1.num = 13;
	dest.num = 1;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest.num = 2;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest,
				 SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x7 = r0, r1, r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0xdc, 0x7, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x2, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x3, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}
