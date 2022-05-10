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
void subtilis_array_type_swap(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t reg1, size_t reg2, subtilis_error_t *err);

/*
 * Create a one element array of the appropriate dimension and type.  The single
 * element is set to the appropriate zero value.  These single element arrays
 * are used by the zero values of function pointers that return arrays.  When
 * invoked these default functions return a single element array.
 */
void subtilis_array_create_1el(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, bool push, subtilis_error_t *err);

/*
 * Initialise type to be an array with dim dimensions, whose values are supplied
 * by the array e, and whose element type is supplied by element_type.
 */
void subtilis_array_type_init(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *element_type,
			      subtilis_type_t *type, subtilis_exp_t **e,
			      size_t dims, subtilis_error_t *err);

/*
 * Initialise type to be a vector whose element type is supplied by
 * element_type. Vector always have a single dynamic dimension.
 */
void subtilis_array_type_vector_init(const subtilis_type_t *element_type,
				     subtilis_type_t *type,
				     subtilis_error_t *err);
/*
 * Initialise the variable (mem_reg/loc) to be a zero length vector of type
 * type.
 */
void subtilis_array_type_vector_zero_ref(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 size_t loc, size_t mem_reg, bool push,
					 subtilis_error_t *err);
/*
 * Calls the builtin memset32 to zero fill the buffer defined by data_reg and
 * size_reg.  The buffer size must be a multiple of 4.
 */
void subtilis_array_zero_buf_i32(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t data_reg,
				 size_t size_reg, subtilis_error_t *err);
/*
 * Calls the builtin memset8 to zero fill the buffer defined by data_reg and
 * size_reg.
 */
void subtilis_array_zero_buf_i8(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t data_reg,
				size_t size_reg, subtilis_error_t *err);

/*
 * Allocates a new vector of size e + 1 and of type t and updates the variable
 * store_reg loc to point to that variable.  If e == -1 an empty vector
 * is returned.
 */
void subtilis_array_type_vector_alloc(subtilis_parser_t *p, size_t loc,
				      const subtilis_type_t *type,
				      subtilis_exp_t *e,
				      subtilis_ir_operand_t store_reg,
				      bool push, subtilis_error_t *err);

/*
 * Allocates a new array of size e + 1 and of type t and updates the variable
 * store_reg loc to point to that variable.
 */
void subtilis_array_type_allocate(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t loc,
				  subtilis_exp_t **e,
				  subtilis_ir_operand_t store_reg, bool push,
				  subtilis_error_t *err);
/*
 * Initialises an array or vector parameter of a function.  The fields of the
 * array passed to the function, denoted by source_reg/source_offset, are copied
 * to a new array created on the new function's stack.  The reference count of
 * the data is incremented.
 */
void subtlis_array_type_copy_param_ref(subtilis_parser_t *p,
				       const subtilis_type_t *t,
				       subtilis_ir_operand_t dest_reg,
				       size_t dest_offset,
				       subtilis_ir_operand_t source_reg,
				       size_t source_offset,
				       subtilis_error_t *err);

/*
 * Returns a temporary variable that slices the vector identified by
 * mem_reg/loc.
 */
subtilis_exp_t *subtilis_array_type_slice_vector(subtilis_parser_t *p,
						 const subtilis_type_t *type,
						 size_t mem_reg, size_t loc,
						 subtilis_exp_t *index1,
						 subtilis_exp_t *index2,
						 subtilis_error_t *err);

/*
 * Returns a temporary variable that slices the 1d array identified by
 * mem_reg/loc.
 */
subtilis_exp_t *subtilis_array_type_slice_array(subtilis_parser_t *p,
						const subtilis_type_t *type,
						size_t mem_reg, size_t loc,
						subtilis_exp_t *index1,
						subtilis_exp_t *index2,
						subtilis_error_t *err);
/*
 * Creates a new array reference, initialising it with the value in e.
 */
void subtilis_array_type_create_ref(subtilis_parser_t *p, const char *var_name,
				    const subtilis_symbol_t *s, size_t mem_reg,
				    subtilis_exp_t *e, subtilis_error_t *err);

/*
 * Used to initialise array references inside a structure, during
 * structure initialisation.  These array references are uninitialised
 * before this function is called, which is unusual for array variables
 * which normally must be always initialised.
 */

void subtilis_array_type_init_field(subtilis_parser_t *p,
				    const subtilis_type_t *type, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err);

/*
 * Initialise a temporary reference on the caller's stack with the
 * contents of an array variable on the callee's stack.  This is called
 * directly after the callee's ret has executed so his stack is still
 * presevered.
 *
 * TODO: This is a bit risky though.  Might be better, and faster, to
 * allocate space for this variable on the caller's stack.
 */
void subtlis_array_type_copy_ret(subtilis_parser_t *p, const subtilis_type_t *t,
				 size_t dest_reg, size_t source_reg,
				 subtilis_error_t *err);
/*
 * Compares the type of two arrays.  t1 is the target array.  If it has a
 * dynamic dim that's treated as a match.  SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH is
 * returned if the two arrays do not match.
 */
void subtilis_array_type_match(subtilis_parser_t *p, const subtilis_type_t *t1,
			       const subtilis_type_t *t2,
			       subtilis_error_t *err);
/*
 * Assigns the array reference source_reg to the array reference
 * dest_mem_reg/loc.  The existing reference pointed to be dest_mem_reg/loc
 * is dereferenced.
 */
void subtilis_array_type_assign_ref_exp(subtilis_parser_t *p,
					const subtilis_type_t *type,
					size_t mem_reg, size_t loc,
					subtilis_exp_t *e,
					subtilis_error_t *err);
void subtilis_array_type_assign_ref(subtilis_parser_t *p,
				    const subtilis_type_t *type,
				    size_t dest_mem_reg, size_t dest_loc,
				    size_t source_reg, subtilis_error_t *err);
void subtilis_array_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
				       subtilis_exp_t *e,
				       subtilis_error_t *err);
/*
 * Returns an expression containing the number of elements (not bytes) in the
 * array whose dimensions are specified in the array e of expression pointers.
 * The returned value may be a constant.
 */
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
subtilis_ir_operand_t subtilis_array_type_error_label(subtilis_parser_t *p);
void subtilis_array_gen_index_error_code(subtilis_parser_t *p,
					 subtilis_error_t *err);

subtilis_exp_t *subtilis_array_get_dim(subtilis_parser_t *p, subtilis_exp_t *ar,
				       subtilis_exp_t *dim,
				       subtilis_error_t *err);

void subtilis_array_type_memcpy(subtilis_parser_t *p, size_t mem_reg,
				size_t loc, size_t src_reg, size_t size_reg,
				subtilis_error_t *err);
subtilis_exp_t *subtilis_array_type_dynamic_size(subtilis_parser_t *p,
						 size_t mem_reg, size_t loc,
						 subtilis_error_t *err);
/*
 * Deferences all the elements in an array of strings.  The function currently
 * only works with arrays of strings, as they are the only type or array whose
 * elements currently need dereffing.  This will change when we have struvtures.
 */
void subtilis_array_type_deref_els(subtilis_parser_t *p, size_t data_reg,
				   size_t size_reg, subtilis_error_t *err);
void subtilis_array_type_copy_els(subtilis_parser_t *p,
				  const subtilis_type_t *el_type,
				  size_t data_reg, size_t size_reg,
				  size_t src_reg, bool deref,
				  subtilis_error_t *err);
/*
 * Appends a single value (a2) to a vector (a1).
 */
void subtilis_array_append_scalar(subtilis_parser_t *p, subtilis_exp_t *a1,
				  subtilis_exp_t *a2, subtilis_error_t *err);
/*
 * Appends all the values in the scalar vector a2 to the scalar vector a1.
 */
void subtilis_array_append_scalar_array(subtilis_parser_t *p,
					subtilis_exp_t *a1, subtilis_exp_t *a2,
					subtilis_error_t *err);
/*
 * Appends all the reference values in a2 to the vector a2.
 */
void subtilis_array_append_ref_array(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
/*
 * Duplicates an array expression.
 */
void subtilis_array_type_dup(subtilis_exp_t *src, subtilis_exp_t *dst,
			     subtilis_error_t *err);

#endif
