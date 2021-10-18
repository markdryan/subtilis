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

#ifndef __SUBTILIS_PARSER_H
#define __SUBTILIS_PARSER_H

#include "../common/backend.h"
#include "../common/ir.h"
#include "../common/lexer.h"
#include "../common/settings.h"
#include "../common/sizet_vector.h"
#include "basic_keywords.h"
#include "call.h"
#include "symbol_table.h"

struct subtilis_parser_t_ {
	subtilis_lexer_t *l;
	subtilis_backend_t backend;
	subtilis_ir_section_t *current;
	subtilis_ir_section_t *main;
	subtilis_ir_prog_t *prog;
	subtilis_symbol_table_t *st;
	subtilis_symbol_table_t *local_st;
	subtilis_symbol_table_t *main_st;
	size_t level;
	size_t num_calls;
	size_t max_calls;
	subtilis_parser_call_t **calls;
	size_t num_call_addrs;
	size_t max_call_addrs;
	subtilis_parser_call_addr_t **call_addrs;
	int32_t eflag_offset;
	int32_t error_offset;
	subtilis_settings_t settings;
};

typedef struct subtilis_parser_t_ subtilis_parser_t;

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       const subtilis_backend_t *backend,
				       const subtilis_settings_t *settings,
				       subtilis_error_t *err);
void subtilis_parse(subtilis_parser_t *p, subtilis_error_t *err);
void subtilis_parser_delete(subtilis_parser_t *p);
void subtilis_parser_statement(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err);
int subtilis_parser_if_compound(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err);

#endif
