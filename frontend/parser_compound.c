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

#include <stdlib.h>
#include <string.h>

#include "parser_array.h"
#include "parser_assignment.h"
#include "parser_call.h"
#include "parser_compound.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "type_if.h"

static void prv_local_array_ref(subtilis_parser_t *p, subtilis_token_t *t,
				const char *var_name, subtilis_type_t *type,
				subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	subtilis_type_t array_type;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")"))) {
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);

	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "="))) {
		subtilis_error_set_exp_expected(err, "= ", p->l->stream->name,
						p->l->line);
		return;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_array_of(p, type, &array_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_create_array_ref(p, var_name, &array_type, e, true,
					 err);
}

static void prv_local_vector_ref(subtilis_parser_t *p, subtilis_token_t *t,
				 const char *var_name, subtilis_type_t *type,
				 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	subtilis_type_t vector_type;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "}"))) {
		subtilis_error_set_expected(err, "}", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);

	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "="))) {
		subtilis_error_set_exp_expected(err, "= ", p->l->stream->name,
						p->l->line);
		return;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_vector_of(p, type, &vector_type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return;
	}

	subtilis_parser_create_array_ref(p, var_name, &vector_type, e, true,
					 err);
}

void subtilis_parser_local(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t type;
	bool value_present;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;
	char *var_name = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_DIM)) {
		subtilis_parser_create_array(p, t, true, err);
		return;
	} else if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	if ((p->current == p->main) && (p->level == 0)) {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (s) {
			subtilis_error_set_local_obscures_global(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}
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
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		prv_local_array_ref(p, t, var_name, &type, err);
		free(var_name);
		return;
	}

	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "{")) {
		prv_local_vector_ref(p, t, var_name, &type, err);
		free(var_name);
		return;
	}

	value_present =
	    (t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "=");
	if (subtilis_type_if_is_numeric(&type)) {
		if (!value_present) {
			e = subtilis_type_if_zero(p, &type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			e = subtilis_parser_assign_local_num(p, t, var_name,
							     &type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, &type, e->exp.ir_op.reg, err);
	} else {
		s = subtilis_symbol_table_insert(p->local_st, var_name, &type,
						 err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (!value_present) {
			subtilis_type_if_zero_ref(
			    p, &type, SUBTILIS_IR_REG_LOCAL, s->loc, true, err);
		} else {
			e = subtilis_parser_expression(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			subtilis_type_if_new_ref(
			    p, &type, SUBTILIS_IR_REG_LOCAL, s->loc, e, err);
			e = NULL;
		}
	}

cleanup:

	subtilis_exp_delete(e);
	free(var_name);
}

void subtilis_parser_compound_at_sym_level(subtilis_parser_t *p,
					   subtilis_token_t *t, int end_key,
					   subtilis_error_t *err)
{
	unsigned int start;
	subtilis_ir_operand_t var_reg;

	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == end_key))
			break;
		subtilis_parser_statement(p, t, err);
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

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_reference_deallocate_refs(p, var_reg, p->local_st, p->level,
					   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->level--;
	subtilis_symbol_table_level_down(p->local_st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
}

void subtilis_parser_compound_at_level(subtilis_parser_t *p,
				       subtilis_token_t *t, int end_key,
				       subtilis_error_t *err)
{
	subtilis_symbol_table_level_up(p->local_st, p->l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_compound_at_sym_level(p, t, end_key, err);
}

void subtilis_parser_compound_statement_at_level(subtilis_parser_t *p,
						 subtilis_token_t *t,
						 subtilis_error_t *err)
{
	subtilis_ir_operand_t var_reg;

	subtilis_symbol_table_level_up(p->local_st, p->l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_statement(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_reference_deallocate_refs(p, var_reg, p->local_st, p->level,
					   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->level--;
	subtilis_symbol_table_level_down(p->local_st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
}

void subtilis_parser_compound(subtilis_parser_t *p, subtilis_token_t *t,
			      int end_key, subtilis_error_t *err)
{
	p->level++;
	subtilis_parser_compound_at_level(p, t, end_key, err);
}
