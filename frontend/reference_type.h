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

#ifndef __SUBTILIS_REFERENCE_TYPE_H
#define __SUBTILIS_REFERENCE_TYPE_H

#include "expression.h"
#include "parser.h"

/*
 * TODO: These offsets are platform specific.
 */

#define SUBTIILIS_REFERENCE_SIZE_OFF 0
#define SUBTIILIS_REFERENCE_DATA_OFF 4
#define SUBTIILIS_REFERENCE_DESTRUCTOR_OFF 8
#define SUBTIILIS_REFERENCE_SIZE 12

void subtilis_reference_type_init_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      bool check_size, subtilis_error_t *err);
void subtilis_reference_type_new_ref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     size_t dest_mem_reg, size_t dest_loc,
				     size_t source_reg, bool check_size,
				     subtilis_error_t *err);
void subtilis_reference_type_copy_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      subtilis_error_t *err);
void subtilis_reference_type_copy_ret(subtilis_parser_t *p,
				      const subtilis_type_t *t, size_t dest_reg,
				      size_t source_reg, subtilis_error_t *err);
void subtilis_reference_type_assign_ref(subtilis_parser_t *p,
					size_t dest_mem_reg, size_t dest_loc,
					size_t source_reg,
					subtilis_error_t *err);
void subtilis_reference_type_ref(subtilis_parser_t *p, size_t mem_reg,
				 size_t loc, bool check_size,
				 subtilis_error_t *err);
void subtilis_reference_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
					   subtilis_exp_t *e, bool check_size,
					   subtilis_error_t *err);
size_t subtilis_reference_get_pointer(subtilis_parser_t *p, size_t reg,
				      size_t offset, subtilis_error_t *err);
size_t subtilis_reference_get_data(subtilis_parser_t *p, size_t reg,
				   size_t offset, subtilis_error_t *err);
void subtilis_reference_set_data(subtilis_parser_t *p, size_t reg,
				 size_t store_reg, size_t loc,
				 subtilis_error_t *err);
void subtilis_reference_type_memcpy(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, size_t src_reg, size_t size_reg,
				    subtilis_error_t *err);
void subtilis_reference_type_memcpy_dest(subtilis_parser_t *p, size_t dest_reg,
					 size_t src_reg, size_t size_reg,
					 subtilis_error_t *err);
size_t subtilis_reference_type_get_size(subtilis_parser_t *p, size_t mem_reg,
					size_t loc, subtilis_error_t *err);
void subtilis_reference_type_set_size(subtilis_parser_t *p, size_t mem_reg,
				      size_t loc, size_t size_reg,
				      subtilis_error_t *err);
size_t subtilis_reference_type_re_malloc(subtilis_parser_t *p, size_t store_reg,
					 size_t loc, size_t data_reg,
					 size_t size_reg, size_t new_size_reg,
					 bool data_known_valid,
					 subtilis_error_t *err);
size_t subtilis_reference_type_copy_on_write(subtilis_parser_t *p,
					     size_t store_reg, size_t loc,
					     size_t size_reg,
					     subtilis_error_t *err);
void subtilis_reference_inc_cleanup_stack(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_error_t *err);
size_t subtilis_reference_type_alloc(subtilis_parser_t *p,
				     const subtilis_type_t *type, size_t loc,
				     size_t store_reg, size_t size_reg,
				     bool push, subtilis_error_t *err);
size_t subtilis_reference_type_realloc(subtilis_parser_t *p, size_t loc,
				       size_t store_reg, size_t data_reg,
				       size_t old_size_reg, size_t new_size_reg,
				       size_t delta_reg, subtilis_error_t *err);
void subtilis_reference_type_push_reference(subtilis_parser_t *p,
					    const subtilis_type_t *type,
					    size_t reg, size_t loc,
					    subtilis_error_t *err);
void subtilis_reference_type_pop_and_deref(subtilis_parser_t *p,
					   bool ref_of_ref,
					   subtilis_error_t *err);
void subtilis_reference_type_deref(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, subtilis_error_t *err);
void subtilis_reference_deallocate_refs(subtilis_parser_t *p,
					subtilis_ir_operand_t load_reg,
					subtilis_symbol_table_t *st,
					size_t level, subtilis_error_t *err);
size_t subtilis_reference_type_raw_alloc(subtilis_parser_t *p, size_t size_reg,
					 subtilis_error_t *err);

#endif
