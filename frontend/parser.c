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

#include "../common/error.h"
#include "../common/error_codes.h"
#include "builtins_ir.h"
#include "expression.h"
#include "globals.h"
#include "parser.h"
#include "type_if.h"
#include "variable.h"

struct subtilis_parser_param_t_ {
	subtilis_type_t type;
	size_t reg;
	size_t nop;
};

typedef struct subtilis_parser_param_t_ subtilis_parser_param_t;

static subtilis_exp_t *prv_call_1_arg_fn(subtilis_parser_t *p, const char *name,
					 size_t reg,
					 subtilis_ir_reg_type_t ptype,
					 const subtilis_type_t *rtype,
					 subtilis_error_t *err);
static subtilis_exp_t *prv_priority1(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err);
static subtilis_exp_t *prv_call(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err);
static subtilis_exp_t *prv_bracketed_2_int_args(subtilis_parser_t *p,
						subtilis_token_t *t,
						subtilis_op_instr_type_t itype,
						subtilis_error_t *err);
static void prv_add_fn_ret(subtilis_parser_t *p, const subtilis_type_t *fn_type,
			   subtilis_error_t *err);
static void prv_set_fn_retval(subtilis_parser_t *p, subtilis_exp_t *e,
			      const subtilis_type_t *fn_type,
			      subtilis_error_t *err);

#define SUBTILIS_MAIN_FN "subtilis_main"

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       subtilis_backend_caps_t caps,
				       subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_parser_t *p = calloc(1, sizeof(*p));
	subtilis_type_section_t *stype = NULL;

	if (!p) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	p->handle_escapes = true;

	p->st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->main_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->local_st = p->main_st;

	p->prog = subtilis_ir_prog_new(err, p->handle_escapes);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	stype = subtilis_type_section_new(&subtilis_type_void, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	s = subtilis_symbol_table_insert(p->st, subtilis_err_hidden_var,
					 &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, SUBTILIS_MAIN_FN, 0, stype, SUBTILIS_BUILTINS_MAX,
	    l->stream->name, l->line, (int32_t)s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	stype = NULL;
	p->main = p->current;

	p->l = l;
	p->caps = caps;
	p->level = 0;

	subtilis_sizet_vector_init(&p->free_list);
	subtilis_sizet_vector_init(&p->local_free_list);
	subtilis_sizet_vector_init(&p->main_free_list);

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
	subtilis_sizet_vector_free(&p->free_list);
	subtilis_sizet_vector_free(&p->local_free_list);
	subtilis_sizet_vector_free(&p->main_free_list);
	free(p);
}

typedef void (*subtilis_keyword_fn)(subtilis_parser_t *p, subtilis_token_t *,
				    subtilis_error_t *);

static const subtilis_keyword_fn keyword_fns[];

static subtilis_exp_t *prv_expression(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_error_t *err);

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
	return subtilis_type_if_unary_minus(p, e, err);
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
	return subtilis_type_if_not(p, e, err);
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

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);
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

	return subtilis_type_if_to_float64(p, e, err);
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

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_real(const_fn(e->exp.ir_op.real), err);
	} else {
		reg = subtilis_ir_section_add_instr2(p->current, itype,
						     e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = subtilis_exp_new_real_var(reg, err);
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

	return subtilis_type_if_to_int(p, e, err);
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

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_real(e->exp.ir_op.real * radian, err);
	} else {
		e3 = subtilis_exp_new_real(radian, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = subtilis_type_if_mul(p, e, e3, err);
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

static subtilis_exp_t *prv_int(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;

	e = prv_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_int32(floor(e->exp.ir_op.real), err);
	} else {
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV_FPRD_I32, e->exp.ir_op,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = subtilis_exp_new_int32_var(reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
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

static subtilis_exp_t *prv_tan(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_TAN, tan, err);
}

static subtilis_exp_t *prv_asn(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ASN, asin, err);
}

static subtilis_exp_t *prv_acs(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ACS, acos, err);
}

static subtilis_exp_t *prv_atn(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ATN, atan, err);
}

static subtilis_exp_t *prv_log(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_op_instr_type_t itype,
			       double (*log_fn)(double), subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = prv_real_unary_fn(p, t, itype, log_fn, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}
	return e;
}

static subtilis_exp_t *prv_rnd_var(subtilis_parser_t *p, subtilis_exp_t *e,
				   subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_section_t *fn;
	size_t zero;
	size_t one;
	size_t cond;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t else_label;
	subtilis_ir_operand_t zero_one_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t result;

	zero_one_label.label = subtilis_ir_section_new_label(p->current);
	else_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	result.reg = p->current->freg_counter++;
	reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);
	e = NULL;

	fn = subtilis_builtins_ir_add_1_arg_int(p, "_rnd_int",
						&subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_rnd_int(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	fn = subtilis_builtins_ir_add_1_arg_int(p, "_rnd_real",
						&subtilis_type_real, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_rnd_real(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = reg;
	op2.integer = 0;
	zero = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 1;
	one = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = zero;
	op2.reg = one;
	cond = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = cond;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, zero_one_label, else_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, zero_one_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_call_1_arg_fn(p, "_rnd_real", reg, SUBTILIS_IR_REG_TYPE_INTEGER,
			      &subtilis_type_real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVFP, result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);
	e = NULL;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, else_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_call_1_arg_fn(p, "_rnd_int", reg, SUBTILIS_IR_REG_TYPE_INTEGER,
			      &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current,
					      SUBTILIS_OP_INSTR_MOV_I32_FP,
					      result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);
	e = NULL;

	subtilis_ir_section_add_label(p->current, end_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_exp_new_real_var(result.reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

static subtilis_exp_t *prv_rnd_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	int32_t val = e->exp.ir_op.integer;

	subtilis_exp_delete(e);
	if (val == 0)
		return subtilis_builtins_ir_rnd_0(p, err);
	else if (val == 1)
		return subtilis_builtins_ir_rnd_1(p, err);
	else if (val < 0)
		return subtilis_builtins_ir_rnd_neg(p, val, err);
	else
		return subtilis_builtins_ir_rnd_pos(p, val, err);
}

static subtilis_exp_t *prv_rnd(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e_dup = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "(")) {
			e = prv_bracketed_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			subtilis_lexer_get(p->l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			e = subtilis_type_if_to_int(p, e, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER)
				return prv_rnd_const(p, e, err);
			return prv_rnd_var(p, e, err);
		}
	}

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	subtilis_exp_delete(e_dup);

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

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);

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
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		if (inkey_value >= 0)
			itype = SUBTILIS_OP_INSTR_GET_TO;
		else
			itype = SUBTILIS_OP_INSTR_INKEY;

		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op1.reg = e->exp.ir_op.reg;
		;
		reg =
		    subtilis_ir_section_add_instr2(p->current, itype, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_exp_delete(e);

	return subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_call_1_arg_fn(subtilis_parser_t *p, const char *name,
					 size_t reg,
					 subtilis_ir_reg_type_t ptype,
					 const subtilis_type_t *rtype,
					 subtilis_error_t *err)
{
	subtilis_ir_arg_t *args = NULL;
	char *name_dup = NULL;

	args = malloc(sizeof(*args) * 1);
	if (!args) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	name_dup = malloc(strlen(name) + 1);
	if (!name_dup) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(name_dup, name);

	args[0].type = ptype;
	args[0].reg = reg;

	return subtilis_exp_add_call(p, name_dup, SUBTILIS_BUILTINS_MAX, NULL,
				     args, rtype, 1, err);

cleanup:

	free(args);

	return NULL;
}

static subtilis_exp_t *prv_inkey(subtilis_parser_t *p, subtilis_token_t *t,
				 subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;
	subtilis_ir_section_t *current;

	e = prv_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER)
		return prv_inkey_const(p, t, e, err);

	reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	current = subtilis_builtins_ir_add_1_arg_int(
	    p, "_inkey", &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_inkey(current, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return prv_call_1_arg_fn(p, "_inkey", reg, SUBTILIS_IR_REG_TYPE_INTEGER,
				 &subtilis_type_integer, err);
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	e = prv_parse_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (e->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		e->exp.ir_op.integer = abs(e->exp.ir_op.integer);
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		e->exp.ir_op.real = fabs(e->exp.ir_op.real);
		break;
	case SUBTILIS_TYPE_INTEGER:
		op2.integer = 31;
		reg = subtilis_ir_section_add_instr(p->current,
						    SUBTILIS_OP_INSTR_ASRI_I32,
						    e->exp.ir_op, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		op2.reg = reg;
		reg = subtilis_ir_section_add_instr(p->current,
						    SUBTILIS_OP_INSTR_ADD_I32,
						    e->exp.ir_op, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		op1.reg = reg;
		reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_EOR_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e->exp.ir_op.reg = reg;
		break;
	case SUBTILIS_TYPE_REAL:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_ABSR, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_exp_delete(e);
		e = subtilis_exp_new_real_var(reg, err);
		break;
	default:
		subtilis_error_set_numeric_expected(err, "", p->l->stream->name,
						    p->l->line);
		goto cleanup;
	}

	return e;

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

static subtilis_exp_t *prv_get_err(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_var_lookup_var(p, subtilis_err_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
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
		e = subtilis_var_lookup_var(p, tbuf, err);
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
		case SUBTILIS_KEYWORD_INT:
			return prv_int(p, t, err);
		case SUBTILIS_KEYWORD_TIME:
			return prv_gettime(p, t, err);
		case SUBTILIS_KEYWORD_SIN:
			return prv_sin(p, t, err);
		case SUBTILIS_KEYWORD_COS:
			return prv_cos(p, t, err);
		case SUBTILIS_KEYWORD_TAN:
			return prv_tan(p, t, err);
		case SUBTILIS_KEYWORD_ACS:
			return prv_acs(p, t, err);
		case SUBTILIS_KEYWORD_ASN:
			return prv_asn(p, t, err);
		case SUBTILIS_KEYWORD_ATN:
			return prv_atn(p, t, err);
		case SUBTILIS_KEYWORD_RND:
			return prv_rnd(p, t, err);
		case SUBTILIS_KEYWORD_SQR:
			return prv_sqr(p, t, err);
		case SUBTILIS_KEYWORD_LOG:
			return prv_log(p, t, SUBTILIS_OP_INSTR_LOG, log10, err);
		case SUBTILIS_KEYWORD_LN:
			return prv_log(p, t, SUBTILIS_OP_INSTR_LN, log, err);
		case SUBTILIS_KEYWORD_RAD:
			return prv_rad(p, t, err);
		case SUBTILIS_KEYWORD_ABS:
			return prv_abs(p, t, err);
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
		case SUBTILIS_KEYWORD_ERR:
			return prv_get_err(p, t, err);
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
				exp_fn = subtilis_type_if_div;
			else if (!strcmp(tbuf, "MOD"))
				exp_fn = subtilis_type_if_mod;
			else
				break;
		} else if (!strcmp(tbuf, "*")) {
			exp_fn = subtilis_type_if_mul;
		} else if (!strcmp(tbuf, "/")) {
			exp_fn = subtilis_type_if_divr;
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
			exp_fn = subtilis_type_if_sub;
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
			exp_fn = subtilis_type_if_eq;
		else if (!strcmp(tbuf, "<>"))
			exp_fn = subtilis_type_if_neq;
		else if (!strcmp(tbuf, ">"))
			exp_fn = subtilis_exp_gt;
		else if (!strcmp(tbuf, "<="))
			exp_fn = subtilis_exp_lte;
		else if (!strcmp(tbuf, "<"))
			exp_fn = subtilis_exp_lt;
		else if (!strcmp(tbuf, ">="))
			exp_fn = subtilis_exp_gte;
		else if (!strcmp(tbuf, "<<"))
			exp_fn = subtilis_type_if_lsl;
		else if (!strcmp(tbuf, ">>"))
			exp_fn = subtilis_type_if_lsr;
		else if (!strcmp(tbuf, ">>>"))
			exp_fn = subtilis_type_if_asr;
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
		e1 = subtilis_type_if_and(p, e1, e2, err);
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
			exp_fn = subtilis_type_if_eor;
		else if (t->tok.keyword.type == SUBTILIS_KEYWORD_OR)
			exp_fn = subtilis_type_if_or;
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
	if ((e->type.type != SUBTILIS_TYPE_CONST_INTEGER) &&
	    (e->type.type != SUBTILIS_TYPE_CONST_REAL) &&
	    (e->type.type != SUBTILIS_TYPE_INTEGER) &&
	    (e->type.type != SUBTILIS_TYPE_REAL)) {
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

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

typedef enum {
	SUBTILIS_ASSIGN_TYPE_EQUAL,
	SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL,
	SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL,
} subtilis_assign_type_t;

static subtilis_exp_t *prv_assignment_operator(subtilis_parser_t *p,
					       const char *var_name,
					       subtilis_exp_t *e,
					       subtilis_assign_type_t at,
					       subtilis_error_t *err)
{
	subtilis_exp_t *var;

	if (at == SUBTILIS_ASSIGN_TYPE_EQUAL)
		return e;

	var = subtilis_var_lookup_var(p, var_name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (at == SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL)
		e = subtilis_exp_add(p, var, e, err);
	else
		e = subtilis_type_if_sub(p, var, e, err);

	var = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(var);
	subtilis_exp_delete(e);

	return NULL;
}

static void prv_assignment(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_assign_type_t at;
	subtilis_type_t type;
	bool new_var = false;
	char *var_name = NULL;
	subtilis_exp_t *e = NULL;

	tbuf = subtilis_token_get_text(t);
	var_name = malloc(strlen(tbuf) + 1);
	if (!var_name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(var_name, tbuf);
	type = t->tok.id_type;
	s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (s) {
		op1.reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (s) {
			op1.reg = SUBTILIS_IR_REG_GLOBAL;
		} else if (p->current == p->main) {
			/*
			 * We explicitly disable statements like
			 *
			 * X% = X% + 1  or
			 * X% += 1
			 *
			 * for the cases where X% has not been defined.
			 */
			new_var = true;
			op1.reg = SUBTILIS_IR_REG_GLOBAL;
		} else {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto cleanup;
		}
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, "=")) {
		at = SUBTILIS_ASSIGN_TYPE_EQUAL;
	} else if (new_var) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	} else if (!strcmp(tbuf, "+=")) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=")) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* Ownership of e is passed to the following functions. */

	e = subtilis_exp_coerce_type(p, e, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (new_var) {
		s = subtilis_symbol_table_insert(p->st, var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	e = prv_assignment_operator(p, var_name, e, at, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (s->is_reg)
		subtilis_type_if_assign_to_reg(p, s->loc, e, err);
	else
		subtilis_type_if_assign_to_mem(p, op1.reg, s->loc, e, err);

cleanup:

	free(var_name);
}

/*
 * Returns 0 and no error if more than max args are read.  Caller is
 * expected to free expressions even in case of error.
 */

static size_t prv_var_bracketed_int_args(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_exp_t **e, size_t max,
					 subtilis_error_t *err)
{
	size_t i;
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return 0;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	e[0] = prv_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	e[0] = subtilis_type_if_to_int(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	for (i = 1; i < max; i++) {
		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return 0;
		}

		if (!strcmp(tbuf, ")"))
			return i;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return 0;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;

		e[i] = prv_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;

		e[i] = subtilis_type_if_to_int(p, e[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
	}

	/*
	 * Too many integers in the list
	 */

	return 0;
}

static void prv_init_array_type(subtilis_parser_t *p,
				const subtilis_type_t *element_type,
				subtilis_type_t *type, subtilis_exp_t **e,
				size_t dims, subtilis_error_t *err)
{
	if (dims > SUBTILIS_MAX_DIMENSIONS) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_type_if_array_of(p, element_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	type->params.array.num_dims = dims;
}

static void prv_deallocate_arrays(subtilis_parser_t *p,
				  subtilis_ir_operand_t load_reg,
				  subtilis_sizet_vector_t *free_list,
				  subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t free_label;
	subtilis_ir_operand_t skip_label;

	free_label.label = subtilis_ir_section_new_label(p->current);
	skip_label.label = subtilis_ir_section_new_label(p->current);

	for (i = 0; i < free_list->len; i++) {
		op1.integer = (int32_t)free_list->vals[i];
		offset.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, load_reg, op1,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		zero.integer = 0;
		zero.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_NEQI_I32, offset, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, zero,
						  free_label, skip_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_label(p->current, free_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_DEREF, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_label(p->current, skip_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

/* Consumes e */

static void prv_generate_error(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t target_label;

	subtilis_var_assign_hidden(p, subtilis_err_hidden_var,
				   &subtilis_type_integer, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (p->current->in_error_handler) {
		/*
		 * We're in an error handler. Let's set the error flag and
		 * return the default value for the function type.
		 */

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_SETE, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else if (p->current->handler_list) {
		/*
		 * We're not in an error handler but one is defined.
		 * Let's jump there.
		 */

		target_label.label = p->current->handler_list->label;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, target_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		/*
		 * We're not in an error handler and none has been defined.
		 * Let's set the error flag and return the default value.
		 */

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_SETE, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

/* Does not consume e */

static void prv_check_dynamic_dim(subtilis_parser_t *p, subtilis_exp_t *e,
				  subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *ecode = NULL;

	error_label.label = subtilis_ir_section_new_label(p->current);
	ok_label.label = subtilis_ir_section_new_label(p->current);

	edup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	edup = subtilis_type_if_lte(p, edup, zero, err);
	zero = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  edup->exp.ir_op, error_label,
					  ok_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, error_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ecode = subtilis_exp_new_int32(SUBTILIS_ERROR_CODE_BAD_DIM, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_generate_error(p, ecode, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, ok_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

cleanup:

	subtilis_exp_delete(edup);
}

static void prv_allocate_array(subtilis_parser_t *p, const char *var_name,
			       subtilis_type_t *type,
			       const subtilis_symbol_t *s, subtilis_exp_t **e,
			       subtilis_ir_operand_t store_reg,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_ir_operand_t op1;
	size_t i;
	subtilis_sizet_vector_t *free_list;
	subtilis_exp_t *edup;
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *zero = NULL;
	int64_t array_size = 1;
	int32_t offset = s->loc + 8;

	/*
	 * TODO: This whole thing is 32 bit specific.  At least the pointer
	 * needs to be platform specific although we may still keep maximum
	 * array sizes to 32 bits.
	 */

	/* TODO:
	 * I don't see any way to detect whether the array dimensions overflow
	 * 32 bits. For constants we can use 64 bit integers to store the size.
	 * This would then limit us to using 32 bits for the array size, even on
	 * 64 bit builds (unless we resort to instrinsics) as there
	 * are no 128 bit integers in C.  For the ARM2 backend, there is
	 * no way to detect overflow of dynamic vars.  This could be done on
	 * StrongARM using UMULL.  Perhaps we could execute different code on
	 * SA here.  For now, be careful with those array bounds!
	 */

	for (i = 0; i < type->params.array.num_dims; i++) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER)
			continue;
		if (e[i]->exp.ir_op.integer <= 0) {
			subtilis_error_bad_dim(err, var_name,
					       p->l->stream->name, p->l->line);
			return;
		}
		array_size = array_size * e[i]->exp.ir_op.integer;
	}

	if (array_size > (int64_t)0x7ffffff) {
		subtilis_error_bad_dim(err, var_name, p->l->stream->name,
				       p->l->line);
		return;
	}

	sizee = subtilis_exp_new_int32((int32_t)array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* Need to compute size of array */

	for (i = 0; i < type->params.array.num_dims; i++) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER) {
			prv_check_dynamic_dim(p, e[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			edup = subtilis_type_if_dup(e[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			sizee = subtilis_type_if_mul(p, sizee, edup, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		op1.integer = offset;
		e[i] = subtilis_type_if_exp_to_var(p, e[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_STOREO_I32, e[i]->exp.ir_op,
		    store_reg, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		offset += sizeof(int32_t);
	}

	op1.integer = s->loc;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, sizee->exp.ir_op,
	    store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * We need to zero out the pointer in case alloc fails.  Our variable
	 * has been created at this point, so the cleanup code will attempt to
	 * free it, unless the pointer is zero.
	 */

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = s->loc + 4;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  zero->exp.ir_op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_ALLOC, sizee->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (store_reg.reg == SUBTILIS_IR_REG_LOCAL)
		free_list = (p->current == p->main) ? &p->main_free_list
						    : &p->local_free_list;
	else
		free_list = &p->free_list;

	subtilis_sizet_vector_append(free_list, s->loc + 4, err);

cleanup:

	subtilis_exp_delete(zero);
	subtilis_exp_delete(sizee);
}

static void prv_create_array(subtilis_parser_t *p, subtilis_token_t *t,
			     bool local, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	const char *tbuf;
	subtilis_exp_t *e[SUBTILIS_MAX_DIMENSIONS];
	subtilis_type_t element_type;
	subtilis_type_t type;
	size_t i;
	subtilis_ir_operand_t local_global;
	size_t dims = 0;
	char *var_name = NULL;

	local_global.reg =
	    local ? SUBTILIS_IR_REG_LOCAL : SUBTILIS_IR_REG_GLOBAL;

	do {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		tbuf = subtilis_token_get_text(t);
		if (subtilis_symbol_table_lookup(p->local_st, tbuf) ||
		    (!local && subtilis_symbol_table_lookup(p->st, tbuf))) {
			subtilis_error_set_already_defined(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		var_name = malloc(strlen(tbuf) + 1);
		if (!var_name) {
			subtilis_error_set_oom(err);
			return;
		}
		strcpy(var_name, tbuf);
		element_type = t->tok.id_type;

		dims = prv_var_bracketed_int_args(p, t, e,
						  SUBTILIS_MAX_DIMENSIONS, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (dims == 0) {
			subtilis_error_too_many_dims(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}

		prv_init_array_type(p, &element_type, &type, &e[0], dims, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		s = subtilis_symbol_table_insert(local ? p->local_st : p->st,
						 var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		prv_allocate_array(p, var_name, &type, s, &e[0], local_global,
				   err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		for (i = 0; i < dims; i++)
			subtilis_exp_delete(e[i]);
		dims = 0;

		free(var_name);
		var_name = NULL;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		tbuf = subtilis_token_get_text(t);
	} while (t->type == SUBTILIS_TOKEN_OPERATOR && !strcmp(tbuf, ","));

cleanup:
	for (i = 0; i < dims; i++)
		subtilis_exp_delete(e[i]);

	free(var_name);
}

static void prv_dim(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	if (p->current != p->main) {
		subtilis_error_dim_in_proc(err, p->l->stream->name, p->l->line);
		return;
	}

	prv_create_array(p, t, false, err);
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
	subtilis_exp_t *e;
	subtilis_type_t type;
	char *var_name = NULL;

	while ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	       (t->tok.keyword.type == SUBTILIS_KEYWORD_LOCAL)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_DIM)) {
			prv_create_array(p, t, true, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			continue;
		} else if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		if (subtilis_symbol_table_lookup(p->local_st, tbuf)) {
			subtilis_error_set_already_defined(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		var_name = malloc(strlen(tbuf) + 1);
		if (!var_name) {
			subtilis_error_set_oom(err);
			return;
		}
		strcpy(var_name, tbuf);
		type = t->tok.id_type;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(t);
		if (!((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		      !strcmp(tbuf, "="))) {
			e = subtilis_type_if_zero(p, &type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			e = prv_expression(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			e = subtilis_type_if_exp_to_var(p, e, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			e = subtilis_exp_coerce_type(p, e, &type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, &type, e->exp.ir_op.reg, err);
		subtilis_exp_delete(e);
		free(var_name);
		var_name = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

cleanup:

	free(var_name);
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
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

cleanup:

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

	retval = subtilis_exp_new_int32_var(reg, err);

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
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

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

	e[0] = subtilis_type_if_exp_to_var(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[2]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

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

	e[0] = subtilis_type_if_exp_to_var(p, e[0], err);
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

	e[2] = subtilis_type_if_exp_to_var(p, e[2], err);
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

	e[0] = subtilis_type_if_exp_to_var(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[1] = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[1] = subtilis_type_if_exp_to_var(p, e[1], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	;

	prv_statement_int_args(p, t, &e[2], 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[4] = subtilis_exp_new_int32_var(e[2]->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[4] = subtilis_type_if_unary_minus(p, e[4], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[5] = subtilis_exp_new_int32_var(e[3]->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[5] = subtilis_type_if_unary_minus(p, e[5], err);
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
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

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
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

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

	subtilis_exp_handle_errors(p, err);
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

	subtilis_exp_handle_errors(p, err);
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

	subtilis_exp_handle_errors(p, err);
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

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
}

static void prv_end(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	if (p->current != p->main) {
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_END, err);
	} else {
		end_label.label = p->current->end_label;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}

static void prv_wait(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_WAIT,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
}

static void prv_vdu_byte(subtilis_parser_t *p, subtilis_exp_t *e,
			 subtilis_error_t *err)
{
	subtilis_op_instr_type_t itype;

	if (e->type.type == SUBTILIS_TYPE_INTEGER) {
		itype = SUBTILIS_OP_INSTR_VDU;
	} else if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		itype = SUBTILIS_OP_INSTR_VDUI;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_ir_section_add_instr_no_reg(p->current, itype, e->exp.ir_op,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

static void prv_vdu_2bytes(subtilis_parser_t *p, subtilis_exp_t *e,
			   subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	if (e->type.type == SUBTILIS_TYPE_INTEGER) {
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
		subtilis_exp_handle_errors(p, err);
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
	} else if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		op1.integer = e->exp.ir_op.integer & 0xff;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDUI, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op1.integer = e->exp.ir_op.integer >> 8;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_VDUI, op1, err);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	subtilis_exp_handle_errors(p, err);
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
		subtilis_exp_handle_errors(p, err);
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

		e = subtilis_type_if_to_int(p, e, err);
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

		e = subtilis_type_if_to_int(p, e, err);
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

	(void)prv_call_1_arg_fn(p, "_print_fp", reg, SUBTILIS_IR_REG_TYPE_REAL,
				&subtilis_type_void, err);
}

static void prv_print(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (e->type.type) {
	case SUBTILIS_TYPE_INTEGER:
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_I32, e->exp.ir_op, err);
		break;
	case SUBTILIS_TYPE_REAL:
		if (p->caps & SUBTILIS_BACKEND_HAVE_PRINT_FP)
			subtilis_ir_section_add_instr_no_reg(
			    p->current, SUBTILIS_OP_INSTR_PRINT_FP,
			    e->exp.ir_op, err);
		else
			prv_print_fp(p, e->exp.ir_op.reg, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_PRINT_NL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

cleanup:
	subtilis_exp_delete(e);
}

static void prv_return(subtilis_parser_t *p, subtilis_token_t *t,
		       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_type_t fn_type;
	subtilis_ir_operand_t end_label;

	if (p->current == p->main) {
		subtilis_error_set_return_in_main(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	fn_type = p->current->type->return_type;
	if (fn_type.type == SUBTILIS_TYPE_VOID) {
		subtilis_error_set_return_in_proc(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Ownership of e is passed to add_fn_ret */

	prv_set_fn_retval(p, e, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	end_label.label = p->current->end_label;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
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
		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
			if (p->current->endproc) {
				subtilis_error_set_useless_statement(
				    err, p->l->stream->name, p->l->line);
				return key_type;
			}
			prv_assignment(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, "<-")) {
			prv_return(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}

		key_type = t->tok.keyword.type;
		if ((key_type == SUBTILIS_KEYWORD_ELSE) ||
		    (key_type == SUBTILIS_KEYWORD_ENDIF))
			break;

		if (p->current->endproc) {
			subtilis_error_set_useless_statement(
			    err, p->l->stream->name, p->l->line);
			return key_type;
		}

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

	p->current->endproc = false;
	p->level--;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
	return key_type;
}

static void prv_statement(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_keyword_fn fn;
	const char *tbuf;
	subtilis_keyword_type_t key_type;

	if ((p->current->endproc) &&
	    !((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	      (t->tok.keyword.type == SUBTILIS_KEYWORD_DEF))) {
		subtilis_error_set_useless_statement(err, p->l->stream->name,
						     p->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(t);
	if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
		prv_assignment(p, t, err);
		return;
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		   !strcmp(tbuf, "<-")) {
		prv_return(p, t, err);
		return;
	} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
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

	if ((end_key != SUBTILIS_KEYWORD_NEXT) &&
	    (end_key != SUBTILIS_KEYWORD_UNTIL) &&
	    (end_key != SUBTILIS_KEYWORD_ENDPROC))
		p->current->endproc = false;
	p->level--;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
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
	//	p->current->endproc = false;
	p->level--;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
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

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		cond->reg = reg;
	} else if (e->type.type == SUBTILIS_TYPE_INTEGER) {
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

static void prv_handle_escape(subtilis_parser_t *p, subtilis_error_t *err)
{
	/* Let's not test for escape in an error handler. */

	if (!p->handle_escapes || p->current->in_error_handler)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_TESTESC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
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

	prv_handle_escape(p, err);
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

	prv_handle_escape(p, err);
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

	var = subtilis_var_lookup_var(p, var_name, err);
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
		subtilis_type_if_assign_to_reg(p, s->loc, inc, err);
	else
		subtilis_type_if_assign_to_mem(p, op1.reg, s->loc, inc, err);
	inc = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_var_lookup_var(p, var_name, err);

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
	switch (step->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (step->exp.ir_op.integer == 0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.integer < 0 ? SUBTILIS_STEP_CONST_DEC
						   : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_TYPE_CONST_REAL:
		if (step->exp.ir_op.real == 0.0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.real < 0.0 ? SUBTILIS_STEP_CONST_DEC
						  : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_TYPE_INTEGER:
	case SUBTILIS_TYPE_REAL:
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

	prv_handle_escape(p, err);
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

	step_dup = subtilis_type_if_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, var_name, step, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (var->type.type != SUBTILIS_TYPE_REAL) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	var_dup = subtilis_type_if_dup(var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_dup = subtilis_type_if_dup(to, err);
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

	if (var->type.type != SUBTILIS_TYPE_INTEGER) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	sub = subtilis_type_if_sub(p, to, var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	var = NULL;
	to = NULL;

	op2.reg = step_dir_reg;
	eor_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, sub->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eor_var = subtilis_exp_new_int32_var(eor_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	eor_var_dup = subtilis_type_if_dup(eor_var, err);
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

	top_bit = subtilis_exp_new_int32_var(step_dir_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_type_if_neq(p, eor_var_dup, top_bit, err);
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
	if ((var_type.type != SUBTILIS_TYPE_REAL) &&
	    (var_type.type != SUBTILIS_TYPE_INTEGER)) {
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
			step = subtilis_type_if_copy_var(p, step, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			if (var_type.type == SUBTILIS_TYPE_INTEGER)
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

static void prv_onerror(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_ir_operand_t handler_label;
	subtilis_ir_operand_t target_label;
	unsigned int start;

	if (p->current->in_error_handler) {
		subtilis_error_set_nested_handler(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	p->current->in_error_handler = true;
	handler_label.label = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, handler_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * The error flag needs to be clear when we're inside an error
	 * handler.  Otherwise and procedures or functions call in that
	 * handler may appear to have failed when they return.
	 */

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_CLEARE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_ENDERROR))
			break;
		prv_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	if (!p->current->endproc) {
		if (!p->current->handler_list) {
			if (p->current != p->main) {
				subtilis_ir_section_add_instr_no_arg(
				    p->current, SUBTILIS_OP_INSTR_SETE, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
			}
			subtilis_exp_return_default_value(p, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else {
			target_label.label = p->current->handler_list->label;
			subtilis_ir_section_add_instr_no_reg(
			    p->current, SUBTILIS_OP_INSTR_JMP, target_label,
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
	}

	p->current->handler_list = subtilis_handler_list_update(
	    p->current->handler_list, p->level - 1, handler_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->level--;
	p->current->endproc = false;

	subtilis_lexer_get(p->l, t, err);

cleanup:
	p->current->in_error_handler = false;
}

static void prv_error(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = prv_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return;
	}

	prv_generate_error(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
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

		switch (t->tok.id_type.type) {
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
		    p->local_st, tbuf, &t->tok.id_type, reg_num, err);
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

	if (t->tok.keyword.type == SUBTILIS_KEYWORD_PROC)
		tbuf += 4;
	else if (t->tok.keyword.type == SUBTILIS_KEYWORD_FN)
		tbuf += 2;
	id_type = t->tok.keyword.id_type;

	name = malloc(strlen(tbuf) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	*type = id_type;
	strcpy(name, tbuf);

	return name;
}

static void prv_add_fn_ret(subtilis_parser_t *p, const subtilis_type_t *fn_type,
			   subtilis_error_t *err)
{
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t ret_reg;

	switch (fn_type->type) {
	case SUBTILIS_TYPE_INTEGER:
		type = SUBTILIS_OP_INSTR_RET_I32;
		break;
	case SUBTILIS_TYPE_REAL:
		type = SUBTILIS_OP_INSTR_RET_REAL;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ret_reg.reg = p->current->ret_reg;
	subtilis_ir_section_add_instr_no_reg(p->current, type, ret_reg, err);
}

static void prv_set_fn_retval(subtilis_parser_t *p, subtilis_exp_t *e,
			      const subtilis_type_t *fn_type,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t ret_reg;

	/* Ownership of e is passed to prv_coerce_type */

	e = subtilis_exp_coerce_type(p, e, fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (e->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		type = SUBTILIS_OP_INSTR_MOVI_I32;
		break;
	case SUBTILIS_TYPE_INTEGER:
		type = SUBTILIS_OP_INSTR_MOV;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		type = SUBTILIS_OP_INSTR_MOVI_REAL;
		break;
	case SUBTILIS_TYPE_REAL:
		type = SUBTILIS_OP_INSTR_MOVFP;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	ret_reg.reg = p->current->ret_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, type, ret_reg,
					      e->exp.ir_op, err);

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
	subtilis_ir_operand_t var_reg;
	size_t num_params = 0;
	subtilis_type_t *params = NULL;
	subtilis_symbol_table_t *local_st = NULL;
	char *name = NULL;

	if (p->current != p->main) {
		subtilis_error_set_nested_procedure(err, p->l->stream->name,
						    p->l->line);
		return;
	}

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
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

	stype = subtilis_type_section_new(&fn_type, num_params, params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	params = NULL;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, name, p->local_st->allocated, stype, SUBTILIS_BUILTINS_MAX,
	    p->l->stream->name, p->l->line, p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_type_section_delete(stype);
		goto on_error;
	}

	prv_parse_locals(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	prv_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	if (fn_type.type != SUBTILIS_TYPE_VOID) {
		prv_fn_compound(p, t, err);
		e = prv_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		/* Ownership of e is passed to add_fn_ret */

		if (p->current->endproc) {
			subtilis_exp_delete(e);
		} else {
			prv_set_fn_retval(p, e, &fn_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
		}
		subtilis_ir_section_add_label(p->current, p->current->end_label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		prv_deallocate_arrays(p, var_reg, &p->local_free_list, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		prv_add_fn_ret(p, &fn_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	} else {
		prv_compound(p, t, SUBTILIS_KEYWORD_ENDPROC, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		subtilis_ir_section_add_label(p->current, p->current->end_label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		prv_deallocate_arrays(p, var_reg, &p->local_free_list, err);
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

	subtilis_ir_merge_errors(p->current, err);

on_error:

	free(params);
	subtilis_symbol_table_delete(local_st);
	free(name);
	p->local_st = p->main_st;
	p->current = p->main;
	subtilis_sizet_vector_free(&p->local_free_list);
	subtilis_sizet_vector_init(&p->local_free_list);
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

		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		params[num_params].type = e->type;
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
		switch (params[i].type.type) {
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
	subtilis_type_section_t *stype = NULL;
	subtilis_type_t *ptypes = NULL;
	char *name = NULL;
	subtilis_parser_param_t *params = NULL;
	size_t num_params = 0;
	subtilis_ir_arg_t *args = NULL;

	name = prv_proc_name_and_type(p, t, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

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

	stype = subtilis_type_section_new(&fn_type, num_params, ptypes, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	/* Ownership of stypes, args and name passed to this function. */

	return subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MAX, stype,
				     args, &fn_type, num_params, err);

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

static void prv_endproc(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	if (p->current == p->main) {
		subtilis_error_set_proc_in_main(err, p->l->stream->name,
						p->l->line);
		return;
	}

	if (p->current->type->return_type.type != SUBTILIS_TYPE_VOID) {
		subtilis_error_set_proc_in_fn(err, p->l->stream->name,
					      p->l->line);
		return;
	}

	p->current->endproc = true;
	end_label.label = p->current->end_label;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
}

static void prv_root(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_exp_t *seed;
	subtilis_ir_operand_t var_reg;

	seed = subtilis_exp_new_int32(0x3fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	seed = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_err_hidden_var,
				   &subtilis_type_integer, seed, err);
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

	subtilis_ir_section_add_label(p->current, p->current->end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	prv_deallocate_arrays(p, var_reg, &p->main_free_list, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var_reg.reg = SUBTILIS_IR_REG_GLOBAL;
	prv_deallocate_arrays(p, var_reg, &p->free_list, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_END,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_merge_errors(p->current, err);
}

static void prv_check_call(subtilis_parser_t *p, subtilis_parser_call_t *call,
			   subtilis_error_t *err)
{
	size_t index;
	size_t call_index;
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
		else if (ct->return_type.type == SUBTILIS_TYPE_VOID)
			subtilis_error_set_unknown_procedure(
			    err, call->name, p->l->stream->name, call->line);
		else
			subtilis_error_set_unknown_function(
			    err, call->name, p->l->stream->name, call->line);
		return;
	}

	/* We need to fix up the call site of procedures call in an error
	 * handler.
	 */

	call_index = call->index;
	if (call->in_error_handler)
		call_index += call->s->handler_offset;

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
		call->s->ops[call_index]->op.call.proc_id = index;
		return;
	}

	st = p->prog->sections[index]->type;

	if ((st->return_type.type == SUBTILIS_TYPE_VOID) &&
	    (ct->return_type.type != SUBTILIS_TYPE_VOID)) {
		subtilis_error_set_procedure_expected(
		    err, call->name, p->l->stream->name, call->line);
		return;
	} else if ((st->return_type.type != SUBTILIS_TYPE_VOID) &&
		   (ct->return_type.type == SUBTILIS_TYPE_VOID)) {
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

	call_site = &call->s->ops[call_index]->op.call;
	for (i = 0; i < st->num_parameters; i++) {
		if (st->parameters[i].type == ct->parameters[i].type)
			continue;

		if ((st->parameters[i].type == SUBTILIS_TYPE_REAL) &&
		    (ct->parameters[i].type == SUBTILIS_TYPE_INTEGER)) {
			itype = SUBTILIS_OP_INSTR_MOV_I32_FP;
			reg_type = SUBTILIS_IR_REG_TYPE_REAL;
		} else if ((st->parameters[i].type == SUBTILIS_TYPE_INTEGER) &&
			   (ct->parameters[i].type == SUBTILIS_TYPE_REAL)) {
			itype = SUBTILIS_OP_INSTR_MOV_FP_I32;
			reg_type = SUBTILIS_IR_REG_TYPE_INTEGER;
		} else {
			expected_typname =
			    subtilis_type_name(&st->parameters[i]);
			got_typname = subtilis_type_name(&ct->parameters[i]);
			subtilis_error_set_bad_arg_type(
			    err, i + 1, expected_typname, got_typname,
			    p->l->stream->name, call->line, __FILE__, __LINE__);
			return;
		}

		switch (call->s->ops[call_index]->type) {
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
	NULL, /* SUBTILIS_KEYWORD_ABS */
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
	prv_dim, /* SUBTILIS_KEYWORD_DIM */
	NULL, /* SUBTILIS_KEYWORD_DIV */
	prv_draw, /* SUBTILIS_KEYWORD_DRAW */
	NULL, /* SUBTILIS_KEYWORD_EDIT */
	NULL, /* SUBTILIS_KEYWORD_ELLIPSE */
	NULL, /* SUBTILIS_KEYWORD_ELSE */
	prv_end, /* SUBTILIS_KEYWORD_END */
	NULL, /* SUBTILIS_KEYWORD_ENDCASE */
	NULL, /* SUBTILIS_KEYWORD_ENDERROR */
	NULL, /* SUBTILIS_KEYWORD_ENDIF */
	prv_endproc, /* SUBTILIS_KEYWORD_ENDPROC */
	NULL, /* SUBTILIS_KEYWORD_ENDWHILE */
	NULL, /* SUBTILIS_KEYWORD_EOF_HASH */
	NULL, /* SUBTILIS_KEYWORD_EOR */
	NULL, /* SUBTILIS_KEYWORD_ERL */
	NULL, /* SUBTILIS_KEYWORD_ERR */
	prv_error, /* SUBTILIS_KEYWORD_ERROR */
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
	prv_onerror, /* SUBTILIS_KEYWORD_ONERROR */
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
