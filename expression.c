/*
 * Copyright (c) 2017 Mark Ryan
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

#include <stdlib.h>

#include "expression.h"

typedef void (*subtilis_const_op_t)(subtilis_exp_t *, subtilis_exp_t *);

struct subtilis_commutative_exp_t_ {
	subtilis_const_op_t op_int_int;
	subtilis_const_op_t op_int_real;
	subtilis_const_op_t op_real_real;
	subtilis_op_instr_type_t in_var_imm;
	subtilis_op_instr_type_t in_var_var;
};

typedef struct subtilis_commutative_exp_t_ subtilis_commutative_exp_t;

/* Swap the arguments if necessary to ensure that the constant comes last
 * Returns true if arguments have been swapped.
 */

static bool prv_order_expressions(subtilis_exp_t **a1, subtilis_exp_t **a2)
{
	subtilis_exp_t *e1 = *a1;
	subtilis_exp_t *e2 = *a2;

	if ((e2->type == SUBTILIS_EXP_INTEGER ||
	     e2->type == SUBTILIS_EXP_REAL ||
	     e2->type == SUBTILIS_EXP_STRING) &&
	    (e1->type == SUBTILIS_EXP_CONST_INTEGER ||
	     e1->type == SUBTILIS_EXP_CONST_REAL ||
	     e1->type == SUBTILIS_EXP_CONST_STRING)) {
		*a1 = e2;
		*a2 = e1;
		return true;
	}

	return false;
}

static subtilis_exp_t *prv_exp_commutative(subtilis_parser_t *p,
					   subtilis_exp_t *a1,
					   subtilis_exp_t *a2,
					   subtilis_commutative_exp_t *com,
					   subtilis_error_t *err)
{
	size_t reg;

	(void)prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			com->op_int_int(a1, a2);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			com->op_int_real(a1, a2);
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			com->op_int_real(a2, a1);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			com->op_real_real(a1, a2);
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p->p, com->in_var_imm, a1->exp.ir_op, a2->exp.ir_op,
			    err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p->p, com->in_var_var, a1->exp.ir_op, a2->exp.ir_op,
			    err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_REAL:
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	}

	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		a1 = NULL;
	}

	return a1;
}

subtilis_exp_t *subtilis_exp_new_var(subtilis_exp_type_t type, unsigned int reg,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = type;
	e->exp.ir_op.reg = reg;

	return e;
}

subtilis_exp_t *subtilis_exp_new_int32(int32_t integer, subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = SUBTILIS_EXP_CONST_INTEGER;
	e->exp.ir_op.integer = integer;

	return e;
}

subtilis_exp_t *subtilis_exp_new_real(double real, subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = SUBTILIS_EXP_CONST_REAL;
	e->exp.ir_op.real = real;

	return e;
}

subtilis_exp_t *subtilis_exp_new_str(subtilis_buffer_t *str,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = SUBTILIS_EXP_CONST_STRING;
	subtilis_buffer_init(&e->exp.str, str->granularity);
	subtilis_buffer_append(&e->exp.str, str->buffer->data,
			       subtilis_buffer_get_size(str), err);

	return e;
}

static void prv_add_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer += a2->exp.ir_op.integer;
}

static void prv_add_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    ((double)a1->exp.ir_op.integer) + a2->exp.ir_op.real;
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_add_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real += a2->exp.ir_op.real;
}

static subtilis_exp_t *prv_subtilis_exp_add_str(subtilis_parser_t *p,
						subtilis_exp_t *a1,
						subtilis_exp_t *a2,
						subtilis_error_t *err)
{
	size_t len;

	(void)prv_order_expressions(&a1, &a2);

	if (a1->type == SUBTILIS_EXP_CONST_STRING) {
		len = subtilis_buffer_get_size(&a2->exp.str);
		subtilis_buffer_remove_terminator(&a1->exp.str);
		subtilis_buffer_append(&a1->exp.str, a2->exp.str.buffer->data,
				       len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		return a1;
	} else if (a2->type == SUBTILIS_EXP_CONST_STRING) {
		subtilis_error_set_asssertion_failed(err);
		return NULL;
	}

	subtilis_error_set_asssertion_failed(err);
	return NULL;
}

subtilis_exp_t *subtilis_exp_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_add_int_int,
	    .op_int_real = prv_add_int_real,
	    .op_real_real = prv_add_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_ADDI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_ADD_I32,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		return prv_subtilis_exp_add_str(p, a1, a2, err);
	}

	return prv_exp_commutative(p, a1, a2, &com, err);
}

subtilis_exp_t *subtilis_exp_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t instr;

	bool swapped = prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.integer -= a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real = ((double)a1->exp.ir_op.integer) -
					     a2->exp.ir_op.real;
			a1->type = SUBTILIS_EXP_CONST_REAL;
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.real -= (double)a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real -= a2->exp.ir_op.real;
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			instr = swapped ? SUBTILIS_OP_INSTR_RSUBI_I32
					: SUBTILIS_OP_INSTR_SUBI_I32;
			reg = subtilis_ir_program_add_instr(
			    p->p, instr, a1->exp.ir_op, a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p->p, SUBTILIS_OP_INSTR_SUB_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_REAL:
		case SUBTILIS_EXP_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_asssertion_failed(err);
		break;
	}

	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		a1 = NULL;
	}

	return a1;
}

static void prv_mul_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer *= a2->exp.ir_op.integer;
}

static void prv_mul_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    ((double)a1->exp.ir_op.integer) * a2->exp.ir_op.real;
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_mul_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real *= a2->exp.ir_op.real;
}

subtilis_exp_t *subtilis_exp_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_mul_int_int,
	    .op_int_real = prv_mul_int_real,
	    .op_real_real = prv_mul_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_MULI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_MUL_I32,
	};

	return prv_exp_commutative(p, a1, a2, &com, err);
}

subtilis_exp_t *subtilis_exp_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t instr;

	bool swapped = prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, p->l->stream->name, p->l->line);
				break;
			}
			a1->exp.ir_op.integer /= a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			/* TODO I need a divide by zero check here? */
			a1->exp.ir_op.real = ((double)a1->exp.ir_op.integer) /
					     a2->exp.ir_op.real;
			a1->type = SUBTILIS_EXP_CONST_REAL;
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			/* TODO I need a divide by zero check here? */
			a1->exp.ir_op.real /= (double)a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			/* TODO I need a divide by zero check here? */
			a1->exp.ir_op.real /= a2->exp.ir_op.real;
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			if (swapped) {
				instr = SUBTILIS_OP_INSTR_RDIVI_I32;
			} else {
				if (a2->exp.ir_op.integer == 0) {
					subtilis_error_set_divide_by_zero(
					    err, p->l->stream->name,
					    p->l->line);
					break;
				}
				instr = SUBTILIS_OP_INSTR_DIVI_I32;
			}
			reg = subtilis_ir_program_add_instr(
			    p->p, instr, a1->exp.ir_op, a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p->p, SUBTILIS_OP_INSTR_DIV_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_REAL:
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		break;
	}

	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		a1 = NULL;
	}

	return a1;
}

subtilis_exp_t *subtilis_exp_unary_minus(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t operand;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		e->exp.ir_op.integer = -e->exp.ir_op.integer;
		break;
	case SUBTILIS_EXP_INTEGER:
		operand.integer = 0;
		reg = subtilis_ir_program_add_instr(p->p,
						    SUBTILIS_OP_INSTR_RSUBI_I32,
						    e->exp.ir_op, operand, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		e->exp.ir_op.real = -e->exp.ir_op.real;
		break;
	case SUBTILIS_EXP_CONST_STRING:
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto cleanup;
	}

	return e;

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_exp_not(subtilis_parser_t *p, subtilis_exp_t *e,
				 subtilis_error_t *err)
{
	size_t reg;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		e->exp.ir_op.integer = ~e->exp.ir_op.integer;
		break;
	case SUBTILIS_EXP_INTEGER:
		reg = subtilis_ir_program_add_instr2(
		    p->p, SUBTILIS_OP_INSTR_NOT_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		e->exp.ir_op.real = (double)~((int32_t)e->exp.ir_op.real);
		break;
	case SUBTILIS_EXP_CONST_STRING:
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto cleanup;
	}

	return e;

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

static void prv_and_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer &= a2->exp.ir_op.integer;
}

static void prv_and_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)(a1->exp.ir_op.integer & ((int32_t)a2->exp.ir_op.real));
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_and_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)((int32_t)a1->exp.ir_op.real & (int32_t)a2->exp.ir_op.real);
}

subtilis_exp_t *subtilis_exp_and(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_and_int_int,
	    .op_int_real = prv_and_int_real,
	    .op_real_real = prv_and_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_ANDI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_AND_I32,
	};

	return prv_exp_commutative(p, a1, a2, &com, err);
}

static void prv_or_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer |= a2->exp.ir_op.integer;
}

static void prv_or_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)(a1->exp.ir_op.integer | ((int32_t)a2->exp.ir_op.real));
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_or_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)((int32_t)a1->exp.ir_op.real | (int32_t)a2->exp.ir_op.real);
}

subtilis_exp_t *subtilis_exp_or(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_or_int_int,
	    .op_int_real = prv_or_int_real,
	    .op_real_real = prv_or_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_ORI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_OR_I32,
	};

	return prv_exp_commutative(p, a1, a2, &com, err);
}

static void prv_eor_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer ^= a2->exp.ir_op.integer;
}

static void prv_eor_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)(a1->exp.ir_op.integer ^ ((int32_t)a2->exp.ir_op.real));
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_eor_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    (double)((int32_t)a1->exp.ir_op.real ^ (int32_t)a2->exp.ir_op.real);
}

subtilis_exp_t *subtilis_exp_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_eor_int_int,
	    .op_int_real = prv_eor_int_real,
	    .op_real_real = prv_eor_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_EORI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_EOR_I32,
	};

	return prv_exp_commutative(p, a1, a2, &com, err);
}

static void prv_eq_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer == a2->exp.ir_op.integer ? -1 : 0;
}

static void prv_eq_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer == ((int32_t)a2->exp.ir_op.real) ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_eq_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real == a2->exp.ir_op.real ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

subtilis_exp_t *subtilis_exp_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_eq_int_int,
	    .op_int_real = prv_eq_int_real,
	    .op_real_real = prv_eq_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_EQI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_EQ_I32,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_asssertion_failed(err);
		return NULL;
	}

	return prv_exp_commutative(p, a1, a2, &com, err);
}

static void prv_neq_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer == a2->exp.ir_op.integer ? 0 : -1;
}

static void prv_neq_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer == ((int32_t)a2->exp.ir_op.real) ? 0 : -1;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_neq_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real == a2->exp.ir_op.real ? 0 : -1;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

subtilis_exp_t *subtilis_exp_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_commutative_exp_t com = {
	    .op_int_int = prv_neq_int_int,
	    .op_int_real = prv_neq_int_real,
	    .op_real_real = prv_neq_real_real,
	    .in_var_imm = SUBTILIS_OP_INSTR_NEQI_I32,
	    .in_var_var = SUBTILIS_OP_INSTR_NEQ_I32,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_asssertion_failed(err);
		return NULL;
	}

	return prv_exp_commutative(p, a1, a2, &com, err);
}

void subtilis_exp_delete(subtilis_exp_t *e)
{
	if (!e)
		return;
	if (e->type == SUBTILIS_EXP_CONST_STRING)
		subtilis_buffer_free(&e->exp.str);
	free(e);
}
