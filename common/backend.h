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

#ifndef __SUBTILIS_BACKEND_H
#define __SUBTILIS_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "backend_caps.h"
#include "error.h"
#include "keywords.h"
#include "lexer.h"
#include "settings.h"

typedef size_t (*subtilis_backend_sys_trans)(const char *call_name);
typedef bool (*subtilis_backend_sys_check)(size_t call_id, uint32_t *in_regs,
					   uint32_t *out_regs,
					   bool *check_for_errors);
typedef void (*subtilis_backend_asm_free_t)(void *asm_code);
typedef void *(*subtilis_backend_asm_parse_t)(subtilis_lexer_t *l,
					      subtilis_token_t *t,
					      void *backend_data,
					      subtilis_type_section_t *stype,
					      const subtilis_settings_t *set,
					      subtilis_error_t *err);

struct subtilis_backend_t_ {
	subtilis_backend_caps_t caps;
	subtilis_backend_sys_trans sys_trans;
	subtilis_backend_sys_check sys_check;
	const subtilis_keyword_t *ass_keywords;
	size_t num_ass_keywords;
	subtilis_backend_asm_parse_t asm_parse;
	subtilis_backend_asm_free_t asm_free;
	void *backend_data;
};

typedef struct subtilis_backend_t_ subtilis_backend_t;

#endif
