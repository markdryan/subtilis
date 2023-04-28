/*
 * Copyright (c) 2023 Mark Ryan
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

#include "rv_bare.h"

#include "../../arch/rv32/rv_gen.h"
#include "../../arch/rv32/rv_peephole.h"
#include "../../arch/rv32/rv_reg_alloc.h"
#include "../../arch/rv32/rv_sub_section.h"

/*
 * The linker is going to generate a simple ELF executable for us
 * With two sections.
 * 1. Text containing program code.
 * 2. BSS section containing heap pointer + global data.
 *
 * Spike pk sets up the stack register for us so we don't need
 * to initialise that.
 *
 * Our program will start with two instructions to load the address
 * of the global pointer into x3.
 *
 * x2 is the stack
 * x3 points to the start of our global data
 * x4 is the heap pointer
 */

const subtilis_ir_rule_raw_t riscos_rv_bare_rules[] = {
	 {"ltii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_lt_imm},
	 {"gtii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_gt_imm},
	 {"lteii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_lte_imm},
	 {"neqii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_neq_imm},
	 {"eqii32 r_1, *, *\n"
	  "jmpc r_1, label_1, *\n"
	  "label_1",
		  subtilis_rv_gen_if_eq_imm},
	 {"gteii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_gte_imm},
	 {"lti32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_lt},
	 {"gti32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_gt},
	 {"ltei32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_lte},
	 {"eqi32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_eq},
	 {"neqi32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_neq},
	 {"gtei32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_rv_gen_if_gte},
/*
	 {"ltir r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_lt_imm},
	 {"gtir r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_gt_imm},
	 {"lteir r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_lte_imm},
	 {"neqir r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_neq_imm},
	 {"eqir r_1, *, *\n"
	  "jmpc r_1, label_1, *\n"
	  "label_1",
		  subtilis_fpa_gen_if_eq_imm},
	 {"gteir r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_gte_imm},
	 {"ltr r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_lt},
	 {"gtr r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_gt},
	 {"lter r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_lte},
	 {"eqr r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_eq},
	 {"neqr r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_neq},
	 {"gter r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_fpa_gen_if_gte},
*/
	 {"jmpc *, label_1, *\n"
	  "label_1\n",
	  subtilis_rv_gen_jmpc},
	 {"jmpc *, *, label_1\n"
	  "label_1\n",
	  subtilis_rv_gen_jmpc_rev},
	 {"jmpc *, *, *\n", subtilis_rv_gen_jmpc_no_label},
	 {"jmpcnf *, *, *\n", subtilis_rv_gen_jmpc_no_label},
/*
	 {"gti32 r_1, *, *\n"
	  "cmovi32 *, r_1, *, *\n", subtilis_arm_gen_cmovi32_gti32},
	 {"lti32 r_1, *, *\n"
	  "cmovi32 *, r_1, *, *\n", subtilis_arm_gen_cmovi32_lti32},
	 {"cmovi32 *, *, *, *\n", subtilis_arm_gen_cmovi32},
*/
	 {"call\n", subtilis_rv_gen_call},
	 {"calli32\n", subtilis_rv_gen_calli32},
/*
	 {"callr\n", subtilis_fpa_gen_callr},
*/
	 {"callptr\n", subtilis_rv_gen_call_ptr},
	 {"calli32ptr\n", subtilis_rv_gen_calli32_ptr},
/*
	 {"callrptr\n", subtilis_fpa_gen_callr_ptr},
*/
	 {"ret\n", subtilis_rv_gen_ret},
	 {"reti32 *\n", subtilis_rv_gen_reti32},
	 {"retii32 *\n", subtilis_rv_gen_retii32},
/*
	 {"retr *\n", subtilis_fpa_gen_retr},
	 {"retir *\n", subtilis_fpa_gen_retir},
	 {"gtii32 *, *, *\n", subtilis_arm_gen_gtii32},
	 {"gtir *, *, *\n", subtilis_fpa_gen_gtir},
	 {"ltii32 *, *, *\n", subtilis_arm_gen_ltii32},
	 {"ltir *, *, *\n", subtilis_fpa_gen_ltir},
	 {"gteii32 *, *, *\n", subtilis_arm_gen_gteii32},
*/
	 {"gteir *, *, *\n", subtilis_rv_gen_gteir},
/*
	 {"lteii32 *, *, *\n", subtilis_arm_gen_lteii32},
	 {"lteir *, *, *\n", subtilis_fpa_gen_lteir},
*/
	 {"eqii32 *, *, *\n", subtilis_rv_gen_eqii32},
/*
	 {"eqir *, *, *\n", subtilis_fpa_gen_eqir},
	 {"neqii32 *, *, *\n", subtilis_arm_gen_neqii32},
*/
	 {"neqir *, *, *\n", subtilis_rv_gen_neqir},
/*
	 {"gti32 *, *, *\n", subtilis_arm_gen_gti32},
	 {"gtr *, *, *\n", subtilis_fpa_gen_gtr},
	 {"lti32 *, *, *\n", subtilis_arm_gen_lti32},
	 {"ltr *, *, *\n", subtilis_fpa_gen_ltr},
	 {"eqi32 *, *, *\n", subtilis_arm_gen_eqi32},
	 {"eqr *, *, *\n", subtilis_fpa_gen_eqr},
	 {"neqi32 *, *, *\n", subtilis_arm_gen_neqi32},
	 {"neqr *, *, *\n", subtilis_fpa_gen_neqr},
	 {"gtei32 *, *, *\n", subtilis_arm_gen_gtei32},
	 {"gter *, *, *\n", subtilis_fpa_gen_gter},
	 {"ltei32 *, *, *\n", subtilis_arm_gen_ltei32},
	 {"lter *, *, *\n", subtilis_fpa_gen_lter},
*/
	 {"mov *, *", subtilis_rv_gen_mov},
	 {"movii32 *, *", subtilis_rv_gen_movii32},
	 {"addii32 *, *, *", subtilis_rv_gen_addii32},
	 {"mulii32 *, *, *", subtilis_rv_gen_mulii32},
	 {"divi32 *, *, *", subtilis_rv_gen_divi32},
	 {"divii32 *, *, *", subtilis_rv_gen_divii32},
	 {"modi32 *, *, *", subtilis_rv_gen_modi32},
	 {"muli32 *, *, *", subtilis_rv_gen_muli32},
	 {"subii32 *, *, *", subtilis_rv_gen_subii32},
	 {"rsubii32 *, *, *", subtilis_rv_gen_rsubii32},
	 {"addi32 *, *, *", subtilis_rv_gen_addi32},
	 {"subi32 *, *, *", subtilis_rv_gen_subi32},
	 {"storeoi8 *, *, *", subtilis_rv_gen_storeoi8},
	 {"storeoi32 *, *, *", subtilis_rv_gen_storeoi32},
	 {"loadoi8 *, *, *", subtilis_rv_gen_loadoi8},
	 {"loadoi32 *, *, *", subtilis_rv_gen_loadoi32},
	 {"label_1", subtilis_rv_gen_label},
	 {"printstr *, *\n", subtilis_rv_bare_printstr},
	 {"printnl\n", subtilis_rv_bare_printnl},
	 {"jmp *\n", subtilis_rv_gen_jump},

	 {"andii32 *, *, *\n", subtilis_rv_gen_andii32},
	 {"orii32 *, *, *\n", subtilis_rv_gen_orii32},
	 {"eorii32 *, *, *\n", subtilis_rv_gen_eorii32},
	 {"noti32 *, *\n", subtilis_rv_gen_mvni32},
	 {"andi32 *, *, *\n", subtilis_rv_gen_andi32},
	 {"ori32 *, *, *\n", subtilis_rv_gen_ori32},
	 {"eori32 *, *, *\n", subtilis_rv_gen_eori32},
	 {"lsli32 *, *, *\n", subtilis_rv_gen_lsli32},
	 {"lslii32 *, *, *\n", subtilis_rv_gen_lslii32},
	 {"lsri32 *, *, *\n", subtilis_rv_gen_lsri32},
	 {"lsrii32 *, *, *\n", subtilis_rv_gen_lsrii32},
	 {"asri32 *, *, *\n", subtilis_rv_gen_asri32},
	 {"asrii32 *, *, *\n", subtilis_rv_gen_asrii32},

	 {"movfp *, *\n", subtilis_rv_gen_movr},
/*
	 {"movir *, *\n", subtilis_fpa_gen_movir},
*/
	 {"movfpi32 *, *\n", subtilis_rv_gen_movri32},
/*
	 {"movfprdi32 *, *\n", subtilis_fpa_gen_movrrdi32},
*/
	 {"movi32fp *, *\n", subtilis_rv_gen_movi32r},
	 {"addr *, *, *\n", subtilis_rv_gen_addr},
	 {"addir *, *, *\n", subtilis_rv_gen_addir},
	 {"subr *, *, *\n", subtilis_rv_gen_subr},
	 {"subir *, *, *\n", subtilis_rv_gen_subir},
/*
	 {"rsubir *, *, *\n", subtilis_fpa_gen_rsubir},
*/
	 {"mulr *, *, *\n", subtilis_rv_gen_mulr},
	 {"mulir *, *, *\n", subtilis_rv_gen_mulir},
	 {"divr *, *, *\n", subtilis_rv_gen_divr},
	 {"divir *, *, *\n", subtilis_rv_gen_divir},
/*
	 {"rdivir *, *, *\n", subtilis_fpa_gen_rdivir},
	 {"storeor *, *, *\n", subtilis_fpa_gen_storeor},
	 {"loador *, *, *\n", subtilis_fpa_gen_loador},
	 {"modei32 *\n", subtilis_riscos_arm_modei32},
	 {"plot *, *, *\n", subtilis_riscos_arm_plot},
	 {"gcol *, *\n", subtilis_riscos_arm_gcol},
	 {"gcoltint *, *, *\n", subtilis_riscos_arm_gcol_tint},
	 {"origin *, *\n", subtilis_riscos_arm_origin},
	 {"gettime *\n", subtilis_riscos_arm_gettime},
	 {"cls\n", subtilis_riscos_arm_cls},
	 {"clg\n", subtilis_riscos_arm_clg},
	 {"on\n", subtilis_riscos_arm_on},
	 {"off\n", subtilis_riscos_arm_off},
	 {"wait\n", subtilis_riscos_arm_wait},
	 {"sin *, *\n", subtilis_fpa_gen_sin},
	 {"cos *, *\n", subtilis_fpa_gen_cos},
	 {"tan *, *\n", subtilis_fpa_gen_tan},
	 {"asn *, *\n", subtilis_fpa_gen_asn},
	 {"acs *, *\n", subtilis_fpa_gen_acs},
	 {"atn *, *\n", subtilis_fpa_gen_atn},
	 {"sqr *, *\n", subtilis_fpa_gen_sqr},
	 {"log *, *\n", subtilis_fpa_gen_log},
	 {"ln *, *\n", subtilis_fpa_gen_ln},
*/
	 {"absr *, *\n", subtilis_rv_gen_absr},
/*
	 {"get *\n", subtilis_riscos_arm_get},
	 {"gettimeout *, *\n", subtilis_riscos_arm_get_to},
	 {"inkey *, *\n", subtilis_riscos_arm_inkey},
	 {"osbyteid *\n", subtilis_riscos_arm_os_byte_id},
	 {"vdui *\n", subtilis_riscos_arm_vdui},
	 {"vdu *\n", subtilis_riscos_arm_vdu},
	 {"point *, *, *\n", subtilis_riscos_arm_point},
	 {"tint *, *, *\n", subtilis_riscos_arm_tint},
	 */
	 {"end\n", subtilis_rv_bare_end},
/*
	 {"testesc\n", subtilis_riscos_arm_testesc},
	 {"ref *\n", subtilis_riscos_arm_ref},
	 {"getref *, *\n", subtilis_riscos_arm_getref},
	 {"pushi32 *\n", subtilis_arm_gen_pushi32},
	 {"popi32 *\n", subtilis_arm_gen_popi32},
*/
	 {"lca *, *\n", subtilis_rv_gen_lca},
/*
	 {"at *, *\n", subtilis_riscos_arm_at},
	 {"pos *\n", subtilis_riscos_arm_pos},
	 {"vpos *\n", subtilis_riscos_arm_vpos},
	 {"powr *, *, *\n", subtilis_fpa_gen_pow},
	 {"expr *, *\n", subtilis_fpa_gen_exp},
	 {"tcol *\n", subtilis_riscos_tcol},
	 {"tcoltint *, *\n", subtilis_riscos_tcol_tint},
	 {"palette *, *, *, *\n", subtilis_riscos_palette},
	 {"i32todec *, *, *\n", subtilis_riscos_arm_i32_to_dec},
	 {"i32tohex *, *, *\n", subtilis_riscos_arm_i32_to_hex},
	 {"heapfree *\n", subtilis_riscos_arm_heap_free_space},
	 {"blockfree *, *\n", subtilis_riscos_arm_block_free_space},
	 {"blockadjust *, *\n", subtilis_riscos_arm_block_adjust},
	 {"syscall\n", subtilis_riscos_arm2_syscall},
	 {"openout *, *\n", subtilis_riscos_openout},
	 {"openup *, *\n", subtilis_riscos_openup},
	 {"openin *, *\n", subtilis_riscos_openin },
	 {"close *\n", subtilis_riscos_close },
	 {"bget *, *\n", subtilis_riscos_bget },
	 {"bput *, *\n", subtilis_riscos_bput },
	 {"blockget *, *, *, *\n", subtilis_riscos_block_get },
	 {"blockput *, *, *\n", subtilis_riscos_block_put },
	 {"eof *, *\n", subtilis_riscos_eof },
	 {"ext *, *\n", subtilis_riscos_ext },
	 {"getptr *, *\n", subtilis_riscos_get_ptr },
	 {"setptr *, *\n", subtilis_riscos_set_ptr },
	 {"signx8to32 *, *\n", subtilis_riscos_signx8to32 },
	 {"movi8tofp *, *\n", subtilis_fpa_gen_movi8tofp },
	 {"movfptoi32i32 *, *, *\n", subtilis_fpa_gen_movfptoi32i32 },
	 {"oscli *\n", subtilis_riscos_oscli },
*/
	 {"getprocaddr *, *", subtilis_rv_gen_get_proc_addr},
/*
	 {"osargs *\n", subtilis_riscos_osargs },
*/
};

const size_t riscos_rv_bare_rules_count = sizeof(riscos_rv_bare_rules) /
	sizeof(subtilis_ir_rule_raw_t);
/* clang-format on */

size_t subtilis_rv_bare_sys_trans(const char *call_name)
{
	return SIZE_MAX;
}

bool subtilis_rv_bare_sys_check(size_t call_id, uint32_t *in_regs,
				uint32_t *out_regs, bool *handle_errors)
{
	return false;
}

void subtilis_rv_bare_syscall(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void *subtilis_rv_bare_asm_parse(subtilis_lexer_t *l, subtilis_token_t *t,
				 void *backend_data,
				 subtilis_type_section_t *stype,
				 const subtilis_settings_t *set,
				 subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
	return NULL;
}

static void prv_add_coda(subtilis_rv_section_t *rv_s, subtilis_error_t *err)
{
	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A7, 93, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_ecall(rv_s, err);
}

void subtilis_rv_bare_end(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;

	prv_add_coda(rv_s, err);
}

static void prv_mmap_heap(subtilis_rv_section_t *rv_s, subtilis_error_t *err)
{
	uint32_t heap_size = 1024 * 1024;

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_lui(rv_s, SUBTILIS_RV_REG_A1, heap_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_addi(rv_s, SUBTILIS_RV_REG_A1,
				     SUBTILIS_RV_REG_A1, heap_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A2, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A3, 0x22, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A4, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A5, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A7, 222, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_ecall(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Store heap pointer in x4.
	 */

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_HEAP,
				   SUBTILIS_RV_REG_A0, err);
}

static void prv_add_preamble(subtilis_rv_section_t *rv_s,
			     subtilis_error_t *err)
{
	/*
	 * Here we store the address of the BSS section into X3.
	 * BSS will always be page aligned so just need auipc here.
	 */

	subtilis_rv_section_add_lui(rv_s, 3, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Set up the heap in X4.
	 */

	prv_mmap_heap(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_compute_sss(subtilis_rv_section_t *rv_s,
			    subtilis_error_t *err)
{
	subtilis_rv_subsections_t sss;

	subtilis_rv_subsections_init(&sss);
	subtilis_rv_subsections_calculate(&sss, rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_rv_subsections_dump(&sss, rv_s);


cleanup:

	subtilis_rv_subsections_free(&sss);
}

static void prv_add_section(subtilis_ir_section_t *s,
			    subtilis_rv_section_t *rv_s,
			    subtilis_ir_rule_t *parsed, size_t rule_count,
			    subtilis_error_t *err)
{
	size_t lui_instr;
	size_t addi_instr;
	size_t sub_instr;
	size_t stack_space;
	subtilis_rv_instr_t *stack_addi;
	subtilis_rv_instr_t *stack_lui;
	subtilis_rv_instr_t *stack_sub;
	subtilis_rv_reg_t tmp;

	/*
	 * Reserve space on the stack.  At this stage we don't know how
	 * much space we'll need to reserve.  If it's more than 2047 bytes
	 * we'll need three instructions, two to load the immediate and one
	 * to reduce the stack pointer.  If it's less than 2048 we can do it
	 * in one subi instruction.  Save pointers to all the instructions
	 * so we can fix them up later.
	 */

	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);
	subtilis_rv_section_add_lui(rv_s, tmp, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	lui_instr = rv_s->last_op;

	subtilis_rv_section_add_addi(rv_s, tmp, tmp, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	addi_instr = rv_s->last_op;

	/*
	 * Reserve space on the stack.
	 */

	subtilis_rv_section_add_sub(rv_s, SUBTILIS_RV_REG_STACK,
				    SUBTILIS_RV_REG_STACK, tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sub_instr = rv_s->last_op;

	/*
	 * Set up local pointer
	 */

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_LOCAL,
				   SUBTILIS_RV_REG_STACK, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_match(s, parsed, rule_count, rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compute_sss(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stack_space = subtilis_rv_reg_alloc(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stack_lui = &rv_s->op_pool->ops[lui_instr].op.instr;
	stack_addi = &rv_s->op_pool->ops[addi_instr].op.instr;
	stack_sub = &rv_s->op_pool->ops[sub_instr].op.instr;
	if (stack_space <= 2048) {
		subtilis_rv_section_nopify_instr(stack_addi);
		subtilis_rv_section_nopify_instr(stack_lui);
		stack_sub->etype = SUBTILIS_RV_I_TYPE;
		stack_sub->itype = SUBTILIS_RV_ADDI;
		stack_sub->operands.i.rd = SUBTILIS_RV_REG_STACK;
		stack_sub->operands.i.rs1 = SUBTILIS_RV_REG_STACK;
		stack_sub->operands.i.imm = -stack_space;
		stack_sub->operands.i.link_type = SUBTILIS_RV_JAL_LINK_VOID;
	} else {
		stack_lui->operands.uj.op.imm = stack_space >> 12;
		stack_addi->operands.i.imm = stack_space & 0xfff;
	}

	subtilis_rv_restore_stack(rv_s, stack_space, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_peephole(rv_s, err);
}

static void prv_add_builtin(subtilis_ir_section_t *s,
			    subtilis_rv_section_t *rv_s,
			    subtilis_error_t *err)
{

}


/* clang-format off */
subtilis_rv_prog_t *
subtilis_rv_bare_generate(
	subtilis_rv_op_pool_t *op_pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals,
	int32_t start_address, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_ir_rule_t *parsed;
	subtilis_rv_prog_t *rv_p = NULL;
	subtilis_rv_section_t *rv_s;
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

	rv_p = subtilis_rv_prog_new(p->num_sections + 2, op_pool,
				    p->string_pool, p->constant_pool,
				    p->settings, start_address, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = p->sections[0];
	rv_s = subtilis_rv_prog_section_new(rv_p, s->type, s->reg_counter,
					    s->freg_counter, s->label_counter,
					    s->locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_preamble(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_add_section(s, rv_s, parsed, rule_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 1; i < p->num_sections; i++) {
		s = p->sections[i];
		if (s->section_type == SUBTILIS_IR_SECTION_ASM) {
			rv_s = (subtilis_rv_section_t *)s->asm_code;

			/*
			 * Ownership of inline assembly transfers to the RV
			 * section.
			 */

			s->asm_code = NULL;
			subtilis_rv_prog_append_section(rv_p, rv_s, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			continue;
		}
		rv_s = subtilis_rv_prog_section_new(
		    rv_p, s->type, s->reg_counter, s->freg_counter,
		    s->label_counter, s->locals, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (s->section_type == SUBTILIS_IR_SECTION_BACKEND_BUILTIN)
			prv_add_builtin(s, rv_s, err);
		else
			prv_add_section(s, rv_s, parsed, rule_count, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	free(parsed);
	parsed = NULL;

	printf("\n\n");
	subtilis_rv_prog_dump(rv_p);

	return rv_p;

cleanup:

	printf("\n\n");
	if (rv_p)
		subtilis_rv_prog_dump(rv_p);

	subtilis_rv_prog_delete(rv_p);
	free(parsed);

	return NULL;
}

void subtilis_rv_bare_printstr(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t str;
	subtilis_rv_reg_t len;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *printi = &s->ops[start]->op.instr;

	str = subtilis_rv_ir_to_rv_reg(printi->operands[0].reg);
	len = subtilis_rv_ir_to_rv_reg(printi->operands[1].reg);

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A0, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A1, str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A2, len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A7, 64, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_ecall(rv_s, err);
//	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}


void subtilis_rv_bare_printnl(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t virt_reg;
	subtilis_rv_section_t *rv_s = user_data;

	virt_reg = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);

	subtilis_rv_section_add_li(rv_s, virt_reg, 10, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_sw(rv_s, SUBTILIS_RV_REG_STACK, virt_reg,
				   -4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_addi(rv_s, SUBTILIS_RV_REG_STACK,
				     SUBTILIS_RV_REG_STACK, -4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A0, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_mv(rv_s, SUBTILIS_RV_REG_A1,
				   SUBTILIS_RV_REG_STACK, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li(rv_s, SUBTILIS_RV_REG_A2, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_li (rv_s, SUBTILIS_RV_REG_A7, 64, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_ecall(rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

//	subtilis_riscos_handle_graphics_error(arm_s, s, err);

	subtilis_rv_section_add_addi(rv_s, SUBTILIS_RV_REG_STACK,
				     SUBTILIS_RV_REG_STACK, 4, err);
}

