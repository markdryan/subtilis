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
#include "parser_output.h"
#include "type_if.h"

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
		    p->current, SUBTILIS_OP_INSTR_VDUI, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op1.integer = (e->exp.ir_op.integer >> 8) & 0xff;
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
		e = subtilis_parser_priority7(p, t, err);
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

void subtilis_parser_vdu(subtilis_parser_t *p, subtilis_token_t *t,
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
		e = subtilis_parser_priority7(p, t, err);
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

static void prv_spc(subtilis_parser_t *p, subtilis_exp_t *e,
		    subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t next_label;
	subtilis_ir_operand_t counter_reg;
	subtilis_exp_t *one = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *space = NULL;
	subtilis_exp_t *cond = NULL;
	subtilis_exp_t *e_dup = NULL;

	start_label.label = subtilis_ir_section_new_label(p->current);
	next_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	counter_reg = e->exp.ir_op;

	subtilis_ir_section_add_label(p->current, start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	one = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	space = subtilis_exp_new_int32(32, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	cond = subtilis_type_if_gte(p, zero, e_dup, err);
	zero = NULL;
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond->exp.ir_op, end_label,
					  next_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, next_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_vdu_byte(p, space, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_sub(p, e, one, err);
	one = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      counter_reg, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);

cleanup:

	subtilis_exp_delete(cond);
	subtilis_exp_delete(space);
	subtilis_exp_delete(one);
	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(zero);
	subtilis_exp_delete(e);
}

static void prv_tab_one_arg(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_exp_t *e, subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t gt_label;
	subtilis_ir_operand_t lt_label;
	subtilis_ir_operand_t end_label;
	subtilis_exp_t *cond = NULL;
	subtilis_exp_t *pos = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *e_dup = NULL;

	gt_label.label = subtilis_ir_section_new_label(p->current);
	lt_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	reg = subtilis_ir_section_add_instr1(p->current, SUBTILIS_OP_INSTR_POS,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	pos = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_sub(p, e, pos, err);
	pos = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	cond = subtilis_type_if_gte(p, zero, e_dup, err);
	zero = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond->exp.ir_op, lt_label, gt_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, gt_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_spc(p, e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, lt_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_PRINT_NL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);

cleanup:

	subtilis_exp_delete(cond);
	subtilis_exp_delete(zero);
	subtilis_exp_delete(pos);
	subtilis_exp_delete(e);
}

static void prv_parse_spc(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_spc(p, e, err);
}

static void prv_parse_tab(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e[2];
	size_t args;
	size_t i;

	memset(&e, 0, sizeof(e));

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR || strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return;
	}

	args = subtilis_var_bracketed_int_args_have_b(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (args == 2) {
		subtilis_ir_section_add_instr_no_reg2(
		    p->current, SUBTILIS_OP_INSTR_AT, e[0]->exp.ir_op,
		    e[1]->exp.ir_op, err);
	} else if (args == 1) {
		prv_tab_one_arg(p, t, e[0], err);
		e[0] = NULL;
	} else {
		subtilis_error_set_integer_expected(
		    err, ")", p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

cleanup:

	for (i = 0; i < args; i++)
		subtilis_exp_delete(e[i]);
}

void subtilis_parser_print(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (t->type == SUBTILIS_TOKEN_KEYWORD) {
		if (t->tok.keyword.type == SUBTILIS_KEYWORD_TAB)
			prv_parse_tab(p, t, err);
		else if (t->tok.keyword.type == SUBTILIS_KEYWORD_SPC)
			prv_parse_spc(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (subtilis_type_if_is_numeric(&e->type)) {
		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_type_if_print(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, ";")))
		subtilis_lexer_get(p->l, t, err);
	else
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_PRINT_NL, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

static subtilis_exp_t *prv_pos(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_op_instr_type_t itype,
			       subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_instr1(p->current, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_int32_var(reg, err);
}

subtilis_exp_t *subtilis_parser_pos(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_pos(p, t, SUBTILIS_OP_INSTR_POS, err);
}

subtilis_exp_t *subtilis_parser_vpos(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	return prv_pos(p, t, SUBTILIS_OP_INSTR_VPOS, err);
}
