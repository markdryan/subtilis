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
#include "parser_assignment.h"
#include "parser_exp.h"
#include "type_if.h"
#include "variable.h"

typedef enum {
	SUBTILIS_ASSIGN_TYPE_EQUAL,
	SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL,
	SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL,
	SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL,
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

void subtilis_parser_create_array_ref(subtilis_parser_t *p,
				      const char *var_name,
				      const subtilis_type_t *id_type,
				      subtilis_exp_t *e, bool local,
				      subtilis_error_t *err)
{
	subtilis_type_t type;
	const subtilis_symbol_t *s;

	subtilis_type_if_array_of(p, id_type, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (type.type != e->type.type) {
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);
		goto cleanup;
	}
	type = e->type;

	/*
	 * If the expression is a temporary variable and we're assigning to
	 * a local variable, we can simply just promote the temporary to a
	 * full local variable and avoid copying the data.
	 */

	if (local && e->temporary) {
		(void)subtilis_symbol_table_promote_tmp(
		    p->local_st, &e->type, e->temporary, var_name, err);

		subtilis_exp_delete(e);
		return;
	}

	s = subtilis_symbol_table_insert(local ? p->local_st : p->st, var_name,
					 &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_create_ref(
	    p, var_name, s,
	    local ? SUBTILIS_IR_REG_LOCAL : SUBTILIS_IR_REG_GLOBAL, e, err);
	return;

cleanup:

	subtilis_exp_delete(e);
}

static void prv_assign_array(subtilis_parser_t *p, subtilis_token_t *t,
			     const char *var_name,
			     const subtilis_type_t *id_type,
			     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_assign_type_t at;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;
	bool new_global = false;
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
	} else if (!strcmp(tbuf, "+=") && (dims > 0)) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=") && (dims > 0)) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else if (!strcmp(tbuf, ":=") && (dims == 0)) {
		at = SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL;
	} else {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type == SUBTILIS_ERROR_UNKNOWN_VARIABLE) {
		if (at != SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL)
			goto cleanup;
		subtilis_error_init(err);
		subtilis_parser_create_array_ref(p, var_name, id_type, e, true,
						 err);
		return;
	} else if (err->type != SUBTILIS_ERROR_OK) {
		goto cleanup;
	} else if (new_global) {
		if (dims != 0) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		subtilis_parser_create_array_ref(p, var_name, id_type, e, false,
						 err);
		return;
	}

	switch (at) {
	case SUBTILIS_ASSIGN_TYPE_EQUAL:
		if (dims == 0)
			/* We're assigning an array reference */
			subtilis_parser_array_assign_reference(p, t, op1.reg, s,
							       e, err);
		else
			subtilis_type_if_indexed_write(p, var_name, &s->t,
						       op1.reg, s->loc, e,
						       indices, dims, err);
		e = NULL;
		break;
	case SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL:
		subtilis_type_if_indexed_add(p, var_name, &s->t, op1.reg,
					     s->loc, e, indices, dims, err);
		e = NULL;
		break;
	case SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL:
		subtilis_type_if_indexed_sub(p, var_name, &s->t, op1.reg,
					     s->loc, e, indices, dims, err);
		e = NULL;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}

cleanup:

	subtilis_exp_delete(e);
	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);
}

subtilis_exp_t *subtilis_parser_assign_local_num(subtilis_parser_t *p,
						 subtilis_token_t *t,
						 const char *var_name,
						 subtilis_type_t *type,
						 subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_type_if_copy_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_type_if_coerce_type(p, e, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return e;
}

static void prv_assignment_local(subtilis_parser_t *p, subtilis_token_t *t,
				 const char *var_name, subtilis_type_t *id_type,
				 subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_exp_t *e;

	if (subtilis_symbol_table_lookup(p->local_st, var_name)) {
		subtilis_error_set_already_defined(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	if (subtilis_type_if_is_numeric(id_type)) {
		e = subtilis_parser_assign_local_num(p, t, var_name, id_type,
						     err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, id_type, e->exp.ir_op.reg, err);
		subtilis_exp_delete(e);
		return;
	}

	/*
	 * We've got a reference type, e.g., a string.
	 */

	s = subtilis_symbol_table_insert(p->local_st, var_name, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_new_ref(p, id_type, SUBTILIS_IR_REG_LOCAL, s->loc, e,
				 err);
}

char *subtilis_parser_get_assignment_var(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_type_t *id_type,
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

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return var_name;

cleanup:

	free(var_name);

	return NULL;
}

/* clang-format off */
void subtilis_parser_lookup_assignment_var(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   const char *var_name,
					   const subtilis_symbol_t **s,
					   size_t *mem_reg, bool *new_global,
					   subtilis_error_t *err)
/* clang-format on */

{
	*s = subtilis_symbol_table_lookup(p->local_st, var_name);
	if (*s) {
		*mem_reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		*s = subtilis_symbol_table_lookup(p->st, var_name);
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
			*new_global = true;
			*mem_reg = SUBTILIS_IR_REG_GLOBAL;
			if (p->level != 0) {
				subtilis_error_variable_bad_level(
				    err, var_name, p->l->stream->name,
				    p->l->line);
			}
		} else {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
		}
	}
}

void subtilis_parser_assignment(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_assign_type_t at;
	subtilis_type_t type;
	bool new_global = false;
	char *var_name = NULL;
	subtilis_exp_t *e = NULL;

	var_name = subtilis_parser_get_assignment_var(p, t, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, "(")) {
		prv_assign_array(p, t, var_name, &type, err);
		free(var_name);
		return;
	}

	if (!strcmp(tbuf, ":=")) {
		prv_assignment_local(p, t, var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		free(var_name);
		return;
	}

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!strcmp(tbuf, "=") || !strcmp(tbuf, ":=")) {
		at = SUBTILIS_ASSIGN_TYPE_EQUAL;
	} else if (new_global) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	} else if (!strcmp(tbuf, "+=")) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=") && subtilis_type_if_is_numeric(&type)) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (new_global) {
		s = subtilis_symbol_table_insert(p->st, var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	/* Ownership of e is passed to the following functions. */

	if (!subtilis_type_if_is_numeric(&s->t)) {
		if (at == SUBTILIS_ASSIGN_TYPE_EQUAL) {
			if (new_global)
				subtilis_type_if_new_ref(p, &s->t, op1.reg,
							 s->loc, e, err);
			else
				subtilis_type_if_assign_ref(p, &s->t, op1.reg,
							    s->loc, e, err);
		} else {
			subtilis_error_set_assertion_failed(err);
		}
		goto cleanup;
	}

	e = subtilis_type_if_coerce_type(p, e, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

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

void subtilis_parser_let(subtilis_parser_t *p, subtilis_token_t *t,
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

	subtilis_parser_assignment(p, t, err);
}
