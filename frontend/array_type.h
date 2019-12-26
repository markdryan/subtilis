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

#ifndef __SUBTILIS_ARRAY_TYPE_H
#define __SUBTILIS_ARRAY_TYPE_H

#include "../common/type.h"
#include "expression.h"
#include "parser.h"

size_t subtilis_array_type_size(const subtilis_type_t *type);
void subtilis_array_type_init(subtilis_parser_t *p,
			      const subtilis_type_t *element_type,
			      subtilis_type_t *type, subtilis_exp_t **e,
			      size_t dims, subtilis_error_t *err);
void subtlis_array_type_allocate(subtilis_parser_t *p, const char *var_name,
				 subtilis_type_t *type, size_t loc,
				 subtilis_exp_t **e,
				 subtilis_ir_operand_t store_reg,
				 subtilis_error_t *err);
void subtlis_array_type_copy_param_ref(subtilis_parser_t *p,
				       const subtilis_type_t *t,
				       subtilis_ir_operand_t dest_reg,
				       size_t dest_offset,
				       subtilis_ir_operand_t source_reg,
				       size_t source_offset,
				       subtilis_error_t *err);
void subtlis_array_type_create_tmp_ref(subtilis_parser_t *p,
				       const subtilis_type_t *t,
				       subtilis_ir_operand_t dest_reg,
				       subtilis_ir_operand_t source_reg,
				       subtilis_error_t *err);
void subtilis_array_type_match(subtilis_parser_t *p, const subtilis_type_t *t1,
			       const subtilis_type_t *t2,
			       subtilis_error_t *err);
void subtilis_array_type_assign_ref(subtilis_parser_t *p,
				    const subtilis_type_array_t *dest_type,
				    size_t dest_mem_reg, size_t dest_loc,
				    size_t source_reg, subtilis_error_t *err);
void subtilis_array_type_deref(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			       subtilis_error_t *err);
void subtilis_array_type_ref(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			     subtilis_error_t *err);
void subtilis_array_type_pop_and_deref(subtilis_parser_t *p,
				       subtilis_error_t *err);
subtilis_exp_t *subtilis_array_size_calc(subtilis_parser_t *p,
					 const char *var_name,
					 subtilis_exp_t **e, size_t index_count,
					 subtilis_error_t *err);
subtilis_exp_t *
subtilis_array_index_calc(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_exp_t **e, size_t index_count,
			  subtilis_error_t *err);
subtilis_exp_t *subtilis_array_read(subtilis_parser_t *p, const char *var_name,
				    const subtilis_type_t *type,
				    const subtilis_type_t *el_type,
				    size_t mem_reg, size_t loc,
				    subtilis_exp_t **indices,
				    size_t index_count, subtilis_error_t *err);
void subtilis_array_write(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type,
			  const subtilis_type_t *el_type, size_t mem_reg,
			  size_t loc, subtilis_exp_t *e,
			  subtilis_exp_t **indices, size_t index_count,
			  subtilis_error_t *err);
void subtilis_array_add(subtilis_parser_t *p, const char *var_name,
			const subtilis_type_t *type,
			const subtilis_type_t *el_type, size_t mem_reg,
			size_t loc, subtilis_exp_t *e, subtilis_exp_t **indices,
			size_t index_count, subtilis_error_t *err);
void subtilis_array_sub(subtilis_parser_t *p, const char *var_name,
			const subtilis_type_t *type,
			const subtilis_type_t *el_type, size_t mem_reg,
			size_t loc, subtilis_exp_t *e, subtilis_exp_t **indices,
			size_t index_count, subtilis_error_t *err);
void subtilis_array_type_create_ref(subtilis_parser_t *p, const char *var_name,
				    const subtilis_symbol_t *s, size_t mem_reg,
				    subtilis_exp_t *e, subtilis_error_t *err);
subtilis_ir_operand_t subtilis_array_type_error_label(subtilis_parser_t *p);
void subtilis_array_gen_index_error_code(subtilis_parser_t *p,
					 subtilis_error_t *err);

subtilis_exp_t *subtilis_array_get_dim(subtilis_parser_t *p,
				       subtilis_exp_t **indices, size_t dims,
				       subtilis_error_t *err);

#endif
