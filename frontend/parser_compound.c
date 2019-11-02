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
#include "parser_call.h"
#include "parser_compound.h"
#include "parser_exp.h"
#include "type_if.h"

void subtilis_parser_local(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	subtilis_type_t type;
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
	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "="))) {
		e = subtilis_type_if_zero(p, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e = subtilis_exp_coerce_type(p, e, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	(void)subtilis_symbol_table_insert_reg(p->local_st, var_name, &type,
					       e->exp.ir_op.reg, err);
	subtilis_exp_delete(e);
	free(var_name);
	var_name = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		return;

cleanup:

	free(var_name);
}

void subtilis_parser_compound(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_keyword_type_t end_key,
			      subtilis_error_t *err)
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
	p->level--;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
}
