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

void subtilis_parser_print(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_print(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_PRINT_NL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}
