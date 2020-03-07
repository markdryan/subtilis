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

#include "parser.h"

#define SUBTIILIS_REFERENCE_SIZE_OFF 0
#define SUBTIILIS_REFERENCE_DATA_OFF 4

void subtilis_reference_type_init_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      bool check_size, subtilis_error_t *err);
void subtilis_reference_type_new_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				     size_t dest_loc, size_t source_reg,
				     bool check_size, subtilis_error_t *err);
void subtilis_reference_type_assign_ref(subtilis_parser_t *p,
					size_t dest_mem_reg, size_t dest_loc,
					size_t source_reg,
					subtilis_error_t *err);
size_t subtilis_reference_get_pointer(subtilis_parser_t *p, size_t reg,
				      size_t offset, subtilis_error_t *err);
void subtilis_reference_type_memcpy(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, size_t src_reg, size_t size_reg,
				    subtilis_error_t *err);
void subtilis_reference_type_memcpy_dest(subtilis_parser_t *p, size_t dest_reg,
					 size_t src_reg, size_t size_reg,
					 subtilis_error_t *err);
void subtilis_reference_inc_cleanup_stack(subtilis_parser_t *p,
					  subtilis_error_t *err);
size_t subtilis_reference_type_alloc(subtilis_parser_t *p, size_t loc,
				     size_t store_reg, size_t size_reg,
				     bool push, subtilis_error_t *err);
void subtilis_reference_type_push_reference(subtilis_parser_t *p, size_t reg,
					    size_t loc, subtilis_error_t *err);
void subtilis_reference_type_pop_and_deref(subtilis_parser_t *p,
					   subtilis_error_t *err);
void subtilis_reference_type_deref(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, bool check,
				   subtilis_error_t *err);

#endif
