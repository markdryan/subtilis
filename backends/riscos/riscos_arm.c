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

#include "../../arch/arm32/arm2_div.h"
#include "../../arch/arm32/arm_gen.h"
#include "../../arch/arm32/arm_peephole.h"
#include "../../arch/arm32/arm_reg_alloc.h"
#include "../../arch/arm32/arm_sub_section.h"
#include "../../common/error_codes.h"
#include "riscos_arm.h"

#define RISCOS_ARM_GLOBAL_ESC_FLAG (-4)
#define RISCOS_ARM_GLOBAL_ESC_HANDLER (-8)
#define RISCOS_ARM_GLOBAL_ESC_R12 (-12)

/*
 * RiscOS memory layout
 *
 * Memory organisation of an application under RiscOS is as follows:
 *
 * TOP      |------------------|-------------------------------------------
 *          |   Globals        | Memory reserved for Global variables
 * R12      |------------------|-------------------------------------------
 *          | Escape Condition | non zero if escape has not been handled
 * R12-4    |------------------| -------------------------------------------
 *          |Old Escape Handler| Address of old escape handler
 * R12-8    |------------------| -------------------------------------------
 *          |Old Escape R12    | Value of R12 for old escape handler
 *R13,R12-12|------------------| -------------------------------------------
 *          |   Stack          | Stack used for function calls and locals
 *R13-8192  |------------------| -------------------------------------------
 *          |   Heap           | Heap, used for strings, arrays and structures
 * 0x8000   |------------------| -------------------------------------------
 */

static void prv_add_escape_handler(subtilis_arm_section_t *arm_s,
				   subtilis_error_t *err)
{
	size_t label;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 9,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, 15, 16,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 12,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* I'm assuming that this call can't fail. */

	/* 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x40, 0x7, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 1, 12,
				   RISCOS_ARM_GLOBAL_ESC_HANDLER, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 2, 12,
				   RISCOS_ARM_GLOBAL_ESC_R12, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	label = arm_s->label_counter++;
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = label;

	/* Code for the handler */

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TST,
				 SUBTILIS_ARM_CCODE_AL, 11, 1 << 6, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_EQ, false, 15, 14,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 0, 12,
				   RISCOS_ARM_GLOBAL_ESC_FLAG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

static void prv_remove_escape_handler(subtilis_arm_section_t *arm_s,
				      subtilis_error_t *err)
{
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 9,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, 1, 12,
				   RISCOS_ARM_GLOBAL_ESC_HANDLER, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, 2, 12,
				   RISCOS_ARM_GLOBAL_ESC_R12, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x40, 0x7, err);
}

static void prv_init_heap(subtilis_arm_section_t *arm_s, uint32_t stack_size,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	/*
	 * On entry,
	 * R11 = 0x8000
	 * R13 is the top of the stack
	 */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, 11,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 3, 13,
				 stack_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = 3;
	datai->op1 = 3;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = 1;

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* No registers are corrupted by this SWI */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x1d, 0, err);
}

static void prv_add_preamble(subtilis_arm_section_t *arm_s, size_t globals,
			     subtilis_riscos_fp_preamble_t fp_premable,
			     subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	size_t needed;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	const uint32_t stack_size = 8192;
	const uint32_t min_heap_size = 8192;

	needed = globals + stack_size + min_heap_size;
	if (arm_s->handle_escapes)
		needed += 12;

	if (needed > 0xffffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/* 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x10, 0x7, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 12;

	/*
	 * Check we have enough memory for globals + escape_handler +
	 * 8KB of stack + 8KB of heap.  At some point we'll make this
	 * configurable.
	 */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 11,
				 0x8000, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 12, 11,
				 (uint32_t)needed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, 12, 1, err);
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
	br->target.label = arm_s->no_cleanup_label;

	op1 = 1;

	/*
	 * TODO:  This can be a simple sub.  Avoid label if possible.  We'd
	 * only need the label if we allow globals in multiple files.
	 */

	(void)subtilis_add_data_imm_ldr_datai(arm_s, SUBTILIS_ARM_INSTR_SUB,
					      SUBTILIS_ARM_CCODE_AL, false,
					      dest, op1, globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 13;
	op2 = 12;

	if (arm_s->handle_escapes) {
		// Reserve 12 bytes for escape condition and old handler details

		subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 dest, op2, 12, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1,
					 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
					   SUBTILIS_ARM_CCODE_AL, 1, 12,
					   RISCOS_ARM_GLOBAL_ESC_FLAG, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		prv_add_escape_handler(arm_s, err);
	} else {
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 dest, op2, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_init_heap(arm_s, stack_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	fp_premable(arm_s, err);
}

static void prv_add_coda(subtilis_arm_section_t *arm_s, bool handle_escapes,
			 subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;

	if (handle_escapes) {
		prv_remove_escape_handler(arm_s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_section_add_label(arm_s, arm_s->no_cleanup_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 1;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest,
				 0x58454241, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 2;
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
	base = 11;
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
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = label;
}

static void prv_add_builtin(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	switch (s->ftype) {
	case SUBTILIS_BUILTINS_IDIV:
		subtilis_arm2_idiv_add(s, arm_s, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_compute_sss(subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	subtilis_arm_subsections_t sss;

	subtilis_arm_subsections_init(&sss);
	subtilis_arm_subsections_calculate(&sss, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:

	subtilis_arm_subsections_free(&sss);
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
	datai->dest = 13;
	datai->op1 = datai->dest;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = 0;

	dest = 11;
	op2 = 13;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (arm_s->locals > 0) {
		prv_clear_locals(arm_s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_ir_match(s, parsed, rule_count, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compute_sss(arm_s, err);
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
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_restore_stack(arm_s, encoded, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_peephole(arm_s, err);
}

/* clang-format off */
subtilis_arm_prog_t *
subtilis_riscos_generate(
	subtilis_arm_op_pool_t *op_pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals,
	subtilis_riscos_fp_preamble_t fp_premable,
	subtilis_error_t *err)
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
				      p->string_pool, p->handle_escapes, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = p->sections[0];
	arm_s = subtilis_arm_prog_section_new(arm_p, s->type, s->reg_counter,
					      s->freg_counter, s->label_counter,
					      s->locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_preamble(arm_s, globals, fp_premable, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_section(s, arm_s, parsed, rule_count, err);
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

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(printi->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1 = 13;
	dest = 1;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 2;
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest,
				 SUBTILIS_RISCOS_PRINT_BUFFER_SIZE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x7 = r0, r1, r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0xdc + 0x20000, 0x7,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0x2 + 0x20000, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_printnl(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x3 + 0x20000, 0,
			     err);
}

void subtilis_riscos_arm_modei32(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *modei = &s->ops[start]->op.instr;
	const size_t vdu = 256 + 0x20000;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(modei->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 22, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_plot(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *plot = &s->ops[start]->op.instr;
	const size_t os_plot = 0x45 + 0x20000;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(plot->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 1;
	op2 = subtilis_arm_ir_to_arm_reg(plot->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 2;
	op2 = subtilis_arm_ir_to_arm_reg(plot->operands[2].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, os_plot, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_gcol(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *gcol = &s->ops[start]->op.instr;
	const size_t vdu = 256 + 0x20000;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 18, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(gcol->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(gcol->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_origin(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *mov;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *origin = &s->ops[start]->op.instr;
	const size_t vdu = 256 + 0x20000;
	size_t i;

	// 0x0 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 29, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < 2; i++) {
		dest = 0;
		op2 = subtilis_arm_ir_to_arm_reg(origin->operands[i].reg);

		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false,
					 dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		// 0x0 = r0
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000,
				     0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_MOV, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		mov = &instr->operands.data;

		mov->status = false;
		mov->ccode = SUBTILIS_ARM_CCODE_VC;
		mov->dest = dest;
		mov->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
		mov->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
		mov->op2.op.shift.reg =
		    subtilis_arm_ir_to_arm_reg(origin->operands[i].reg);
		mov->op2.op.shift.shift_reg = false;
		mov->op2.op.shift.shift.integer = 8;

		// 0x0 = r0
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000,
				     0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_riscos_arm_gettime(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *origin = &s->ops[start]->op.instr;
	size_t ir_dest = origin->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);

	/*
	 * TOOD: Need to check for stack overflow.
	 */

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_SUB,
				  SUBTILIS_ARM_CCODE_AL, false, 1, 13, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x07 + 0x20000, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_VC, dest, 1, 0, err);
}

void subtilis_riscos_arm_cls(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	const size_t vdu = 256 + 0x20000;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 12, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_clg(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	const size_t vdu = 256 + 0x20000;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 16, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_on(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x37 + 0x20000, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_off(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x36 + 0x20000, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_wait(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 19,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x6 = r1, r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 6, 0x6 + 0x20000,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtilis_riscos_arm_get(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *get = &s->ops[start]->op.instr;
	size_t ir_dest = get->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);

	// 0x1 = r0
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x4 + 0x20000, 0x1,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, 0,
				 err);
}

void subtilis_riscos_arm_get_to(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *getto = &s->ops[start]->op.instr;
	size_t ir_dest = getto->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);
	size_t ir_op1 = getto->operands[1].reg;
	subtilis_arm_reg_t op1 = subtilis_arm_ir_to_arm_reg(ir_op1);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 129,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_AND,
				  SUBTILIS_ARM_CCODE_GE, false, 1, op1, 0xff,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 0xff,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_AND, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_GE;
	datai->dest = 2;
	datai->op1 = 2;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = false;
	datai->op2.op.shift.reg = op1;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	datai->op2.op.shift.shift.integer = 8;

	// 0x6 = r1 and r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 0x6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, 1,
				 err);
}

void subtilis_riscos_arm_inkey(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *inkey = &s->ops[start]->op.instr;
	size_t ir_dest = inkey->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);
	size_t ir_op1 = inkey->operands[1].reg;
	subtilis_arm_reg_t op1 = subtilis_arm_ir_to_arm_reg(ir_op1);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 129,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, op1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 0xff,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x6 = r1, r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 0x6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, true, dest, 1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mvn_imm(arm_s, SUBTILIS_ARM_CCODE_NE, false, dest, 0,
				 err);
}

void subtilis_riscos_arm_os_byte_id(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *getto = &s->ops[start]->op.instr;
	size_t ir_dest = getto->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 129,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 0xff,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x6 = r1 and r2
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 0x6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, 1,
				 err);
}

void subtilis_riscos_arm_vdui(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *vdu = &s->ops[start]->op.instr;
	size_t ch = vdu->operands[0].integer & 0xff;

	// 0x0 = No register corruption
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 256 + ch + 0x20000,
			     0x0, err);
}

void subtilis_riscos_arm_vdu(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *vdu = &s->ops[start]->op.instr;
	size_t ir_src = vdu->operands[0].reg;
	subtilis_arm_reg_t src = subtilis_arm_ir_to_arm_reg(ir_src);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, src,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x0 = No register corruption
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x0 + 0x20000, 0x0,
			     err);
}

static void prv_point_tint(subtilis_ir_section_t *s, size_t start,
			   void *user_data, size_t res_reg,
			   subtilis_error_t *err)
{
	subtilis_arm_reg_t x;
	subtilis_arm_reg_t y;
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *plot = &s->ops[start]->op.instr;

	x = subtilis_arm_ir_to_arm_reg(plot->operands[1].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, x,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	y = subtilis_arm_ir_to_arm_reg(plot->operands[2].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, y,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// 0x1C = R2, R3, R4 are corrupted
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x32 + 0x20000, 0x1C,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(plot->operands[0].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest,
				 res_reg, err);
}

void subtilis_riscos_arm_point(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	prv_point_tint(s, start, user_data, 2, err);
}

void subtilis_riscos_arm_tint(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_point_tint(s, start, user_data, 3, err);
}

void subtilis_riscos_arm_end(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	prv_add_coda(arm_s, arm_s->handle_escapes, err);
}

void subtilis_riscos_arm_testesc(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	size_t label;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, 0, 12,
				   RISCOS_ARM_GLOBAL_ESC_FLAG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TEQ,
				 SUBTILIS_ARM_CCODE_AL, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	label = arm_s->label_counter++;
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = label;

	/* Clear escape variable */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 0, 12,
				   RISCOS_ARM_GLOBAL_ESC_FLAG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Clear the escape condition */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 124,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* 0x6 = R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 0x6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0,
				 SUBTILIS_ERROR_CODE_ESCAPE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 0, 12,
				   s->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_gen_sete(s, start, user_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

void subtilis_riscos_arm_alloc(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t reg;
	subtilis_arm_reg_t one;
	subtilis_ir_inst_t *alloc = &s->ops[start]->op.instr;
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, 0x8000,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Allocate an extra 8 bytes for the reference count.
	 */

	reg = subtilis_arm_ir_to_arm_reg(alloc->operands[1].reg);
	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 3, reg, 8,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* 0x4 = R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x1d + 0x20000, 0x4,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We want to return allocated block + 8 to program.
	 */

	reg = subtilis_arm_ir_to_arm_reg(alloc->operands[0].reg);
	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_VC, false, reg, 2, 8,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_VS, false, one,
				 SUBTILIS_ERROR_CODE_OOM, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_VS, one, 12,
				   s->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store reference count of 1 in first few bytes of new block
	 */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_VC, false, one, 1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_VC, one, 2, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store size of block
	 */

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_VC, 3, 2, 4, err);
}

void subtilis_riscos_arm_realloc(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	size_t reg;
	subtilis_arm_reg_t block;
	subtilis_arm_reg_t new_size;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_ir_inst_t *realloc = &s->ops[start]->op.instr;
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 4,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, 0x8000,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Subtract 8 from the memory address provided by program
	 */

	block = subtilis_arm_ir_to_arm_reg(realloc->operands[0].reg);
	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, block,
				 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Compute delta for new size in R3.  We need to load the old size first
	 */

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_VC, 3, block, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	new_size = subtilis_arm_ir_to_arm_reg(realloc->operands[1].reg);
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_RSB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = 3;
	datai->op1 = 3;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = new_size;

	/* 0x4 = R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x1d + 0x20000, 0x4,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We want to return allocated block + 8 to program.
	 */

	reg = subtilis_arm_ir_to_arm_reg(realloc->operands[2].reg);
	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, reg, 2, 8,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store size of block
	 */

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_VC, new_size, 2, 4, err);
}

void subtilis_riscos_arm_ref(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t count;
	subtilis_arm_reg_t block;
	subtilis_ir_inst_t *ref = &s->ops[start]->op.instr;
	subtilis_arm_section_t *arm_s = user_data;

	/*
	 * Subtract 8 from the memory address provided by program
	 */

	block = subtilis_arm_ir_to_arm_reg(ref->operands[0].reg);
	count = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, count, block, -8,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, count,
				 count, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, count, block, -8,
				   err);
}

void subtilis_riscos_arm_deref(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t count;
	subtilis_arm_reg_t ptr;
	subtilis_arm_reg_t block;
	subtilis_ir_inst_t *deref = &s->ops[start]->op.instr;
	subtilis_arm_section_t *arm_s = user_data;

	ptr = subtilis_arm_ir_to_arm_reg(deref->operands[0].reg);
	block = 2;
	count = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	/*
	 * Subtract 8 from the memory address provided by program
	 */

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, block,
				 ptr, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, count, block, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, true, count,
				 count, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_EQ, false, 0, 3,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_EQ, false, 1, 0x8000,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Nothing corrupted */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_EQ, 0x1d + 0x20000, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_NE, count, block, 0, err);
}
