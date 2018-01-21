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

#include "error.h"
#include "expression.h"
#include "parser.h"

#define SUBTILIS_MAIN_FN "subtilis_main"

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       subtilis_error_t *err)
{
	subtilis_parser_t *p = calloc(1, sizeof(*p));

	if (!p) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	p->st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->prog = subtilis_ir_prog_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current =
	    subtilis_ir_prog_section_new(p->prog, SUBTILIS_MAIN_FN, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->main = p->current;

	p->l = l;
	p->level = 0;

	return p;

on_error:

	subtilis_parser_delete(p);

	return NULL;
}

void subtilis_parser_delete(subtilis_parser_t *p)
{
	if (!p)
		return;

	subtilis_ir_prog_delete(p->prog);
	subtilis_symbol_table_delete(p->st);
	free(p);
}

typedef void (*subtilis_keyword_fn)(subtilis_parser_t *p, subtilis_token_t *,
				    subtilis_error_t *);

static const subtilis_keyword_fn keyword_fns[];

static subtilis_exp_t *prv_expression(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_error_t *err);

static subtilis_exp_t *prv_variable(subtilis_parser_t *p, subtilis_token_t *t,
				    const char *tbuf, subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	const subtilis_symbol_t *s;

	s = subtilis_symbol_table_lookup(p->st, tbuf);
	if (!s) {
		subtilis_error_set_unknown_variable(
		    err, tbuf, p->l->stream->name, p->l->line);
		return NULL;
	}

	/* TODO temporary check */
	if (t->tok.id_type != SUBTILIS_IDENTIFIER_INTEGER) {
		subtilis_error_set_integer_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return NULL;
	}
	op1.reg = SUBTILIS_IR_REG_GLOBAL;
	op2.integer = s->loc;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
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

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_unary_minus(p, e, err);
}

static subtilis_exp_t *prv_not_exp(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_exp_not(p, e, err);
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
	case SUBTILIS_TOKEN_OPERATOR:
		if (!strcmp(tbuf, "(")) {
			e = prv_bracketed_exp(p, t, err);
		} else {
			if (!strcmp(tbuf, "-")) {
				e = prv_unary_minus_exp(p, t, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				/* we don't want to read another token here.It's
				 * already been read by the recursive call to
				 * prv_expression.
				 */
				return e;
			}
			subtilis_error_set_exp_expected(
			    err, "( or - ", p->l->stream->name, p->l->line);
			goto cleanup;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_IDENTIFIER:
		e = prv_variable(p, t, tbuf, err);
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
		default:
			subtilis_error_set_exp_expected(
			    err, "TRUE or FALSE or NOTexpected",
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
	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "*"))
			exp_fn = subtilis_exp_mul;
		else if (!strcmp(tbuf, "/"))
			exp_fn = subtilis_exp_div;
		else
			break;
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

static void prv_assignment(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_op_instr_type_t instr;
	const subtilis_symbol_t *s;
	size_t reg;

	tbuf = subtilis_token_get_text(t);

	if (p->current == p->main) {
		s = subtilis_symbol_table_insert(p->st, tbuf, t->tok.id_type,
						 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (!s) {
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
	op1.reg = SUBTILIS_IR_REG_GLOBAL;
	op2.integer = s->loc;
	subtilis_ir_section_add_instr_reg(p->current, instr, e->exp.ir_op, op1,
					  op2, err);

cleanup:
	subtilis_exp_delete(e);
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

static void prv_print(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	size_t reg;

	e = prv_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		/* TODO this code is duplicated with prv_assignment */
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(e);
		e = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	case SUBTILIS_EXP_INTEGER:
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_I32, e->exp.ir_op, err);
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

static void prv_compound(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_keyword_type_t end_key, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;
	unsigned int start;
	subtilis_keyword_type_t key_type;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		key_type = t->tok.keyword.type;
		if (key_type == end_key) {
			if ((end_key != SUBTILIS_KEYWORD_ENDPROC) ||
			    (p->level == 1))
				break;
		}

		fn = keyword_fns[key_type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}
		fn(p, t, err);
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

	/* TODO: Probably need to allow for floating point numbers as well
	 * here
	 */

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

static void prv_def(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;

	if (p->current != p->main) {
		subtilis_error_set_nested_procedure(err, p->l->stream->name,
						    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (t->tok.keyword.type != SUBTILIS_KEYWORD_PROC)) {
		subtilis_error_set_expected(err, "PROC", tbuf,
					    p->l->stream->name, p->l->line);
		return;
	}

	tbuf += 4;

	p->current = subtilis_ir_prog_section_new(p->prog, tbuf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compound(p, t, SUBTILIS_KEYWORD_ENDPROC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current = p->main;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void prv_root(subtilis_parser_t *p, subtilis_token_t *t, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		fn = keyword_fns[t->tok.keyword.type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}
		fn(p, t, err);
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
	NULL, /* SUBTILIS_KEYWORD_CIRCLE */
	NULL, /* SUBTILIS_KEYWORD_CLEAR */
	NULL, /* SUBTILIS_KEYWORD_CLG */
	NULL, /* SUBTILIS_KEYWORD_CLOSE_HASH */
	NULL, /* SUBTILIS_KEYWORD_CLS */
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
	NULL, /* SUBTILIS_KEYWORD_DRAW */
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
	NULL, /* SUBTILIS_KEYWORD_FILL */
	NULL, /* SUBTILIS_KEYWORD_FN */
	NULL, /* SUBTILIS_KEYWORD_FOR */
	NULL, /* SUBTILIS_KEYWORD_GCOL */
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
	NULL, /* SUBTILIS_KEYWORD_LINE */
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
	NULL, /* SUBTILIS_KEYWORD_MODE */
	NULL, /* SUBTILIS_KEYWORD_MOUSE */
	NULL, /* SUBTILIS_KEYWORD_MOVE */
	NULL, /* SUBTILIS_KEYWORD_NEW */
	NULL, /* SUBTILIS_KEYWORD_NEXT */
	NULL, /* SUBTILIS_KEYWORD_NOT */
	NULL, /* SUBTILIS_KEYWORD_OF */
	NULL, /* SUBTILIS_KEYWORD_OFF */
	NULL, /* SUBTILIS_KEYWORD_OLD */
	NULL, /* SUBTILIS_KEYWORD_ON */
	NULL, /* SUBTILIS_KEYWORD_OPENIN */
	NULL, /* SUBTILIS_KEYWORD_OPENOUT */
	NULL, /* SUBTILIS_KEYWORD_OPENUP */
	NULL, /* SUBTILIS_KEYWORD_OR */
	NULL, /* SUBTILIS_KEYWORD_ORIGIN */
	NULL, /* SUBTILIS_KEYWORD_OSCLI */
	NULL, /* SUBTILIS_KEYWORD_OTHERWISE */
	NULL, /* SUBTILIS_KEYWORD_OVERLAY */
	NULL, /* SUBTILIS_KEYWORD_PAGE */
	NULL, /* SUBTILIS_KEYWORD_PI */
	NULL, /* SUBTILIS_KEYWORD_PLOT */
	NULL, /* SUBTILIS_KEYWORD_POINT */
	NULL, /* SUBTILIS_KEYWORD_POS */
	prv_print, /* SUBTILIS_KEYWORD_PRINT */
	NULL, /* SUBTILIS_KEYWORD_PRINT_HASH */
	NULL, /* SUBTILIS_KEYWORD_PROC */
	NULL, /* SUBTILIS_KEYWORD_PTR_HASH */
	NULL, /* SUBTILIS_KEYWORD_QUIT */
	NULL, /* SUBTILIS_KEYWORD_RAD */
	NULL, /* SUBTILIS_KEYWORD_READ */
	NULL, /* SUBTILIS_KEYWORD_RECTANGLE */
	NULL, /* SUBTILIS_KEYWORD_REM */
	NULL, /* SUBTILIS_KEYWORD_RENUMBER */
	NULL, /* SUBTILIS_KEYWORD_REPEAT */
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
	NULL, /* SUBTILIS_KEYWORD_VDU */
	NULL, /* SUBTILIS_KEYWORD_VOICES */
	NULL, /* SUBTILIS_KEYWORD_VPOS */
	NULL, /* SUBTILIS_KEYWORD_WAIT */
	NULL, /* SUBTILIS_KEYWORD_WHEN */
	prv_while, /* SUBTILIS_KEYWORD_WHILE */
	NULL, /* SUBTILIS_KEYWORD_WIDTH */
};

/* clang-format on */
