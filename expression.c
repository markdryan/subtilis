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
#include <string.h>

#include "expression.h"

typedef int32_t (*subtilis_const_shift_t)(int32_t a, int32_t b);

typedef void (*subtilis_const_op_t)(subtilis_exp_t *, subtilis_exp_t *);
typedef void (*subtilis_var_op_t)(subtilis_ir_section_t *, subtilis_exp_t *,
				  subtilis_exp_t *, bool, subtilis_error_t *);
typedef void (*subtilis_var_op_div_t)(subtilis_parser_t *, subtilis_exp_t *,
				      subtilis_exp_t *, subtilis_error_t *);

struct subtilis_commutative_exp_t_ {
	subtilis_const_op_t op_int_int;
	subtilis_const_op_t op_int_real;
	subtilis_const_op_t op_real_real;
	subtilis_op_instr_type_t in_var_imm;
	subtilis_op_instr_type_t in_var_var;
};

typedef struct subtilis_commutative_exp_t_ subtilis_commutative_exp_t;

struct subtilis_non_commutative_exp_t_ {
	subtilis_const_op_t op_int_int;
	subtilis_const_op_t op_int_real;
	subtilis_const_op_t op_real_int;
	subtilis_const_op_t op_real_real;
	subtilis_var_op_t op_intvar_int;
	subtilis_var_op_t op_intvar_intvar;
};

typedef struct subtilis_non_commutative_exp_t_ subtilis_non_commutative_exp_t;

static void prv_add_call(subtilis_parser_t *p, subtilis_parser_call_t *call,
			 subtilis_error_t *err)
{
	subtilis_parser_call_t **new_calls;
	size_t new_max;

	if (p->num_calls == p->max_calls) {
		new_max = p->max_calls + SUBTILIS_CONFIG_PROC_GRAN;
		new_calls = realloc(p->calls, new_max * sizeof(*new_calls));
		if (!new_calls) {
			subtilis_error_set_oom(err);
			return;
		}
		p->calls = new_calls;
		p->max_calls = new_max;
	}
	p->calls[p->num_calls++] = call;
}

/*
 * Takes ownership of name and args and stype. stype may be NULL if we're
 * calling a builtin function that's actually used to implement an operator
 * e.g., DIV. Passing NULL for stype means the compiler won't perform ANY
 * type checking when issuing the call, which is fine as it's already been
 * done by the expression parser.
 */

subtilis_exp_t *subtilis_exp_add_call(subtilis_parser_t *p, char *name,
				      subtilis_builtin_type_t ftype,
				      subtilis_type_section_t *stype,
				      subtilis_ir_arg_t *args,
				      subtilis_type_t fn_type,
				      size_t num_params, subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;
	subtilis_type_section_t *ts = NULL;
	subtilis_parser_call_t *call = NULL;

	if (fn_type != SUBTILIS_TYPE_VOID) {
		reg = subtilis_ir_section_add_fn_call(p->current, num_params,
						      args, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = NULL;
		e = subtilis_exp_new_var(subtilis_type_to_exp_type(fn_type),
					 reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	} else {
		subtilis_ir_section_add_call(p->current, num_params, args, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = NULL;
	}

	if (ftype != SUBTILIS_BUILTINS_MAX) {
		ts = subtilis_builtin_ts(ftype, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		(void)subtilis_ir_prog_section_new(
		    p->prog, name, 0, num_params, ts, ftype, "builtin", 0, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
				goto on_error;
			subtilis_error_init(err);
			subtilis_type_section_delete(ts);
		}
		ts = NULL;
	}

	call = subtilis_parser_call_new(p->current, p->current->len - 1, name,
					stype, p->l->line, ftype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	name = NULL;
	stype = NULL;

	prv_add_call(p, call, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return e;

on_error:

	subtilis_type_section_delete(ts);
	subtilis_exp_delete(e);
	free(args);
	subtilis_parser_call_delete(call);
	subtilis_type_section_delete(stype);
	free(name);

	return NULL;
}

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
			reg = subtilis_ir_section_add_instr(
			    p->current, com->in_var_imm, a1->exp.ir_op,
			    a2->exp.ir_op, err);
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_section_add_instr(
			    p->current, com->in_var_var, a1->exp.ir_op,
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

static subtilis_exp_t *
prv_exp_non_commutative(subtilis_parser_t *p, subtilis_exp_t *a1,
			subtilis_exp_t *a2, subtilis_non_commutative_exp_t *no,
			subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			no->op_int_int(a1, a2);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			no->op_int_real(a1, a2);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			no->op_real_int(a1, a2);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			no->op_real_real(a1, a2);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			no->op_intvar_int(p->current, a1, a2, swapped, err);
			break;
		case SUBTILIS_EXP_CONST_REAL:
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			no->op_intvar_intvar(p->current, a1, a2, swapped, err);
			break;
		case SUBTILIS_EXP_REAL:
		case SUBTILIS_EXP_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
		}
		break;
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_assertion_failed(err);
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

subtilis_exp_t *subtilis_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e2;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		e2 = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		subtilis_exp_delete(e);
		return e2;
	case SUBTILIS_EXP_CONST_REAL:
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	case SUBTILIS_EXP_CONST_STRING:
	case SUBTILIS_EXP_INTEGER:
	case SUBTILIS_EXP_REAL:
	case SUBTILIS_EXP_STRING:
		return e;
	}

on_error:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_type_t subtilis_exp_type(subtilis_exp_t *e)
{
	subtilis_type_t typ;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
	case SUBTILIS_EXP_INTEGER:
		typ = SUBTILIS_TYPE_INTEGER;
		break;
	case SUBTILIS_EXP_CONST_REAL:
	case SUBTILIS_EXP_REAL:
		typ = SUBTILIS_TYPE_REAL;
		break;
	case SUBTILIS_EXP_CONST_STRING:
	case SUBTILIS_EXP_STRING:
		typ = SUBTILIS_TYPE_STRING;
		break;
	}

	return typ;
}

subtilis_exp_type_t subtilis_type_to_exp_type(subtilis_type_t type)
{
	subtilis_exp_type_t typ;

	switch (type) {
	case SUBTILIS_TYPE_INTEGER:
		typ = SUBTILIS_EXP_INTEGER;
		break;
	case SUBTILIS_TYPE_REAL:
		typ = SUBTILIS_EXP_REAL;
		break;
	case SUBTILIS_TYPE_STRING:
		typ = SUBTILIS_EXP_STRING;
		break;
	default:
		typ = SUBTILIS_EXP_INTEGER;
		break;
	}

	return typ;
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
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	subtilis_error_set_assertion_failed(err);
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

static void prv_sub_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer -= a2->exp.ir_op.integer;
}

static void prv_sub_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    ((double)a1->exp.ir_op.integer) - a2->exp.ir_op.real;
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_sub_real_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real =
	    a1->exp.ir_op.real - ((double)a2->exp.ir_op.integer);
	a1->type = SUBTILIS_EXP_CONST_REAL;
}

static void prv_sub_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.real -= a2->exp.ir_op.real;
}

static void prv_sub_intvar_int(subtilis_ir_section_t *s, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	size_t reg;

	instr =
	    swapped ? SUBTILIS_OP_INSTR_RSUBI_I32 : SUBTILIS_OP_INSTR_SUBI_I32;
	reg = subtilis_ir_section_add_instr(s, instr, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_sub_intvar_intvar(subtilis_ir_section_t *s, subtilis_exp_t *a1,
				  subtilis_exp_t *a2, bool swapped,
				  subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr(s, SUBTILIS_OP_INSTR_SUB_I32,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

subtilis_exp_t *subtilis_exp_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_non_commutative_exp_t no = {
	    .op_int_int = prv_sub_int_int,
	    .op_int_real = prv_sub_int_real,
	    .op_real_int = prv_sub_real_int,
	    .op_real_real = prv_sub_real_real,
	    .op_intvar_int = prv_sub_intvar_int,
	    .op_intvar_intvar = prv_sub_intvar_intvar,
	};

	return prv_exp_non_commutative(p, a1, a2, &no, err);
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

static size_t prv_div_mod_vars(subtilis_parser_t *p, subtilis_ir_operand_t a,
			       subtilis_ir_operand_t b,
			       subtilis_op_instr_type_t type,
			       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_ir_arg_t *args;
	size_t res;
	size_t div_flag;
	subtilis_ir_operand_t div_mod;
	const char idiv[] = "_idiv";
	char *name = NULL;

	if (p->caps & SUBTILIS_BACKEND_HAVE_DIV)
		return subtilis_ir_section_add_instr(p->current, type, a, b,
						     err);

	name = malloc(sizeof(idiv));
	if (!name) {
		subtilis_error_set_oom(err);
		return 0;
	}
	strcpy(name, idiv);

	args = malloc(sizeof(*args) * 3);
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

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = a.reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = b.reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = div_flag;
	e = subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_IDIV, NULL, args,
				  SUBTILIS_TYPE_INTEGER, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	res = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	return res;
}

static size_t prv_div_vars(subtilis_parser_t *p, subtilis_ir_operand_t a,
			   subtilis_ir_operand_t b, subtilis_error_t *err)
{
	return prv_div_mod_vars(p, a, b, SUBTILIS_OP_INSTR_DIV_I32, err);
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

	return prv_div_vars(p, a, b, err);
}

static bool prv_optimise_mod(subtilis_parser_t *p, subtilis_ir_operand_t a,
			     subtilis_ir_operand_t b, size_t *result,
			     subtilis_error_t *err)
{
	return false;
}

static size_t prv_mod_vars(subtilis_parser_t *p, subtilis_ir_operand_t a,
			   subtilis_ir_operand_t b, subtilis_error_t *err)
{
	return prv_div_mod_vars(p, a, b, SUBTILIS_OP_INSTR_MOD_I32, err);
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

	return prv_mod_vars(p, a, b, err);
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

static subtilis_exp_t *prv_exp_mod_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				       subtilis_exp_t *a2,
				       subtilis_const_op_t const_op,
				       subtilis_var_op_div_t var_const_op,
				       subtilis_var_op_div_t var_op,
				       subtilis_error_t *err)
{
	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_REAL:
			a2->exp.ir_op.integer = (int32_t)a2->exp.ir_op.real;
			a2->type = SUBTILIS_EXP_CONST_INTEGER;
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, p->l->stream->name, p->l->line);
				break;
			}
			const_op(a1, a2);
			break;
		case SUBTILIS_EXP_INTEGER:
			a1->type = SUBTILIS_EXP_INTEGER;
			a1->exp.ir_op.reg = subtilis_ir_section_add_instr2(
			    p->current, SUBTILIS_OP_INSTR_MOVI_I32,
			    a1->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return NULL;
			var_op(p, a1, a2, err);
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_REAL:
			a2->exp.ir_op.integer = (int32_t)a2->exp.ir_op.real;
			a2->type = SUBTILIS_EXP_CONST_INTEGER;
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, p->l->stream->name, p->l->line);
				break;
			}
			a1->type = SUBTILIS_EXP_CONST_INTEGER;
			a1->exp.ir_op.integer = (int32_t)a1->exp.ir_op.real;
			const_op(a1, a2);
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
		case SUBTILIS_EXP_CONST_REAL:
			a2->exp.ir_op.integer = (int32_t)a2->exp.ir_op.real;
			a2->type = SUBTILIS_EXP_CONST_INTEGER;
		case SUBTILIS_EXP_CONST_INTEGER:
			if (a2->exp.ir_op.integer == 0) {
				subtilis_error_set_divide_by_zero(
				    err, p->l->stream->name, p->l->line);
				break;
			}
			var_const_op(p, a1, a2, err);
			break;
		case SUBTILIS_EXP_CONST_STRING:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			break;
		case SUBTILIS_EXP_INTEGER:
			var_op(p, a1, a2, err);
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

static void prv_mod_const_op(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer %= a2->exp.ir_op.integer;
}

static void prv_mod_var_const_op(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->exp.ir_op.reg =
	    prv_mod_by_constant(p, a1->exp.ir_op, a2->exp.ir_op, err);
}

static void prv_mod_var_op(subtilis_parser_t *p, subtilis_exp_t *a1,
			   subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->exp.ir_op.reg = prv_mod_vars(p, a1->exp.ir_op, a2->exp.ir_op, err);
}

subtilis_exp_t *subtilis_exp_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_exp_mod_div(p, a1, a2, prv_mod_const_op,
			       prv_mod_var_const_op, prv_mod_var_op, err);
}

static void prv_div_const_op(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer /= a2->exp.ir_op.integer;
}

static void prv_div_var_const_op(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->exp.ir_op.reg =
	    prv_div_by_constant(p, a1->exp.ir_op, a2->exp.ir_op, err);
}

static void prv_div_var_op(subtilis_parser_t *p, subtilis_exp_t *a1,
			   subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1->exp.ir_op.reg = prv_div_vars(p, a1->exp.ir_op, a2->exp.ir_op, err);
}

subtilis_exp_t *subtilis_exp_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_exp_mod_div(p, a1, a2, prv_div_const_op,
			       prv_div_var_const_op, prv_div_var_op, err);
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
		reg = subtilis_ir_section_add_instr(p->current,
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
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_NOT_I32, e->exp.ir_op, err);
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
		subtilis_error_set_assertion_failed(err);
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
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_exp_commutative(p, a1, a2, &com, err);
}

static void prv_gt_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer > a2->exp.ir_op.integer ? -1 : 0;
}

static void prv_gt_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    ((double)a1->exp.ir_op.integer) > a2->exp.ir_op.real ? -1 : 0;
}

static void prv_gt_real_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real > ((double)a2->exp.ir_op.integer) ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_gt_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real > a2->exp.ir_op.real ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_gt_intvar_int(subtilis_ir_section_t *s, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	size_t reg;

	instr =
	    swapped ? SUBTILIS_OP_INSTR_LTEI_I32 : SUBTILIS_OP_INSTR_GTI_I32;
	reg = subtilis_ir_section_add_instr(s, instr, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_gt_intvar_intvar(subtilis_ir_section_t *s, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, bool swapped,
				 subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr(s, SUBTILIS_OP_INSTR_GT_I32,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

subtilis_exp_t *subtilis_exp_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_non_commutative_exp_t no = {
	    .op_int_int = prv_gt_int_int,
	    .op_int_real = prv_gt_int_real,
	    .op_real_int = prv_gt_real_int,
	    .op_real_real = prv_gt_real_real,
	    .op_intvar_int = prv_gt_intvar_int,
	    .op_intvar_intvar = prv_gt_intvar_intvar,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_exp_non_commutative(p, a1, a2, &no, err);
}

static void prv_lte_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer <= a2->exp.ir_op.integer ? -1 : 0;
}

static void prv_lte_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    ((double)a1->exp.ir_op.integer) <= a2->exp.ir_op.real ? -1 : 0;
}

static void prv_lte_real_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real <= ((double)a2->exp.ir_op.integer) ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_lte_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real <= a2->exp.ir_op.real ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_lte_intvar_int(subtilis_ir_section_t *s, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	size_t reg;

	instr =
	    swapped ? SUBTILIS_OP_INSTR_GTI_I32 : SUBTILIS_OP_INSTR_LTEI_I32;
	reg = subtilis_ir_section_add_instr(s, instr, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_lte_intvar_intvar(subtilis_ir_section_t *s, subtilis_exp_t *a1,
				  subtilis_exp_t *a2, bool swapped,
				  subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr(s, SUBTILIS_OP_INSTR_LTE_I32,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

subtilis_exp_t *subtilis_exp_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_non_commutative_exp_t no = {
	    .op_int_int = prv_lte_int_int,
	    .op_int_real = prv_lte_int_real,
	    .op_real_int = prv_lte_real_int,
	    .op_real_real = prv_lte_real_real,
	    .op_intvar_int = prv_lte_intvar_int,
	    .op_intvar_intvar = prv_lte_intvar_intvar,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_exp_non_commutative(p, a1, a2, &no, err);
}

static void prv_lt_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer < a2->exp.ir_op.integer ? -1 : 0;
}

static void prv_lt_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    ((double)a1->exp.ir_op.integer) < a2->exp.ir_op.real ? -1 : 0;
}

static void prv_lt_real_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real < ((double)a2->exp.ir_op.integer) ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_lt_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real < a2->exp.ir_op.real ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_lt_intvar_int(subtilis_ir_section_t *s, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	size_t reg;

	instr =
	    swapped ? SUBTILIS_OP_INSTR_GTEI_I32 : SUBTILIS_OP_INSTR_LTI_I32;
	reg = subtilis_ir_section_add_instr(s, instr, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_lt_intvar_intvar(subtilis_ir_section_t *s, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, bool swapped,
				 subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr(s, SUBTILIS_OP_INSTR_LT_I32,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

subtilis_exp_t *subtilis_exp_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_non_commutative_exp_t no = {
	    .op_int_int = prv_lt_int_int,
	    .op_int_real = prv_lt_int_real,
	    .op_real_int = prv_lt_real_int,
	    .op_real_real = prv_lt_real_real,
	    .op_intvar_int = prv_lt_intvar_int,
	    .op_intvar_intvar = prv_lt_intvar_intvar,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_exp_non_commutative(p, a1, a2, &no, err);
}

static void prv_gte_int_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.integer >= a2->exp.ir_op.integer ? -1 : 0;
}

static void prv_gte_int_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    ((double)a1->exp.ir_op.integer) >= a2->exp.ir_op.real ? -1 : 0;
}

static void prv_gte_real_int(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real >= ((double)a2->exp.ir_op.integer) ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_gte_real_real(subtilis_exp_t *a1, subtilis_exp_t *a2)
{
	a1->exp.ir_op.integer =
	    a1->exp.ir_op.real >= a2->exp.ir_op.real ? -1 : 0;
	a1->type = SUBTILIS_EXP_CONST_INTEGER;
}

static void prv_gte_intvar_int(subtilis_ir_section_t *s, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	size_t reg;

	instr =
	    swapped ? SUBTILIS_OP_INSTR_LTI_I32 : SUBTILIS_OP_INSTR_GTEI_I32;
	reg = subtilis_ir_section_add_instr(s, instr, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

static void prv_gte_intvar_intvar(subtilis_ir_section_t *s, subtilis_exp_t *a1,
				  subtilis_exp_t *a2, bool swapped,
				  subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr(s, SUBTILIS_OP_INSTR_GTE_I32,
					    a1->exp.ir_op, a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	a1->exp.ir_op.reg = reg;
}

subtilis_exp_t *subtilis_exp_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_non_commutative_exp_t no = {
	    .op_int_int = prv_gte_int_int,
	    .op_int_real = prv_gte_int_real,
	    .op_real_int = prv_gte_real_int,
	    .op_real_real = prv_gte_real_real,
	    .op_intvar_int = prv_gte_intvar_int,
	    .op_intvar_intvar = prv_gte_intvar_intvar,
	};

	if ((a1->type == SUBTILIS_EXP_CONST_STRING ||
	     a1->type == SUBTILIS_EXP_STRING) &&
	    (a2->type == SUBTILIS_EXP_CONST_STRING ||
	     a2->type == SUBTILIS_EXP_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_exp_non_commutative(p, a1, a2, &no, err);
}

static subtilis_exp_t *
prv_exp_shift(subtilis_parser_t *p, subtilis_exp_t *a1, subtilis_exp_t *a2,
	      subtilis_const_shift_t cfn, subtilis_op_instr_type_t op_type,
	      subtilis_op_instr_type_t opi_type, subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	size_t reg;

	switch (a1->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->exp.ir_op.integer =
			    cfn(a1->exp.ir_op.integer, a2->exp.ir_op.integer);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->exp.ir_op.integer = cfn(
			    a1->exp.ir_op.integer, (int32_t)a2->exp.ir_op.real);
			break;
		case SUBTILIS_EXP_INTEGER:
			op.reg = subtilis_ir_section_add_instr2(
			    p->current, SUBTILIS_OP_INSTR_MOVI_I32,
			    a1->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
			reg = subtilis_ir_section_add_instr(
			    p->current, op_type, op, a2->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
			a1->exp.ir_op.reg = reg;
			a1->type = SUBTILIS_EXP_INTEGER;
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			goto on_error;
		}
		break;
	case SUBTILIS_EXP_CONST_REAL:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_INTEGER:
			a1->type = SUBTILIS_EXP_CONST_INTEGER;
			a1->exp.ir_op.integer = cfn((int32_t)a1->exp.ir_op.real,
						    a2->exp.ir_op.integer);
			break;
		case SUBTILIS_EXP_CONST_REAL:
			a1->type = SUBTILIS_EXP_CONST_INTEGER;
			a1->exp.ir_op.integer =
			    cfn((int32_t)a1->exp.ir_op.real,
				(int32_t)a2->exp.ir_op.integer);
			break;
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			goto on_error;
		}
		break;
	case SUBTILIS_EXP_INTEGER:
		switch (a2->type) {
		case SUBTILIS_EXP_CONST_REAL:
			a2->exp.ir_op.integer = (int32_t)a2->exp.ir_op.real;
		case SUBTILIS_EXP_CONST_INTEGER:
			reg = subtilis_ir_section_add_instr(
			    p->current, opi_type, a1->exp.ir_op, a2->exp.ir_op,
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_INTEGER:
			reg = subtilis_ir_section_add_instr(p->current, op_type,
							    a1->exp.ir_op,
							    a2->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
			a1->exp.ir_op.reg = reg;
			break;
		case SUBTILIS_EXP_REAL:
		default:
			subtilis_error_set_bad_expression(
			    err, p->l->stream->name, p->l->line);
			goto on_error;
		}
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		goto on_error;
	}

	subtilis_exp_delete(a2);
	return a1;

on_error:

	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);

	return NULL;
}

static int32_t prv_lsl_const(int32_t a, int32_t b) { return a << (b & 63); }

subtilis_exp_t *subtilis_exp_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_exp_shift(p, a1, a2, prv_lsl_const,
			     SUBTILIS_OP_INSTR_LSL_I32,
			     SUBTILIS_OP_INSTR_LSLI_I32, err);
}

static int32_t prv_lsr_const(int32_t a, int32_t b)
{
	return (int32_t)((uint32_t)a >> ((uint32_t)b & 63));
}

subtilis_exp_t *subtilis_exp_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_exp_shift(p, a1, a2, prv_lsr_const,
			     SUBTILIS_OP_INSTR_LSR_I32,
			     SUBTILIS_OP_INSTR_LSRI_I32, err);
}

static int32_t prv_asr_const(int32_t a, int32_t b) { return a >> (b & 63); }

subtilis_exp_t *subtilis_exp_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_exp_shift(p, a1, a2, prv_asr_const,
			     SUBTILIS_OP_INSTR_ASR_I32,
			     SUBTILIS_OP_INSTR_ASRI_I32, err);
}

void subtilis_exp_delete(subtilis_exp_t *e)
{
	if (!e)
		return;
	if (e->type == SUBTILIS_EXP_CONST_STRING)
		subtilis_buffer_free(&e->exp.str);
	free(e);
}
