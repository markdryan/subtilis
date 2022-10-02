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

#ifndef __SUBTILIS_PARSER_ARRAY_H
#define __SUBTILIS_PARSER_ARRAY_H

#include "expression.h"
#include "parser.h"

struct subtilis_array_desc_t_ {
	const char *name;
	const subtilis_type_t *t;
	size_t reg;
	size_t loc;
};

typedef struct subtilis_array_desc_t_ subtilis_array_desc_t;

subtilis_exp_t *subtils_parser_read_array(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  const char *var_name,
					  subtilis_error_t *err);

/* clang-format off */
subtilis_exp_t *subtils_parser_read_array_with_type(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    const subtilis_type_t *type,
						    size_t mem_reg, size_t loc,
						    const char *var_name,
						    subtilis_error_t *err);
/* clang-format on */

size_t subtils_parser_array_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
				   const subtilis_symbol_t *s, size_t mem_reg,
				   subtilis_type_t *type,
				   subtilis_error_t *err);
size_t subtils_parser_vector_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
				    const subtilis_symbol_t *s, size_t mem_reg,
				    subtilis_type_t *type,
				    subtilis_error_t *err);
subtilis_exp_t *subtils_parser_read_vector(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   const char *var_name,
					   subtilis_error_t *err);

/* clang-format off */
subtilis_exp_t *subtils_parser_read_vector_with_type(subtilis_parser_t *p,
						     subtilis_token_t *t,
						     const subtilis_type_t *typ,
						     size_t mem_reg, size_t loc,
						     const char *var_name,
						     subtilis_error_t *err);
/* clang-format on */

size_t parser_array_var_bracketed_int_args(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_exp_t **e, size_t max,
					   bool *vec, subtilis_error_t *err);
subtilis_exp_t *
subtils_parser_element_address(subtilis_parser_t *p, subtilis_token_t *t,
			       const subtilis_symbol_t *s, size_t mem_reg,
			       const char *var_name, subtilis_error_t *err);
void subtilis_parser_create_array(subtilis_parser_t *p, subtilis_token_t *t,
				  bool local, subtilis_error_t *err);
void subtilis_parser_array_init_list(subtilis_parser_t *p, subtilis_token_t *t,
				     const subtilis_array_desc_t *d,
				     subtilis_exp_t *e, subtilis_error_t *err);
void subtilis_parser_array_assign_reference(subtilis_parser_t *p,
					    subtilis_token_t *t,
					    const subtilis_array_desc_t *d,
					    subtilis_exp_t *e,
					    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_get_dim(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_constant_array(subtilis_parser_t *p,
					       subtilis_token_t *t,
					       const subtilis_type_t *type,
					       subtilis_error_t *err);
#endif
