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

#include <math.h>

#include "builtins_ir.h"
#include "float64_type.h"
#include "parser_exp.h"

static size_t prv_size(const subtilis_type_t *type) { return 8; }

static subtilis_exp_t *prv_zero_const(subtilis_parser_t *p,
				      subtilis_error_t *err)
{
	return subtilis_exp_new_real(0.0, err);
}

static void prv_const_of(const subtilis_type_t *type,
			 subtilis_type_t *const_type)
{
	const_type->type = SUBTILIS_TYPE_CONST_REAL;
}

static subtilis_exp_t *prv_returne(subtilis_parser_t *p, subtilis_exp_t *e,
				   subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_exp_to_var_const(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e2;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e2 = subtilis_exp_new_real_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	subtilis_exp_delete(e);

	return e2;

on_error:
	subtilis_exp_delete(e);
	return NULL;
}

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static subtilis_exp_t *prv_copy_const(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_assign_to_reg_const(subtilis_parser_t *p, size_t reg,
				    subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_MOVI_REAL, op0,
					  e->exp.ir_op, op2, err);
	subtilis_exp_delete(e);
}

static void prv_assign_to_mem_const(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op0.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_REAL, op0, op1, op2, err);

cleanup:
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
	    p->current, SUBTILIS_OP_INSTR_LOADO_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_real_var(reg, err);
}

static subtilis_exp_t *prv_to_int32_const(subtilis_parser_t *p,
					  subtilis_exp_t *e,
					  subtilis_error_t *err)
{
	e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
	e->exp.ir_op.integer = (int32_t)e->exp.ir_op.real;
	return e;
}

static subtilis_exp_t *prv_coerce_type_const(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     const subtilis_type_t *type,
					     subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_CONST_REAL:
		break;
	case SUBTILIS_TYPE_CONST_INTEGER:
		e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		e->exp.ir_op.integer = (int32_t)e->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_REAL:
		e = subtilis_type_if_exp_to_var(p, e, err);
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		e = subtilis_type_if_to_int(p, e, err);
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

static subtilis_exp_t *prv_unary_minus_const(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err)
{
	e->exp.ir_op.real = -e->exp.ir_op.real;
	return e;
}

static subtilis_exp_t *prv_add_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.real =
		    ((double)a2->exp.ir_op.integer) + a1->exp.ir_op.real;
		a1->type.type = SUBTILIS_TYPE_CONST_REAL;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real += a2->exp.ir_op.real;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_mul_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.real =
		    ((double)a2->exp.ir_op.integer) * a1->exp.ir_op.real;
		a1->type.type = SUBTILIS_TYPE_CONST_REAL;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real *= a2->exp.ir_op.real;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static void prv_args_to_int(subtilis_parser_t *p, subtilis_exp_t *a1,
			    subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1 = subtilis_type_if_to_int(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a2);
		return;
	}

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return;
	}
}

static subtilis_exp_t *prv_and_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	a1->exp.ir_op.integer &= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_or_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	a1->exp.ir_op.integer |= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_eor_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	a1->exp.ir_op.integer ^= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_not_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
	e->exp.ir_op.integer = ~(int32_t)e->exp.ir_op.real;
	return e;
}

static subtilis_exp_t *prv_eq_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    (a1->exp.ir_op.real) == (double)a2->exp.ir_op.integer ? -1
									  : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real == a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_neq_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real != ((double)a2->exp.ir_op.integer) ? -1
									  : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real != a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_sub_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.real =
		    a1->exp.ir_op.real - ((double)a2->exp.ir_op.integer);
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real -= a2->exp.ir_op.real;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_divr_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				      subtilis_exp_t *a2, bool swapped,
				      subtilis_error_t *err)
{
	/* a2 must be const and a float64*/

	if (a2->exp.ir_op.real == 0) {
		subtilis_error_set_divide_by_zero(err, p->l->stream->name,
						  p->l->line);
		subtilis_exp_delete(a2);
		subtilis_exp_delete(a1);
		return NULL;
	}

	a1->exp.ir_op.real /= a2->exp.ir_op.real;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_gt_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, bool swapped,
				    subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real > ((double)a2->exp.ir_op.integer) ? -1
									 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real > a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_lte_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real <= ((double)a2->exp.ir_op.integer) ? -1
									  : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real <= a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_lt_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, bool swapped,
				    subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real < ((double)a2->exp.ir_op.integer) ? -1
									 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real < a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_gte_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real >= ((double)a2->exp.ir_op.integer) ? -1
									  : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real >= a2->exp.ir_op.real ? -1 : 0;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_pow_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (a2->exp.ir_op.integer == 0) {
			a1->exp.ir_op.integer = 1;
			a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
			break;
		} else if (a2->exp.ir_op.integer == 1) {
			break;
		}
		a2->exp.ir_op.real = (double)a2->exp.ir_op.integer;
	case SUBTILIS_TYPE_CONST_REAL:
		if (a2->exp.ir_op.real == 0.0) {
			a1->exp.ir_op.integer = 1;
			a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
			break;
		} else if (a2->exp.ir_op.real == 1) {
			break;
		}
		a1->exp.ir_op.real =
		    pow(a1->exp.ir_op.real, a2->exp.ir_op.real);
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_abs_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	e->exp.ir_op.real = fabs(e->exp.ir_op.real);

	return e;
}

static subtilis_exp_t *prv_sgn_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	if (e->exp.ir_op.real < 0)
		e->exp.ir_op.integer = -1;
	else if (e->exp.ir_op.real > 0)
		e->exp.ir_op.integer = 1;
	else
		e->exp.ir_op.integer = 0;

	e->type.type = SUBTILIS_TYPE_CONST_INTEGER;

	return e;
}

/* clang-format off */
subtilis_type_if subtilis_type_const_float64 = {
	.is_const = true,
	.is_numeric = true,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_REAL,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero_const,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.zero_reg = NULL,
	.const_of = prv_const_of,
	.array_of = NULL,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var_const,
	.copy_var = prv_copy_const,
	.dup = prv_dup,
	.assign_reg = prv_assign_to_reg_const,
	.assign_mem = prv_assign_to_mem_const,
	.indexed_write = NULL,
	.indexed_read = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.load_mem = NULL,
	.to_int32 = prv_to_int32_const,
	.to_float64 = prv_returne,
	.coerce = prv_coerce_type_const,
	.unary_minus = prv_unary_minus_const,
	.add = prv_add_const,
	.mul = prv_mul_const,
	.and = prv_and_const,
	.or = prv_or_const,
	.eor = prv_eor_const,
	.not = prv_not_const,
	.eq = prv_eq_const,
	.neq = prv_neq_const,
	.sub = prv_sub_const,
	.div = NULL,
	.mod = NULL,
	.divr = prv_divr_const,
	.gt = prv_gt_const,
	.lte = prv_lte_const,
	.lt = prv_lt_const,
	.gte = prv_gte_const,
	.pow = prv_pow_const,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = prv_abs_const,
	.sgn = prv_sgn_const,
	.call = NULL,
	.ret = NULL,
	.print = NULL,
	.destructor = NULL,
};

/* clang-format on */

static subtilis_exp_t *prv_zero(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;

	op1.real = 0;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_real_var(reg_num, err);
}

static void prv_zero_reg(subtilis_parser_t *p, size_t reg,
			 subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = reg;
	op1.real = 0.0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, op0, op1, err);
}

static void prv_array_of(const subtilis_type_t *element_type,
			 subtilis_type_t *type)
{
	type->type = SUBTILIS_TYPE_ARRAY_REAL;
}

static subtilis_exp_t *prv_copy_var(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e2;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVFP, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e2 = subtilis_exp_new_real_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	subtilis_exp_delete(e);
	return e2;

on_error:
	subtilis_exp_delete(e);
	return NULL;
}

static void prv_assign_to_reg(subtilis_parser_t *p, size_t reg,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_MOVFP,
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
					  SUBTILIS_OP_INSTR_STOREO_REAL,
					  e->exp.ir_op, op1, op2, err);
	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_to_int32(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_FP_I32, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e->type.type = SUBTILIS_TYPE_INTEGER;
	e->exp.ir_op.reg = reg;

	return e;

on_error:
	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_coerce_type(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_CONST_REAL:
		e = subtilis_type_if_exp_to_var(p, e, err);
		break;
	case SUBTILIS_TYPE_CONST_INTEGER:
		e->type.type = SUBTILIS_TYPE_CONST_REAL;
		e->exp.ir_op.real = (double)e->exp.ir_op.integer;
		e = subtilis_type_if_exp_to_var(p, e, err);
		break;
	case SUBTILIS_TYPE_REAL:
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = subtilis_type_if_to_int(p, e, err);
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

	operand.real = 0;
	reg = subtilis_ir_section_add_instr(p->current,
					    SUBTILIS_OP_INSTR_RSUBI_REAL,
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
				       subtilis_op_instr_type_t real_var_imm,
				       subtilis_op_instr_type_t real_var_var,
				       subtilis_error_t *err)
{
	size_t reg;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a2->exp.ir_op.real = (double)a2->exp.ir_op.integer;
	case SUBTILIS_TYPE_CONST_REAL:
		reg = subtilis_ir_section_add_instr(p->current, real_var_imm,
						    a1->exp.ir_op,
						    a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_INTEGER:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, a2->exp.ir_op,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a2->type.type = SUBTILIS_TYPE_REAL;
		a2->exp.ir_op.reg = reg;
	case SUBTILIS_TYPE_REAL:
		reg = subtilis_ir_section_add_instr(p->current, real_var_var,
						    a1->exp.ir_op,
						    a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

/* clang-format off */
static subtilis_exp_t *prv_commutative_logical(
	subtilis_parser_t *p, subtilis_exp_t *a1, subtilis_exp_t *a2,
	subtilis_op_instr_type_t real_var_imm,
	subtilis_op_instr_type_t real_var_var, subtilis_error_t *err)

/* clang-format on */
{
	size_t reg;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a2->exp.ir_op.real = (double)a2->exp.ir_op.integer;
	case SUBTILIS_TYPE_CONST_REAL:
		reg = subtilis_ir_section_add_instr(p->current, real_var_imm,
						    a1->exp.ir_op,
						    a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		a1->type.type = SUBTILIS_TYPE_INTEGER;
		break;
	case SUBTILIS_TYPE_INTEGER:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, a2->exp.ir_op,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a2->exp.ir_op.reg = reg;
	case SUBTILIS_TYPE_REAL:
		reg = subtilis_ir_section_add_instr(p->current, real_var_var,
						    a1->exp.ir_op,
						    a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		a1->type.type = SUBTILIS_TYPE_INTEGER;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_ADDI_REAL,
			       SUBTILIS_OP_INSTR_ADD_REAL, err);
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(p, a1, a2, SUBTILIS_OP_INSTR_MULI_REAL,
			       SUBTILIS_OP_INSTR_MUL_REAL, err);
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_and(p, a1, a2, err);
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_or(p, a1, a2, err);
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	prv_args_to_int(p, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_eor(p, a1, a2, err);
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_not(p, e, err);
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative_logical(p, a1, a2, SUBTILIS_OP_INSTR_EQI_REAL,
				       SUBTILIS_OP_INSTR_EQ_REAL, err);
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative_logical(p, a1, a2, SUBTILIS_OP_INSTR_NEQI_REAL,
				       SUBTILIS_OP_INSTR_NEQ_REAL, err);
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;

	real_var_imm = (swapped) ? SUBTILIS_OP_INSTR_RSUBI_REAL
				 : SUBTILIS_OP_INSTR_SUBI_REAL;

	return prv_commutative(p, a1, a2, real_var_imm,
			       SUBTILIS_OP_INSTR_SUB_REAL, err);
}

static subtilis_exp_t *prv_divr(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, bool swapped,
				subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;
	bool div_by_const = false;
	subtilis_exp_t *e;

	if (swapped) {
		real_var_imm = SUBTILIS_OP_INSTR_RDIVI_REAL;
	} else {
		div_by_const = a2->type.type == SUBTILIS_TYPE_CONST_REAL;
		if (div_by_const && (a2->exp.ir_op.real == 0.0)) {
			subtilis_error_set_divide_by_zero(
			    err, p->l->stream->name, p->l->line);
			subtilis_exp_delete(a1);
			subtilis_exp_delete(a2);
			return NULL;
		}
		real_var_imm = SUBTILIS_OP_INSTR_DIVI_REAL;
	}

	e = prv_commutative(p, a1, a2, real_var_imm, SUBTILIS_OP_INSTR_DIV_REAL,
			    err);

	if (!div_by_const) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(e);
			return NULL;
		}
	}

	return e;
}

static subtilis_exp_t *prv_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;

	real_var_imm = (swapped) ? SUBTILIS_OP_INSTR_LTEI_REAL
				 : SUBTILIS_OP_INSTR_GTI_REAL;

	return prv_commutative(p, a1, a2, real_var_imm,
			       SUBTILIS_OP_INSTR_GT_REAL, err);
}

static subtilis_exp_t *prv_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;

	real_var_imm = (swapped) ? SUBTILIS_OP_INSTR_GTEI_REAL
				 : SUBTILIS_OP_INSTR_LTEI_REAL;

	return prv_commutative(p, a1, a2, real_var_imm,
			       SUBTILIS_OP_INSTR_LTE_REAL, err);
}

static subtilis_exp_t *prv_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;

	real_var_imm = (swapped) ? SUBTILIS_OP_INSTR_GTEI_REAL
				 : SUBTILIS_OP_INSTR_LTI_REAL;

	return prv_commutative(p, a1, a2, real_var_imm,
			       SUBTILIS_OP_INSTR_LT_REAL, err);
}

static subtilis_exp_t *prv_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t real_var_imm;

	real_var_imm = (swapped) ? SUBTILIS_OP_INSTR_LTEI_REAL
				 : SUBTILIS_OP_INSTR_GTEI_REAL;

	return prv_commutative(p, a1, a2, real_var_imm,
			       SUBTILIS_OP_INSTR_GTE_REAL, err);
}

static subtilis_exp_t *prv_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_exp_t *tmp;

	if (!swapped) {
		if (((a2->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
		     (a2->exp.ir_op.integer == 0)) ||
		    (((a2->type.type == SUBTILIS_TYPE_CONST_REAL) &&
		      (a2->exp.ir_op.real == 0.0)))) {
			a1->exp.ir_op.integer = 1;
			a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
			subtilis_exp_delete(a2);
			return a1;
		} else if (((a2->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
			    (a2->exp.ir_op.integer == 1)) ||
			   (((a2->type.type == SUBTILIS_TYPE_CONST_REAL) &&
			     (a2->exp.ir_op.real == 1.0)))) {
			subtilis_exp_delete(a2);
			return a1;
		}
	}

	a2 = subtilis_type_if_exp_to_var(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_INTEGER:
		a2 = subtilis_type_if_to_float64(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	case SUBTILIS_TYPE_REAL:
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	if (swapped) {
		tmp = a1;
		a1 = a2;
		a2 = tmp;
	}

	a1->exp.ir_op.reg =
	    subtilis_ir_section_add_instr(p->current, SUBTILIS_OP_INSTR_POWR,
					  a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_exp_delete(a2);

	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr2(p->current, SUBTILIS_OP_INSTR_ABSR,
					     e->exp.ir_op, err);
	subtilis_exp_delete(e);

	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_real_var(reg, err);

	return e;
}

static subtilis_exp_t *prv_sgn(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t gte_zero;
	size_t reg;
	size_t reg2;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	less_zero.label = subtilis_ir_section_new_label(p->current);
	gte_zero.label = subtilis_ir_section_new_label(p->current);

	op2.real = 0.0;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_REAL, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, less_zero, gte_zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, gte_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg2 = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTI_REAL, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e->exp.ir_op.reg = reg2;
	e->type.type = SUBTILIS_TYPE_INTEGER;

	e = subtilis_type_if_abs(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_reg(p, reg, e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, less_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

static subtilis_exp_t *prv_call(subtilis_parser_t *p,
				const subtilis_type_t *type,
				subtilis_ir_arg_t *args, size_t num_args,
				subtilis_error_t *err)
{
	size_t reg;

	reg =
	    subtilis_ir_section_add_real_call(p->current, num_args, args, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(type, reg, err);
}

static void prv_ret(subtilis_parser_t *p, size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t ret_reg;

	ret_reg.reg = reg;
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_RET_REAL, ret_reg, err);
}

static void prv_print_fp(subtilis_parser_t *p, size_t reg,
			 subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;

	fn = subtilis_builtins_ir_add_1_arg_real(p, "_print_fp",
						 &subtilis_type_void, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_print_fp(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	(void)subtilis_parser_call_1_arg_fn(p, "_print_fp", reg,
					    SUBTILIS_IR_REG_TYPE_REAL,
					    &subtilis_type_void, err);
}

static void prv_print(subtilis_parser_t *p, subtilis_exp_t *e,
		      subtilis_error_t *err)
{
	if (p->caps & SUBTILIS_BACKEND_HAVE_PRINT_FP)
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_FP, e->exp.ir_op, err);
	else
		prv_print_fp(p, e->exp.ir_op.reg, err);

	subtilis_exp_delete(e);
}

/* clang-format off */
subtilis_type_if subtilis_type_float64 = {
	.is_const = false,
	.is_numeric = true,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_REAL,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.zero_reg = prv_zero_reg,
	.const_of = prv_const_of,
	.array_of = prv_array_of,
	.element_type = NULL,
	.exp_to_var = prv_returne,
	.copy_var = prv_copy_var,
	.dup = prv_dup,
	.assign_reg = prv_assign_to_reg,
	.assign_mem = prv_assign_to_mem,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.load_mem = prv_load_from_mem,
	.to_int32 = prv_to_int32,
	.to_float64 = prv_returne,
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
	.div = NULL,
	.mod = NULL,
	.divr = prv_divr,
	.gt = prv_gt,
	.lte = prv_lte,
	.lt = prv_lt,
	.gte = prv_gte,
	.pow = prv_pow,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = prv_abs,
	.sgn = prv_sgn,
	.call = prv_call,
	.ret = prv_ret,
	.print = prv_print,
	.destructor = NULL,
};

/* clang-format on */
