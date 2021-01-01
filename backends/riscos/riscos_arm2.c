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

#include "../../arch/arm32/arm_core.h"
#include "../../arch/arm32/arm_gen.h"
#include "../../arch/arm32/assembler.h"
#include "../../arch/arm32/fpa_gen.h"
#include "../riscos_common/riscos_arm.h"
#include "riscos_arm2.h"
#include "riscos_swi.h"

/* clang-format off */

const subtilis_ir_rule_raw_t riscos_arm2_rules[] = {
	 {"ltii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_lt_imm},
	 {"gtii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_gt_imm},
	 {"lteii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_lte_imm},
	 {"neqii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_neq_imm},
	 {"eqii32 r_1, *, *\n"
	  "jmpc r_1, label_1, *\n"
	  "label_1",
		  subtilis_arm_gen_if_eq_imm},
	 {"gteii32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_gte_imm},
	 {"lti32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_lt},
	 {"gti32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_gt},
	 {"ltei32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_lte},
	 {"eqi32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_eq},
	 {"neqi32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_neq},
	 {"gtei32 r_1, *, *\n"
	 "jmpc r_1, label_1, *\n"
	 "label_1",
		  subtilis_arm_gen_if_gte},
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
	 {"jmpc *, label_1, *\n"
	  "label_1\n",
	  subtilis_arm_gen_jmpc},
	 {"jmpc *, *, label_1\n"
	  "label_1\n",
	  subtilis_arm_gen_jmpc_rev},
	 {"jmpc *, *, *\n", subtilis_arm_gen_jmpc_no_label},
	 {"jmpcnf *, *, *\n", subtilis_arm_gen_jmpc_no_label},
	 {"gti32 r_1, *, *\n"
	  "cmovi32 *, r_1, *, *\n", subtilis_arm_gen_cmovi32_gti32},
	 {"lti32 r_1, *, *\n"
	  "cmovi32 *, r_1, *, *\n", subtilis_arm_gen_cmovi32_lti32},
	 {"cmovi32 *, *, *, *\n", subtilis_arm_gen_cmovi32},
	 {"call\n", subtilis_arm_gen_call},
	 {"calli32\n", subtilis_arm_gen_calli32},
	 {"callr\n", subtilis_fpa_gen_callr},
	 {"ret\n", subtilis_arm_gen_ret},
	 {"reti32 *\n", subtilis_arm_gen_reti32},
	 {"retii32 *\n", subtilis_arm_gen_retii32},
	 {"retr *\n", subtilis_fpa_gen_retr},
	 {"retir *\n", subtilis_fpa_gen_retir},
	 {"gtii32 *, *, *\n", subtilis_arm_gen_gtii32},
	 {"gtir *, *, *\n", subtilis_fpa_gen_gtir},
	 {"ltii32 *, *, *\n", subtilis_arm_gen_ltii32},
	 {"ltir *, *, *\n", subtilis_fpa_gen_ltir},
	 {"gteii32 *, *, *\n", subtilis_arm_gen_gteii32},
	 {"gteir *, *, *\n", subtilis_fpa_gen_gteir},
	 {"lteii32 *, *, *\n", subtilis_arm_gen_lteii32},
	 {"lteir *, *, *\n", subtilis_fpa_gen_lteir},
	 {"eqii32 *, *, *\n", subtilis_arm_gen_eqii32},
	 {"eqir *, *, *\n", subtilis_fpa_gen_eqir},
	 {"neqii32 *, *, *\n", subtilis_arm_gen_neqii32},
	 {"neqir *, *, *\n", subtilis_fpa_gen_neqir},
	 {"gti32 *, *, *\n", subtilis_arm_gen_gti32},
	 {"gtr *, *, *\n", subtilis_fpa_gen_gtr},
	 {"lti32 *, *, *\n", subtilis_arm_gen_lti32},
	 {"ltr *, *, *\n", subtilis_fpa_gen_ltr},
	 {"eqi32 *, *, *\n", subtilis_arm_gen_eqi32},
	 {"eqr *, *, *\n", subtilis_fpa_gen_eqr},
	 {"neqi32 *, *, *\n", subtilis_arm_gen_neqi32},
	 {"neqr *, *, *\n", subtilis_fpa_gen_neqr},
	 {"gtei32 *, *, *\n", subtilis_arm_gen_gtei32},
	 {"gtr *, *, *\n", subtilis_fpa_gen_gtr},
	 {"ltei32 *, *, *\n", subtilis_arm_gen_ltei32},
	 {"lter *, *, *\n", subtilis_fpa_gen_lter},
	 {"mov *, *", subtilis_arm_gen_mov},
	 {"movii32 *, *", subtilis_arm_gen_movii32},
	 {"addii32 *, *, *", subtilis_arm_gen_addii32},
	 {"mulii32 *, *, *", subtilis_arm_gen_mulii32},
	 {"muli32 *, *, *", subtilis_arm_gen_muli32},
	 {"subii32 *, *, *", subtilis_arm_gen_subii32},
	 {"rsubii32 *, *, *", subtilis_arm_gen_rsubii32},
	 {"addi32 *, *, *", subtilis_arm_gen_addi32},
	 {"subi32 *, *, *", subtilis_arm_gen_subi32},
	 {"storeoi8 *, *, *", subtilis_arm_gen_storeoi8},
	 {"storeoi32 *, *, *", subtilis_arm_gen_storeoi32},
	 {"loadoi8 *, *, *", subtilis_arm_gen_loadoi8},
	 {"loadoi32 *, *, *", subtilis_arm_gen_loadoi32},
	 {"label_1", subtilis_arm_gen_label},
	 {"printstr *, *\n", subtilis_riscos_arm_printstr},
	 {"printnl\n", subtilis_riscos_arm_printnl},
	 {"jmp *\n", subtilis_arm_gen_jump},
	 {"andii32 *, *, *\n", subtilis_arm_gen_andii32},
	 {"orii32 *, *, *\n", subtilis_arm_gen_orii32},
	 {"eorii32 *, *, *\n", subtilis_arm_gen_eorii32},
	 {"noti32 *, *\n", subtilis_arm_gen_mvni32},
	 {"andi32 *, *, *\n", subtilis_arm_gen_andi32},
	 {"ori32 *, *, *\n", subtilis_arm_gen_ori32},
	 {"eori32 *, *, *\n", subtilis_arm_gen_eori32},
	 {"lsli32 *, *, *\n", subtilis_arm_gen_lsli32},
	 {"lslii32 *, *, *\n", subtilis_arm_gen_lslii32},
	 {"lsri32 *, *, *\n", subtilis_arm_gen_lsri32},
	 {"lsrii32 *, *, *\n", subtilis_arm_gen_lsrii32},
	 {"asri32 *, *, *\n", subtilis_arm_gen_asri32},
	 {"asrii32 *, *, *\n", subtilis_arm_gen_asrii32},
	 {"movfp *, *\n", subtilis_fpa_gen_movr},
	 {"movir *, *\n", subtilis_fpa_gen_movir},
	 {"movfpi32 *, *\n", subtilis_fpa_gen_movri32},
	 {"movfprdi32 *, *\n", subtilis_fpa_gen_movrrdi32},
	 {"movi32fp *, *\n", subtilis_fpa_gen_movi32r},
	 {"addr *, *, *\n", subtilis_fpa_gen_addr},
	 {"addir *, *, *\n", subtilis_fpa_gen_addir},
	 {"subr *, *, *\n", subtilis_fpa_gen_subr},
	 {"subir *, *, *\n", subtilis_fpa_gen_subir},
	 {"rsubir *, *, *\n", subtilis_fpa_gen_rsubir},
	 {"mulr *, *, *\n", subtilis_fpa_gen_mulr},
	 {"mulir *, *, *\n", subtilis_fpa_gen_mulir},
	 {"divr *, *, *\n", subtilis_fpa_gen_divr},
	 {"divir *, *, *\n", subtilis_fpa_gen_divir},
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
	 {"absr *, *\n", subtilis_fpa_gen_absr},
	 {"get *\n", subtilis_riscos_arm_get},
	 {"gettimeout *, *\n", subtilis_riscos_arm_get_to},
	 {"inkey *, *\n", subtilis_riscos_arm_inkey},
	 {"osbyteid *\n", subtilis_riscos_arm_os_byte_id},
	 {"vdui *\n", subtilis_riscos_arm_vdui},
	 {"vdu *\n", subtilis_riscos_arm_vdu},
	 {"point *, *, *\n", subtilis_riscos_arm_point},
	 {"tint *, *, *\n", subtilis_riscos_arm_tint},
	 {"end\n", subtilis_riscos_arm_end},
	 {"testesc\n", subtilis_riscos_arm_testesc},
	 {"ref *\n", subtilis_riscos_arm_ref},
	 {"getref *, *\n", subtilis_riscos_arm_getref},
	 {"pushi32 *\n", subtilis_arm_gen_pushi32},
	 {"popi32 *\n", subtilis_arm_gen_popi32},
	 {"lca *, *\n", subtilis_arm_gen_lca},
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
};

const size_t riscos_arm2_rules_count = sizeof(riscos_arm2_rules) /
	sizeof(subtilis_ir_rule_raw_t);
/* clang-format on */

static int prv_sys_string_lookup(const void *av, const void *bv)
{
	const char *a = (const char *)av;
	size_t *b = (size_t *)bv;

	return strcmp(a, subtilis_riscos_swi_list[*b].name);
}

size_t subtilis_riscos_arm2_sys_trans(const char *call_name)
{
	size_t *found;
	size_t error_bit = 0;

	if (call_name[0] == 'X') {
		call_name++;
		error_bit = 0x20000;
	}

	found = bsearch(call_name, &subtilis_riscos_swi_index[0],
			subtilis_riscos_known_swis, sizeof(*found),
			prv_sys_string_lookup);

	if (!found)
		return SIZE_MAX;

	return subtilis_riscos_swi_list[*found].num | error_bit;
}

bool subtilis_riscos_arm2_sys_check(size_t call_id, uint32_t *in_regs,
				    uint32_t *out_regs, bool *handle_errors)
{
	return subtilis_riscos_sys_check(
	    call_id, in_regs, out_regs, handle_errors, subtilis_riscos_swi_list,
	    subtilis_riscos_known_swis);
}

void subtilis_riscos_arm2_syscall(subtilis_ir_section_t *s, size_t start,
				  void *user_data, subtilis_error_t *err)
{
	size_t flags_reg;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_ir_sys_call_t *sys_call = &s->ops[start]->op.sys_call;
	subtilis_arm_section_t *arm_s = user_data;

	subtilis_riscos_arm_syscall(s, start, user_data,
				    subtilis_riscos_swi_list,
				    subtilis_riscos_known_swis, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (sys_call->flags_reg != SIZE_MAX) {
		if (sys_call->flags_local)
			flags_reg =
			    subtilis_arm_ir_to_arm_reg(sys_call->flags_reg);
		else
			flags_reg =
			    subtilis_arm_ir_to_arm_reg(arm_s->reg_counter++);
		subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
					 flags_reg, 15, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_MOV, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		datai = &instr->operands.data;
		datai->ccode = SUBTILIS_ARM_CCODE_AL;
		datai->status = false;
		datai->dest = flags_reg;
		datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
		datai->op2.op.shift.reg = flags_reg;
		datai->op2.op.shift.shift_reg = false;
		datai->op2.op.shift.shift.integer = 28;
		datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;

		if (!sys_call->flags_local)
			subtilis_arm_add_stran_imm(
			    arm_s, SUBTILIS_ARM_INSTR_STR,
			    SUBTILIS_ARM_CCODE_AL, flags_reg,
			    subtilis_arm_ir_to_arm_reg(sys_call->flags_reg), 0,
			    false, err);
	}
}

void *subtilis_riscos_arm2_asm_parse(subtilis_lexer_t *l, subtilis_token_t *t,
				     void *backend_data,
				     subtilis_type_section_t *stype,
				     const subtilis_settings_t *set,
				     subtilis_error_t *err)
{
	subtilis_arm_op_pool_t *pool = backend_data;

	return subtilis_arm_asm_parse(l, t, pool, stype, set,
				      subtilis_riscos_arm2_sys_trans,
				      SUBTILIS_RISCOS_ARM2_PROGRAM_START, err);
}

static void prv_do_tint(subtilis_arm_section_t *arm_s, subtilis_arm_reg_t col,
			subtilis_arm_reg_t tint, uint32_t add,
			subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_VC;
	datai->dest = 0;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.reg = col;
	datai->op2.op.shift.shift_reg = false;
	datai->op2.op.shift.shift.integer = 7;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_AND,
				  SUBTILIS_ARM_CCODE_VC, false, 0, 0, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (add != 0) {
		subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ADD,
					  SUBTILIS_ARM_CCODE_VC, false, 0, 0,
					  add, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 256 + 23 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 256 + 17 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* OS_WriteC */
	/* read_mask = 0x1 = r0 */
	/* write_mask = 0 */

	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC, 0 + 0x20000, 1, 0,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, 0, tint,
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

	for (i = 0; i < 6; i++) {
		subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_VC,
				     256 + 0x20000, 0, 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_riscos_arm_gcol_tint(subtilis_ir_section_t *s, size_t start,
				   void *user_data, subtilis_error_t *err)
{
	size_t col;
	size_t tint;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op2;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *gcol = &s->ops[start]->op.instr;
	const size_t vdu = 256 + 0x20000;

	col = subtilis_arm_ir_to_arm_reg(gcol->operands[1].reg);
	tint = subtilis_arm_ir_to_arm_reg(gcol->operands[2].reg);

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, vdu + 18, 0, 0, err);
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

	dest = 0;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_VC, false, dest, col,
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

	prv_do_tint(arm_s, col, tint, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_tcol_tint(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	size_t col;
	size_t tint;
	subtilis_arm_section_t *arm_s = user_data;
	subtilis_ir_inst_t *tcol = &s->ops[start]->op.instr;

	col = subtilis_arm_ir_to_arm_reg(tcol->operands[0].reg);
	tint = subtilis_arm_ir_to_arm_reg(tcol->operands[1].reg);

	/* read_mask = 0 */
	/* write_mask = 0 */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_AL, 256 + 17 + 0x20000,
			     0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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

	prv_do_tint(arm_s, col, tint, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_riscos_handle_graphics_error(arm_s, s, err);
}

void subtilis_riscos_arm_tint(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_riscos_arm_point_tint(s, start, user_data, 3, err);
}
