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

#include "riscos_arm2.h"
#include "../../arch/arm32/arm_core.h"
#include "../../arch/arm32/arm_gen.h"
#include "../../arch/arm32/fpa_gen.h"
#include "riscos_arm.h"

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
	{"syscall\n", subtilis_riscos_arm_syscall},
};

const size_t riscos_arm2_rules_count = sizeof(riscos_arm2_rules) /
	sizeof(subtilis_ir_rule_raw_t);
/* clang-format on */
