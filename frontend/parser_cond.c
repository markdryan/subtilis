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

#include "parser.h"
#include "parser_assignment.h"
#include "parser_compound.h"
#include "parser_cond.h"
#include "parser_exp.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_conditional_exp(subtilis_parser_t *p,
						subtilis_token_t *t,
						subtilis_ir_operand_t *cond,
						subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;

	e = subtilis_parser_expression(p, t, err);
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

void subtilis_parser_if(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_exp_t *e;
	const char *tbuf;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;
	subtilis_ir_operand_t end_label;
	int key_type;

	e = subtilis_parser_conditional_exp(p, t, &cond, err);
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

	key_type = subtilis_parser_if_compound(p, t, err);
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
		subtilis_parser_compound(p, t, SUBTILIS_KEYWORD_ENDIF, err);
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
