/*
 * Copyright (c) 2020 Mark Ryan
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

#include "parser_exp.h"
#include "parser_string.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_chrstr(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_exp_t *retval;
	subtilis_type_t type;
	size_t reg;
	const subtilis_symbol_t *s;
	subtilis_buffer_t buf;
	char c_str[2];

	subtilis_buffer_init(&buf, 2);

	e = subtilis_parser_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (subtilis_type_if_is_const(&e->type)) {
		c_str[0] = e->exp.ir_op.integer & 255;
		c_str[1] = 0;
		subtilis_buffer_append(&buf, c_str, 2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		retval = subtilis_exp_new_str(&buf, err);
		subtilis_buffer_free(&buf);
		subtilis_exp_delete(e);
		return retval;
	}

	type.type = SUBTILIS_TYPE_STRING;
	s = subtilis_symbol_table_insert_tmp(p->local_st, &type, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_new_ref_from_char(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					       e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&s->t, reg, err);

cleanup:
	subtilis_buffer_free(&buf);
	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_parser_asc(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_asc(p, e, err);
}

subtilis_exp_t *subtilis_parser_len(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_len(p, e, err);
}

static void prv_left_right_args(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_exp_t **e, subtilis_error_t *err)
{
	size_t args;
	size_t i;
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return;
	}

	args = subtilis_var_bracketed_args_have_b(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (args == 1) {
		e[1] = subtilis_exp_new_int32(1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return;

cleanup:

	for (i = 0; i < args; i++)
		subtilis_exp_delete(e[i]);
}

subtilis_exp_t *subtilis_parser_left_str(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	subtilis_exp_t *e[2];

	prv_left_right_args(p, t, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_left(p, e[0], e[1], err);
}

subtilis_exp_t *subtilis_parser_right_str(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err)
{
	subtilis_exp_t *e[2];

	prv_left_right_args(p, t, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_right(p, e[0], e[1], err);
}

subtilis_exp_t *subtilis_parser_mid_str(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	subtilis_exp_t *e[3];
	size_t args;
	size_t i;
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return NULL;
	}

	args = subtilis_var_bracketed_args_have_b(p, t, e, 3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (args == 0) {
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		return NULL;
	}

	if (args == 1) {
		subtilis_error_set_expected(err, ",", ")", p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	return subtilis_string_type_mid(p, e[0], e[1], e[2], err);

cleanup:

	for (i = 0; i < args; i++)
		subtilis_exp_delete(e[i]);

	return NULL;
}

subtilis_exp_t *subtilis_parser_string_str(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e[2];
	const char *tbuf;
	size_t args;
	size_t i;

	e[0] = NULL;
	e[1] = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return NULL;
	}

	args = subtilis_var_bracketed_args_have_b(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (args != 2 || ((e[1]->type.type != SUBTILIS_TYPE_CONST_STRING) &&
			  (e[1]->type.type != SUBTILIS_TYPE_STRING))) {
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	e[0] = subtilis_type_if_to_int(p, e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e[0] = subtilis_string_type_string(p, e[0], e[1], err);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e[0];

cleanup:

	for (i = 0; i < args; i++)
		subtilis_exp_delete(e[i]);

	return NULL;
}

subtilis_exp_t *subtilis_parser_str_str(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	subtilis_exp_t *val;

	val = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_to_string(p, val, err);
}
