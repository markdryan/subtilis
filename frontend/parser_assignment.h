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

#ifndef __SUBTILIS_PARSER_ASSIGNMENT_H
#define __SUBTILIS_PARSER_ASSIGNMENT_H

#include "expression.h"
#include "parser.h"
char *subtilis_parser_get_assignment_var(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_type_t *id_type,
					 subtilis_error_t *err);
void subtilis_parser_lookup_assignment_var(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   const char *var_name,
					   const subtilis_symbol_t **s,
					   size_t *mem_reg, bool *new_global,
					   subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_assign_local_num(subtilis_parser_t *p,
						 subtilis_token_t *t,
						 const char *var_name,
						 const subtilis_type_t *id_type,
						 subtilis_error_t *err);
void subtilis_parser_create_array_ref(subtilis_parser_t *p,
				      const char *var_name,
				      const subtilis_type_t *id_type,
				      subtilis_exp_t *e, bool local,
				      subtilis_error_t *err);
void subtilis_parser_assignment(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err);
void subtilis_parser_let(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err);

#endif
