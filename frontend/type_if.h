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

#include "expression.h"
#include "parser.h"

typedef size_t (*subtilis_type_if_size_t)(const subtilis_type_t *type);
typedef void (*subtilis_type_if_typeof_t)(const subtilis_type_t *element_type,
					  subtilis_type_t *type);
typedef subtilis_exp_t *(*subtilis_type_if_none_t)(subtilis_parser_t *p,
						   subtilis_error_t *err);
typedef void (*subtilis_type_if_reg_t)(subtilis_parser_t *p, size_t reg,
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

typedef void (*subtilis_type_if_sizet_exp_t)(subtilis_parser_t *p, size_t reg,
					     subtilis_exp_t *e,
					     subtilis_error_t *err);
typedef void (*subtilis_type_if_sizet2_exp_t)(subtilis_parser_t *p, size_t reg,
					      size_t loc, subtilis_exp_t *e,
					      subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_load_t)(subtilis_parser_t *p,
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
typedef subtilis_exp_t *(*subtilis_type_if_call_t)(subtilis_parser_t *p,
						   const subtilis_type_t *type,
						   subtilis_ir_arg_t *args,
						   size_t num_args,
						   subtilis_error_t *err);

/* clang-format on */

typedef void (*subtilis_type_if_print_t)(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err);

struct subtilis_type_if_ {
	bool is_const;
	bool is_numeric;
	subtilis_type_if_size_t size;
	subtilis_type_if_unary_t data_size;
	subtilis_type_if_none_t zero;
	subtilis_type_if_reg_t zero_reg;
	subtilis_type_if_typeof_t array_of;
	subtilis_type_if_typeof_t element_type;
	subtilis_type_if_unary_t exp_to_var;
	subtilis_type_if_unary_t copy_var;
	subtilis_type_if_dup_t dup;
	subtilis_type_if_sizet_exp_t assign_reg;
	subtilis_type_if_sizet2_exp_t assign_mem;
	subtilis_type_if_iwrite_t indexed_write;
	subtilis_type_if_iwrite_t indexed_add;
	subtilis_type_if_iwrite_t indexed_sub;
	subtilis_type_if_iread_t indexed_read;
	subtilis_type_if_load_t load_mem;
	subtilis_type_if_unary_t to_int32;
	subtilis_type_if_unary_t to_float64;
	subtilis_type_if_unary_t unary_minus;
	subtilis_type_if_binary_t add;
	subtilis_type_if_binary_t mul;
	subtilis_type_if_binary_t and;
	/* clang-format off */
	subtilis_type_if_binary_t or;
	/* clang-format on */

	subtilis_type_if_unary_t not;
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
	subtilis_type_if_binary_t lsl;
	subtilis_type_if_binary_t lsr;
	subtilis_type_if_binary_t asr;
	subtilis_type_if_unary_t abs;
	subtilis_type_if_unary_t sgn;
	subtilis_type_if_call_t call;
	subtilis_type_if_print_t print;
};

typedef struct subtilis_type_if_ subtilis_type_if;

/*
 * Returns the size of the type in bytes.  For reference types
 * this is the size of the reference, e.g., 12 bytes for a one
 * dimensional array on 32 bit bit builds.
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
 * Initialises a register of the correct type to its zero value.  For
 * example, for a 32 bit integer this function would generate a
 * movii32 reg, 0.  For reference types we treat the register as an
 * integer register and set it to zero.
 */

void subtilis_type_if_zero_reg(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t reg,
			       subtilis_error_t *err);

/*
 * Writes the type of an array of the scalar type specified in
 * element_type into type.
 */

void subtilis_type_if_array_of(subtilis_parser_t *p,
			       const subtilis_type_t *element_type,
			       subtilis_type_t *type, subtilis_error_t *err);

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
 * Returns a copy of the register expression e.  Only used in for loops
 * for the step variable.
 */

subtilis_exp_t *subtilis_type_if_copy_var(subtilis_parser_t *p,
					  subtilis_exp_t *e,
					  subtilis_error_t *err);

/*
 * Returns a duplicate of the expression e.  e does not need to be
 * a register expression.  Only implemented for scalar types.
 */

subtilis_exp_t *subtilis_type_if_dup(subtilis_exp_t *e, subtilis_error_t *err);

/*
 * Assigns the value in expression e to the given register of the appropriate
 * type.  Only implemented for scalar types.
 */

void subtilis_type_if_assign_to_reg(subtilis_parser_t *p, size_t reg,
				    subtilis_exp_t *e, subtilis_error_t *err);

/*
 * Assigns the value in expression e to the memory location represented by
 * mem_reg and offset loc.  Only implemented for scalar types.
 */

void subtilis_type_if_assign_to_mem(subtilis_parser_t *p, size_t mem_reg,
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
 * Returns the scalar value stored in the memory location represented by
 * mem_reg and offset loc.  Only implemented for scalar types.
 */

subtilis_exp_t *subtilis_type_if_load_from_mem(subtilis_parser_t *p,
					       const subtilis_type_t *type,
					       size_t mem_reg, size_t loc,
					       subtilis_error_t *err);

/*
 * Converts a scalar type to an int.
 */

subtilis_exp_t *subtilis_type_if_to_int(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err);

/*
 * Converts a scalar type to a float64.
 */

subtilis_exp_t *subtilis_type_if_to_float64(subtilis_parser_t *p,
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

#endif
