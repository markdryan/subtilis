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

#include "parser_assignment.h"
#include "parser_exp.h"
#include "type_if.h"
#include "variable.h"

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

static void prv_assign_array(subtilis_parser_t *p, subtilis_token_t *t,
			     const subtilis_symbol_t *s, size_t mem_reg,
			     const char *var_name, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	subtilis_exp_t *e;
	size_t i;
	subtilis_assign_type_t at;
	size_t dims = 0;

	dims = subtilis_var_bracketed_int_args_have_b(
	    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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
	} else if (!strcmp(tbuf, "+=")) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=")) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (at) {
	case SUBTILIS_ASSIGN_TYPE_EQUAL:
		subtilis_type_if_indexed_write(p, var_name, &s->t, mem_reg,
					       s->loc, e, indices, dims, err);
		break;
	case SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL:
		subtilis_type_if_indexed_add(p, var_name, &s->t, mem_reg,
					     s->loc, e, indices, dims, err);
		break;
	case SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL:
		subtilis_type_if_indexed_sub(p, var_name, &s->t, mem_reg,
					     s->loc, e, indices, dims, err);
		break;
	}

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);
}

char *subtilis_parser_lookup_assignment_var(subtilis_parser_t *p,
					    subtilis_token_t *t,
					    subtilis_type_t *id_type,
					    const subtilis_symbol_t **s,
					    size_t *mem_reg, bool *new_var,
					    subtilis_error_t *err)
{
	const char *tbuf;
	char *var_name = NULL;

	tbuf = subtilis_token_get_text(t);
	var_name = malloc(strlen(tbuf) + 1);
	if (!var_name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(var_name, tbuf);
	*id_type = t->tok.id_type;
	*s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (*s) {
		*mem_reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		*s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (*s) {
			*mem_reg = SUBTILIS_IR_REG_GLOBAL;
		} else if (p->current == p->main) {
			/*
			 * We explicitly disable statements like
			 *
			 * X% = X% + 1  or
			 * X% += 1
			 *
			 * for the cases where X% has not been defined.
			 */
			*new_var = true;
			*mem_reg = SUBTILIS_IR_REG_GLOBAL;
		} else {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto cleanup;
		}
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return var_name;

cleanup:

	free(var_name);

	return NULL;
}

void subtilis_parser_assignment(subtilis_parser_t *p, subtilis_token_t *t,
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

	var_name = subtilis_parser_lookup_assignment_var(
	    p, t, &type, &s, &op1.reg, &new_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, "(")) {
		if (new_var) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}

		prv_assign_array(p, t, s, op1.reg, var_name, err);
		free(var_name);
		return;
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

	e = subtilis_parser_expression(p, t, err);
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
