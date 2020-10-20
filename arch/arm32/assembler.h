/*
 * Copyright (c) 2020 Mark Ryan
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

#ifndef __SUBTILIS_ARM_ASSEMBLER_H
#define __SUBTILIS_ARM_ASSEMBLER_H

#include "../../common/backend_caps.h"
#include "../../common/string_pool.h"
#include "arm_core.h"
#include "arm_swi.h"

struct subtilis_arm_ass_context_t_ {
	subtilis_arm_section_t *arm_s;
	subtilis_lexer_t *l;
	subtilis_token_t *t;
	subtilis_type_section_t *stype;
	const subtilis_settings_t *set;
	subtilis_backend_sys_trans sys_trans;
	subtilis_string_pool_t *label_pool;
};

typedef struct subtilis_arm_ass_context_t_ subtilis_arm_ass_context_t;

/* clang-format off */
subtilis_arm_section_t *subtilis_arm_asm_parse(
	subtilis_lexer_t *l, subtilis_token_t *t, subtilis_arm_op_pool_t *pool,
	subtilis_type_section_t *stype, const subtilis_settings_t *set,
	subtilis_backend_sys_trans sys_trans, subtilis_error_t *err);
/* clang-format on */

#endif
