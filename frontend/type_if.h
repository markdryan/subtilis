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

#ifndef __SUBTILIS_TYPE_IF_H
#define __SUBTILIS_TYPE_IF_H

#include "../common/ir.h"
#include "expression.h"
#include "parser.h"

typedef size_t (*subtilis_type_if_size_t)(const subtilis_type_t *type);
typedef size_t (*subtilis_type_if_destruct_t)(subtilis_parser_t *p,
					      const subtilis_type_t *type,
					      subtilis_error_t *err);
typedef void (*subtilis_type_if_typeof_t)(const subtilis_type_t *element_type,
					  subtilis_type_t *type,
					  subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_zero_t)(subtilis_parser_t *p,
						   const subtilis_type_t *type,
						   subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_none_t)(subtilis_parser_t *p,
						   subtilis_error_t *err);
typedef void (*subtilis_type_if_zeroref_t)(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   size_t mem_reg, size_t loc,
					   bool push, subtilis_error_t *err);
typedef void (*subtilis_type_if_initref_t)(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   size_t mem_reg, size_t loc,
					   subtilis_exp_t *e,
					   subtilis_error_t *err);
typedef void (*subtilis_type_if_zeroreg_t)(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   size_t reg, subtilis_error_t *err);
typedef void (*subtilis_type_if_reg_t)(subtilis_parser_t *p, size_t reg,
				       subtilis_error_t *err);
typedef void (*subtilis_type_if_reg2_t)(subtilis_parser_t *p,
					const subtilis_type_t *type,
					size_t reg1, size_t reg2,
					subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_unary_t)(subtilis_parser_t *p,
						    subtilis_exp_t *e,
						    subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_binary_t)(subtilis_parser_t *p,
						     subtilis_exp_t *a1,
						     subtilis_exp_t *a2,
						     subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_binary_nc_t)(subtilis_parser_t *p,
							subtilis_exp_t *a1,
							subtilis_exp_t *a2,
							bool swapped,
							subtilis_error_t *err);
typedef void (*subtilis_type_if_dup_t)(subtilis_exp_t *e1, subtilis_exp_t *e2,
				       subtilis_error_t *err);
typedef void (*subtilis_type_if_copy_collection_t)(subtilis_parser_t *p,
						   subtilis_exp_t *e1,
						   subtilis_exp_t *e2,
						   subtilis_error_t *err);

typedef void (*subtilis_type_if_sizet_exp_t)(subtilis_parser_t *p, size_t reg,
					     subtilis_exp_t *e,
					     subtilis_error_t *err);
typedef void (*subtilis_type_if_sizet2_exp_t)(subtilis_parser_t *p, size_t reg,
					      size_t loc, subtilis_exp_t *e,
					      subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_load_t)(subtilis_parser_t *p,
						   const subtilis_type_t *type,
						   size_t reg, size_t loc,
						   subtilis_error_t *err);
/* clang-format off */
typedef void (*subtilis_type_if_iwrite_t)(subtilis_parser_t *p,
					  const char *var_name,
					  const subtilis_type_t *type,
					  size_t mem_reg, size_t loc,
					  subtilis_exp_t *e,
					  subtilis_exp_t **indices,
					  size_t index_count,
					  subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_iread_t)(subtilis_parser_t *p,
						    const char *var_name,
						    const subtilis_type_t *type,
						    size_t mem_reg, size_t loc,
						    subtilis_exp_t **indices,
						    size_t index_count,
						    subtilis_error_t *err);
typedef void (*subtilis_type_if_set_t)(subtilis_parser_t *p,
					const char *var_name,
					const subtilis_type_t *type,
					size_t mem_reg, size_t loc,
					subtilis_exp_t *e,
					subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_call_t)(subtilis_parser_t *p,
						   const subtilis_type_t *type,
						   subtilis_ir_arg_t *args,
						   size_t num_args,
						   subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_call_ptr_t)(
	subtilis_parser_t *p, const subtilis_type_t *type,
	subtilis_ir_arg_t *args, size_t num_args,
	size_t ptr, subtilis_error_t *err);

typedef subtilis_exp_t *(*subtilis_type_if_coerce_t)(
	subtilis_parser_t *p, subtilis_exp_t *e,
	const subtilis_type_t *type,
	subtilis_error_t *err);

/* clang-format on */

typedef void (*subtilis_type_if_print_t)(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err);

typedef void (*subtilis_type_if_swap_t)(subtilis_parser_t *p,
					const subtilis_type_t *type,
					size_t reg1, size_t reg2,
					subtilis_error_t *err);

struct subtilis_type_if_ {
	bool is_const;
	bool is_numeric;
	bool is_integer;
	bool is_array;
	bool is_vector;
	subtilis_ir_reg_type_t param_type;
	subtilis_type_if_size_t size;
	subtilis_type_if_unary_t data_size;
	subtilis_type_if_zero_t zero;
	subtilis_type_if_zeroref_t zero_ref;
	subtilis_type_if_initref_t new_ref;
	subtilis_type_if_initref_t assign_ref;
	subtilis_type_if_initref_t assign_ref_no_rc;
	subtilis_type_if_none_t top_bit;
	subtilis_type_if_zeroreg_t zero_reg;
	subtilis_type_if_reg2_t copy_ret;
	subtilis_type_if_typeof_t const_of;
	subtilis_type_if_typeof_t array_of;
	subtilis_type_if_typeof_t vector_of;
	subtilis_type_if_typeof_t element_type;
	subtilis_type_if_unary_t exp_to_var;
	subtilis_type_if_unary_t copy_var;
	subtilis_type_if_dup_t dup;
	subtilis_type_if_copy_collection_t copy_col;
	subtilis_type_if_sizet_exp_t assign_reg;
	subtilis_type_if_sizet2_exp_t assign_mem;
	subtilis_type_if_sizet2_exp_t assign_new_mem;
	subtilis_type_if_iwrite_t indexed_write;
	subtilis_type_if_iwrite_t indexed_add;
	subtilis_type_if_iwrite_t indexed_sub;
	subtilis_type_if_iread_t indexed_read;
	subtilis_type_if_set_t set;
	subtilis_type_if_reg2_t zero_buf;
	subtilis_type_if_iread_t indexed_address;
	subtilis_type_if_copy_collection_t append;
	subtilis_type_if_load_t load_mem;
	subtilis_type_if_unary_t to_int32;
	subtilis_type_if_coerce_t zerox;
	subtilis_type_if_unary_t to_byte;
	subtilis_type_if_unary_t to_float64;
	subtilis_type_if_unary_t to_string;
	subtilis_type_if_unary_t to_hex_string;
	subtilis_type_if_coerce_t coerce;
	subtilis_type_if_unary_t unary_minus;
	subtilis_type_if_binary_nc_t add;
	subtilis_type_if_binary_t mul;
	subtilis_type_if_binary_t and;

	/* clang-format off */
	subtilis_type_if_binary_t or;
	subtilis_type_if_unary_t not;
	/* clang-format on */

	subtilis_type_if_binary_t eor;
	subtilis_type_if_binary_t eq;
	subtilis_type_if_binary_t neq;
	subtilis_type_if_binary_nc_t sub;
	subtilis_type_if_binary_t div;
	subtilis_type_if_binary_t mod;
	subtilis_type_if_binary_nc_t divr;
	subtilis_type_if_binary_nc_t gt;
	subtilis_type_if_binary_nc_t lte;
	subtilis_type_if_binary_nc_t lt;
	subtilis_type_if_binary_nc_t gte;
	subtilis_type_if_binary_nc_t pow;
	subtilis_type_if_binary_t lsl;
	subtilis_type_if_binary_t lsr;
	subtilis_type_if_binary_t asr;
	subtilis_type_if_unary_t abs;
	subtilis_type_if_unary_t sgn;
	subtilis_type_if_unary_t is_inf;
	subtilis_type_if_call_t call;
	subtilis_type_if_call_ptr_t call_ptr;
	subtilis_type_if_reg_t ret;
	subtilis_type_if_print_t print;
	subtilis_type_if_destruct_t destructor;
	subtilis_type_if_swap_t swap_reg_reg;
	subtilis_type_if_swap_t swap_reg_mem;
	subtilis_type_if_swap_t swap_mem_mem;
};

typedef struct subtilis_type_if_ subtilis_type_if;

/*
 * Returns the size of the type in bytes.  For reference types
 * this is the size of the reference, e.g., 16 bytes for a one
 * dimensional array on 32 bit builds.
 */

size_t subtilis_type_if_size(const subtilis_type_t *type,
			     subtilis_error_t *err);

/*
 * For reference types only.  Returns the size of the data pointed
 * to by the reference.  For an array this is the number of elements
 * multiplied by the element size, as returned by
 * subtilis_type_if_size.
 */

subtilis_exp_t *subtilis_type_if_data_size(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   subtilis_exp_t *e,
					   subtilis_error_t *err);

/*
 * Returns an expression containing the zero value of a value type.
 * Not implemented for reference types.
 */

subtilis_exp_t *subtilis_type_if_zero(subtilis_parser_t *p,
				      const subtilis_type_t *type,
				      subtilis_error_t *err);

/*
 * Initialises a new reference type identified by the mem_reg and loc parameters
 * to its zero value.
 */

void subtilis_type_if_zero_ref(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, bool push, subtilis_error_t *err);

/*
 * Initialises a new reference type from a value returned from a function.
 * Here we sort of adopt the returned reference, i.e., we don't decrement
 * its count when the called function exits, nor do we increase it.  We do
 * need to add it to the cleanup stack of the calling function though.
 */

void subtilis_type_if_copy_ret(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t dest_reg,
			       size_t source_reg, subtilis_error_t *err);

/*
 * Initialises a new reference type identified by the mem_reg and loc parameters
 * to the value contained in e.
 */

void subtilis_type_if_new_ref(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t mem_reg, size_t loc, subtilis_exp_t *e,
			      subtilis_error_t *err);

/*
 * Initialises an existing reference type identified by the mem_reg and
 * loc parameters to the value contained in e.
 */

void subtilis_type_if_assign_ref(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, subtilis_exp_t *e,
				 subtilis_error_t *err);

/*
 * Similar to subtilis_type_if_assign_ref except no reference counting logic
 * code is generated.  That means that no code is generated to deref the
 * existing object pointed to by mem_reg and loc and no code is generated to
 * ref the object e.  This is useful for cases where the compiler is certain
 * that it is safe to copy an object without the need to mess with the
 * reference counts, for example in the range loop.
 */

void subtilis_type_if_assign_ref_no_rc(subtilis_parser_t *p,
				       const subtilis_type_t *type,
				       size_t mem_reg, size_t loc,
				       subtilis_exp_t *e,
				       subtilis_error_t *err);

/*
 * Only defined for integer types.  Returns an expression containing an
 * integer of the appropriate type with its top bit (its sign bit) set.
 */

subtilis_exp_t *subtilis_type_if_top_bit(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 subtilis_error_t *err);

/*
 * Initialises a register of the correct type to its zero value.  For
 * example, for a 32 bit integer this function would generate a
 * movii32 reg, 0.  For reference types we treat the register as an
 * integer register and set it to zero.
 */

void subtilis_type_if_zero_reg(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t reg,
			       subtilis_error_t *err);

/*
 * Returns the const type of a type
 */

void subtilis_type_if_const_of(const subtilis_type_t *type,
			       subtilis_type_t *const_type,
			       subtilis_error_t *err);

/*
 * Writes the type of an array of the scalar type specified in
 * element_type into type.
 */

void subtilis_type_if_array_of(const subtilis_type_t *element_type,
			       subtilis_type_t *type, subtilis_error_t *err);

/*
 * Writes the type of a vector of the scalar type specified in
 * element_type into type.
 */

void subtilis_type_if_vector_of(const subtilis_type_t *element_type,
				subtilis_type_t *type, subtilis_error_t *err);

/*
 * Returns true if type is an array of numeric types of a string.
 */

bool subtilis_type_if_is_scalar_ref(const subtilis_type_t *type,
				    subtilis_error_t *err);

/*
 * Writes the type of the elements contained within an array of type "type"
 * into element_type.
 */

void subtilis_type_if_element_type(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_type_t *element_type,
				   subtilis_error_t *err);

/*
 * Returns a register expression that contains a copy of the value
 * in e.  If e is already a register expression, e is returned.
 */

subtilis_exp_t *subtilis_type_if_exp_to_var(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err);

/*
 * Returns a copy of the register expression e.  Used in for loops
 * for the step variable, and when initialising a new local scalar
 * variable from an existing variable.
 */

subtilis_exp_t *subtilis_type_if_copy_var(subtilis_parser_t *p,
					  subtilis_exp_t *e,
					  subtilis_error_t *err);

/*
 * Returns a duplicate of the expression e.  e does not need to be
 * a register expression.
 */

subtilis_exp_t *subtilis_type_if_dup(subtilis_exp_t *e, subtilis_error_t *err);

/*
 * Only implemented for arrays and strings.  Copies the contents of e2 into
 * e1.  e1 and e2 can be of different types provided that they are both
 * either strings or arrays of scalar types.
 */

void subtilis_type_if_memcpy(subtilis_parser_t *p, subtilis_exp_t *e1,
			     subtilis_exp_t *e2, subtilis_error_t *err);

/*
 * Assigns the value in expression e to the given register of the appropriate
 * type.  If e represents a reference type, e's reference is incremented.
 */

void subtilis_type_if_assign_to_reg(subtilis_parser_t *p, size_t reg,
				    subtilis_exp_t *e, subtilis_error_t *err);

/*
 * Assigns the value in expression e to the memory location represented by
 * mem_reg and offset loc.  Not implemented for arrays.
 */

void subtilis_type_if_assign_to_mem(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err);

/*
 * Identical to subtilis_type_if_assign_to_mem for numeric types.  It's
 * different for referenced counted types in that it doesn't try to
 * dereference whatever is at the destination location.  This is inteded
 * to be invoked on newly allocated memory that has never had a Subtilis
 * object.
 */

void subtilis_type_if_assign_to_new_mem(subtilis_parser_t *p, size_t mem_reg,
					size_t loc, subtilis_exp_t *e,
					subtilis_error_t *err);

/*
 * Writes the scalar expression represented by e to an array identified by
 * mem_reg and loc, to the location identified by the indices array.  Compile
 * and runtime bounds checking is performed.  Not implemented for scalar types.
 */

void subtilis_type_if_indexed_write(subtilis_parser_t *p, const char *var_name,
				    const subtilis_type_t *type, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_exp_t **indices,
				    size_t index_count, subtilis_error_t *err);

/*
 * Adds the scalar expression represented by e to an element, identified by
 * indices, of an array identified by mem_reg and loc.  The result is stored
 * in the eleemnt represented by indices.  Compile and runtime bounds checking
 * is performed.  Not implemented for scalar types.
 */

void subtilis_type_if_indexed_add(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_exp_t **indices, size_t index_count,
				  subtilis_error_t *err);

/*
 * Subtract the scalar expression represented by e from an element, identified
 * by indices, of an array identified by mem_reg and loc.  The result is stored
 * in the eleemnt represented by indices.  Compile and runtime bounds checking
 * is performed.  Not implemented for scalar types.
 */

void subtilis_type_if_indexed_sub(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_exp_t **indices, size_t index_count,
				  subtilis_error_t *err);

/*
 * Returns the scalar element, identified by the indices array, of an array
 * identified by mem_reg and loc.  Compile and runtime bounds checking is
 * performed.  Not implemented for scalar types.
 */

subtilis_exp_t *
subtilis_type_if_indexed_read(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t **indices,
			      size_t index_count, subtilis_error_t *err);

/*
 * Only implemented for arrays.  Sets every element of the array to the
 * value e.
 */

void subtilis_type_if_array_set(subtilis_parser_t *p, const char *var_name,
				const subtilis_type_t *type, size_t mem_reg,
				size_t loc, subtilis_exp_t *val,
				subtilis_error_t *err);

/*
 * Only implemented for arrays and vectors.  Sets every element of the array to
 * its zero value.  data points to the allocated data buffer, not the array or
 * vector.  size is the size in bytes of the buffer.
 */

void subtilis_type_if_zero_buf(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t data_reg,
			       size_t size_reg, subtilis_error_t *err);

/*
 * Returns a pointer to the scalar element, identified by the indices array,
 * of an array identified by mem_reg and loc.  Compile and runtime bounds
 * checking isperformed.  Not implemented for scalar types.
 */

subtilis_exp_t *
subtilis_type_if_indexed_address(subtilis_parser_t *p, const char *var_name,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, subtilis_exp_t **indices,
				 size_t index_count, subtilis_error_t *err);

/*
 * Appends a2 to a1.  After the operation has been performed, a2 may point to
 * a different memory block, depending on whether a realloc was required.  Any
 * variables pointed to the original memory block will be unaffected, e.g.,
 *
 * dim a%(10)
 * b% := a%()
 * append(a%(), 11)
 *
 * b% will still have 10 elements and will still point to the original block
 * of data created by dim a%.  a% may or may not point to this original block
 * of data.  Only implemented for strings and 1d arrays.
 */

void subtilis_type_if_append(subtilis_parser_t *p, subtilis_exp_t *a1,
			     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the scalar value stored in the memory location represented by
 * mem_reg and offset loc.  Not implemented for arrays.
 */

subtilis_exp_t *subtilis_type_if_load_from_mem(subtilis_parser_t *p,
					       const subtilis_type_t *type,
					       size_t mem_reg, size_t loc,
					       subtilis_error_t *err);

/*
 * Converts a scalar type to an int.  The constant status of e is
 * perseved.  So if e is a constant double, a constant integer expression,
 * is returned.
 */

subtilis_exp_t *subtilis_type_if_to_int(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err);

/*
 * Converts a scalar type to a a byte.  As there are no constant bytes this
 * function will assert if e is a constant.
 */

subtilis_exp_t *subtilis_type_if_to_byte(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err);

/*
 * Converts a scalar type to a float64.  The constant status of e is
 * perseved.  So if e is a constant integer, a constant double expression,
 * is returned.
 */

subtilis_exp_t *subtilis_type_if_to_float64(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err);

/*
 * Converts an integer type (currently this has to be a byte) to a
 * larger integer, whose type is specified by type, by zero-extending.
 */

subtilis_exp_t *subtilis_type_if_zerox(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err);

/*
 * Returns a string representation of expression e
 */

subtilis_exp_t *subtilis_type_if_to_string(subtilis_parser_t *p,
					   subtilis_exp_t *e,
					   subtilis_error_t *err);

/*
 * Returns a hex string representation of expression e
 */

subtilis_exp_t *subtilis_type_if_to_hex_string(subtilis_parser_t *p,
					       subtilis_exp_t *e,
					       subtilis_error_t *err);

/*
 * Unary minus for scalar types.
 */

subtilis_exp_t *subtilis_type_if_unary_minus(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err);

/*
 * Returns the sum of two expressions.
 */

subtilis_exp_t *subtilis_type_if_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the product of two expressions.
 */

subtilis_exp_t *subtilis_type_if_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs a bitwise AND on two expressions and returns the result.
 */

subtilis_exp_t *subtilis_type_if_and(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs a bitwise OR on two expressions and returns the result.
 */

subtilis_exp_t *subtilis_type_if_or(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs a bitwise XOR on two expressions and returns the result.
 */

subtilis_exp_t *subtilis_type_if_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns a bitwise NOT of the expression.
 */

subtilis_exp_t *subtilis_type_if_not(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err);

/*
 * Compares two expressions for equality returning TRUE if equal or FALSE
 * if not.
 */

subtilis_exp_t *subtilis_type_if_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns FALSE if the two expressions are equal, TRUE if not.
 */

subtilis_exp_t *subtilis_type_if_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the difference of two expressions.
 */

subtilis_exp_t *subtilis_type_if_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the integer quotient of two expressions.
 */

subtilis_exp_t *subtilis_type_if_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the integer modulo of two expressions.
 */

subtilis_exp_t *subtilis_type_if_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the quotient of two expressions.  The result is a floating point
 * number.
 */

subtilis_exp_t *subtilis_type_if_divr(subtilis_parser_t *p, subtilis_exp_t *a1,
				      subtilis_exp_t *a2,
				      subtilis_error_t *err);

/*
 * Returns TRUE if a1 is greater than a2.
 */

subtilis_exp_t *subtilis_type_if_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns TRUE if a1 is less than or equal to a2.
 */

subtilis_exp_t *subtilis_type_if_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns TRUE if a1 is less than a2.
 */

subtilis_exp_t *subtilis_type_if_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns TRUE if a1 is greater than or equal to a2.
 */

subtilis_exp_t *subtilis_type_if_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns pow(a1, a2)
 */

subtilis_exp_t *subtilis_type_if_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs a logical shift left of a1 by a2 and returns the result, which will
 * be an integer.
 */

subtilis_exp_t *subtilis_type_if_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs a logical shift right of a1 by a2 and returns the result, which will
 * be an integer.
 */

subtilis_exp_t *subtilis_type_if_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Performs an arithmetic shift right of a1 by a2 and returns the result, which
 * will be an integer.
 */

subtilis_exp_t *subtilis_type_if_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

/*
 * Returns the absolute value of expression e.
 */

subtilis_exp_t *subtilis_type_if_abs(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err);

/*
 * Returns -1 if e is < 0, 0 if e = 0 and 1 if e > 0.
 */

subtilis_exp_t *subtilis_type_if_sgn(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err);

/*
 * Returns -1 if e is either + or - infinity, 0 otherwise.
 */

subtilis_exp_t *subtilis_type_if_is_inf(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err);

/*
 * Generates a call instruction for a function that returns a value of type
 * type.  A register of the appropriate type is returned.  This register will
 * hold either the return value or a reference to the return value.  Ownership
 * of args is transferred to the call on success.
 */

subtilis_exp_t *subtilis_type_if_call(subtilis_parser_t *p,
				      const subtilis_type_t *type,
				      subtilis_ir_arg_t *args, size_t num_args,
				      subtilis_error_t *err);

/*
 * Generates an indirect call instruction for a function that returns a value of
 * type type.  The address of the function is stored in the register ptr.  A
 * register of the appropriate type is returned.  This register will hold either
 * the return value or a reference to the return value.  Ownership of args is
 * transferred to the call on success.
 */

subtilis_exp_t *subtilis_type_if_call_ptr(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_ir_arg_t *args,
					  size_t num_args, size_t ptr,
					  subtilis_error_t *err);

/*
 * Generate a return instruction for a type of type that returns that
 * value in register reg.
 */

void subtilis_type_if_ret(subtilis_parser_t *p, const subtilis_type_t *type,
			  size_t reg, subtilis_error_t *err);

/*
 * Prints the register expression e to the current output device.
 */

void subtilis_type_if_print(subtilis_parser_t *p, subtilis_exp_t *e,
			    subtilis_error_t *err);

/*
 * Returns true if the given type is const.
 */

bool subtilis_type_if_is_const(const subtilis_type_t *type);

/*
 * Returns true if the given type is a numeric type.
 */

bool subtilis_type_if_is_numeric(const subtilis_type_t *type);

/*
 * Returns true if the given type is an integer of some kind.
 */

bool subtilis_type_if_is_integer(const subtilis_type_t *type);

/*
 * Returns true if the given type is an array of some kind.
 */

bool subtilis_type_if_is_array(const subtilis_type_t *type);

/*
 * Returns true if the given type is a vector of some kind.
 */

bool subtilis_type_if_is_vector(const subtilis_type_t *type);

/*
 * Returns true if the given type is a refernce type,
 * currently a string, vector or array
 */

bool subtilis_type_if_is_reference(const subtilis_type_t *type);

/*
 * Converts expression e to type type, if possible.
 */

subtilis_exp_t *subtilis_type_if_coerce_type(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     const subtilis_type_t *type,
					     subtilis_error_t *err);

/*
 * Returns the type of register used to pass a parameter of type type to
 * functions and procedures.
 */

subtilis_ir_reg_type_t subtilis_type_if_reg_type(const subtilis_type_t *type);

/*
 * Returns the destructor of a reference type.  0 if no destructor is needed.
 */

/*
 * Swaps the values in the register reg1 and reg2.
 */

void subtilis_type_if_swap_reg_reg(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t reg1,
				   size_t reg2, subtilis_error_t *err);

/*
 * Swaps the values in the register reg1 and the memory location pointed to
 * by reg2.
 */

void subtilis_type_if_swap_reg_mem(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t reg1,
				   size_t reg2, subtilis_error_t *err);

/*
 * Swaps the values pointed to by reg1 and reg2.
 */

void subtilis_type_if_swap_mem_mem(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t reg1,
				   size_t reg2, subtilis_error_t *err);

size_t subtilis_type_if_destructor(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_error_t *err);

typedef void (*subtilis_type_cmp_diag_fn_t)(subtilis_parser_t *p,
					    const char *expected,
					    const char *got, void *ud,
					    subtilis_error_t *err);
void subtilis_type_compare_diag_custom(subtilis_parser_t *p,
				       const subtilis_type_t *t1,
				       const subtilis_type_t *t2, void *ud,
				       subtilis_type_cmp_diag_fn_t diag_fn,
				       subtilis_error_t *err);
void subtilis_type_compare_diag(subtilis_parser_t *p, const subtilis_type_t *t1,
				const subtilis_type_t *t2,
				subtilis_error_t *err);
size_t subtilis_type_if_destruct_deref(subtilis_parser_t *p,
				       const subtilis_type_t *type,
				       subtilis_error_t *err);

#endif
