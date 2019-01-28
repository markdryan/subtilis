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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "expression.h"
#include "parser.h"

const char *subtilis_rnd_hidden_var = "_RND";

struct subtilis_parser_param_t_ {
	subtilis_type_t type;
	size_t reg;
	size_t nop;
};

typedef struct subtilis_parser_param_t_ subtilis_parser_param_t;

static subtilis_exp_t *prv_priority1(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err);
static subtilis_exp_t *prv_call(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err);
static void prv_assign_hidden(subtilis_parser_t *p, const char *var_name,
			      subtilis_type_t id_type, subtilis_exp_t *e,
			      subtilis_error_t *err);
static subtilis_exp_t *prv_bracketed_2_int_args(subtilis_parser_t *p,
						subtilis_token_t *t,
						subtilis_op_instr_type_t itype,
						subtilis_error_t *err);

#define SUBTILIS_MAIN_FN "subtilis_main"

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       subtilis_backend_caps_t caps,
				       subtilis_error_t *err)
{
	subtilis_parser_t *p = calloc(1, sizeof(*p));
	subtilis_type_section_t *stype = NULL;

	if (!p) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	p->st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->main_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->local_st = p->main_st;

	p->prog = subtilis_ir_prog_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	stype = subtilis_type_section_new(SUBTILIS_TYPE_VOID, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->current = subtilis_ir_prog_section_new(
	    p->prog, SUBTILIS_MAIN_FN, 0, stype, SUBTILIS_BUILTINS_MAX,
	    l->stream->name, l->line, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	stype = NULL;
	p->main = p->current;

	p->l = l;
	p->caps = caps;
	p->level = 0;

	return p;

on_error:

	subtilis_type_section_delete(stype);
	subtilis_parser_delete(p);

	return NULL;
}

void subtilis_parser_delete(subtilis_parser_t *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < p->num_calls; i++)
		subtilis_parser_call_delete(p->calls[i]);
	free(p->calls);

	subtilis_ir_prog_delete(p->prog);
	subtilis_symbol_table_delete(p->main_st);
	subtilis_symbol_table_delete(p->st);
	free(p);
}

typedef void (*subtilis_keyword_fn)(subtilis_parser_t *p, subtilis_token_t *,
				    subtilis_error_t *);

static const subtilis_keyword_fn keyword_fns[];

static subtilis_exp_t *prv_expression(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_error_t *err);

static subtilis_exp_t *prv_variable(subtilis_parser_t *p, const char *tbuf,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	const subtilis_symbol_t *s;
	subtilis_exp_type_t type;
	subtilis_op_instr_type_t itype;

	op1.reg = SUBTILIS_IR_REG_LOCAL;
	s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (!s) {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (!s) {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			return NULL;
		}
		op1.reg = SUBTILIS_IR_REG_GLOBAL;
	}

	if (s->t == SUBTILIS_TYPE_INTEGER) {
		type = SUBTILIS_EXP_INTEGER;
		itype = SUBTILIS_OP_INSTR_LOADO_I32;
	} else if (s->t == SUBTILIS_TYPE_REAL) {
		type = SUBTILIS_EXP_REAL;
		itype = SUBTILIS_OP_INSTR_LOADO_REAL;
	} else {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	if (!s->is_reg) {
		op2.integer = s->loc;
		reg = subtilis_ir_section_add_instr(p->current, itype, op1, op2,
						    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	} else {
		reg = s->loc;
	}

	return subtilis_exp_new_var(type, reg, err);
}

static subtilis_exp_t *prv_bracketed_exp(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_unary_minus_exp(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_unary_minus(p, e, err);
}

static subtilis_exp_t *prv_not_exp(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_not(p, e, err);
}

static subtilis_exp_t *prv_gettime(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_GETTIME, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return e;

on_error:
	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_parse_bracketed_exp(subtilis_parser_t *p,
					       subtilis_token_t *t,
					       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return NULL;
	}

	e = prv_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_real_bracketed_exp(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = prv_parse_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_to_real(p, e, err);
}

static subtilis_exp_t *prv_real_unary_fn(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_op_instr_type_t itype,
					 double (*const_fn)(double),
					 subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;

	e = prv_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type == SUBTILIS_EXP_CONST_REAL) {
		e2 = subtilis_exp_new_real(const_fn(e->exp.ir_op.real), err);
	} else {
		reg = subtilis_ir_section_add_instr2(p->current, itype,
						     e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = subtilis_exp_new_var(SUBTILIS_EXP_REAL, reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_integer_bracketed_exp(subtilis_parser_t *p,
						 subtilis_token_t *t,
						 subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = prv_parse_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_to_int(p, e, err);
}

static subtilis_exp_t *prv_rad(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_t *e3 = NULL;
	const double radian = ((22 / 7.0) / 180);

	e = prv_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type == SUBTILIS_EXP_CONST_REAL) {
		e2 = subtilis_exp_new_real(e->exp.ir_op.real * radian, err);
	} else {
		e3 = subtilis_exp_new_real(radian, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = subtilis_exp_mul(p, e, e3, err);
		e = NULL;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_pi(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_exp_new_real(22 / 7.0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_sin(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_SIN, sin, err);
}

static subtilis_exp_t *prv_cos(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_COS, cos, err);
}

static subtilis_exp_t *prv_rnd(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *a = NULL;
	subtilis_exp_t *c = NULL;
	subtilis_exp_t *m = NULL;
	subtilis_exp_t *e_dup = NULL;
	subtilis_exp_t *top_bit = NULL;

	a = subtilis_exp_new_int32(1664525, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	c = subtilis_exp_new_int32(1013904223, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_variable(p, subtilis_rnd_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_mul(p, a, e, err);
	a = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_add(p, e, c, err);
	c = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_exp_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "(")) {
			m = prv_bracketed_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			m = subtilis_exp_to_int(p, m, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			subtilis_lexer_get(p->l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			top_bit =
			    subtilis_exp_new_int32((int32_t)0x7fffffff, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			e = subtilis_exp_and(p, e, top_bit, err);
			top_bit = NULL;
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			e = subtilis_exp_mod(p, e, m, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			top_bit = subtilis_exp_new_int32(1, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			e = subtilis_exp_add(p, e, top_bit, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
	}

	prv_assign_hidden(p, subtilis_rnd_hidden_var, SUBTILIS_TYPE_INTEGER,
			  e_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(e);
	subtilis_exp_delete(a);
	subtilis_exp_delete(c);

	return NULL;
}

static subtilis_exp_t *prv_sqr(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_SQR, sqrt, err);
}

static subtilis_exp_t *prv_get(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e;

	reg = subtilis_ir_section_add_instr1(p->current, SUBTILIS_OP_INSTR_GET,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_inkey_const(subtilis_parser_t *p,
				       subtilis_token_t *t, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t itype;
	subtilis_ir_operand_t op1;
	int32_t inkey_value = e->exp.ir_op.integer;

	if (inkey_value == -256) {
		reg = subtilis_ir_section_add_instr1(
		    p->current, SUBTILIS_OP_INSTR_OS_BYTE_ID, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		if (inkey_value >= 0)
			itype = SUBTILIS_OP_INSTR_GET_TO;
		else
			itype = SUBTILIS_OP_INSTR_INKEY;

		e = subtilis_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op1.reg = e->exp.ir_op.reg;
		;
		reg =
		    subtilis_ir_section_add_instr2(p->current, itype, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_exp_delete(e);

	return subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_inkey(subtilis_parser_t *p, subtilis_token_t *t,
				 subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;
	subtilis_ir_operand_t end_label;

	e = prv_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type == SUBTILIS_EXP_CONST_INTEGER)
		return prv_inkey_const(p, t, e, err);

	/*
	 * TODO: This needs to go in a builtin.  It's too large to inline.
	 */

	op0.reg = p->current->reg_counter++;

	op1.reg = e->exp.ir_op.reg;
	op2.integer = 0;
	cond.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTEI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);
	end_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_GET_TO, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = -256;
	cond.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_OS_BYTE_ID, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_INKEY, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, op0.reg, err);

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_get_point(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	return prv_bracketed_2_int_args(p, t, SUBTILIS_OP_INSTR_POINT, err);
}

static subtilis_exp_t *prv_get_tint(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_bracketed_2_int_args(p, t, SUBTILIS_OP_INSTR_TINT, err);
}

static subtilis_exp_t *prv_priority1(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	tbuf = subtilis_token_get_text(t);
	switch (t->type) {
	case SUBTILIS_TOKEN_INTEGER:
		e = subtilis_exp_new_int32(t->tok.integer, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_REAL:
		e = subtilis_exp_new_real(t->tok.real, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_OPERATOR:
		if (!strcmp(tbuf, "(")) {
			e = prv_bracketed_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			if (!strcmp(tbuf, "-")) {
				e = prv_unary_minus_exp(p, t, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;

				/* we don't want to read another token here.It's
				 * already been read by the recursive call to
				 * prv_priority1.
				 */

				return e;
			}
			subtilis_error_set_exp_expected(
			    err, "( or - ", p->l->stream->name, p->l->line);
			goto cleanup;
		}
		break;
	case SUBTILIS_TOKEN_IDENTIFIER:
		e = prv_variable(p, tbuf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_KEYWORD:
		switch (t->tok.keyword.type) {
		case SUBTILIS_KEYWORD_TRUE:
			e = subtilis_exp_new_int32(-1, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_KEYWORD_FALSE:
			e = subtilis_exp_new_int32(0, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_KEYWORD_NOT:
			e = prv_not_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			return e;
		case SUBTILIS_KEYWORD_FN:
			return prv_call(p, t, err);
		case SUBTILIS_KEYWORD_TIME:
			return prv_gettime(p, t, err);
		case SUBTILIS_KEYWORD_SIN:
			return prv_sin(p, t, err);
		case SUBTILIS_KEYWORD_COS:
			return prv_cos(p, t, err);
		case SUBTILIS_KEYWORD_RND:
			return prv_rnd(p, t, err);
		case SUBTILIS_KEYWORD_SQR:
			return prv_sqr(p, t, err);
		case SUBTILIS_KEYWORD_RAD:
			return prv_rad(p, t, err);
		case SUBTILIS_KEYWORD_PI:
			return prv_pi(p, t, err);
		case SUBTILIS_KEYWORD_GET:
			return prv_get(p, t, err);
		case SUBTILIS_KEYWORD_INKEY:
			return prv_inkey(p, t, err);
		case SUBTILIS_KEYWORD_POINT:
			return prv_get_point(p, t, err);
		case SUBTILIS_KEYWORD_TINT:
			return prv_get_tint(p, t, err);
		default:
			subtilis_error_set_exp_expected(
			    err, "Unexpected keyword in expression",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}
		break;
	default:
		subtilis_error_set_exp_expected(err, tbuf, p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_priority2(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	return prv_priority1(p, t, err);
}

static subtilis_exp_t *prv_priority3(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority2(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while ((t->type == SUBTILIS_TOKEN_OPERATOR) ||
	       (t->type == SUBTILIS_TOKEN_KEYWORD)) {
		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_KEYWORD) {
			if (!strcmp(tbuf, "DIV"))
				exp_fn = subtilis_exp_div;
			else if (!strcmp(tbuf, "MOD"))
				exp_fn = subtilis_exp_mod;
			else
				break;
		} else if (!strcmp(tbuf, "*")) {
			exp_fn = subtilis_exp_mul;
		} else if (!strcmp(tbuf, "/")) {
			exp_fn = subtilis_exp_divr;
		} else {
			break;
		}
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority2(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority4(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority3(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	tbuf = subtilis_token_get_text(t);
	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "+"))
			exp_fn = subtilis_exp_add;
		else if (!strcmp(tbuf, "-"))
			exp_fn = subtilis_exp_sub;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority3(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority5(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority4(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "="))
			exp_fn = subtilis_exp_eq;
		else if (!strcmp(tbuf, "<>"))
			exp_fn = subtilis_exp_neq;
		else if (!strcmp(tbuf, ">"))
			exp_fn = subtilis_exp_gt;
		else if (!strcmp(tbuf, "<="))
			exp_fn = subtilis_exp_lte;
		else if (!strcmp(tbuf, "<"))
			exp_fn = subtilis_exp_lt;
		else if (!strcmp(tbuf, ">="))
			exp_fn = subtilis_exp_gte;
		else if (!strcmp(tbuf, "<<"))
			exp_fn = subtilis_exp_lsl;
		else if (!strcmp(tbuf, ">>"))
			exp_fn = subtilis_exp_lsr;
		else if (!strcmp(tbuf, ">>>"))
			exp_fn = subtilis_exp_asr;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority4(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority6(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;

	e1 = prv_priority5(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (t->type == SUBTILIS_TOKEN_KEYWORD &&
	       t->tok.keyword.type == SUBTILIS_KEYWORD_AND) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority5(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = subtilis_exp_and(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority7(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority6(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (t->type == SUBTILIS_TOKEN_KEYWORD) {
		if (t->tok.keyword.type == SUBTILIS_KEYWORD_EOR)
			exp_fn = subtilis_exp_eor;
		else if (t->tok.keyword.type == SUBTILIS_KEYWORD_OR)
			exp_fn = subtilis_exp_or;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority6(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_expression(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return prv_priority7(p, t, err);
}

static subtilis_exp_t *prv_numeric_expression(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = prv_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	if ((e->type != SUBTILIS_EXP_CONST_INTEGER) &&
	    (e->type != SUBTILIS_EXP_CONST_REAL) &&
	    (e->type != SUBTILIS_EXP_INTEGER) &&
	    (e->type != SUBTILIS_EXP_REAL)) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_int_var_expression(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;

	e = prv_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static void prv_assign_to_reg(subtilis_parser_t *p, subtilis_token_t *t,
			      const char *tbuf, size_t loc, subtilis_exp_t *e,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		instr = SUBTILIS_OP_INSTR_MOVI_I32;
		break;
	case SUBTILIS_EXP_INTEGER:
		instr = SUBTILIS_OP_INSTR_MOV;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		instr = SUBTILIS_OP_INSTR_MOVI_REAL;
		break;
	case SUBTILIS_EXP_REAL:
		instr = SUBTILIS_OP_INSTR_MOVFP;
		break;
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	}
	op0.reg = loc;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, instr, op0, e->exp.ir_op,
					  op2, err);

cleanup:
	subtilis_exp_delete(e);
}

static void prv_assign_to_mem(subtilis_parser_t *p, const char *tbuf,
			      subtilis_ir_operand_t op1, size_t loc,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t instr;
	subtilis_ir_operand_t op2;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(e);
		e = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		instr = SUBTILIS_OP_INSTR_STOREO_I32;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(e);
		e = subtilis_exp_new_var(SUBTILIS_EXP_REAL, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		instr = SUBTILIS_OP_INSTR_STOREO_REAL;
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	case SUBTILIS_EXP_INTEGER:
		instr = SUBTILIS_OP_INSTR_STOREO_I32;
		break;
	case SUBTILIS_EXP_REAL:
		instr = SUBTILIS_OP_INSTR_STOREO_REAL;
		break;
	case SUBTILIS_EXP_STRING:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	}
	op2.integer = loc;
	subtilis_ir_section_add_instr_reg(p->current, instr, e->exp.ir_op, op1,
					  op2, err);

cleanup:
	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_coerce_type(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_type_t type,
				       subtilis_error_t *err)
{
	switch (type) {
	case SUBTILIS_TYPE_REAL:
		e = subtilis_exp_to_real(p, e, err);
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = subtilis_exp_to_int(p, e, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		subtilis_exp_delete(e);
		e = NULL;
	}
	return e;
}

static void prv_assign_hidden(subtilis_parser_t *p, const char *var_name,
			      subtilis_type_t id_type, subtilis_exp_t *e,
			      subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t op1;

	s = subtilis_symbol_table_lookup(p->st, var_name);
	if (!s) {
		s = subtilis_symbol_table_insert(p->st, var_name, id_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = SUBTILIS_IR_REG_GLOBAL;

	e = prv_coerce_type(p, e, s->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->is_reg)
		prv_assign_to_reg(p, NULL, var_name, s->loc, e, err);
	else
		prv_assign_to_mem(p, var_name, op1, s->loc, e, err);

	return;

cleanup:

	subtilis_exp_delete(e);
}

static void prv_assignment(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;

	tbuf = subtilis_token_get_text(t);
	s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (s) {
		op1.reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (s) {
			op1.reg = SUBTILIS_IR_REG_GLOBAL;
		} else if (p->current == p->main) {
			s = subtilis_symbol_table_insert(p->st, tbuf,
							 t->tok.id_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			op1.reg = SUBTILIS_IR_REG_GLOBAL;
		} else {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	if (strcmp(tbuf, "=") && strcmp(tbuf, "+=") && strcmp(tbuf, "-=")) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Ownership of e is passed to the following functions. */

	e = prv_coerce_type(p, e, s->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->is_reg)
		prv_assign_to_reg(p, t, tbuf, s->loc, e, err);
	else
		prv_assign_to_mem(p, tbuf, op1, s->loc, e, err);
}

static void prv_let(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	prv_assignment(p, t, err);
}

static void prv_parse_locals(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err)
{
	const char *tbuf;
	size_t reg_num;
	subtilis_ir_operand_t op1;
	subtilis_type_t type;

	while ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	       (t->tok.keyword.type == SUBTILIS_KEYWORD_LOCAL)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		if (subtilis_symbol_table_lookup(p->local_st, tbuf)) {
			subtilis_error_set_already_defined(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		switch (t->tok.id_type) {
		case SUBTILIS_TYPE_INTEGER:
			op1.integer = 0;
			reg_num = subtilis_ir_section_add_instr2(
			    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			type = SUBTILIS_TYPE_INTEGER;
			break;
		case SUBTILIS_TYPE_REAL:
			op1.real = 0;
			reg_num = subtilis_ir_section_add_instr2(
			    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, op1, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			type = SUBTILIS_TYPE_REAL;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			return;
		}

		(void)subtilis_symbol_table_insert_reg(p->local_st, tbuf, type,
						       reg_num, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_mode(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = prv_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_MODE_I32, e->exp.ir_op, err);

	subtilis_exp_delete(e);
}

static void prv_statement_int_args(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_exp_t **e, size_t expected,
				   subtilis_error_t *err)
{
	size_t i;
	const char *tbuf;

	e[0] = prv_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 1; i < expected; i++) {
		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return;
		}

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		e[i] = prv_int_var_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_bracketed_int_args(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_exp_t **e, size_t expected,
				   subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_statement_int_args(p, t, e, expected, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, ")")) {
		subtilis_error_set_exp_expected(err, ") ", p->l->stream->name,
						p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
}

static subtilis_exp_t *prv_bracketed_2_int_args(subtilis_parser_t *p,
						subtilis_token_t *t,
						subtilis_op_instr_type_t itype,
						subtilis_error_t *err)
{
	subtilis_exp_t *e[2];
	size_t reg;
	size_t i;
	subtilis_exp_t *retval = NULL;

	memset(&e, 0, sizeof(e));

	prv_bracketed_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	reg = subtilis_ir_section_add_instr(p->current, itype, e[0]->exp.ir_op,
					    e[1]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	retval = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);

cleanup:

	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);

	return retval;
}

static void prv_plot(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[3];

	memset(&e, 0, sizeof(e));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_statement_int_args(p, t, e, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[2]->exp.ir_op, err);

cleanup:
	for (i = 0; i < 3; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_simple_plot(subtilis_parser_t *p, subtilis_token_t *t,
			    int32_t plot_code, subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[3];

	memset(&e, 0, sizeof(e));

	prv_statement_int_args(p, t, &e[1], 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[0] = subtilis_exp_new_int32(plot_code, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e[0] = subtilis_exp_to_var(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[2]->exp.ir_op, err);

cleanup:
	for (i = 0; i < 3; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_move_draw(subtilis_parser_t *p, subtilis_token_t *t,
			  int32_t plot_code, subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_BY)) {
		plot_code -= 4;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_simple_plot(p, t, plot_code, err);
}

static void prv_move(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	prv_move_draw(p, t, 4, err);
}

static void prv_fill(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_simple_plot(p, t, 133, err);
}

static void prv_line(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_simple_plot(p, t, 4, err);

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	if (strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_simple_plot(p, t, 5, err);
}

static void prv_circle(subtilis_parser_t *p, subtilis_token_t *t,
		       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e[3];
	size_t i;
	int32_t plot_code = 145;

	memset(&e, 0, sizeof(e));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_FILL)) {
		plot_code = 153;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_simple_plot(p, t, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	if (strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	e[0] = subtilis_exp_new_int32(plot_code, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e[0] = subtilis_exp_to_var(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[1] = prv_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[2] = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[2] = subtilis_exp_to_var(p, e[2], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[2]->exp.ir_op, err);

cleanup:

	for (i = 0; i < 3; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_rectangle_outline(subtilis_parser_t *p, subtilis_token_t *t,
				  subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[6];

	memset(&e, 0, sizeof(e));

	e[0] = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e[0] = subtilis_exp_to_var(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[1] = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[1] = subtilis_exp_to_var(p, e[1], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	;

	prv_statement_int_args(p, t, &e[2], 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[4] = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, e[2]->exp.ir_op.reg,
				    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[4] = subtilis_exp_unary_minus(p, e[4], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[5] = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, e[3]->exp.ir_op.reg,
				    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[5] = subtilis_exp_unary_minus(p, e[5], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[2]->exp.ir_op,
					  e[1]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[3]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[4]->exp.ir_op,
					  e[1]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[5]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:

	for (i = 0; i < 6; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_rectangle(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;
	bool fill = false;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_FILL)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		fill = true;
	}

	prv_simple_plot(p, t, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	if (strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (fill)
		prv_simple_plot(p, t, 97, err);
	else
		prv_rectangle_outline(p, t, err);
}

static void prv_draw(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	prv_move_draw(p, t, 5, err);
}

static void prv_point(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	int32_t plot_code = 69;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * TODO: Implement POINT TO, once I figure out how
	 */

	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_BY)) {
		plot_code -= 4;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_simple_plot(p, t, plot_code, err);
}

static void prv_gcol(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[2];
	subtilis_ir_operand_t op2;

	memset(&e, 0, sizeof(e));
	memset(&op2, 0, sizeof(op2));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_statement_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_GCOL,
					  e[0]->exp.ir_op, e[1]->exp.ir_op, op2,
					  err);

cleanup:
	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_origin(subtilis_parser_t *p, subtilis_token_t *t,
		       subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[2];
	subtilis_ir_operand_t op2;

	memset(&e, 0, sizeof(e));
	memset(&op2, 0, sizeof(op2));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_statement_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_ORIGIN,
					  e[0]->exp.ir_op, e[1]->exp.ir_op, op2,
					  err);

cleanup:
	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);
}

static void prv_cls(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_CLS,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(p->l, t, err);
}

static void prv_clg(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_CLG,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(p->l, t, err);
}

static void prv_on(subtilis_parser_t *p, subtilis_token_t *t,
		   subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_ON,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(p->l, t, err);
}

static void prv_off(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_OFF,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(p->l, t, err);
}

static void prv_wait(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_WAIT,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(p->l, t, err);
}

static void prv_vdu_byte(subtilis_parser_t *p, subtilis_exp_t *e,
			 subtilis_error_t *err)
{
	subtilis_op_instr_type_t itype;

	if (e->type == SUBTILIS_EXP_INTEGER) {
		itype = SUBTILIS_OP_INSTR_VDU;
	} else if (e->type == SUBTILIS_EXP_CONST_INTEGER) {
		itype = SUBTILIS_OP_INSTR_VDUI;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_ir_section_add_instr_no_reg(p->current, itype, e->exp.ir_op,
					     err);
}

static void prv_vdu_2bytes(subtilis_parser_t *p, subtilis_exp_t *e,
			   subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	if (e->type == SUBTILIS_EXP_INTEGER) {
		op2.integer = 0xff;
		op1.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ANDI_I32, e->exp.ir_op, op2,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDU, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op2.integer = 8;
		op1.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LSRI_I32, e->exp.ir_op, op2,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDU, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else if (e->type == SUBTILIS_EXP_CONST_INTEGER) {
		op1.integer = e->exp.ir_op.integer & 0xff;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDUI, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op1.integer = e->exp.ir_op.integer >> 8;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDUI, op1, err);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}
}

static void prv_vdu_1byte9zeros(subtilis_parser_t *p, subtilis_exp_t *e,
				subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t op1;

	prv_vdu_byte(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	for (i = 0; i < 9; i++) {
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDUI, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_vdu_brackets(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err)
{
	subtilis_exp_t *e;
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	do {
		e = prv_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		e = subtilis_exp_to_int(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_OPERATOR) {
			if (strcmp(tbuf, ",") == 0) {
				prv_vdu_byte(p, e, err);
			} else if (strcmp(tbuf, "]") == 0) {
				prv_vdu_byte(p, e, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				subtilis_exp_delete(e);
				e = NULL;
				break;
			} else if (strcmp(tbuf, ";") == 0) {
				prv_vdu_2bytes(p, e, err);
			} else if (strcmp(tbuf, "|") == 0) {
				prv_vdu_1byte9zeros(p, e, err);
			} else {
				subtilis_error_set_expected(
				    err, ">, ','. ; or |", tbuf,
				    p->l->stream->name, p->l->line);
				goto cleanup;
			}
		} else {
			subtilis_error_set_expected(err, "], ','. ; or |", tbuf,
						    p->l->stream->name,
						    p->l->line);
			goto cleanup;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_exp_delete(e);
		e = NULL;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (t->type == SUBTILIS_TOKEN_OPERATOR) {
			tbuf = subtilis_token_get_text(t);
			if (strcmp(tbuf, "]") == 0)
				break;
		}
	} while (true);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return;

cleanup:

	subtilis_exp_delete(e);
}

static void prv_vdu(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_exp_t *e;
	const char *tbuf;

	/*
	 * NOTE that there is a problem with the grammar here.  We cannot
	 * allow a VDU statement to terminate with an operator otherwise
	 * we will not know whether a variable following the operator
	 * should be sent to the VDU driver or be used to assign a value to
	 * a variable, e.g,
	 *
	 * VDU 19,2,4;0;
	 * A% = 10
	 *
	 * will fail to parse.  Consequently, subtilis will allow
	 *
	 * VDU [19,2,4;0;]
	 */

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (strcmp(tbuf, "[") == 0) {
			prv_vdu_brackets(p, t, err);
			return;
		}
	}

	do {
		e = prv_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		e = subtilis_exp_to_int(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_OPERATOR) {
			if (strcmp(tbuf, ",") == 0) {
				prv_vdu_byte(p, e, err);
			} else if (strcmp(tbuf, ";") == 0) {
				prv_vdu_2bytes(p, e, err);
			} else if (strcmp(tbuf, "|") == 0) {
				prv_vdu_1byte9zeros(p, e, err);
			} else {
				subtilis_error_set_expected(
				    err, ",. ; or |", tbuf, p->l->stream->name,
				    p->l->line);
				goto cleanup;
			}
		} else {
			prv_vdu_byte(p, e, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_exp_delete(e);
		e = NULL;

		if (t->type != SUBTILIS_TOKEN_OPERATOR)
			break;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

	} while (true);

cleanup:
	subtilis_exp_delete(e);
}

static void prv_print(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (e->type) {
	case SUBTILIS_EXP_INTEGER:
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_I32, e->exp.ir_op, err);
		break;
	case SUBTILIS_EXP_REAL:
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_FP, e->exp.ir_op, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

cleanup:
	subtilis_exp_delete(e);
}

static subtilis_keyword_type_t
prv_if_compund(subtilis_parser_t *p, subtilis_token_t *t, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;
	subtilis_keyword_type_t key_type = SUBTILIS_KEYWORD_MAX;
	unsigned int start;

	p->level++;
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
			prv_assignment(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		}
		if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}

		key_type = t->tok.keyword.type;
		if ((key_type == SUBTILIS_KEYWORD_ELSE) ||
		    (key_type == SUBTILIS_KEYWORD_ENDIF))
			break;

		fn = keyword_fns[key_type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}
		fn(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return key_type;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	p->level--;
	return key_type;
}

static void prv_statement(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_keyword_fn fn;
	const char *tbuf;
	subtilis_keyword_type_t key_type;

	if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
		prv_assignment(p, t, err);
		return;
	} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_keyword_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	key_type = t->tok.keyword.type;
	fn = keyword_fns[key_type];
	if (!fn) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		return;
	}
	fn(p, t, err);
}

static void prv_compound(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_keyword_type_t end_key, subtilis_error_t *err)
{
	unsigned int start;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == end_key)) {
			if ((end_key != SUBTILIS_KEYWORD_ENDPROC) ||
			    (p->level == 1))
				break;
		}
		prv_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);
	p->level--;
}

static void prv_fn_compound(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	unsigned int start;
	const char *tbuf;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		tbuf = subtilis_token_get_text(t);
		if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    !strcmp(tbuf, "<-") && (p->level == 1))
			break;

		prv_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);
	p->level--;
}

static subtilis_exp_t *prv_conditional_exp(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_ir_operand_t *cond,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type == SUBTILIS_EXP_CONST_INTEGER) {
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		cond->reg = reg;
	} else if (e->type == SUBTILIS_EXP_INTEGER) {
		cond->reg = e->exp.ir_op.reg;
	} else {
		subtilis_error_set_integer_exp_expected(err, p->l->stream->name,
							p->l->line);
		goto cleanup;
	}

	return e;

cleanup:
	subtilis_exp_delete(e);
	return NULL;
}

static void prv_if(subtilis_parser_t *p, subtilis_token_t *t,
		   subtilis_error_t *err)
{
	subtilis_exp_t *e;
	const char *tbuf;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;
	subtilis_ir_operand_t end_label;
	subtilis_keyword_type_t key_type;

	e = prv_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (t->tok.keyword.type != SUBTILIS_KEYWORD_THEN)) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_expected(err, "THEN", tbuf,
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	key_type = prv_if_compund(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (key_type == SUBTILIS_KEYWORD_ELSE) {
		end_label.reg = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_label(p->current, false_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		prv_compound(p, t, SUBTILIS_KEYWORD_ENDIF, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_label(p->current, end_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		subtilis_ir_section_add_label(p->current, false_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

static void prv_while(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;

	start_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_compound(p, t, SUBTILIS_KEYWORD_ENDWHILE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

static void prv_repeat(subtilis_parser_t *p, subtilis_token_t *t,
		       subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;

	start_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_compound(p, t, SUBTILIS_KEYWORD_UNTIL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_increment_var(subtilis_parser_t *p,
					 const char *var_name,
					 subtilis_exp_t *inc,
					 subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_exp_t *var = NULL;
	subtilis_ir_operand_t op1;

	var = prv_variable(p, var_name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	inc = subtilis_exp_add(p, var, inc, err);
	var = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = subtilis_symbol_table_lookup(p->local_st, var_name);
	if (!s) {
		s = subtilis_symbol_table_lookup(p->st, var_name);
		if (!s) {
			subtilis_error_set_assertion_failed(err);
			goto cleanup;
		}
		op1.reg = SUBTILIS_IR_REG_GLOBAL;
	} else {
		op1.reg = SUBTILIS_IR_REG_LOCAL;
	}

	if (s->is_reg)
		prv_assign_to_reg(p, NULL, var_name, s->loc, inc, err);
	else
		prv_assign_to_mem(p, var_name, op1, s->loc, inc, err);
	inc = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return prv_variable(p, var_name, err);

cleanup:

	subtilis_exp_delete(var);
	subtilis_exp_delete(inc);

	return NULL;
}

typedef enum {
	SUBTILIS_STEP_CONST_INC,
	SUBTILIS_STEP_CONST_DEC,
	SUBTILIS_STEP_VAR,
	SUBTILIS_STEP_MAX
} subtilis_step_type_t;

static subtilis_step_type_t prv_compute_step_type(subtilis_parser_t *p,
						  subtilis_exp_t *step,
						  subtilis_error_t *err)
{
	switch (step->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		if (step->exp.ir_op.integer == 0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.integer < 0 ? SUBTILIS_STEP_CONST_DEC
						   : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_EXP_CONST_REAL:
		if (step->exp.ir_op.real == 0.0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.real < 0.0 ? SUBTILIS_STEP_CONST_DEC
						  : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_EXP_INTEGER:
	case SUBTILIS_EXP_REAL:
		return SUBTILIS_STEP_VAR;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}

	return SUBTILIS_STEP_MAX;
}

static void prv_for_loop_start(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_ir_operand_t *start_label,
			       subtilis_ir_operand_t *true_label,
			       subtilis_error_t *err)
{
	start_label->reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label->reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compound(p, t, SUBTILIS_KEYWORD_NEXT, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label->reg = subtilis_ir_section_new_label(p->current);
}

static void prv_for_step_real_var(subtilis_parser_t *p, subtilis_token_t *t,
				  const char *var_name, subtilis_exp_t *to,
				  subtilis_exp_t *step, subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t true_label2;
	subtilis_ir_operand_t inc_label;
	subtilis_ir_operand_t dec_label;
	subtilis_exp_t *step_dup = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *var_dup = NULL;
	subtilis_exp_t *to_dup = NULL;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *zero = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	step_dup = subtilis_exp_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, var_name, step, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (var->type != SUBTILIS_EXP_REAL) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	var_dup = subtilis_exp_dup(var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_dup = subtilis_exp_dup(to, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	conde = subtilis_exp_gt(p, step_dup, zero, err);
	step_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	inc_label.reg = subtilis_ir_section_new_label(p->current);
	dec_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, inc_label,
					  dec_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, inc_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_exp_gt(p, var, to, err);
	var = NULL;
	to = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, dec_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_exp_lt(p, var_dup, to_dup, err);
	var_dup = NULL;
	to_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	true_label2.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label2,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label2.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var_dup);
	subtilis_exp_delete(to_dup);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step_dup);
	subtilis_exp_delete(step);
}

static void prv_for_step_int_var(subtilis_parser_t *p, subtilis_token_t *t,
				 const char *var_name, subtilis_exp_t *to,
				 subtilis_exp_t *step, subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t final_label;
	subtilis_ir_operand_t op2;
	size_t eor_reg;
	size_t step_dir_reg = SIZE_MAX;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *eor_var = NULL;
	subtilis_exp_t *eor_var_dup = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *sub = NULL;
	subtilis_exp_t *top_bit = NULL;

	/*
	 * TODO: Add a runtime check for step variable of 0.
	 * and generate an error.
	 */

	op2.integer = (int32_t)0x80000000;
	step_dir_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, step->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, var_name, step, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (var->type != SUBTILIS_EXP_INTEGER) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	sub = subtilis_exp_sub(p, to, var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	var = NULL;
	to = NULL;

	op2.reg = step_dir_reg;
	eor_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, sub->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eor_var = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, eor_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	eor_var_dup = subtilis_exp_dup(eor_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	conde = subtilis_exp_lt(p, eor_var, zero, err);
	eor_var = NULL;

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, step_dir_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_exp_neq(p, eor_var_dup, top_bit, err);
	eor_var_dup = NULL;
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	final_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, final_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, final_label.reg, err);

cleanup:

	subtilis_exp_delete(sub);
	subtilis_exp_delete(eor_var_dup);
	subtilis_exp_delete(eor_var);
	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step);
}

static void prv_for_step_const(subtilis_parser_t *p, subtilis_token_t *t,
			       const char *var_name, subtilis_exp_t *to,
			       subtilis_exp_t *step,
			       subtilis_step_type_t step_type,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *var = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, var_name, step, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (step_type == SUBTILIS_STEP_CONST_INC) {
		conde = subtilis_exp_gt(p, var, to, err);
		var = NULL;
		to = NULL;
	} else {
		conde = subtilis_exp_lt(p, var, to, err);
		var = NULL;
		to = NULL;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step);
}

static void prv_for_no_step(subtilis_parser_t *p, subtilis_token_t *t,
			    const char *var_name, subtilis_exp_t *to,
			    subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_exp_t *inc = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *conde = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	inc = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, var_name, inc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	conde = subtilis_exp_gt(p, var, to, err);
	var = NULL;
	to = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
}

static void prv_for(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t var_type;
	char *var_name = NULL;
	subtilis_exp_t *to = NULL;
	subtilis_exp_t *step = NULL;
	subtilis_step_type_t step_type = SUBTILIS_STEP_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	var_type = t->tok.id_type;
	if ((var_type != SUBTILIS_TYPE_REAL) &&
	    (var_type != SUBTILIS_TYPE_INTEGER)) {
		subtilis_error_set_numeric_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	var_name = malloc(subtilis_buffer_get_size(&t->buf));
	if (!var_name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(var_name, tbuf);

	prv_assignment(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (t->tok.keyword.type != SUBTILIS_KEYWORD_TO)) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_expected(err, "TO", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	to = prv_numeric_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_STEP)) {
		step = prv_numeric_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		step_type = prv_compute_step_type(p, step, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		/* Takes ownership of to and step */

		if (step_type == SUBTILIS_STEP_VAR) {
			step = subtilis_exp_copy_var(p, step, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			if (var_type == SUBTILIS_TYPE_INTEGER)
				prv_for_step_int_var(p, t, var_name, to, step,
						     err);
			else
				prv_for_step_real_var(p, t, var_name, to, step,
						      err);
		} else {
			prv_for_step_const(p, t, var_name, to, step, step_type,
					   err);
		}
		step = NULL;
	} else {
		/* Takes ownership of to */

		prv_for_no_step(p, t, var_name, to, err);
	}
	to = NULL;

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);

cleanup:

	subtilis_exp_delete(step);
	free(var_name);
	subtilis_exp_delete(to);
}

static subtilis_type_t *prv_def_parameters(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   size_t *num_parameters,
					   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t *new_params;
	size_t new_max;
	size_t max_params = 0;
	size_t num_params = 0;
	size_t num_iparams = 0;
	size_t num_fparams = 0;
	subtilis_type_t *params = NULL;
	size_t reg_num;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")"))
		goto read_next;

	for (;;) {
		if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		/* TODO: maybe these checks should go in symbol table insert */

		if (subtilis_symbol_table_lookup(p->local_st, tbuf)) {
			subtilis_error_set_already_defined(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		switch (t->tok.id_type) {
		case SUBTILIS_TYPE_INTEGER:
			reg_num = SUBTILIS_IR_REG_TEMP_START + num_iparams;
			num_iparams++;
			break;
		case SUBTILIS_TYPE_REAL:
			reg_num = num_fparams++;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			goto on_error;
		}

		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, tbuf, t->tok.id_type, reg_num, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		if (num_params == max_params) {
			new_max = max_params + 16;
			new_params = realloc(params, new_max * sizeof(*params));
			if (!new_params) {
				subtilis_error_set_oom(err);
				goto on_error;
			}
			max_params = new_max;
			params = new_params;
		}
		params[num_params++] = t->tok.id_type;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		tbuf = subtilis_token_get_text(t);

		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		if (!strcmp(tbuf, ")"))
			goto read_next;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		tbuf = subtilis_token_get_text(t);
	}

read_next:

	*num_parameters = num_params;
	subtilis_lexer_get(p->l, t, err);
	return params;

on_error:

	free(params);
	return NULL;
}

static char *prv_proc_name_and_type(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_type_t *type,
				    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t id_type;
	char *name = NULL;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_expected(err, "PROC or FN", tbuf,
					    p->l->stream->name, p->l->line);
		return NULL;
	}

	if (t->tok.keyword.ftype != SUBTILIS_BUILTINS_MAX) {
		id_type = subtilis_builtin_list[t->tok.keyword.ftype].ret_type;
	} else {
		if (t->tok.keyword.type == SUBTILIS_KEYWORD_PROC)
			tbuf += 4;
		else if (t->tok.keyword.type == SUBTILIS_KEYWORD_FN)
			tbuf += 2;
		id_type = t->tok.keyword.id_type;
	}
	name = malloc(strlen(tbuf) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	*type = id_type;
	strcpy(name, tbuf);

	return name;
}

static void prv_add_fn_ret(subtilis_parser_t *p, subtilis_exp_t *e,
			   subtilis_type_t fn_type, subtilis_error_t *err)
{
	subtilis_op_instr_type_t type;

	/* Ownership of e is passed to prv_coerce_type */

	e = prv_coerce_type(p, e, fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		type = SUBTILIS_OP_INSTR_RETI_I32;
		break;
	case SUBTILIS_EXP_INTEGER:
		type = SUBTILIS_OP_INSTR_RET_I32;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		type = SUBTILIS_OP_INSTR_RETI_REAL;
		break;
	case SUBTILIS_EXP_REAL:
		type = SUBTILIS_OP_INSTR_RET_REAL;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	subtilis_ir_section_add_instr_no_reg(p->current, type, e->exp.ir_op,
					     err);

cleanup:

	subtilis_exp_delete(e);
}

static void prv_def(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_section_t *stype;
	subtilis_type_t fn_type;
	subtilis_exp_t *e;
	size_t num_params = 0;
	subtilis_type_t *params = NULL;
	subtilis_symbol_table_t *local_st = NULL;
	char *name = NULL;

	if (p->current != p->main) {
		subtilis_error_set_nested_procedure(err, p->l->stream->name,
						    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	name = prv_proc_name_and_type(p, t, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	local_st = subtilis_symbol_table_new(err);
	p->local_st = local_st;
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		params = prv_def_parameters(p, t, &num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	stype = subtilis_type_section_new(fn_type, num_params, params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	params = NULL;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, name, p->local_st->allocated, stype, SUBTILIS_BUILTINS_MAX,
	    p->l->stream->name, p->l->line, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_type_section_delete(stype);
		goto on_error;
	}

	prv_parse_locals(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	if (fn_type != SUBTILIS_TYPE_VOID) {
		prv_fn_compound(p, t, err);
		e = prv_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		/* Ownership of e is passed to add_fn_ret */

		prv_add_fn_ret(p, e, fn_type, err);
	} else {
		prv_compound(p, t, SUBTILIS_KEYWORD_ENDPROC, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_RET, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		subtilis_lexer_get(p->l, t, err);
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

on_error:

	free(params);
	subtilis_symbol_table_delete(local_st);
	free(name);
	p->local_st = p->main_st;
	p->current = p->main;
}

static subtilis_parser_param_t *prv_call_parameters(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    size_t *num_parameters,
						    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_parser_param_t *new_params;
	size_t nop;
	size_t new_max;
	size_t max_params = 0;
	size_t num_params = 0;
	subtilis_parser_param_t *params = NULL;
	subtilis_exp_t *e = NULL;

	for (;;) {
		e = prv_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		if (num_params == max_params) {
			new_max = max_params + 16;
			new_params = realloc(params, new_max * sizeof(*params));
			if (!new_params) {
				subtilis_error_set_oom(err);
				goto on_error;
			}
			max_params = new_max;
			params = new_params;
		}

		tbuf = subtilis_token_get_text(t);

		params[num_params].type = subtilis_exp_type(e);
		e = subtilis_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		nop = subtilis_ir_section_add_nop(p->current, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		params[num_params].reg = e->exp.ir_op.reg;
		params[num_params].nop = nop;
		num_params++;
		subtilis_exp_delete(e);
		e = NULL;

		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		if (!strcmp(tbuf, ")"))
			goto read_next;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}
	}

read_next:

	*num_parameters = num_params;
	subtilis_lexer_get(p->l, t, err);
	return params;

on_error:

	subtilis_exp_delete(e);
	free(params);
	return NULL;
}

static subtilis_ir_arg_t *prv_parser_to_ir_args(subtilis_parser_param_t *params,
						size_t num_params,
						subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_arg_t *args = malloc(sizeof(*args) * num_params);

	if (!args) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	for (i = 0; i < num_params; i++) {
		switch (params[i].type) {
		case SUBTILIS_TYPE_REAL:
			args[i].type = SUBTILIS_IR_REG_TYPE_REAL;
			break;
		case SUBTILIS_TYPE_INTEGER:
			args[i].type = SUBTILIS_IR_REG_TYPE_INTEGER;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			free(args);
			return NULL;
		}
		args[i].reg = params[i].reg;
		args[i].nop = params[i].nop;
	}

	return args;
}

static subtilis_type_t *prv_parser_to_ptypes(subtilis_parser_param_t *params,
					     size_t num_params,
					     subtilis_error_t *err)
{
	size_t i;
	subtilis_type_t *ptypes = malloc(sizeof(*ptypes) * num_params);

	if (!ptypes) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	for (i = 0; i < num_params; i++)
		ptypes[i] = params[i].type;

	return ptypes;
}

static subtilis_exp_t *prv_call(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t fn_type;
	subtilis_builtin_type_t ftype;
	subtilis_type_section_t *stype = NULL;
	subtilis_type_t *ptypes = NULL;
	char *name = NULL;
	subtilis_parser_param_t *params = NULL;
	size_t num_params = 0;
	subtilis_ir_arg_t *args = NULL;

	name = prv_proc_name_and_type(p, t, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	ftype = t->tok.keyword.ftype;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		params = prv_call_parameters(p, t, &num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = prv_parser_to_ir_args(params, num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		ptypes = prv_parser_to_ptypes(params, num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	free(params);

	stype = subtilis_type_section_new(fn_type, num_params, ptypes, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	/* Ownership of stypes, args and name passed to this function. */

	return subtilis_exp_add_call(p, name, ftype, stype, args, fn_type,
				     num_params, err);

on_error:

	free(params);
	free(ptypes);
	free(args);
	free(name);

	return NULL;
}

static void prv_proc(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	(void)prv_call(p, t, err);
}

static void prv_root(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_exp_t *zero;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_assign_hidden(p, subtilis_rnd_hidden_var, SUBTILIS_TYPE_INTEGER,
			  zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_parse_locals(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	p->main->locals = p->main_st->allocated;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		prv_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_check_call(subtilis_parser_t *p, subtilis_parser_call_t *call,
			   subtilis_error_t *err)
{
	size_t index;
	size_t i;
	subtilis_type_section_t *st;
	const char *expected_typname;
	const char *got_typname;
	size_t new_reg;
	subtilis_op_instr_type_t itype;
	subtilis_ir_reg_type_t reg_type;
	subtilis_ir_call_t *call_site;
	subtilis_type_section_t *ct = call->call_type;

	if (!subtilis_string_pool_find(p->prog->string_pool, call->name,
				       &index)) {
		if (!ct)
			subtilis_error_set_assertion_failed(err);
		else if (ct->return_type == SUBTILIS_TYPE_VOID)
			subtilis_error_set_unknown_procedure(
			    err, call->name, p->l->stream->name, call->line);
		else
			subtilis_error_set_unknown_function(
			    err, call->name, p->l->stream->name, call->line);
		return;
	}

	/*
	 * If the call has no type section that it is not a real function.
	 * It's an operator that is being implemented as a function call
	 * as the underlying CPU doesn't have an instruction that maps
	 * nicely onto the operator, e.g., DIV on ARM.  In these cases
	 * there's no need to perform the type checks as the parser has
	 * already done them.
	 */

	/*
	 * This assumes that builtins don't require any type promotions.
	 */

	if (!ct) {
		call->s->ops[call->index]->op.call.proc_id = index;
		return;
	}

	st = p->prog->sections[index]->type;

	if ((st->return_type == SUBTILIS_TYPE_VOID) &&
	    (ct->return_type != SUBTILIS_TYPE_VOID)) {
		subtilis_error_set_procedure_expected(
		    err, call->name, p->l->stream->name, call->line);
		return;
	} else if ((st->return_type != SUBTILIS_TYPE_VOID) &&
		   (ct->return_type == SUBTILIS_TYPE_VOID)) {
		subtilis_error_set_function_expected(
		    err, call->name, p->l->stream->name, call->line);
		return;
	}

	if (st->num_parameters != ct->num_parameters) {
		subtilis_error_set_bad_arg_count(
		    err, ct->num_parameters, st->num_parameters,
		    p->l->stream->name, call->line);
		return;
	}

	call_site = &call->s->ops[call->index]->op.call;
	for (i = 0; i < st->num_parameters; i++) {
		if (st->parameters[i] == ct->parameters[i])
			continue;

		if ((st->parameters[i] == SUBTILIS_TYPE_REAL) &&
		    (ct->parameters[i] == SUBTILIS_TYPE_INTEGER)) {
			itype = SUBTILIS_OP_INSTR_MOV_I32_FP;
			reg_type = SUBTILIS_IR_REG_TYPE_REAL;
		} else if ((st->parameters[i] == SUBTILIS_TYPE_INTEGER) &&
			   (ct->parameters[i] == SUBTILIS_TYPE_REAL)) {
			itype = SUBTILIS_OP_INSTR_MOV_FP_I32;
			reg_type = SUBTILIS_IR_REG_TYPE_INTEGER;
		} else {
			expected_typname =
			    subtilis_type_name(st->parameters[i]);
			got_typname = subtilis_type_name(ct->parameters[i]);
			subtilis_error_set_bad_arg_type(
			    err, i + 1, expected_typname, got_typname,
			    p->l->stream->name, call->line, __FILE__, __LINE__);
			return;
		}

		switch (call->s->ops[call->index]->type) {
		case SUBTILIS_OP_CALL:
		case SUBTILIS_OP_CALLI32:
		case SUBTILIS_OP_CALLREAL:
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			return;
		}

		new_reg = subtilis_ir_section_promote_nop(
		    call->s, call_site->args[i].nop, itype,
		    call_site->args[i].reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		call_site->args[i].reg = new_reg;
		call_site->args[i].type = reg_type;
	}

	call_site->proc_id = index;
}

static void prv_check_calls(subtilis_parser_t *p, subtilis_error_t *err)
{
	size_t i;

	for (i = 0; i < p->num_calls; i++) {
		prv_check_call(p, p->calls[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_parse(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_token_t *t = NULL;

	t = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_root(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_check_calls(p, err);

cleanup:

	subtilis_token_delete(t);
}

/* The ordering of this table is very important.  The functions it
 * contains must correspond to the enumerated types in
 * subtilis_keyword_type_t
 */

/* clang-format off */
static const subtilis_keyword_fn keyword_fns[] = {
	NULL, /* SUBTILIS_KEYWORD_ACS */
	NULL, /* SUBTILIS_KEYWORD_ADVAL */
	NULL, /* SUBTILIS_KEYWORD_AND */
	NULL, /* SUBTILIS_KEYWORD_APPEND */
	NULL, /* SUBTILIS_KEYWORD_ASC */
	NULL, /* SUBTILIS_KEYWORD_ASN */
	NULL, /* SUBTILIS_KEYWORD_ATN */
	NULL, /* SUBTILIS_KEYWORD_AUTO */
	NULL, /* SUBTILIS_KEYWORD_BEAT */
	NULL, /* SUBTILIS_KEYWORD_BEATS */
	NULL, /* SUBTILIS_KEYWORD_BGET_HASH */
	NULL, /* SUBTILIS_KEYWORD_BPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_BY */
	NULL, /* SUBTILIS_KEYWORD_CALL */
	NULL, /* SUBTILIS_KEYWORD_CASE */
	NULL, /* SUBTILIS_KEYWORD_CHAIN */
	NULL, /* SUBTILIS_KEYWORD_CHR_STR */
	prv_circle, /* SUBTILIS_KEYWORD_CIRCLE */
	NULL, /* SUBTILIS_KEYWORD_CLEAR */
	prv_clg, /* SUBTILIS_KEYWORD_CLG */
	NULL, /* SUBTILIS_KEYWORD_CLOSE_HASH */
	prv_cls, /* SUBTILIS_KEYWORD_CLS */
	NULL, /* SUBTILIS_KEYWORD_COLOR */
	NULL, /* SUBTILIS_KEYWORD_COLOUR */
	NULL, /* SUBTILIS_KEYWORD_COS */
	NULL, /* SUBTILIS_KEYWORD_COUNT */
	NULL, /* SUBTILIS_KEYWORD_CRUNCH */
	NULL, /* SUBTILIS_KEYWORD_DATA */
	prv_def, /* SUBTILIS_KEYWORD_DEF */
	NULL, /* SUBTILIS_KEYWORD_DEG */
	NULL, /* SUBTILIS_KEYWORD_DELETE */
	NULL, /* SUBTILIS_KEYWORD_DIM */
	NULL, /* SUBTILIS_KEYWORD_DIV */
	prv_draw, /* SUBTILIS_KEYWORD_DRAW */
	NULL, /* SUBTILIS_KEYWORD_EDIT */
	NULL, /* SUBTILIS_KEYWORD_ELLIPSE */
	NULL, /* SUBTILIS_KEYWORD_ELSE */
	NULL, /* SUBTILIS_KEYWORD_END */
	NULL, /* SUBTILIS_KEYWORD_ENDCASE */
	NULL, /* SUBTILIS_KEYWORD_ENDIF */
	NULL, /* SUBTILIS_KEYWORD_ENDPROC */
	NULL, /* SUBTILIS_KEYWORD_ENDWHILE */
	NULL, /* SUBTILIS_KEYWORD_EOF_HASH */
	NULL, /* SUBTILIS_KEYWORD_EOR */
	NULL, /* SUBTILIS_KEYWORD_ERL */
	NULL, /* SUBTILIS_KEYWORD_ERR */
	NULL, /* SUBTILIS_KEYWORD_ERROR */
	NULL, /* SUBTILIS_KEYWORD_EVAL */
	NULL, /* SUBTILIS_KEYWORD_EXP */
	NULL, /* SUBTILIS_KEYWORD_EXT_HASH */
	NULL, /* SUBTILIS_KEYWORD_FALSE */
	prv_fill, /* SUBTILIS_KEYWORD_FILL */
	NULL, /* SUBTILIS_KEYWORD_FN */
	prv_for, /* SUBTILIS_KEYWORD_FOR */
	prv_gcol, /* SUBTILIS_KEYWORD_GCOL */
	NULL, /* SUBTILIS_KEYWORD_GET */
	NULL, /* SUBTILIS_KEYWORD_GET_STR */
	NULL, /* SUBTILIS_KEYWORD_GET_STR_HASH */
	NULL, /* SUBTILIS_KEYWORD_GOSUB */
	NULL, /* SUBTILIS_KEYWORD_GOTO */
	NULL, /* SUBTILIS_KEYWORD_HELP */
	NULL, /* SUBTILIS_KEYWORD_HIMEM */
	prv_if, /* SUBTILIS_KEYWORD_IF */
	NULL, /* SUBTILIS_KEYWORD_INKEY */
	NULL, /* SUBTILIS_KEYWORD_INKEY_STR */
	NULL, /* SUBTILIS_KEYWORD_INPUT */
	NULL, /* SUBTILIS_KEYWORD_INPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_INSTALL */
	NULL, /* SUBTILIS_KEYWORD_INSTR */
	NULL, /* SUBTILIS_KEYWORD_INT */
	NULL, /* SUBTILIS_KEYWORD_LEFT_STR */
	NULL, /* SUBTILIS_KEYWORD_LEN */
	prv_let, /* SUBTILIS_KEYWORD_LET */
	NULL, /* SUBTILIS_KEYWORD_LIBRARY */
	prv_line, /* SUBTILIS_KEYWORD_LINE */
	NULL, /* SUBTILIS_KEYWORD_LIST */
	NULL, /* SUBTILIS_KEYWORD_LISTO */
	NULL, /* SUBTILIS_KEYWORD_LN */
	NULL, /* SUBTILIS_KEYWORD_LOAD */
	NULL, /* SUBTILIS_KEYWORD_LOCAL */
	NULL, /* SUBTILIS_KEYWORD_LOG */
	NULL, /* SUBTILIS_KEYWORD_LOMEM */
	NULL, /* SUBTILIS_KEYWORD_LVAR */
	NULL, /* SUBTILIS_KEYWORD_MID_STR */
	NULL, /* SUBTILIS_KEYWORD_MOD */
	prv_mode, /* SUBTILIS_KEYWORD_MODE */
	NULL, /* SUBTILIS_KEYWORD_MOUSE */
	prv_move, /* SUBTILIS_KEYWORD_MOVE */
	NULL, /* SUBTILIS_KEYWORD_NEW */
	NULL, /* SUBTILIS_KEYWORD_NEXT */
	NULL, /* SUBTILIS_KEYWORD_NOT */
	NULL, /* SUBTILIS_KEYWORD_OF */
	prv_off, /* SUBTILIS_KEYWORD_OFF */
	NULL, /* SUBTILIS_KEYWORD_OLD */
	prv_on, /* SUBTILIS_KEYWORD_ON */
	NULL, /* SUBTILIS_KEYWORD_OPENIN */
	NULL, /* SUBTILIS_KEYWORD_OPENOUT */
	NULL, /* SUBTILIS_KEYWORD_OPENUP */
	NULL, /* SUBTILIS_KEYWORD_OR */
	prv_origin, /* SUBTILIS_KEYWORD_ORIGIN */
	NULL, /* SUBTILIS_KEYWORD_OSCLI */
	NULL, /* SUBTILIS_KEYWORD_OTHERWISE */
	NULL, /* SUBTILIS_KEYWORD_OVERLAY */
	NULL, /* SUBTILIS_KEYWORD_PAGE */
	NULL, /* SUBTILIS_KEYWORD_PI */
	prv_plot, /* SUBTILIS_KEYWORD_PLOT */
	prv_point, /* SUBTILIS_KEYWORD_POINT */
	NULL, /* SUBTILIS_KEYWORD_POS */
	prv_print, /* SUBTILIS_KEYWORD_PRINT */
	NULL, /* SUBTILIS_KEYWORD_PRINT_HASH */
	prv_proc, /* SUBTILIS_KEYWORD_PROC */
	NULL, /* SUBTILIS_KEYWORD_PTR_HASH */
	NULL, /* SUBTILIS_KEYWORD_QUIT */
	NULL, /* SUBTILIS_KEYWORD_RAD */
	NULL, /* SUBTILIS_KEYWORD_READ */
	prv_rectangle, /* SUBTILIS_KEYWORD_RECTANGLE */
	NULL, /* SUBTILIS_KEYWORD_REM */
	NULL, /* SUBTILIS_KEYWORD_RENUMBER */
	prv_repeat, /* SUBTILIS_KEYWORD_REPEAT */
	NULL, /* SUBTILIS_KEYWORD_REPORT */
	NULL, /* SUBTILIS_KEYWORD_REPORT_STR */
	NULL, /* SUBTILIS_KEYWORD_RESTORE */
	NULL, /* SUBTILIS_KEYWORD_RETURN */
	NULL, /* SUBTILIS_KEYWORD_RIGHT_STR */
	NULL, /* SUBTILIS_KEYWORD_RND */
	NULL, /* SUBTILIS_KEYWORD_RUN */
	NULL, /* SUBTILIS_KEYWORD_SAVE */
	NULL, /* SUBTILIS_KEYWORD_SGN */
	NULL, /* SUBTILIS_KEYWORD_SIN */
	NULL, /* SUBTILIS_KEYWORD_SOUND */
	NULL, /* SUBTILIS_KEYWORD_SPC */
	NULL, /* SUBTILIS_KEYWORD_SQR */
	NULL, /* SUBTILIS_KEYWORD_STEP */
	NULL, /* SUBTILIS_KEYWORD_STEREO */
	NULL, /* SUBTILIS_KEYWORD_STOP */
	NULL, /* SUBTILIS_KEYWORD_STRING_STR */
	NULL, /* SUBTILIS_KEYWORD_STRS */
	NULL, /* SUBTILIS_KEYWORD_SUM */
	NULL, /* SUBTILIS_KEYWORD_SUMLEN */
	NULL, /* SUBTILIS_KEYWORD_SWAP */
	NULL, /* SUBTILIS_KEYWORD_SYS */
	NULL, /* SUBTILIS_KEYWORD_TAB */
	NULL, /* SUBTILIS_KEYWORD_TAN */
	NULL, /* SUBTILIS_KEYWORD_TEMPO */
	NULL, /* SUBTILIS_KEYWORD_TEXTLOAD */
	NULL, /* SUBTILIS_KEYWORD_TEXTSAVE */
	NULL, /* SUBTILIS_KEYWORD_THEN */
	NULL, /* SUBTILIS_KEYWORD_TIME */
	NULL, /* SUBTILIS_KEYWORD_TIME_STR */
	NULL, /* SUBTILIS_KEYWORD_TINT */
	NULL, /* SUBTILIS_KEYWORD_TO */
	NULL, /* SUBTILIS_KEYWORD_TOP */
	NULL, /* SUBTILIS_KEYWORD_TRACE */
	NULL, /* SUBTILIS_KEYWORD_TRUE */
	NULL, /* SUBTILIS_KEYWORD_TWIN */
	NULL, /* SUBTILIS_KEYWORD_UNTIL */
	NULL, /* SUBTILIS_KEYWORD_USR */
	NULL, /* SUBTILIS_KEYWORD_VAL */
	prv_vdu, /* SUBTILIS_KEYWORD_VDU */
	NULL, /* SUBTILIS_KEYWORD_VOICES */
	NULL, /* SUBTILIS_KEYWORD_VPOS */
	prv_wait, /* SUBTILIS_KEYWORD_WAIT */
	NULL, /* SUBTILIS_KEYWORD_WHEN */
	prv_while, /* SUBTILIS_KEYWORD_WHILE */
	NULL, /* SUBTILIS_KEYWORD_WIDTH */
};

/* clang-format on */
