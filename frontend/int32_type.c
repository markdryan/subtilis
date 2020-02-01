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
#include <stdlib.h>
#include <string.h>

#include "int32_type.h"

static size_t prv_size(const subtilis_type_t *type) { return 4; }

static subtilis_exp_t *prv_zero_const(subtilis_parser_t *p,
				      subtilis_error_t *err)
{
	return subtilis_exp_new_int32(0, err);
}

static subtilis_exp_t *prv_top_bit_const(subtilis_parser_t *p,
					 subtilis_error_t *err)
{
	return subtilis_exp_new_int32((int32_t)0x80000000, err);
}

static void prv_const_of(const subtilis_type_t *type,
			 subtilis_type_t *const_type)
{
	const_type->type = SUBTILIS_TYPE_CONST_INTEGER;
}

static subtilis_exp_t *prv_exp_to_var_const(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e2;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	e2 = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	subtilis_exp_delete(e);
	return e2;

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_copy_const(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static void prv_assign_to_reg_const(subtilis_parser_t *p, size_t reg,
				    subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_MOVI_I32, op0,
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
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);

cleanup:

	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_to_int32(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_to_float64_const(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	e->type.type = SUBTILIS_TYPE_CONST_REAL;
	e->exp.ir_op.real = (double)e->exp.ir_op.integer;
	return e;
}

static subtilis_exp_t *prv_coerce_type_const(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     const subtilis_type_t *type,
					     subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_CONST_REAL:
		e->type.type = SUBTILIS_TYPE_CONST_REAL;
		e->exp.ir_op.real = (double)e->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_CONST_INTEGER:
		break;
	case SUBTILIS_TYPE_REAL:
		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		e = subtilis_type_if_to_float64(p, e, err);
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = subtilis_type_if_exp_to_var(p, e, err);
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
	e->exp.ir_op.integer = -e->exp.ir_op.integer;
	return e;
}

static subtilis_exp_t *prv_add_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer += a2->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real =
		    ((double)a1->exp.ir_op.integer) + a2->exp.ir_op.real;
		a1->type.type = SUBTILIS_TYPE_CONST_REAL;
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
		a1->exp.ir_op.integer *= a2->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real =
		    ((double)a1->exp.ir_op.integer) * a2->exp.ir_op.real;
		a1->type.type = SUBTILIS_TYPE_CONST_REAL;
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

static subtilis_exp_t *prv_and_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	a1->exp.ir_op.integer &= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_or_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	a1->exp.ir_op.integer |= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_eor_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	a1->exp.ir_op.integer ^= a2->exp.ir_op.integer;
	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_not_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	e->exp.ir_op.integer = ~e->exp.ir_op.integer;
	return e;
}

static subtilis_exp_t *prv_eq_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.integer == a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    ((double)a1->exp.ir_op.integer) == a2->exp.ir_op.real ? -1
									  : 0;
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
		    a1->exp.ir_op.integer != a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    ((double)a1->exp.ir_op.integer) != a2->exp.ir_op.real ? -1
									  : 0;
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
		a1->exp.ir_op.integer -= a2->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.real =
		    ((double)a1->exp.ir_op.integer) - a2->exp.ir_op.real;
		a1->type.type = SUBTILIS_TYPE_CONST_REAL;
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

static size_t prv_div_mod_vars(subtilis_parser_t *p, subtilis_ir_operand_t a,
			       subtilis_ir_operand_t b,
			       subtilis_op_instr_type_t type,
			       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_ir_arg_t *args;
	size_t res;
	size_t div_flag;
	size_t eflag_reg;
	size_t err_reg;
	subtilis_ir_operand_t div_mod;
	subtilis_ir_operand_t eflag_offset;
	subtilis_ir_operand_t err_offset;
	static const char idiv[] = "_idiv";
	char *name = NULL;

	if (p->caps & SUBTILIS_BACKEND_HAVE_DIV) {
		res =
		    subtilis_ir_section_add_instr(p->current, type, a, b, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
		return res;
	}

	name = malloc(sizeof(idiv));
	if (!name) {
		subtilis_error_set_oom(err);
		return 0;
	}
	strcpy(name, idiv);

	args = malloc(sizeof(*args) * 5);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return 0;
	}

	div_mod.integer = type == SUBTILIS_OP_INSTR_DIV_I32 ? 0 : 1;
	div_flag = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, div_mod, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	eflag_offset.integer = p->eflag_offset;
	eflag_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, eflag_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	err_offset.integer = p->error_offset;
	err_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, err_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = a.reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = b.reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = div_flag;
	args[3].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[3].reg = eflag_reg;
	args[4].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[4].reg = err_reg;
	e = subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_IDIV, NULL, args,
				  &subtilis_type_integer, 5, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	res = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	return res;
}

static void prv_div_mod_const_var(subtilis_parser_t *p, subtilis_exp_t *a1,
				  subtilis_exp_t *a2,
				  subtilis_op_instr_type_t type,
				  subtilis_error_t *err)
{
	a1->type.type = SUBTILIS_TYPE_INTEGER;
	a1->exp.ir_op.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg =
	    prv_div_mod_vars(p, a1->exp.ir_op, a2->exp.ir_op, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static subtilis_exp_t *prv_div_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/*
	 * Oddly, a2 may not be const as we don't currently swap operands
	 * for integer division as the ARM backend has no implementation
	 * for rdiv.  Some day we'll have to as some CPUs have native
	 * support for integer division, but for the time being we'll have to
	 * handle the unusual case of having to deal with a variable here.
	 * a2 is at least guaranteed to be an integer.
	 */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (a2->exp.ir_op.integer == 0) {
			subtilis_error_set_divide_by_zero(
			    err, p->l->stream->name, p->l->line);
			goto on_error;
		}
		a1->exp.ir_op.integer /= a2->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_INTEGER:
		prv_div_mod_const_var(p, a1, a2, SUBTILIS_OP_INSTR_DIV_I32,
				      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_mod_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/*
	 * Oddly, a2 may not be const as we don't currently swap operands
	 * for integer division as the ARM backend has no implementation
	 * for rdiv.  Some day we'll have to as some CPUs have native
	 * support for integer division, but for the time being we'll have to
	 * handle the unusual case of having to deal with a variable here.
	 * a2 is at least guaranteed to be an integer.
	 */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (a2->exp.ir_op.integer == 0) {
			subtilis_error_set_divide_by_zero(
			    err, p->l->stream->name, p->l->line);
			goto on_error;
		}
		a1->exp.ir_op.integer %= a2->exp.ir_op.integer;
		break;
	case SUBTILIS_TYPE_INTEGER:
		prv_div_mod_const_var(p, a1, a2, SUBTILIS_OP_INSTR_MOD_I32,
				      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_gt_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, bool swapped,
				    subtilis_error_t *err)
{
	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.integer > a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real > ((double)a2->exp.ir_op.integer) ? -1
									 : 0;
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
		    a1->exp.ir_op.integer <= a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real <= ((double)a2->exp.ir_op.integer) ? -1
									  : 0;
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
		    a1->exp.ir_op.integer < a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real < ((double)a2->exp.ir_op.integer) ? -1
									 : 0;
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
		    a1->exp.ir_op.integer >= a2->exp.ir_op.integer ? -1 : 0;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.real >= ((double)a2->exp.ir_op.integer) ? -1
									  : 0;
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

static void prv_shift_const_var(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2,
				subtilis_op_instr_type_t op_type,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	size_t reg;

	op.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	reg = subtilis_ir_section_add_instr(p->current, op_type, op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
	a1->type.type = SUBTILIS_TYPE_INTEGER;
}

static subtilis_exp_t *prv_lsl_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 may not be const as we don't order the arguments */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer = a1->exp.ir_op.integer
					<< (a2->exp.ir_op.integer & 63);
		break;
	case SUBTILIS_TYPE_INTEGER:
		prv_shift_const_var(p, a1, a2, SUBTILIS_OP_INSTR_LSL_I32, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_lsr_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 may not be const as we don't order the arguments */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    (int32_t)((uint32_t)a1->exp.ir_op.integer >>
			      ((uint32_t)a2->exp.ir_op.integer & 63));
		break;
	case SUBTILIS_TYPE_INTEGER:
		prv_shift_const_var(p, a1, a2, SUBTILIS_OP_INSTR_LSR_I32, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_asr_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/* a2 may not be const as we don't order the arguments */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		a1->exp.ir_op.integer =
		    a1->exp.ir_op.integer >> (a2->exp.ir_op.integer & 63);
		break;
	case SUBTILIS_TYPE_INTEGER:
		prv_shift_const_var(p, a1, a2, SUBTILIS_OP_INSTR_ASR_I32, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_abs_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	e->exp.ir_op.integer = abs(e->exp.ir_op.integer);

	return e;
}

static subtilis_exp_t *prv_sgn_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	if (e->exp.ir_op.integer < 0)
		e->exp.ir_op.integer = -1;
	else if (e->exp.ir_op.integer > 0)
		e->exp.ir_op.integer = 1;

	return e;
}

/* clang-format off */
subtilis_type_if subtilis_type_const_int32 = {
	.is_const = true,
	.is_numeric = true,
	.is_integer = true,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero_const,
	.top_bit = prv_top_bit_const,
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
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.load_mem = NULL,
	.to_int32 = prv_to_int32,
	.to_float64 = prv_to_float64_const,
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
	.div = prv_div_const,
	.mod = prv_mod_const,
	.gt = prv_gt_const,
	.lte = prv_lte_const,
	.lt = prv_lt_const,
	.gte = prv_gte_const,
	.lsl = prv_lsl_const,
	.lsr = prv_lsr_const,
	.asr = prv_asr_const,
	.abs = prv_abs_const,
	.sgn = prv_sgn_const,
	.call = NULL,
	.ret = NULL,
	.print = NULL,
};

/* clang-format on */

static subtilis_exp_t *prv_zero(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;

	op1.integer = 0;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_int32_var(reg_num, err);
}

static subtilis_exp_t *prv_top_bit(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;

	op1.integer = (int32_t)0x80000000;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_int32_var(reg_num, err);
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
	type->type = SUBTILIS_TYPE_ARRAY_INTEGER;
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
	e2 = subtilis_exp_new_int32_var(reg, err);
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
					  SUBTILIS_OP_INSTR_STOREO_I32,
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
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_int32_var(reg, err);
}

static subtilis_exp_t *prv_to_float64(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	size_t reg;

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

static subtilis_exp_t *prv_coerce_type(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_CONST_REAL:
		e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		e->exp.ir_op.integer = (int32_t)e->exp.ir_op.real;
		e = subtilis_type_if_exp_to_var(p, e, err);
		break;
	case SUBTILIS_TYPE_CONST_INTEGER:
		e = subtilis_type_if_exp_to_var(p, e, err);
		break;
	case SUBTILIS_TYPE_REAL:
		e = subtilis_type_if_to_float64(p, e, err);
		break;
	case SUBTILIS_TYPE_INTEGER:
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

static void prv_commutative_const_real(subtilis_parser_t *p, subtilis_exp_t *a1,
				       subtilis_exp_t *a2,
				       subtilis_op_instr_type_t real_var_imm,
				       const subtilis_type_t *result_type,
				       subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->type = *result_type;
	a1->exp.ir_op.reg = reg;
	reg = subtilis_ir_section_add_instr(p->current, real_var_imm,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_commutative_real(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2,
				 subtilis_op_instr_type_t real_var_var,
				 const subtilis_type_t *result_type,
				 subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->type = *result_type;
	a1->exp.ir_op.reg = reg;
	reg = subtilis_ir_section_add_instr(p->current, real_var_var,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static subtilis_exp_t *prv_commutative(subtilis_parser_t *p, subtilis_exp_t *a1,
				       subtilis_exp_t *a2,
				       subtilis_op_instr_type_t in_var_imm,
				       subtilis_op_instr_type_t in_var_var,
				       subtilis_op_instr_type_t real_var_imm,
				       subtilis_op_instr_type_t real_var_var,
				       subtilis_error_t *err)
{
	size_t reg;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_imm, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		prv_commutative_const_real(p, a1, a2, real_var_imm,
					   &subtilis_type_real, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	case SUBTILIS_TYPE_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_var, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_REAL:
		prv_commutative_real(p, a1, a2, real_var_var,
				     &subtilis_type_real, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
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
	subtilis_op_instr_type_t in_var_imm,
	subtilis_op_instr_type_t in_var_var,
	subtilis_op_instr_type_t real_var_imm,
	subtilis_op_instr_type_t real_var_var, subtilis_error_t *err)

/* clang-format on */
{
	size_t reg;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_imm, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		prv_commutative_const_real(p, a1, a2, real_var_imm,
					   &subtilis_type_integer, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	case SUBTILIS_TYPE_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, in_var_var, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_REAL:
		prv_commutative_real(p, a1, a2, real_var_var,
				     &subtilis_type_integer, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
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
	return prv_commutative(
	    p, a1, a2, SUBTILIS_OP_INSTR_ADDI_I32, SUBTILIS_OP_INSTR_ADD_I32,
	    SUBTILIS_OP_INSTR_ADDI_REAL, SUBTILIS_OP_INSTR_ADD_REAL, err);
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative(
	    p, a1, a2, SUBTILIS_OP_INSTR_MULI_I32, SUBTILIS_OP_INSTR_MUL_I32,
	    SUBTILIS_OP_INSTR_MULI_REAL, SUBTILIS_OP_INSTR_MUL_REAL, err);
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	/* last two parameters are not used. */

	return prv_commutative(
	    p, a1, a2, SUBTILIS_OP_INSTR_ANDI_I32, SUBTILIS_OP_INSTR_AND_I32,
	    SUBTILIS_OP_INSTR_ANDI_I32, SUBTILIS_OP_INSTR_AND_I32, err);
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	/* last two parameters are not used. */

	return prv_commutative(
	    p, a1, a2, SUBTILIS_OP_INSTR_ORI_I32, SUBTILIS_OP_INSTR_OR_I32,
	    SUBTILIS_OP_INSTR_ORI_I32, SUBTILIS_OP_INSTR_OR_I32, err);
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	/* last two parameters are not used. */

	return prv_commutative(
	    p, a1, a2, SUBTILIS_OP_INSTR_EORI_I32, SUBTILIS_OP_INSTR_EOR_I32,
	    SUBTILIS_OP_INSTR_EORI_I32, SUBTILIS_OP_INSTR_EOR_I32, err);
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
	return prv_commutative_logical(
	    p, a1, a2, SUBTILIS_OP_INSTR_EQI_I32, SUBTILIS_OP_INSTR_EQ_I32,
	    SUBTILIS_OP_INSTR_EQI_REAL, SUBTILIS_OP_INSTR_EQ_REAL, err);
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_commutative_logical(
	    p, a1, a2, SUBTILIS_OP_INSTR_NEQI_I32, SUBTILIS_OP_INSTR_NEQ_I32,
	    SUBTILIS_OP_INSTR_NEQI_REAL, SUBTILIS_OP_INSTR_NEQ_REAL, err);
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t int_var_imm;
	subtilis_op_instr_type_t real_var_imm;

	if (swapped) {
		int_var_imm = SUBTILIS_OP_INSTR_RSUBI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_RSUBI_REAL;
	} else {
		int_var_imm = SUBTILIS_OP_INSTR_SUBI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_SUBI_REAL;
	}

	return prv_commutative(p, a1, a2, int_var_imm,
			       SUBTILIS_OP_INSTR_SUB_I32, real_var_imm,
			       SUBTILIS_OP_INSTR_SUB_REAL, err);
}

static bool prv_optimise_div(subtilis_parser_t *p, subtilis_ir_operand_t a,
			     subtilis_ir_operand_t b, size_t *result,
			     subtilis_error_t *err)
{
	int32_t s;
	uint32_t abs_b = (uint32_t)(b.integer > 0) ? b.integer : -b.integer;
	subtilis_ir_operand_t c;
	subtilis_ir_operand_t tmp;

	for (s = 0; abs_b / 2 >= (1u << s); s++)
		;

	if (abs_b != 1u << s)
		return false;

	c.integer = 31;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ASRI_I32, a, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	c.integer = 32 - s;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LSRI_I32, tmp, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a, tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	c.integer = s;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ASRI_I32, tmp, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	if (b.integer < 0) {
		c.integer = 0;
		tmp.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_RSUBI_I32, tmp, c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
	}

	*result = tmp.reg;

	return true;
}

static size_t prv_div_by_constant(subtilis_parser_t *p, subtilis_ir_operand_t a,
				  subtilis_ir_operand_t b,
				  subtilis_error_t *err)
{
	bool can_optimise;
	subtilis_op_instr_type_t instr;
	size_t reg;
	size_t res = 0;

	if (b.integer == 0) {
		subtilis_error_set_divide_by_zero(err, p->l->stream->name,
						  p->l->line);
		return 0;
	}

	can_optimise = prv_optimise_div(p, a, b, &res, err);
	if ((err->type != SUBTILIS_ERROR_OK) || can_optimise)
		return res;

	if (p->caps & SUBTILIS_BACKEND_HAVE_DIV) {
		instr = SUBTILIS_OP_INSTR_DIVI_I32;
		return subtilis_ir_section_add_instr(p->current, instr, a, b,
						     err);
	}

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, b, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	b.reg = reg;

	return prv_div_mod_vars(p, a, b, SUBTILIS_OP_INSTR_DIV_I32, err);
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (a2->exp.ir_op.integer == 0) {
			subtilis_error_set_divide_by_zero(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		a1->exp.ir_op.reg =
		    prv_div_by_constant(p, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	case SUBTILIS_TYPE_INTEGER:
		a1->exp.ir_op.reg =
		    prv_div_mod_vars(p, a1->exp.ir_op, a2->exp.ir_op,
				     SUBTILIS_OP_INSTR_DIV_I32, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static bool prv_optimise_mod(subtilis_parser_t *p, subtilis_ir_operand_t a,
			     subtilis_ir_operand_t b, size_t *result,
			     subtilis_error_t *err)
{
	int32_t s;
	uint32_t abs_b = (uint32_t)(b.integer > 0) ? b.integer : -b.integer;
	subtilis_ir_operand_t c;
	subtilis_ir_operand_t shifted;
	subtilis_ir_operand_t tmp;

	for (s = 0; abs_b / 2 >= (1u << s); s++)
		;

	if (abs_b != 1u << s)
		return false;

	c.integer = 31;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ASRI_I32, a, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	c.integer = 32 - s;
	shifted.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LSRI_I32, tmp, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a, shifted, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	c.integer = abs_b - 1;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, tmp, c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_SUB_I32, tmp, shifted, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	*result = tmp.reg;

	return true;
}

static size_t prv_mod_by_constant(subtilis_parser_t *p, subtilis_ir_operand_t a,
				  subtilis_ir_operand_t b,
				  subtilis_error_t *err)
{
	bool can_optimise;
	size_t reg;
	size_t res = 0;

	if (b.integer == 0) {
		subtilis_error_set_divide_by_zero(err, p->l->stream->name,
						  p->l->line);
		return 0;
	}

	can_optimise = prv_optimise_mod(p, a, b, &res, err);
	if ((err->type != SUBTILIS_ERROR_OK) || can_optimise)
		return res;

	reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, b, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	b.reg = reg;

	return prv_div_mod_vars(p, a, b, SUBTILIS_OP_INSTR_MOD_I32, err);
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (a2->exp.ir_op.integer == 0) {
			subtilis_error_set_divide_by_zero(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		a1->exp.ir_op.reg =
		    prv_mod_by_constant(p, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	case SUBTILIS_TYPE_INTEGER:
		a1->exp.ir_op.reg =
		    prv_div_mod_vars(p, a1->exp.ir_op, a2->exp.ir_op,
				     SUBTILIS_OP_INSTR_MOD_I32, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t int_var_imm;
	subtilis_op_instr_type_t real_var_imm;

	if (swapped) {
		int_var_imm = SUBTILIS_OP_INSTR_LTEI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_LTEI_REAL;
	} else {
		int_var_imm = SUBTILIS_OP_INSTR_GTI_I32;
		;
		real_var_imm = SUBTILIS_OP_INSTR_GTI_REAL;
	}

	return prv_commutative(p, a1, a2, int_var_imm, SUBTILIS_OP_INSTR_GT_I32,
			       real_var_imm, SUBTILIS_OP_INSTR_GT_REAL, err);
}

static subtilis_exp_t *prv_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t int_var_imm;
	subtilis_op_instr_type_t real_var_imm;

	if (swapped) {
		int_var_imm = SUBTILIS_OP_INSTR_GTEI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_GTEI_REAL;
	} else {
		int_var_imm = SUBTILIS_OP_INSTR_LTEI_I32;
		;
		real_var_imm = SUBTILIS_OP_INSTR_LTEI_REAL;
	}

	return prv_commutative(p, a1, a2, int_var_imm,
			       SUBTILIS_OP_INSTR_LTE_I32, real_var_imm,
			       SUBTILIS_OP_INSTR_LTE_REAL, err);
}

static subtilis_exp_t *prv_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t int_var_imm;
	subtilis_op_instr_type_t real_var_imm;

	if (swapped) {
		int_var_imm = SUBTILIS_OP_INSTR_GTEI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_GTEI_REAL;
	} else {
		int_var_imm = SUBTILIS_OP_INSTR_LTI_I32;
		;
		real_var_imm = SUBTILIS_OP_INSTR_LTI_REAL;
	}

	return prv_commutative(p, a1, a2, int_var_imm, SUBTILIS_OP_INSTR_LT_I32,
			       real_var_imm, SUBTILIS_OP_INSTR_LT_REAL, err);
}

static subtilis_exp_t *prv_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t int_var_imm;
	subtilis_op_instr_type_t real_var_imm;

	if (swapped) {
		int_var_imm = SUBTILIS_OP_INSTR_LTEI_I32;
		real_var_imm = SUBTILIS_OP_INSTR_LTEI_REAL;
	} else {
		int_var_imm = SUBTILIS_OP_INSTR_GTEI_I32;
		;
		real_var_imm = SUBTILIS_OP_INSTR_GTEI_REAL;
	}

	return prv_commutative(p, a1, a2, int_var_imm,
			       SUBTILIS_OP_INSTR_GTE_I32, real_var_imm,
			       SUBTILIS_OP_INSTR_GTE_REAL, err);
}

static subtilis_exp_t *prv_shift(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2,
				 subtilis_op_instr_type_t op_type,
				 subtilis_op_instr_type_t opi_type,
				 subtilis_error_t *err)
{
	size_t reg;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, opi_type, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_INTEGER:
		reg = subtilis_ir_section_add_instr(
		    p->current, op_type, a1->exp.ir_op, a2->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		a1->exp.ir_op.reg = reg;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);
	return NULL;
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_shift(p, a1, a2, SUBTILIS_OP_INSTR_LSL_I32,
			 SUBTILIS_OP_INSTR_LSLI_I32, err);
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_shift(p, a1, a2, SUBTILIS_OP_INSTR_LSR_I32,
			 SUBTILIS_OP_INSTR_LSRI_I32, err);
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_shift(p, a1, a2, SUBTILIS_OP_INSTR_ASR_I32,
			 SUBTILIS_OP_INSTR_ASRI_I32, err);
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op2.integer = 31;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ASRI_I32, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	op2.reg = reg;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	op1.reg = reg;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	e->exp.ir_op.reg = reg;

	return e;

cleanup:

	subtilis_exp_delete(e);

	return NULL;
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

	op2.integer = 0;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, e->exp.ir_op, op2, err);
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
	    p->current, SUBTILIS_OP_INSTR_GTI_I32, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e->exp.ir_op.reg = reg2;
	e = prv_abs(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_assign_to_reg(p, reg, e, err);
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
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PRINT_I32, e->exp.ir_op, err);
	subtilis_exp_delete(e);
}

/* clang-format off */
subtilis_type_if subtilis_type_int32 = {
	.is_const = false,
	.is_numeric = true,
	.is_integer = true,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero,
	.top_bit = prv_top_bit,
	.zero_reg = prv_zero_reg,
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
	.load_mem = prv_load_from_mem,
	.to_int32 = prv_to_int32,
	.to_float64 = prv_to_float64,
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
	.lsl = prv_lsl,
	.lsr = prv_lsr,
	.asr = prv_asr,
	.abs = prv_abs,
	.sgn = prv_sgn,
	.call = prv_call,
	.ret = prv_ret,
	.print = prv_print,
};

/* clang-format on */
