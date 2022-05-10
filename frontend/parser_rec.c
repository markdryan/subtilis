/*
 * Copyright (c) 2022 Mark Ryan
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

#include "parser_rec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array_type.h"
#include "parser_array.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "type_if.h"

static void prv_check_id(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_type_t *field_type, subtilis_error_t *err)
{
	const char *tbuf;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	subtilis_type_init_copy(field_type, &t->tok.id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((field_type->type == SUBTILIS_TYPE_FN) ||
	    (field_type->type == SUBTILIS_TYPE_REC)) {
		subtilis_complete_custom_type(p, tbuf, field_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			subtilis_type_free(field_type);
	}
}

static void prv_parse_dim(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_type_t *type, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t element_type;
	size_t i;
	subtilis_type_t field;
	size_t size;
	size_t align;
	subtilis_exp_t *e[SUBTILIS_MAX_DIMENSIONS];
	char *var_name = NULL;
	bool vec = false;
	size_t dims = 0;
	int32_t vec_dim = 0;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_check_id(p, t, &element_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	var_name = malloc(strlen(tbuf) + 1);
	if (!var_name) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(var_name, tbuf);

	dims = parser_array_var_bracketed_int_args(
	    p, t, e, SUBTILIS_MAX_DIMENSIONS, &vec, err);
	if (err->type == SUBTILIS_ERROR_RIGHT_BKT_EXPECTED) {
		subtilis_error_too_many_dims(err, var_name, p->l->stream->name,
					     p->l->line);
		goto cleanup;
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!vec && dims == 0) {
		subtilis_error_set_const_integer_expected(
		    err, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	for (i = 0; i < dims; i++) {
		if (e[i]->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			continue;

		subtilis_error_set_const_integer_expected(
		    err, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (vec) {
		subtilis_array_type_vector_init(&element_type, &field, err);
		vec_dim = dims > 0 ? e[0]->exp.ir_op.integer : -1;
	} else {
		subtilis_array_type_init(p, var_name, &element_type, &field,
					 &e[0], dims, err);
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size = subtilis_type_if_size(&field, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_type_free(&field);
		goto cleanup;
	}
	align = subtilis_type_if_align(&field, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_type_free(&field);
		goto cleanup;
	}
	subtilis_type_rec_add_field(type, var_name, &field, align, size,
				    vec_dim, err);
	subtilis_type_free(&field);

	if (err->type == SUBTILIS_ERROR_OK)
		subtilis_lexer_get(p->l, t, err);

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(e[i]);

	subtilis_type_free(&element_type);
	free(var_name);
}

static void prv_parse_fields(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_type_t *type, subtilis_error_t *err)
{
	const char *tbuf;
	size_t size;
	size_t align;
	subtilis_type_t field;

	tbuf = subtilis_token_get_text(t);

	while (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")"))) {
		if (t->type == SUBTILIS_TOKEN_KEYWORD) {
			if (t->tok.keyword.type != SUBTILIS_KEYWORD_DIM) {
				subtilis_error_set_expected(err, ")", tbuf,
							    p->l->stream->name,
							    p->l->line);
				return;
			}

			prv_parse_dim(p, t, type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			tbuf = subtilis_token_get_text(t);
			continue;
		}

		prv_check_id(p, t, &field, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		size = subtilis_type_if_size(&field, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		align = subtilis_type_if_align(&field, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_type_rec_add_field(type, tbuf, &field, align, size, 0,
					    err);
		subtilis_type_free(&field);
		if (err->type != SUBTILIS_ERROR_OK) {
			if (err->type == SUBTILIS_ERROR_TYPE_ALREADY_DEFINED)
				subtilis_error_set_already_defined(
				    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(t);
	}

	return;

on_error:
	subtilis_type_free(&field);
}

char *subtilis_parser_parse_rec_type(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_type_t *type,
				     subtilis_error_t *err)
{
	const char *tbuf;
	char *name = NULL;

	tbuf = subtilis_token_get_text(t);
	subtilis_type_init_rec(type, tbuf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	name = malloc(strlen(tbuf) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(name, tbuf);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "(", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_parse_fields(p, t, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (type->params.rec.num_fields == 0) {
		subtilis_error_set_empty_rec(err, name, p->l->stream->name,
					     p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return name;

cleanup:

	free(name);

	return NULL;
}

subtilis_exp_t *subtilis_parser_rec_exp(subtilis_parser_t *p,
					subtilis_token_t *t,
					const subtilis_type_t *type,
					size_t mem_reg, size_t loc,
					subtilis_error_t *err)
{
	size_t id;
	const subtilis_type_t *field_type;
	const subtilis_type_rec_t *rec;
	const char *tbuf;
	size_t reg;
	subtilis_exp_t *e = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_IDENTIFIER)) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return NULL;
	}

	rec = &type->params.rec;
	id = subtilis_type_rec_find_field(rec, tbuf);
	if (id == SIZE_MAX) {
		subtilis_error_set_unknown_field(err, tbuf, p->l->stream->name,
						 p->l->line);
		return NULL;
	}

	loc += (size_t)subtilis_type_rec_field_offset_id(rec, id);
	field_type = &rec->field_types[id];

	if ((subtilis_type_if_is_numeric(field_type) ||
	     field_type->type == SUBTILIS_TYPE_FN)) {
		e = subtilis_type_if_load_from_mem(p, field_type, mem_reg, loc,
						   err);
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if ((field_type->type == SUBTILIS_TYPE_STRING) ||
		   (field_type->type == SUBTILIS_TYPE_REC)) {
		reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		e = subtilis_exp_new_var(field_type, reg, err);
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if (subtilis_type_if_is_array(field_type)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
			subtilis_error_set_exp_expected(
			    err, "(", p->l->stream->name, p->l->line);
			return NULL;
		}
		e = subtils_parser_read_array_with_type(
		    p, t, field_type, mem_reg, loc, rec->fields[id].name, err);
	} else if (subtilis_type_if_is_vector(field_type)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "{")) {
			subtilis_error_set_exp_expected(
			    err, "{", p->l->stream->name, p->l->line);
			return NULL;
		}
		e = subtils_parser_read_vector_with_type(
		    p, t, field_type, mem_reg, loc, rec->fields[id].name, err);
	} else {
		subtilis_error_set_assertion_failed(err);
	}

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}
