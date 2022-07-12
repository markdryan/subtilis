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

#ifndef __SUBTILIS_BUILTINS_IR_H__
#define __SUBTILIS_BUILTINS_IR_H__

#include "../common/ir.h"
#include "expression.h"
#include "parser.h"

void subtilis_builtins_ir_inkey(subtilis_ir_section_t *current,
				subtilis_error_t *err);

subtilis_exp_t *subtilis_builtins_ir_basic_rnd(subtilis_parser_t *p,
					       subtilis_error_t *err);
subtilis_exp_t *subtilis_builtins_ir_rnd_0(subtilis_parser_t *p,
					   subtilis_error_t *err);
subtilis_exp_t *subtilis_builtins_ir_rnd_pos(subtilis_parser_t *p, int32_t val,
					     subtilis_error_t *err);
subtilis_exp_t *subtilis_builtins_ir_rnd_neg(subtilis_parser_t *p, int32_t val,
					     subtilis_error_t *err);
subtilis_exp_t *subtilis_builtins_ir_rnd_1(subtilis_parser_t *p,
					   subtilis_error_t *err);
void subtilis_builtins_ir_rnd_int(subtilis_parser_t *p,
				  subtilis_ir_section_t *current,
				  subtilis_error_t *err);

void subtilis_builtins_ir_rnd_real(subtilis_parser_t *p,
				   subtilis_ir_section_t *current,
				   subtilis_error_t *err);

void subtilis_builtins_ir_fp_to_str(subtilis_parser_t *p,
				    subtilis_ir_section_t *current,
				    subtilis_error_t *err);

void subtilis_builtins_ir_str_to_fp(subtilis_parser_t *p,
				    subtilis_ir_section_t *current,
				    subtilis_error_t *err);

subtilis_ir_section_t *
subtilis_builtins_ir_add_1_arg_int(subtilis_parser_t *p, const char *name,
				   const subtilis_type_t *rtype,
				   subtilis_error_t *err);
subtilis_ir_section_t *
subtilis_builtins_ir_add_1_arg_real(subtilis_parser_t *p, const char *name,
				    const subtilis_type_t *rtype,
				    subtilis_error_t *err);

subtilis_exp_t *subtilis_builtin_ir_call_dec_to_str(subtilis_parser_t *p,
						    size_t val_reg,
						    size_t buf_reg,
						    subtilis_error_t *err);

subtilis_exp_t *subtilis_builtin_ir_call_hex_to_str(subtilis_parser_t *p,
						    size_t val_reg,
						    size_t buf_reg,
						    subtilis_error_t *err);

subtilis_exp_t *subtilis_builtin_ir_call_fp_to_str(subtilis_parser_t *p,
						   size_t val_reg,
						   size_t buf_reg,
						   subtilis_error_t *err);
subtilis_exp_t *subtilis_builtin_ir_call_str_to_fp(subtilis_parser_t *p,
						   size_t str_reg,
						   subtilis_error_t *err);
subtilis_exp_t *subtilis_builtin_ir_call_hexstr_to_int32(subtilis_parser_t *p,
							 size_t str_reg,
							 subtilis_error_t *err);
subtilis_exp_t *subtilis_builtin_ir_call_str_to_int32(subtilis_parser_t *p,
						      size_t str_reg,
						      size_t base_reg,
						      subtilis_error_t *err);
size_t subtilis_builtin_ir_deref_array_els(subtilis_parser_t *p,
					   subtilis_error_t *err);
size_t subtilis_builtin_ir_deref_array_recs(subtilis_parser_t *p,
					    const subtilis_type_t *el_type,
					    subtilis_error_t *err);
size_t subtilis_builtin_ir_call_deref(subtilis_parser_t *p,
				      subtilis_error_t *err);
size_t subtilis_builtin_ir_rec_deref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     subtilis_error_t *err);
void subtilis_builtin_ir_rec_zero(subtilis_parser_t *p,
				  const subtilis_type_t *type, size_t base_reg,
				  subtilis_error_t *err);
void subtilis_builtin_ir_rec_copy(subtilis_parser_t *p,
				  const subtilis_type_t *type, size_t dest_reg,
				  size_t src_reg, subtilis_error_t *err);

#endif
