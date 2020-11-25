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

#ifndef __SUBTILIS_RISCOS_ARM2_H
#define __SUBTILIS_RISCOS_ARM2_H

#include "../../common/ir.h"

extern const subtilis_ir_rule_raw_t riscos_arm2_rules[];
extern const size_t riscos_arm2_rules_count;

size_t subtilis_riscos_arm2_sys_trans(const char *call_name);
bool subtilis_riscos_arm2_sys_check(size_t call_id, uint32_t *in_regs,
				    uint32_t *out_regs, bool *handle_errors);
void subtilis_riscos_arm2_syscall(subtilis_ir_section_t *s, size_t start,
				  void *user_data, subtilis_error_t *err);

void *subtilis_riscos_arm2_asm_parse(subtilis_lexer_t *l, subtilis_token_t *t,
				     void *backend_data,
				     subtilis_type_section_t *stype,
				     const subtilis_settings_t *set,
				     subtilis_error_t *err);

void subtilis_riscos_arm_gcol_tint(subtilis_ir_section_t *s, size_t start,
				   void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_tint(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_tcol_tint(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);

#define SUBTILIS_RISCOS_ARM2_PROGRAM_START 0x8000

#define SUBTILIS_RISCOS_ARM_CAPS                                               \
	(SUBTILIS_BACKEND_HAVE_I32_TO_DEC | SUBTILIS_BACKEND_HAVE_I32_TO_HEX | \
	 SUBTILIS_BACKEND_REVERSE_DOUBLES | SUBTILIS_BACKEND_HAVE_TINT)

#endif
