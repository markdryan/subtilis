/*
 * Copyright (c) 2021 Mark Ryan
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "builtins_ir.h"
#include "byte_type_if.h"
#include "int32_type.h"
#include "reference_type.h"
#include "string_type.h"

static size_t prv_size(const subtilis_type_t *type) { return 1; }

/*
 * subtilis_type_const_byte is only a partial type.  It's not possible
 * for the programmer to create one of these for general use.  They
 * only exist to allow the initialisation of byte arrays.
 */

/* clang-format off */
subtilis_type_if subtilis_type_const_byte = {
	.is_const = true,
	.is_numeric = true,
	.is_integer = true,
	.is_array = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = NULL,
	.zero = NULL,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.top_bit = NULL,
	.zero_reg = NULL,
	.copy_ret = NULL,
	.const_of = NULL,
	.array_of = NULL,
	.element_type = NULL,
	.exp_to_var = NULL,
	.copy_var = NULL,
	.dup = NULL,
	.assign_reg = NULL,
	.assign_mem = NULL,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.set = NULL,
	.indexed_address = NULL,
	.load_mem = NULL,
	.to_int32 = NULL,
	.zerox = NULL,
	.to_byte = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.coerce = NULL,
	.unary_minus = NULL,
	.add = NULL,
	.mul = NULL,
	.and = NULL,
	.or = NULL,
	.eor = NULL,
	.not = NULL,
	.eq = NULL,
	.neq = NULL,
	.sub = NULL,
	.div = NULL,
	.mod = NULL,
	.gt = NULL,
	.lte = NULL,
	.lt = NULL,
	.gte = NULL,
	.pow = NULL,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = NULL,
	.sgn = NULL,
	.is_inf = NULL,
	.call = NULL,
	.ret = NULL,
	.print = NULL,
	.destructor = NULL,
};

/* clang-format on */

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static subtilis_exp_t *prv_zero(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;

	op1.integer = 0;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_byte_var(reg_num, err);
}

static void prv_const_of(const subtilis_type_t *type,
			 subtilis_type_t *const_type)
{
	const_type->type = SUBTILIS_TYPE_CONST_BYTE;
}

static subtilis_exp_t *prv_top_bit(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;

	op1.integer = (int32_t)0x80;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_byte_var(reg_num, err);
}

static void prv_zero_reg(subtilis_parser_t *p, size_t reg,
			 subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = reg;
	op1.integer = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, op1, err);
}

static void prv_array_of(const subtilis_type_t *element_type,
			 subtilis_type_t *type)
{
	type->type = SUBTILIS_TYPE_ARRAY_BYTE;
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_copy_var(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e2;

	reg = subtilis_ir_section_add_instr2(p->current, SUBTILIS_OP_INSTR_MOV,
					     e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e2 = subtilis_exp_new_byte_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	subtilis_exp_delete(e);
	return e2;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_to_int32(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_SIGNX_8_TO_32, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->type.type = SUBTILIS_TYPE_INTEGER;
	e->exp.ir_op.reg = reg;
	return e;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_zerox(subtilis_parser_t *p, subtilis_exp_t *e,
				 subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op;

	op.integer = 255;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, e->exp.ir_op, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->type.type = SUBTILIS_TYPE_INTEGER;
	e->exp.ir_op.reg = reg;
	return e;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_zerox_from_type(subtilis_parser_t *p,
					   subtilis_exp_t *e,
					   const subtilis_type_t *type,
					   subtilis_error_t *err)
{
	if (type->type != SUBTILIS_TYPE_INTEGER) {
		subtilis_error_set_bad_zero_extend(
		    err, subtilis_type_name(&e->type), subtilis_type_name(type),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return prv_zerox(p, e, err);
}

static void prv_assign_to_reg(subtilis_parser_t *p, size_t reg,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_MOV,
					  op0, e->exp.ir_op, op2, err);
	subtilis_exp_delete(e);
}

static void prv_assign_to_mem(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I8,
					  e->exp.ir_op, op1, op2, err);
	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_load_from_mem(subtilis_parser_t *p, size_t mem_reg,
					 size_t loc, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t reg;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_byte_var(reg, err);
}

static subtilis_exp_t *prv_to_byte(subtilis_parser_t *p, subtilis_exp_t *e,
				   subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_to_float64(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	size_t reg;

	e = prv_to_int32(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->type.type = SUBTILIS_TYPE_REAL;
	e->exp.ir_op.reg = reg;

	return e;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_to_string(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	e = prv_to_int32(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_type_if_to_string(p, e, err);
}

static subtilis_exp_t *prv_to_hex_string(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err)
{
	e = prv_zerox(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_type_if_to_hex_string(p, e, err);
}

static subtilis_exp_t *prv_coerce_type(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_REAL:
		e = prv_to_float64(p, e, err);
		break;
	case SUBTILIS_TYPE_BYTE:
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = prv_to_int32(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		break;
	default:
		subtilis_error_set_bad_conversion(
		    err, subtilis_type_name(&e->type), subtilis_type_name(type),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_unary_minus(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t operand;

	operand.integer = 0;
	reg = subtilis_ir_section_add_instr(p->current,
					    SUBTILIS_OP_INSTR_RSUBI_I32,
					    e->exp.ir_op, operand, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->exp.ir_op.reg = reg;
	return e;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_commutative(subtilis_parser_t *p, subtilis_exp_t *a1,
				       subtilis_exp_t *a2,
				       subtilis_op_instr_type_t in_var_var,
				       subtilis_type_if_binary_t binary_fn,
				       subtilis_error_t *err)
{
	size_t reg;

	if (a2->type.type == SUBTILIS_TYPE_BYTE) {
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_var, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		subtilis_exp_delete(a2);
		return a1;
	}

	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	return binary_fn(p, a1, a2, err);

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *
prv_commutative_nc(subtilis_parser_t *p, subtilis_exp_t *a1, subtilis_exp_t *a2,
		   subtilis_op_instr_type_t in_var_var,
		   subtilis_type_if_binary_nc_t binary_nc_fn, bool swapped,
		   subtilis_error_t *err)
{
	size_t reg;

	if (a2->type.type == SUBTILIS_TYPE_BYTE) {
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_var, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		subtilis_exp_delete(a2);
		return a1;
	}

	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	return binary_nc_fn(p, a1, a2, swapped, err);

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_compare(subtilis_parser_t *p, subtilis_exp_t *a1,
				   subtilis_exp_t *a2,
				   subtilis_op_instr_type_t in_var_var,
				   subtilis_type_if_binary_t binary_fn,
				   subtilis_error_t *err)
{
	size_t reg;

	/*
	 * Just used for eq and neq.   Here we can zero extend instead of sign
	 * extending which may be cheaper.
	 */

	if (a2->type.type == SUBTILIS_TYPE_BYTE) {
		a1 = prv_zerox(p, a1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		a2 = prv_zerox(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_var, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		subtilis_exp_delete(a2);
		return a1;
	}

	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	return binary_fn(p, a1, a2, err);

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_compare_nc(subtilis_parser_t *p, subtilis_exp_t *a1,
				      subtilis_exp_t *a2,
				      subtilis_type_if_binary_nc_t binary_nc_fn,
				      bool swapped, subtilis_error_t *err)
{
	/*
	 * Here we have to sign extend otherwise comparsions will not work
	 * properly on negative values.
	 */

	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	return binary_nc_fn(p, a1, a2, swapped, err);

on_error:

	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_commutative_nc(p, a1, a2, SUBTILIS_OP_INSTR_ADD_I32,
				  subtilis_type_int32.add, swapped, err);
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_MUL_I32,
			       subtilis_type_int32.mul, err);
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_AND_I32,
			       subtilis_type_int32.and, err);
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_OR_I32,
			       subtilis_type_int32.or, err);
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_EOR_I32,
			       subtilis_type_int32.eor, err);
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t operand;

	operand.integer = 0;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_NOT_I32, e->exp.ir_op, operand, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->exp.ir_op.reg = reg;
	return e;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, SUBTILIS_OP_INSTR_EQ_I32,
			   subtilis_type_int32.eq, err);
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, SUBTILIS_OP_INSTR_NEQ_I32,
			   subtilis_type_int32.neq, err);
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_commutative_nc(p, a1, a2, SUBTILIS_OP_INSTR_SUB_I32,
				  subtilis_type_int32.sub, swapped, err);
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return subtilis_type_int32.div(p, a1, a2, err);

on_error:

	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return subtilis_type_int32.mod(p, a1, a2, err);

on_error:

	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	return prv_compare_nc(p, a1, a2, subtilis_type_int32.gt, swapped, err);
}

static subtilis_exp_t *prv_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_compare_nc(p, a1, a2, subtilis_type_int32.lte, swapped, err);
}

static subtilis_exp_t *prv_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	return prv_compare_nc(p, a1, a2, subtilis_type_int32.lt, swapped, err);
}

static subtilis_exp_t *prv_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_compare_nc(p, a1, a2, subtilis_type_int32.gte, swapped, err);
}

static subtilis_exp_t *prv_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_compare_nc(p, a1, a2, subtilis_type_int32.pow, swapped, err);
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->type.type = SUBTILIS_TYPE_INTEGER;
	a1 = subtilis_type_int32.lsl(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	a1->type.type = SUBTILIS_TYPE_BYTE;
	return a1;
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->type.type = SUBTILIS_TYPE_INTEGER;
	a1 = subtilis_type_int32.lsr(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	a1->type.type = SUBTILIS_TYPE_BYTE;
	return a1;
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	/*
	 * This one needs to be sign extended.
	 */

	a1 = prv_to_int32(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a2);
		return NULL;
	}
	a1 = subtilis_type_int32.asr(p, a1, a2, err);
	a1->type.type = SUBTILIS_TYPE_BYTE;
	return a1;
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	/*
	 * This one needs to be sign extended.
	 */

	e = prv_to_int32(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = (subtilis_type_int32.abs)(p, e, err);
	e->type.type = SUBTILIS_TYPE_BYTE;
	return e;
}

static subtilis_exp_t *prv_sgn(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	/*
	 * This one needs to be sign extended.
	 */

	e = prv_to_int32(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_int32.sgn(p, e, err);
}

static subtilis_exp_t *prv_is_inf(subtilis_parser_t *p, subtilis_exp_t *e,
				  subtilis_error_t *err)
{
	e->exp.ir_op.integer = 0;
	e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
	return e;
}

static subtilis_exp_t *prv_call(subtilis_parser_t *p,
				const subtilis_type_t *type,
				subtilis_ir_arg_t *args, size_t num_args,
				subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_i32_call(p->current, num_args, args, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(type, reg, err);
}

static void prv_ret(subtilis_parser_t *p, size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t ret_reg;

	ret_reg.reg = reg;
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_RET_I32, ret_reg, err);
}

static void prv_print(subtilis_parser_t *p, subtilis_exp_t *e,
		      subtilis_error_t *err)
{
	/*
	 * This one needs to be sign extended.
	 */

	e = prv_to_int32(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_int32.print(p, e, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_byte_if = {
	.is_const = false,
	.is_numeric = true,
	.is_integer = true,
	.is_array = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.top_bit = prv_top_bit,
	.zero_reg = prv_zero_reg,
	.copy_ret = NULL,
	.const_of = prv_const_of,
	.array_of = prv_array_of,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var,
	.copy_var = prv_copy_var,
	.dup = prv_dup,
	.assign_reg = prv_assign_to_reg,
	.assign_mem = prv_assign_to_mem,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.indexed_address = NULL,
	.load_mem = prv_load_from_mem,
	.to_int32 = prv_to_int32,
	.zerox = prv_zerox_from_type,
	.to_byte = prv_to_byte,
	.to_float64 = prv_to_float64,
	.to_string = prv_to_string,
	.to_hex_string = prv_to_hex_string,
	.coerce = prv_coerce_type,
	.unary_minus = prv_unary_minus,
	.add = prv_add,
	.mul = prv_mul,
	.and = prv_and,
	.or = prv_or,
	.eor = prv_eor,
	.not = prv_not,
	.eq = prv_eq,
	.neq = prv_neq,
	.sub = prv_sub,
	.div = prv_div,
	.mod = prv_mod,
	.gt = prv_gt,
	.lte = prv_lte,
	.lt = prv_lt,
	.gte = prv_gte,
	.pow = prv_pow,
	.lsl = prv_lsl,
	.lsr = prv_lsr,
	.asr = prv_asr,
	.abs = prv_abs,
	.sgn = prv_sgn,
	.is_inf = prv_is_inf,
	.call = prv_call,
	.ret = prv_ret,
	.print = prv_print,
	.destructor = NULL,
};

/* clang-format on */
