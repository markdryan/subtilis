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

bool subtilis_exp_get_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_ir_operand_t *op, subtilis_type_t *type,
			     subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_priority7(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err);
/* clang-format off */
subtilis_exp_t *subtilis_parser_call_1_arg_fn(subtilis_parser_t *p,
					      const char *name, size_t reg,
					      subtilis_builtin_type_t ftype,
					      subtilis_ir_reg_type_t ptype,
					      const subtilis_type_t *rtype,
					      bool check_errors,
					      subtilis_error_t *err);

subtilis_exp_t *subtilis_parser_call_2_arg_fn(subtilis_parser_t *p,
					      const char *name, size_t arg1,
					      size_t arg2,
					      subtilis_ir_reg_type_t ptype1,
					      subtilis_ir_reg_type_t ptype2,
					      const subtilis_type_t *rtype,
					      bool check_errors,
					      subtilis_error_t *err);
/* clang-format on */

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
/*
 * Parses a vector used in expression form.  The return value can be
 * - 0 indicating we have a vector reference.
 * - 1 indicating that we are indexing a vector.
 * - 2 indicating that we have a slice.  e[1] may be NULL in this case
 *     indicating that we're slicing to the end of the array.
 */
size_t subtilis_curly_bracketed_slice_have_b(subtilis_parser_t *p,
					     subtilis_token_t *t,
					     subtilis_exp_t **ee,
					     subtilis_error_t *err);
/*
 * Parses a 1d array used in expression form.   Return values are the
 * same as subtilis_curly_bracketed_slice_have_b.
 */
size_t subtilis_round_bracketed_slice_have_b(subtilis_parser_t *p,
					     subtilis_token_t *t,
					     subtilis_exp_t **ee,
					     subtilis_error_t *err);
subtilis_exp_t *subtilis_curly_bracketed_arg_have_b(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    subtilis_error_t *err);
subtilis_exp_t *subtilis_var_lookup_ref(subtilis_parser_t *p,
					subtilis_token_t *t, bool *local,
					subtilis_error_t *err);
void subtilis_complete_custom_type(subtilis_parser_t *p, const char *var_name,
				   subtilis_type_t *type,
				   subtilis_error_t *err);
const subtilis_symbol_t *subtilis_parser_get_symbol(subtilis_parser_t *p,
						    const char *var_name,
						    size_t *mem_reg,
						    subtilis_error_t *err);

void subtilis_exp_swap_int32_mem(subtilis_parser_t *p, size_t dest_reg,
				 size_t source_reg, int32_t off,
				 subtilis_error_t *err);
void subtilis_exp_swap_int8_mem(subtilis_parser_t *p, size_t reg1, size_t reg2,
				int32_t off, subtilis_error_t *err);

#endif
