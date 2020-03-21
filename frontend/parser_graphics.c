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

#include <string.h>

#include "parser_exp.h"
#include "parser_graphics.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_get_point(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err)
{
	return subtilis_parser_bracketed_2_int_args(
	    p, t, SUBTILIS_OP_INSTR_POINT, err);
}

subtilis_exp_t *subtilis_parser_get_tint(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	return subtilis_parser_bracketed_2_int_args(
	    p, t, SUBTILIS_OP_INSTR_TINT, err);
}

void subtilis_parser_mode(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_parser_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_MODE_I32, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!p->settings.ignore_graphics_errors)
		subtilis_exp_handle_errors(p, err);

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_parser_plot(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[3];

	memset(&e, 0, sizeof(e));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_statement_int_args(p, t, e, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_PLOT,
					  e[0]->exp.ir_op, e[1]->exp.ir_op,
					  e[2]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!p->settings.ignore_graphics_errors)
		subtilis_exp_handle_errors(p, err);

cleanup:
	for (i = 0; i < 3; i++)
		subtilis_exp_delete(e[i]);
}

void subtilis_parser_wait(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_WAIT,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!p->settings.ignore_graphics_errors) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

static void prv_simple_plot(subtilis_parser_t *p, subtilis_token_t *t,
			    int32_t plot_code, subtilis_error_t *err)
{
	size_t i;
	subtilis_exp_t *e[3];

	memset(&e, 0, sizeof(e));

	subtilis_parser_statement_int_args(p, t, &e[1], 2, err);
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

	if (!p->settings.ignore_graphics_errors)
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

void subtilis_parser_move(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	prv_move_draw(p, t, 4, err);
}

void subtilis_parser_fill(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_simple_plot(p, t, 133, err);
}

void subtilis_parser_line(subtilis_parser_t *p, subtilis_token_t *t,
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

void subtilis_parser_circle(subtilis_parser_t *p, subtilis_token_t *t,
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

	e[1] = subtilis_parser_int_var_expression(p, t, err);
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

	subtilis_parser_statement_int_args(p, t, &e[2], 2, err);
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

void subtilis_parser_rectangle(subtilis_parser_t *p, subtilis_token_t *t,
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

void subtilis_parser_draw(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	prv_move_draw(p, t, 5, err);
}

void subtilis_parser_point(subtilis_parser_t *p, subtilis_token_t *t,
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

void subtilis_parser_gcol(subtilis_parser_t *p, subtilis_token_t *t,
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

	subtilis_parser_statement_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_GCOL,
					  e[0]->exp.ir_op, e[1]->exp.ir_op, op2,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!p->settings.ignore_graphics_errors)
		subtilis_exp_handle_errors(p, err);

cleanup:
	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);
}

void subtilis_parser_origin(subtilis_parser_t *p, subtilis_token_t *t,
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

	subtilis_parser_statement_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_ORIGIN,
					  e[0]->exp.ir_op, e[1]->exp.ir_op, op2,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!p->settings.ignore_graphics_errors)
		subtilis_exp_handle_errors(p, err);

cleanup:
	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);
}

void subtilis_parser_cls(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_CLS,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!p->settings.ignore_graphics_errors) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

void subtilis_parser_clg(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_CLG,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!p->settings.ignore_graphics_errors) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

void subtilis_parser_on(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_ON,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!p->settings.ignore_graphics_errors) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

void subtilis_parser_off(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_OFF,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!p->settings.ignore_graphics_errors) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}
