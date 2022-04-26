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
#define SUBTIILIS_REFERENCE_HEAP_OFF 8
#define SUBTIILIS_REFERENCE_ORIG_SIZE_OFF 12
#define SUBTIILIS_REFERENCE_SIZE 16

void subtilis_reference_type_init_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      bool check_size, bool ref,
				      subtilis_error_t *err);
void subtilis_reference_push_ref(subtilis_parser_t *p, const subtilis_type_t *t,
				 size_t reg, subtilis_error_t *err);
void subtilis_reference_type_new_ref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     size_t dest_mem_reg, size_t dest_loc,
				     size_t source_reg, bool check_size,
				     subtilis_error_t *err);
void subtilis_reference_type_copy_ref(subtilis_parser_t *p,
				      const subtilis_type_t *t,
				      size_t dest_mem_reg, size_t dest_loc,
				      size_t source_reg, subtilis_error_t *err);
void subtilis_reference_type_copy_ret(subtilis_parser_t *p,
				      const subtilis_type_t *t, size_t dest_reg,
				      size_t source_reg, subtilis_error_t *err);
void reference_type_call_deref(subtilis_parser_t *p,
			       subtilis_ir_operand_t address,
			       subtilis_error_t *err);
void subtilis_reference_type_assign_ref(subtilis_parser_t *p,
					size_t dest_mem_reg, size_t dest_loc,
					size_t source_reg, bool check_size,
					subtilis_error_t *err);
void subtilis_reference_type_assign_no_rc(subtilis_parser_t *p,
					  size_t dest_mem_reg, size_t dest_loc,
					  size_t source_reg, bool check_size,
					  subtilis_error_t *err);
void subtilis_reference_type_ref(subtilis_parser_t *p, size_t mem_reg,
				 size_t loc, bool check_size,
				 subtilis_error_t *err);
void subtilis_reference_type_swap(subtilis_parser_t *p, size_t reg1,
				  size_t reg2, int32_t limit,
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
size_t subtilis_reference_get_heap(subtilis_parser_t *p, size_t reg,
				   size_t offset, subtilis_error_t *err);
void subtilis_reference_set_heap(subtilis_parser_t *p, size_t reg,
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
size_t subtilis_reference_type_get_orig_size(subtilis_parser_t *p,
					     size_t mem_reg, size_t loc,
					     subtilis_error_t *err);
void subtilis_reference_type_set_orig_size(subtilis_parser_t *p, size_t mem_reg,
					   size_t loc, size_t size_reg,
					   subtilis_error_t *err);

/*
 * Called when we're doing copy on write for strings and when growing
 * vectors, or adding two strings together.  Only called when the
 * reference count > 1.  We allocate a new buffer, memcpy the old
 * contents, deref the old heap pointer and replace the data and heap pointers.
 * A register containing the data/heap pointer is returned.
 */
size_t subtilis_reference_type_re_malloc(subtilis_parser_t *p, size_t store_reg,
					 size_t loc, size_t heap_reg,
					 size_t data_reg, size_t size_reg,
					 size_t new_size_reg,
					 bool heap_known_valid,
					 subtilis_error_t *err);
/*
 * Called when modifying string contents using left$, right$ or mid$.
 * We check the reference count.  If it's == 1 we just return the data
 * pointer.  Otherwise we do a subtilis_reference_type_re_malloc and
 * return the heap/data pointer which are equal.
 */
size_t subtilis_reference_type_copy_on_write(subtilis_parser_t *p,
					     size_t store_reg, size_t loc,
					     size_t size_reg,
					     subtilis_error_t *err);
void subtilis_reference_inc_cleanup_stack(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_error_t *err);
/*
 * Allocates memory for a new reference type and sets all the reference fields
 * including size, data, heap and destructor.  It optionally pushes the
 * reference onto the cleanup stack.
 */
size_t subtilis_reference_type_alloc(subtilis_parser_t *p,
				     const subtilis_type_t *type, size_t loc,
				     size_t store_reg, size_t size_reg,
				     bool push, subtilis_error_t *err);
/*
 * Increases the size of the heap block used by the variable (store_reg/loc).
 * It is assumed that the variable is the sole owner of the heap block.
 * The data, heap and size fields of the variable are all updated correctly.
 * The data reg is returned.  This may or may not be equal to the heap field.
 */
size_t subtilis_reference_type_realloc(subtilis_parser_t *p, size_t loc,
				       size_t store_reg, size_t heap_reg,
				       size_t data_reg, size_t old_size_reg,
				       size_t new_size_reg, size_t delta_reg,
				       subtilis_error_t *err);

/*
 * Increases the size of the heap block assigned to the variable represented
 * by a1_mem_reg/a1_loc.  There are three options here.  If the size of the
 * variable is currently zero, we just do a malloc, if its > 0 and is the sole
 * owner of its block we can realloc it (which might be a nop if the current
 * heap block has enough space available), or we do a malloc and a copy.  In
 * any case a register is returned that points to the first free byte of the
 * variable's heap block.
 */
size_t subtilis_reference_type_grow(subtilis_parser_t *p, size_t a1_loc,
				    size_t a1_mem_reg, size_t a1_size_reg,
				    size_t new_size_reg, size_t a2_size_reg,
				    subtilis_error_t *err);

void subtilis_reference_type_push_reference(subtilis_parser_t *p,
					    const subtilis_type_t *type,
					    size_t reg, size_t loc,
					    subtilis_error_t *err);
void subtilis_reference_type_pop_and_deref(subtilis_parser_t *p,
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
