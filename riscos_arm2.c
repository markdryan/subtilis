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
		 subtilis_arm_gen_if_lt},
	{"jmpc *, label_1, *\n"
	"label_1\n",
		 subtilis_arm_gen_jmpc},
	{"movii32 *, *", subtilis_arm_gen_movii32},
	{"addii32 *, *, *", subtilis_arm_gen_addii32},
	{"storeoi32 *, *, *", subtilis_arm_gen_storeoi32},
	{"loadoi32 *, *, *", subtilis_arm_gen_loadoi32},
	{"label_1", subtilis_arm_gen_label},
	{"printi32 *\n", subtilis_riscos_arm_printi},
	{"jmp *\n", subtilis_arm_gen_jump},
};

const size_t riscos_arm2_rules_count = sizeof(riscos_arm2_rules) /
	sizeof(subtilis_ir_rule_raw_t);
/* clang-format on */
