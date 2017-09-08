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

subtilis_exp_t *subtilis_exp_add(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	size_t len;
	size_t reg;

	(void)prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.integer += a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real = ((double)a1->exp.ir_op.integer) +
					     a2->exp.ir_op.real;
			a1->type = SUBTILIS_EXP_CONST_REAL;
			break;
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.real += (double)a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real += a2->exp.ir_op.real;
			break;
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_STRING:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
		case SUBTILIS_EXP_CONST_REAL:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		case SUBTILIS_EXP_CONST_STRING:
			len = subtilis_buffer_get_size(&a2->exp.str);
			subtilis_buffer_remove_terminator(&a1->exp.str);
			subtilis_buffer_append(
			    &a1->exp.str, a2->exp.str.buffer->data, len, err);
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_ADDI_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_ADD_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
		case SUBTILIS_EXP_REAL:
		case SUBTILIS_EXP_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
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

subtilis_exp_t *subtilis_exp_sub(subtilis_ir_program_t *p, subtilis_exp_t *a1,
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
			    p, instr, a1->exp.ir_op, a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_SUB_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
		case SUBTILIS_EXP_REAL:
		case SUBTILIS_EXP_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
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

subtilis_exp_t *subtilis_exp_mul(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	size_t reg;

	(void)prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.integer *= a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real = ((double)a1->exp.ir_op.integer) *
					     a2->exp.ir_op.real;
			a1->type = SUBTILIS_EXP_CONST_REAL;
			break;
		default:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.real *= (double)a2->exp.ir_op.integer;
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.real *= a2->exp.ir_op.real;
			break;
		default:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_bad_expression(err, l->stream->name,
						  l->line);
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_MULI_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_MUL_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
		case SUBTILIS_EXP_REAL:
		default:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, l->stream->name,
						  l->line);
		break;
	}

	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		a1 = NULL;
	}

	return a1;
}

subtilis_exp_t *subtilis_exp_div(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	size_t reg;

	// TODO: this can't be correct.  We need to preserve order.

	prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, l->stream->name, l->line);
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
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
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
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_bad_expression(err, l->stream->name,
						  l->line);
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, l->stream->name, l->line);
				break;
			}
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_DIV_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_program_add_instr(
			    p, SUBTILIS_OP_INSTR_DIV_I32, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
		case SUBTILIS_EXP_REAL:
		default:
			subtilis_error_set_bad_expression(err, l->stream->name,
							  l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_bad_expression(err, l->stream->name,
						  l->line);
		break;
	}

	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		a1 = NULL;
	}

	return a1;
}

void subtilis_exp_delete(subtilis_exp_t *e)
{
	if (!e)
		return;
	if (e->type == SUBTILIS_EXP_CONST_STRING)
		subtilis_buffer_free(&e->exp.str);
	free(e);
}
