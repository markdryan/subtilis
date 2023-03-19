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

#ifndef SUBTILIS_RV_BARE_H
#define SUBTILIS_RV_BARE_H

#include "../../arch/rv32/rv32_core.h"
#include "../../common/backend_caps.h"
#include "../../common/ir.h"

extern const subtilis_ir_rule_raw_t riscos_rv_bare_rules[];
extern const size_t riscos_rv_bare_rules_count;

size_t subtilis_rv_bare_sys_trans(const char *call_name);
bool subtilis_rv_bare_sys_check(size_t call_id, uint32_t *in_regs,
				uint32_t *out_regs, bool *handle_errors);
void subtilis_rv_bare_syscall(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);

void *subtilis_rv_bare_asm_parse(subtilis_lexer_t *l, subtilis_token_t *t,
				 void *backend_data,
				 subtilis_type_section_t *stype,
				 const subtilis_settings_t *set,
				 subtilis_error_t *err);

/* clang-format off */
subtilis_rv_prog_t *
subtilis_rv_bare_generate(
	subtilis_rv_op_pool_t *pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals,
	int32_t start_address, subtilis_error_t *err);
/* clang-format on */


#define SUBTILIS_RV_PROGRAM_START 0x8000
#define SUBTILIS_RV_PROGRAM_SIZE (640*1024)

#define SUBTILIS_RV_CAPS SUBTILIS_BACKEND_HAVE_DIV

void subtilis_rv_bare_end(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);

void subtilis_rv_bare_printstr(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_bare_printnl(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);

#endif
