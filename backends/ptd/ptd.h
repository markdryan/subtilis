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

#ifndef __SUBTILIS_PTD_H
#define __SUBTILIS_PTD_H

#include "../../arch/arm32/arm_core.h"
#include "../../common/ir.h"

extern const subtilis_ir_rule_raw_t ptd_rules[];
extern const size_t ptd_rules_count;

size_t subtilis_ptd_sys_trans(const char *call_name);
bool subtilis_ptd_sys_check(size_t call_id, uint32_t *in_regs,
			    uint32_t *out_regs, bool *handle_errors);
void subtilis_ptd_syscall(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);

void *subtilis_ptd_asm_parse(subtilis_lexer_t *l, subtilis_token_t *t,
			     void *backend_data, subtilis_type_section_t *stype,
			     const subtilis_settings_t *set,
			     subtilis_error_t *err);

#define SUBTILIS_PTD_PROGRAM_START 0xF000

#define SUBTILIS_PTD_CAPS 0

void subtilis_ptd_arm_on(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err);
void subtilis_ptd_arm_off(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_ptd_eof(subtilis_ir_section_t *s, size_t start, void *user_data,
		      subtilis_error_t *err);

#endif
