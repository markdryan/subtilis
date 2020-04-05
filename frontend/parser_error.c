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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "parser_array.h"
#include "parser_call.h"
#include "parser_error.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "type_if.h"
#include "variable.h"

subtilis_exp_t *subtilis_parser_get_err(subtilis_parser_t *p,
					subtilis_token_t *t,
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

void subtilis_parser_handle_escape(subtilis_parser_t *p, subtilis_error_t *err)
{
	/* Let's not test for escape in an error handler. */

	if (!p->settings.handle_escapes || p->current->in_error_handler)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_TESTESC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

void subtilis_parser_onerror(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err)
{
	subtilis_ir_operand_t handler_label;
	subtilis_ir_operand_t target_label;
	unsigned int start;
	subtilis_ir_operand_t var_reg;

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

	subtilis_var_set_eflag(p, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_symbol_table_level_up(p->local_st, p->l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_ENDERROR))
			break;
		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_reference_deallocate_refs(p, var_reg, p->local_st, p->level,
					   err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!p->current->endproc) {
		if (!p->current->handler_list) {
			if (p->current != p->main) {
				subtilis_var_set_eflag(p, true, err);
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
	subtilis_symbol_table_level_down(p->local_st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	p->current->endproc = false;

	subtilis_lexer_get(p->l, t, err);

cleanup:
	p->current->in_error_handler = false;
}

void subtilis_parser_error(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return;
	}

	subtilis_exp_generate_error(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}
