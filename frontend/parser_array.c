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

#include "array_type.h"
#include "parser_array.h"
#include "parser_exp.h"
#include "type_if.h"

subtilis_exp_t *subtils_parser_read_array(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  const char *var_name,
					  subtilis_error_t *err)
{
	size_t mem_reg;
	const subtilis_symbol_t *s;
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_exp_t *e = NULL;
	size_t dims = 0;

	s = subtilis_symbol_table_lookup(p->local_st, var_name);
	if (s) {
		mem_reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		s = subtilis_symbol_table_lookup(p->st, var_name);
		if (!s) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			return NULL;
		}
		mem_reg = SUBTILIS_IR_REG_GLOBAL;
	}

	dims = subtilis_var_bracketed_int_args_have_b(
	    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (dims == 0)
		/* What we have here is an array reference. */
		e = subtilis_exp_new_var_block(p, &s->t, mem_reg, s->loc, err);
	else
		e = subtilis_type_if_indexed_read(p, var_name, &s->t, mem_reg,
						  s->loc, indices, dims, err);

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return e;
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

	return subtilis_var_bracketed_int_args_have_b(p, t, e, max, err);
}

static void prv_allocate_array(subtilis_parser_t *p, const char *var_name,
			       subtilis_type_t *type, size_t loc,
			       subtilis_exp_t **e,
			       subtilis_ir_operand_t store_reg,
			       subtilis_error_t *err)
{
	subtlis_array_type_allocate(p, var_name, type, loc, e, store_reg, err);
}

void subtilis_parser_array_assign_reference(subtilis_parser_t *p,
					    size_t mem_reg,
					    const subtilis_symbol_t *s,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	subtilis_array_type_match(p, &s->t, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_assign_ref(p, mem_reg, s->loc, e->exp.ir_op.reg,
				       err);

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_parser_create_array(subtilis_parser_t *p, subtilis_token_t *t,
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
		if (err->type == SUBTILIS_ERROR_RIGHT_BKT_EXPECTED) {
			subtilis_error_too_many_dims(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (!local && (p->level != 0)) {
			subtilis_error_variable_bad_level(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}

		subtilis_array_type_init(p, &element_type, &type, &e[0], dims,
					 err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		s = subtilis_symbol_table_insert(local ? p->local_st : p->st,
						 var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		prv_allocate_array(p, var_name, &type, s->loc, &e[0],
				   local_global, err);
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

void subtilis_parser_deallocate_arrays(subtilis_parser_t *p,
				       subtilis_ir_operand_t load_reg,
				       subtilis_symbol_table_t *st,
				       size_t level, subtilis_error_t *err)
{
	size_t i;
	const subtilis_symbol_t *s;
	subtilis_symbol_level_t *l = &st->levels[level];

	/*
	 * TODO: This whole thing needs to be more generic when we have more
	 * heap based types, e.g., strings.
	 */
	for (i = 0; i < l->size; i++) {
		s = l->symbols[i];
		if ((s->t.type != SUBTILIS_TYPE_ARRAY_REAL) &&
		    (s->t.type != SUBTILIS_TYPE_ARRAY_INTEGER))
			continue;

		subtilis_array_type_pop_and_deref(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

subtilis_exp_t *subtilis_parser_get_dim(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	const size_t max_args = 2;
	subtilis_exp_t *indices[max_args];
	const char *tbuf;
	size_t i;
	size_t dims = 0;
	subtilis_exp_t *e = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "("))) {
		subtilis_error_set_expected(err, "(", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	dims = subtilis_var_bracketed_args_have_b(p, t, indices, max_args, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (dims == 0) {
		subtilis_error_set_exp_expected(err, "array reference",
						p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if ((indices[0]->type.type != SUBTILIS_TYPE_ARRAY_REAL) &&
	    (indices[0]->type.type != SUBTILIS_TYPE_ARRAY_INTEGER)) {
		subtilis_error_not_array(err, "First argument to dim",
					 p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_array_get_dim(p, indices, dims, err);
	dims = 0;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return NULL;
}
