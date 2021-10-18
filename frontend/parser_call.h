/*
 * Copyright (c) 2019 Mark Ryan
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

#ifndef __SUBTILIS_PARSER_CALLP_H
#define __SUBTILIS_PARSER_CALLP_H

#include "expression.h"
#include "parser.h"

subtilis_exp_t *subtilis_parser_call(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_call_known_ptr(subtilis_parser_t *p,
					       subtilis_token_t *t,
					       const subtilis_type_t *type,
					       size_t reg,
					       subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_call_ptr(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 const subtilis_symbol_t *s,
					 const char *var_name, size_t mem_reg,
					 subtilis_error_t *err);

void subtilis_parser_def(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_lambda(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err);

void subtilis_parser_return(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err);
void subtilis_parser_endproc(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err);
void subtilis_parser_check_calls(subtilis_parser_t *p, subtilis_error_t *err);
void subtilis_parser_unwind(subtilis_parser_t *p, subtilis_error_t *err);
char *subtilis_parser_parse_call_type(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_type_t *type,
				      subtilis_error_t *err);
void subtilis_parser_call_add_addr(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_exp_t *e, subtilis_error_t *err);

#endif
