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
#include <string.h>

#include "../../arch/arm32/arm2_div.h"
#include "../../arch/arm32/arm_gen.h"
#include "../../arch/arm32/arm_heap.h"
#include "../../arch/arm32/arm_mem.h"
#include "../../arch/arm32/arm_peephole.h"
#include "../../arch/arm32/arm_reg_alloc.h"
#include "../../arch/arm32/arm_sub_section.h"
#include "../../arch/arm32/assembler.h"
#include "../../common/error_codes.h"
#include "riscos_arm.h"

static void prv_alloc(subtilis_ir_section_t *s, subtilis_arm_section_t *arm_s,
		      subtilis_error_t *err);
static void prv_deref(subtilis_ir_section_t *s, subtilis_arm_section_t *arm_s,
		      subtilis_error_t *err);

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
 * codeend  |------------------| -------------------------------------------
 *          |   Code           | Program code ( Heap start at code_start + 4)
 * codestart|------------------| -------------------------------------------
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

	/* read_mask = 0x7 = R0, R1, R2 */
	/* write_mask = 0xe = R1, R2, R3 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x40, 0x7, 0xe, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 1, 12,
				   RISCOS_ARM_GLOBAL_ESC_HANDLER, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, 2, 12,
				   RISCOS_ARM_GLOBAL_ESC_R12, false, err);
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
				   RISCOS_ARM_GLOBAL_ESC_FLAG, false, err);
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
				   RISCOS_ARM_GLOBAL_ESC_HANDLER, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, 2, 12,
				   RISCOS_ARM_GLOBAL_ESC_R12, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* read_mask = 0x7 = R0, R1, R2 */
	/* write_mask = 0xe = R1, R2, R3 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x40, 0x7, 0xe, err);
}

static void prv_init_heap(subtilis_arm_section_t *arm_s, uint32_t stack_size,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	/*
	 * On entry,
	 * R11 = heap start
	 * R13 is the top of the stack
	 */

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

	subtilis_arm_heap_init(arm_s, err);
}

static void prv_load_heap_pointer(subtilis_arm_section_t *arm_s,
				  subtilis_arm_reg_t reg,
				  subtilis_arm_reg_t scratch,
				  subtilis_error_t *err)
{
	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, reg,
				 arm_s->start_address, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, scratch, reg, 4,
				   false, err);
}

static void prv_add_preamble(subtilis_arm_section_t *arm_s, size_t globals,
			     subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	size_t needed;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	const uint32_t stack_size = 8192;
	const uint32_t min_heap_size = subtilis_arm_heap_min_size();

	needed = globals + stack_size + min_heap_size;
	if (arm_s->settings->handle_escapes)
		needed += 12;

	if (needed > 0xffffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/*
	 * The second word is going to contain the start of the heap.  We can't
	 * execute this code so we need to skip it with a mov pc, pc.
	 */

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 15,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Just a dummy instruction to save space for the start of the heap.
	 * We'll write the heap start in here later on when linking.
	 */

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 15,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_GetEnv */
	/* read_mask = 0 */
	/* write_mask = 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x10, 0, 0x7, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 12;

	/* Load heap start into R11 */

	prv_load_heap_pointer(arm_s, 10, 11, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Check we have enough memory for code + globals + escape_handler +
	 * 8KB of stack + 8KB of heap.  At some point we'll make this
	 * configurable.
	 */

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

	if (arm_s->settings->handle_escapes) {
		// Reserve 12 bytes for escape condition and old handler details

		subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 dest, op2, 12, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1,
					 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_arm_add_stran_imm(
		    arm_s, SUBTILIS_ARM_INSTR_STR, SUBTILIS_ARM_CCODE_AL, 1, 12,
		    RISCOS_ARM_GLOBAL_ESC_FLAG, false, err);
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

	if (!arm_s->fp_if) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_s->fp_if->preamble_fn(arm_s, err);
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

	/* OS_Exit  */
	/* read_mask = 0x7 = R0, R1, R2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x11, 0x7, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_add_builtin(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	switch (s->ftype) {
	case SUBTILIS_BUILTINS_IDIV:
		subtilis_arm2_idiv_add(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_MEMSETI32:
		subtilis_arm_mem_memseti32(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_MEMCPY:
		subtilis_arm_mem_memcpy(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_MEMCMP:
		subtilis_arm_mem_memcmp(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_COMPARE:
		subtilis_arm_mem_strcmp(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_MEMSETI8:
		subtilis_arm_mem_memseti8(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_ALLOC:
		prv_alloc(s, arm_s, err);
		break;
	case SUBTILIS_BUILTINS_DEREF:
		prv_deref(s, arm_s, err);
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
	const subtilis_arm_fp_if_t *fp_if,
	int32_t start_address, subtilis_error_t *err)
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
				      p->string_pool, p->constant_pool,
				      p->settings, fp_if, start_address, err);
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

	for (i = 1; i < p->num_sections; i++) {
		s = p->sections[i];
		if (s->section_type == SUBTILIS_IR_SECTION_ASM) {
			arm_s = (subtilis_arm_section_t *)s->asm_code;

			/*
			 * Ownership of inline assembly transfers to the ARM
			 * section.
			 */

			s->asm_code = NULL;
			subtilis_arm_prog_append_section(arm_p, arm_s, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			continue;
		}
		arm_s = subtilis_arm_prog_section_new(
		    arm_p, s->type, s->reg_counter, s->freg_counter,
		    s->label_counter, s->locals, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (s->section_type == SUBTILIS_IR_SECTION_BACKEND_BUILTIN)
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

void subtilis_riscos_handle_graphics_error(subtilis_arm_section_t *arm_s,
					   subtilis_ir_section_t *s,
					   subtilis_error_t *err)
{
	subtilis_arm_reg_t one;

	if (arm_s->settings->ignore_graphics_errors)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_GRAPHICS, err);
}

void subtilis_riscos_arm_printstr(subtilis_ir_section_t *s, size_t start,
				  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *printi = &s->ops[start]->op.instr;

	op2 = subtilis_arm_ir_to_arm_reg(printi->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2 = subtilis_arm_ir_to_arm_reg(printi->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteN */
	/* read_mask = 0x3 = r0, r1 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x46 + 0x20000, 0x3,
			     0x0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_printnl(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	/* OS_NewLine */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x3 + 0x20000, 0, 0,
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

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 22, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
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

	/* OS_Plot */
	/* read_mask = 0x7 = r0, r1, r2 */
	/* write_mask = 0x7 = r0, r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, os_plot, 0x7, 0x7,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

static void prv_pos(subtilis_ir_section_t *s, size_t start, void *user_data,
		    size_t src_reg, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *pos = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 134,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_Byte */
	/* read_mask = 1 = r0  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 1, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(pos->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest,
				 src_reg, err);
}

void subtilis_riscos_arm_pos(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_pos(s, start, user_data, 1, err);
}

void subtilis_riscos_arm_vpos(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_pos(s, start, user_data, 2, err);
}

void subtilis_riscos_arm_at(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *at = &s->ops[start]->op.instr;
	const size_t os_tab = 256 + 31 + 0x20000;

	/* vdu 31 */
	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, os_tab, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(at->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(at->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_gcol(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *gcol = &s->ops[start]->op.instr;
	const size_t vdu = 256 + 0x20000;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 18, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(gcol->operands[0].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = 0;
	op2 = subtilis_arm_ir_to_arm_reg(gcol->operands[1].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
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

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 29, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < 2; i++) {
		dest = 0;
		op2 = subtilis_arm_ir_to_arm_reg(origin->operands[i].reg);

		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false,
					 dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		/* OS_WriteC */
		/* read_mask = 0x1 = r0 */
		/* write_mask = 0 */
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000,
				     1, 0, err);
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

		/* OS_WriteC */
		/* read_mask = 0x1 = r0 */
		/* write_mask = 0 */
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000,
				     1, 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_riscos_handle_graphics_error(arm_s, s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_riscos_arm_gettime(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t one;
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

	/* OS_Word */
	/* read_mask = 0x3 = r0, r1 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x07 + 0x20000, 3, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_GRAPHICS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_VC, dest, 1, 0, false,
				   err);
}

void subtilis_riscos_arm_cls(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	const size_t vdu = 256 + 0x20000;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 12, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_clg(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	const size_t vdu = 256 + 0x20000;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 16, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_on(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	/* OS_RestoreCursors */
	/* read_mask = 0  */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x37 + 0x20000, 0, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_off(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	/* OS_RemoveCursors */
	/* read_mask = 0  */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x36 + 0x20000, 0, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_wait(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 19,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_Byte */
	/* read_mask = 1 = r0  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 1, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_get(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t one;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *get = &s->ops[start]->op.instr;
	size_t ir_dest = get->operands[0].reg;
	subtilis_arm_reg_t dest = subtilis_arm_ir_to_arm_reg(ir_dest);

	/* OS_ReadC */
	/* read_mask = 0 */
	/* write_mask = 1 = r0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x4 + 0x20000, 0, 1,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_BAD_INPUT, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, 0,
				 err);
}

void subtilis_riscos_arm_get_to(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	size_t label;
	subtilis_arm_reg_t one;
	subtilis_arm_br_instr_t *br;
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
				  SUBTILIS_ARM_CCODE_AL, false, 1, op1, 0xff,
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
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = 2;
	datai->op1 = 2;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = false;
	datai->op2.op.shift.reg = op1;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	datai->op2.op.shift.shift.integer = 8;

	/* OS_Byte */
	/* read_mask = 7 = r0, r1, r2  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 7, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_BAD_TIME, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	label = arm_s->label_counter++;
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_VS;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = label;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, 2, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_EQ, false, dest, 1,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mvn_imm(arm_s, SUBTILIS_ARM_CCODE_NE, false, dest, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

void subtilis_riscos_arm_inkey(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t one;
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

	/* OS_Byte */
	/* read_mask = 7 = r0, r1, r2  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 7, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_BAD_INPUT, err);
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
	subtilis_arm_reg_t one;
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

	/* OS_Byte */
	/* read_mask = 7 = r0, r1, r2  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 7, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
			      SUBTILIS_ERROR_CODE_BAD_OS_ID, err);
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

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 256 + ch + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
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

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_point_tint(subtilis_ir_section_t *s, size_t start,
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

	/* OS_ReadPoint */
	/* read_mask = 0x3 = r0, r1 */
	/* write_mask = 0x1c = r2, r3, r4 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x32 + 0x20000, 3,
			     0x1C, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(plot->operands[0].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest,
				 res_reg, err);
}

void subtilis_riscos_arm_point(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_riscos_arm_point_tint(s, start, user_data, 2, err);
}

void subtilis_riscos_arm_end(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;

	prv_add_coda(arm_s, arm_s->settings->handle_escapes, err);
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
				   RISCOS_ARM_GLOBAL_ESC_FLAG, false, err);
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
				   RISCOS_ARM_GLOBAL_ESC_FLAG, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Clear the escape condition */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 124,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_Byte */
	/* read_mask = 1 = r0  */
	/* write_mask = 6 = r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0x6 + 0x20000, 1, 6,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_AL, 0,
			      SUBTILIS_ERROR_CODE_ESCAPE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
}

/*
 * This is a built-in function that receives the sizes in bytes to allocate
 * in r0 and returns the allocated memory in r0, or 0 in case of failure.
 * Should a failure occur we also set the error flags appropriately.
 */

static void prv_alloc(subtilis_ir_section_t *s, subtilis_arm_section_t *arm_s,
		      subtilis_error_t *err)
{
	size_t good_label;
	size_t bad_label;
	subtilis_arm_reg_t heap_start = 0;
	subtilis_arm_reg_t requested_size = 1;
	subtilis_arm_reg_t scratch = 2;
	subtilis_arm_reg_t actual_size = 11;

	good_label = arm_s->label_counter++;
	bad_label = arm_s->label_counter++;

	/*
	 * We need to add 8 bytes for
	 * 1. The ref count.
	 * 2. The requested size + 16 bytes.  This information can be used to
	 *    determine how much data is actually left in the allocated block
	 *    which helps with re-alloc.
	 */

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 requested_size, 0, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 actual_size, requested_size, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_load_heap_pointer(arm_s, scratch, heap_start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_heap_alloc(arm_s, good_label, bad_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, good_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store reference count of 1 in first few bytes of new block
	 */

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch,
				 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, scratch, 0, 0, false,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store actual size of block.  Real size is R0 - 8
	 */

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, actual_size, 0, 4,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We want to return allocated block + 8 to program.
	 */

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0, 8,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, bad_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_gen_sete(arm_s, s, SUBTILIS_ARM_CCODE_AL, scratch,
			      SUBTILIS_ERROR_CODE_OOM, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
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
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, count,
				 count, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, count, block, -8,
				   false, err);
}

static void prv_deref(subtilis_ir_section_t *s, subtilis_arm_section_t *arm_s,
		      subtilis_error_t *err)
{
	size_t label;
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t ptr = 0;
	subtilis_arm_reg_t block = 2;
	subtilis_arm_reg_t count = 3;

	/*
	 * Subtract 8 from the memory address provided by program
	 */

	label = arm_s->label_counter++;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, block,
				 ptr, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, count, block, 0,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, true, count,
				 count, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, count, block, 0,
				   false, err);

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_NE;
	br->link = false;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = label;

	prv_load_heap_pointer(arm_s, ptr, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * R1 - contains the start of the heap
	 * R2 - block contains the start of the block
	 */

	subtilis_arm_heap_free(arm_s, 1, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
}

void subtilis_riscos_arm_getref(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t count;
	subtilis_arm_reg_t ptr;
	subtilis_arm_reg_t block;
	subtilis_ir_inst_t *getref = &s->ops[start]->op.instr;
	subtilis_arm_section_t *arm_s = user_data;

	count = subtilis_arm_ir_to_arm_reg(getref->operands[0].reg);
	ptr = subtilis_arm_ir_to_arm_reg(getref->operands[1].reg);
	block = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	/*
	 * Subtract 8 from the memory address provided by program
	 */

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, block,
				 ptr, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, count, block, 0,
				   false, err);
}

void subtilis_riscos_tcol(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *tcol = &s->ops[start]->op.instr;
	size_t col;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 256 + 17 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	col = subtilis_arm_ir_to_arm_reg(tcol->operands[0].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, 0, col,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_palette(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *tcol = &s->ops[start]->op.instr;
	size_t col;
	size_t i;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 256 + 19 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	col = subtilis_arm_ir_to_arm_reg(tcol->operands[0].reg);
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, 0, col,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 256 + 16 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 1; i < 4; i++) {
		col = subtilis_arm_ir_to_arm_reg(tcol->operands[i].reg);
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, 0,
					 col, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		/* OS_WriteC */
		/* read_mask = 0x1 = r0 */
		/* write_mask = 0 */
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000,
				     1, 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_i32_to_dec(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t buffer;
	subtilis_arm_reg_t val;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *to_deci = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(to_deci->operands[0].reg);
	val = subtilis_arm_ir_to_arm_reg(to_deci->operands[1].reg);
	buffer = subtilis_arm_ir_to_arm_reg(to_deci->operands[2].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, val,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, buffer,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 12,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_ConvertInteger4 */
	/* read_mask = 0x7 = r0, r1, r2 */
	/* write_mask = 0x7 = r0, r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0xdc, 0x7, 0x7, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_AL, false, dest, 2, 12,
				  err);
}

void subtilis_riscos_arm_i32_to_hex(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t buffer;
	subtilis_arm_reg_t val;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *to_hexi = &s->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(to_hexi->operands[0].reg);
	val = subtilis_arm_ir_to_arm_reg(to_hexi->operands[1].reg);
	buffer = subtilis_arm_ir_to_arm_reg(to_hexi->operands[2].reg);

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 0, val,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 1, buffer,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, 2, 11,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_ConvertHex8 */
	/* read_mask = 0x7 = r0, r1, r2 */
	/* write_mask = 0x7 = r0, r1, r2 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 0xd4, 0x7, 0x7, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_RSB,
				  SUBTILIS_ARM_CCODE_AL, false, dest, 2, 11,
				  err);
}

void subtilis_riscos_arm_heap_free_space(subtilis_ir_section_t *s, size_t start,
					 void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t heap_start;
	subtilis_arm_reg_t result;
	subtilis_arm_reg_t scratch;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *free_space = &s->ops[start]->op.instr;

	heap_start = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	result = subtilis_arm_ir_to_arm_reg(free_space->operands[0].reg);
	scratch = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	prv_load_heap_pointer(arm_s, scratch, heap_start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_heap_free_space(arm_s, heap_start, result, err);
}

void subtilis_riscos_arm_block_free_space(subtilis_ir_section_t *s,
					  size_t start, void *user_data,
					  subtilis_error_t *err)
{
	subtilis_arm_reg_t total;
	subtilis_arm_reg_t space_used;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *block_space = &s->ops[start]->op.instr;
	subtilis_arm_reg_t dest =
	    subtilis_arm_ir_to_arm_reg(block_space->operands[0].reg);
	subtilis_arm_reg_t block =
	    subtilis_arm_ir_to_arm_reg(block_space->operands[1].reg);

	total = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	space_used = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, space_used, block, -4,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, total, block, -16,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = dest;
	datai->op1 = total;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = space_used;
}

void subtilis_riscos_arm_block_adjust(subtilis_ir_section_t *s, size_t start,
				      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t new_space_used;
	subtilis_arm_reg_t space_used;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *block_adjust = &s->ops[start]->op.instr;
	subtilis_arm_reg_t block =
	    subtilis_arm_ir_to_arm_reg(block_adjust->operands[0].reg);
	subtilis_arm_reg_t increment =
	    subtilis_arm_ir_to_arm_reg(block_adjust->operands[1].reg);

	space_used = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
	new_space_used = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, space_used, block, -4,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = new_space_used;
	datai->op1 = space_used;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = increment;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, new_space_used, block,
				   -4, false, err);
}

static int prv_sys_num_lookup(const void *av, const void *bv)
{
	size_t *a = (size_t *)av;
	subtilis_arm_swi_t *b = (subtilis_arm_swi_t *)bv;

	if (*a == b->num)
		return 0;
	if (*a < b->num)
		return -1;
	return 1;
}

bool subtilis_riscos_sys_check(size_t call_id, uint32_t *in_regs,
			       uint32_t *out_regs, bool *handle_errors,
			       const subtilis_arm_swi_t *swi_list,
			       size_t swi_count)
{
	subtilis_arm_swi_t *found;

	if (call_id & 0x20000) {
		*handle_errors = true;
		call_id &= ~0x20000;
	} else {
		*handle_errors = false;
	}

	if (call_id >= 0x100 && call_id <= 0x1ff) {
		*in_regs = 0;
		return true;
	}

	found = bsearch(&call_id, &swi_list[0], swi_count, sizeof(*found),
			prv_sys_num_lookup);

	if (!found)
		return false;

	*in_regs = found->in_regs;
	*out_regs = found->out_regs;

	return true;
}

static uint32_t prv_check_out_regs(size_t call_id,
				   const subtilis_arm_swi_t *swi_list,
				   size_t swi_count, subtilis_error_t *err)
{
	subtilis_arm_swi_t *found;

	if (call_id >= 0x100 && call_id <= 0x1ff)
		return 0;

	found = bsearch(&call_id, &swi_list[0], swi_count, sizeof(*found),
			prv_sys_num_lookup);

	if (!found) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	return found->out_regs;
}

void subtilis_riscos_arm_syscall(subtilis_ir_section_t *s, size_t start,
				 void *user_data,
				 const subtilis_arm_swi_t *swi_list,
				 size_t swi_count, subtilis_error_t *err)
{
	uint32_t out_regs;
	size_t in_reg;
	size_t out_reg;
	size_t i;
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t one;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_ccode_type_t ccode = SUBTILIS_ARM_CCODE_AL;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_sys_call_t *sys_call = &s->ops[start]->op.sys_call;
	size_t call_id = sys_call->call_id & ~(0x20000);
	size_t flags_reg;

	out_regs = prv_check_out_regs(call_id, swi_list, swi_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i <= 10; i++) {
		if (!((1 << i) & sys_call->in_mask))
			continue;
		in_reg = subtilis_arm_ir_to_arm_reg(sys_call->in_regs[i]);
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, i,
					 in_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, sys_call->call_id,
			     sys_call->in_mask, out_regs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((sys_call->call_id & 0x20000) &&
	    (sys_call->flags_reg == SIZE_MAX)) {
		subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
					   SUBTILIS_ARM_CCODE_VS, 0, 0, 0,
					   false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		one = subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);

		subtilis_arm_gen_sete_reg(arm_s, s, SUBTILIS_ARM_CCODE_VS, one,
					  0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ccode = SUBTILIS_ARM_CCODE_VC;
	}

	for (i = 0; i <= 10; i++) {
		if (!((1 << i) & sys_call->out_mask))
			continue;
		out_reg = subtilis_arm_ir_to_arm_reg(sys_call->out_regs[i].reg);
		if (sys_call->out_regs[i].local)
			subtilis_arm_add_mov_reg(arm_s, ccode, false, out_reg,
						 i, err);
		else
			subtilis_arm_add_stran_imm(
			    arm_s, SUBTILIS_ARM_INSTR_STR, ccode, i, out_reg, 0,
			    false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (sys_call->flags_reg != SIZE_MAX) {
		if (sys_call->flags_local)
			flags_reg =
			    subtilis_arm_ir_to_arm_reg(sys_call->flags_reg);
		else
			flags_reg =
			    subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
		subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 flags_reg, 0xf << 28, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_AND, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		datai = &instr->operands.data;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->status = false;
		datai->dest = flags_reg;
		datai->op1 = flags_reg;
		datai->op2.type = SUBTILIS_ARM_OP2_REG;
		datai->op2.op.reg = 15;

		if (!sys_call->flags_local)
			subtilis_arm_add_stran_imm(
			    arm_s, SUBTILIS_ARM_INSTR_STR,
			    SUBTILIS_ARM_CCODE_AL, flags_reg,
			    subtilis_arm_ir_to_arm_reg(sys_call->flags_reg), 0,
			    false, err);
	}
}

void subtilis_riscos_asm_free(void *asm_code)
{
	subtilis_arm_section_t *arm_s = asm_code;

	subtilis_arm_section_delete(arm_s);
}
