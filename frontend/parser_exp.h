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

#ifndef __SUBTILIS_PARSER_EXP_H
#define __SUBTILIS_PARSER_EXP_H

#include "expression.h"
#include "parser.h"

subtilis_exp_t *subtilis_parser_priority7(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_call_1_arg_fn(subtilis_parser_t *p,
					      const char *name, size_t reg,
					      subtilis_ir_reg_type_t ptype,
					      const subtilis_type_t *rtype,
					      subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_expression(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_bracketed_exp_internal(subtilis_parser_t *p,
						       subtilis_token_t *t,
						       subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_int_var_expression(subtilis_parser_t *p,
						   subtilis_token_t *t,
						   subtilis_error_t *err);
void subtilis_parser_statement_int_args(subtilis_parser_t *p,
					subtilis_token_t *t, subtilis_exp_t **e,
					size_t expected, subtilis_error_t *err);
subtilis_exp_t *
subtilis_parser_bracketed_2_int_args(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_op_instr_type_t itype,
				     subtilis_error_t *err);
void subtilis_parser_bracketed_2_int_args_void(subtilis_parser_t *p,
					       subtilis_token_t *t,
					       subtilis_op_instr_type_t itype,
					       subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_bracketed_exp(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_integer_bracketed_exp(subtilis_parser_t *p,
						      subtilis_token_t *t,
						      subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_real_bracketed_exp(subtilis_parser_t *p,
						   subtilis_token_t *t,
						   subtilis_error_t *err);

size_t subtilis_var_bracketed_int_args_have_b(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_exp_t **e, size_t max,
					      subtilis_error_t *err);
size_t subtilis_var_bracketed_args_have_b(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_exp_t **e, size_t max,
					  subtilis_error_t *err);

#endif
