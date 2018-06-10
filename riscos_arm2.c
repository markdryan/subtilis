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
#include "arm_core.h"
#include "arm_gen.h"
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
		 subtilis_arm_gen_if_gte},
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
	{"jmpc *, label_1, *\n"
	"label_1\n",
		 subtilis_arm_gen_jmpc},
	{"call\n", subtilis_arm_gen_call},
	{"calli32\n", subtilis_arm_gen_calli32},
	{"ret\n", subtilis_arm_gen_ret},
	{"reti32\n", subtilis_arm_gen_reti32},
	{"retii32\n", subtilis_arm_gen_retii32},
	{"gtii32 *, *, *\n", subtilis_arm_gen_gtii32},
	{"ltii32 *, *, *\n", subtilis_arm_gen_ltii32},
	{"gteii32 *, *, *\n", subtilis_arm_gen_gteii32},
	{"lteii32 *, *, *\n", subtilis_arm_gen_lteii32},
	{"eqii32 *, *, *\n", subtilis_arm_gen_eqii32},
	{"neqii32 *, *, *\n", subtilis_arm_gen_neqii32},
	{"gti32 *, *, *\n", subtilis_arm_gen_gti32},
	{"lti32 *, *, *\n", subtilis_arm_gen_lti32},
	{"eqi32 *, *, *\n", subtilis_arm_gen_eqi32},
	{"neqi32 *, *, *\n", subtilis_arm_gen_neqi32},
	{"gtei32 *, *, *\n", subtilis_arm_gen_gtei32},
	{"ltei32 *, *, *\n", subtilis_arm_gen_ltei32},
	{"mov *, *", subtilis_arm_gen_mov},
	{"movii32 *, *", subtilis_arm_gen_movii32},
	{"addii32 *, *, *", subtilis_arm_gen_addii32},
	{"mulii32 *, *, *", subtilis_arm_gen_mulii32},
	{"muli32 *, *, *", subtilis_arm_gen_muli32},
	{"subii32 *, *, *", subtilis_arm_gen_subii32},
	{"rsubii32 *, *, *", subtilis_arm_gen_rsubii32},
	{"addi32 *, *, *", subtilis_arm_gen_addi32},
	{"subi32 *, *, *", subtilis_arm_gen_subi32},
	{"storeoi32 *, *, *", subtilis_arm_gen_storeoi32},
	{"loadoi32 *, *, *", subtilis_arm_gen_loadoi32},
	{"label_1", subtilis_arm_gen_label},
	{"printi32 *\n", subtilis_riscos_arm_printi},
	{"jmp *\n", subtilis_arm_gen_jump},
	{"andii32 *, *, *\n", subtilis_arm_gen_andii32},
	{"orii32 *, *, *\n", subtilis_arm_gen_orii32},
	{"eorii32 *, *, *\n", subtilis_arm_gen_eorii32},
	{"noti32 *, *\n", subtilis_arm_gen_mvni32},
	{"andi32 *, *, *\n", subtilis_arm_gen_andi32},
	{"ori32 *, *, *\n", subtilis_arm_gen_ori32},
	{"eori32 *, *, *\n", subtilis_arm_gen_eori32},
};

const size_t riscos_arm2_rules_count = sizeof(riscos_arm2_rules) /
	sizeof(subtilis_ir_rule_raw_t);
/* clang-format on */
