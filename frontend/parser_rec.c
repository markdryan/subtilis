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
#include "rec_type.h"
#include "reference_type.h"
#include "string_type.h"
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
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if ((field_type->type == SUBTILIS_TYPE_STRING) ||
		   (field_type->type == SUBTILIS_TYPE_REC)) {
		reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		e = subtilis_exp_new_var(field_type, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
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

static void prv_array_field_init(subtilis_parser_t *p, subtilis_token_t *t,
				 const subtilis_type_field_t *field,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	subtilis_array_desc_t d;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		d.name = field->name;
		d.t = type;
		d.reg = mem_reg;
		d.loc = loc;

		if (subtilis_type_if_is_array(type))
			subtilis_rec_type_init_array(p, type, d.name, mem_reg,
						     loc, err);
		else
			subtilis_rec_type_init_vector(p, type, mem_reg, loc,
						      field->vec_dim, err);

		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_parser_array_init_list(p, t, &d, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
			subtilis_error_set_expected(
			    err, ")", tbuf, p->l->stream->name, p->l->line);
			return;
		}
		subtilis_lexer_get(p->l, t, err);
		return;
	}

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_init_field(p, type, mem_reg, loc, e, err);
}

static void prv_array_field_reset(subtilis_parser_t *p, subtilis_token_t *t,
				  const subtilis_type_field_t *field,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	subtilis_array_desc_t d;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		d.name = field->name;
		d.t = type;
		d.reg = mem_reg;
		d.loc = loc;

		subtilis_parser_array_init_list(p, t, &d, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
			subtilis_error_set_expected(
			    err, ")", tbuf, p->l->stream->name, p->l->line);
			return;
		}
		subtilis_lexer_get(p->l, t, err);
		return;
	}

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_reset_field(p, type, mem_reg, loc, e, err);
}

static void prv_rec_init(subtilis_parser_t *p, subtilis_token_t *t,
			 const subtilis_type_t *type, size_t mem_reg,
			 size_t loc, bool push, subtilis_error_t *err)
{
	const char *tbuf;
	size_t id;
	const subtilis_type_rec_t *rec;
	const subtilis_type_t *field_type;
	uint32_t offset;
	subtilis_exp_t *e;

	tbuf = subtilis_token_get_text(t);
	if (!((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "("))) {
		/*
		 * We're copying one REC into another.  There's no
		 * initialisation list here.
		 */
		e = subtilis_parser_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		e = subtilis_type_if_coerce_type(p, e, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_rec_type_copy(p, type, mem_reg, loc, e->exp.ir_op.reg,
				       true, err);
		subtilis_exp_delete(e);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (push && subtilis_type_rec_need_deref(type))
			subtilis_reference_type_push_reference(p, type, mem_reg,
							       loc, err);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")")) {
		subtilis_rec_type_zero(p, type, mem_reg, loc, push, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_lexer_get(p->l, t, err);
		return;
	}

	rec = &type->params.rec;

	/*
	 * TODO: All of this will need to be re-written as will the existing
	 * code to initialise arrays.  Basically, we want parse the expressions
	 * build up some sort of tree, and then anaylse the tree to figure out
	 * the most efficient way of initialising things.  This sort of thing
	 * will be easier though when have a two stage parser.  Othwerwise,
	 * we'll end up loading using too many regs.  We could bomb out if
	 * we hit a variable.  Can I do this withouth the re-write?
	 */

	for (id = 0; id < rec->num_fields; id++) {
		field_type = &rec->field_types[id];
		offset = subtilis_type_rec_field_offset_id(rec, id);
		if (subtilis_type_if_is_numeric(field_type) ||
		    field_type->type == SUBTILIS_TYPE_FN) {
			e = subtilis_parser_priority7(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			e = subtilis_type_if_coerce_type(p, e, field_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_to_mem(p, mem_reg, loc + offset,
						       e, err);
		} else if (field_type->type == SUBTILIS_TYPE_STRING) {
			e = subtilis_parser_priority7(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_string_type_new_field_ref(
			    p, field_type, mem_reg, loc + offset, e, err);
		} else if (subtilis_type_if_is_array(field_type) ||
			   subtilis_type_if_is_vector(field_type)) {
			prv_array_field_init(p, t, &rec->fields[id], field_type,
					     mem_reg, loc + offset, err);
		} else if (field_type->type == SUBTILIS_TYPE_REC) {
			prv_rec_init(p, t, field_type, mem_reg, loc + offset,
				     false, err);
		} else {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_OPERATOR) {
			if (!strcmp(tbuf, ",")) {
				if (id < rec->num_fields - 1) {
					subtilis_lexer_get(p->l, t, err);
					if (err->type != SUBTILIS_ERROR_OK)
						return;
				}
				continue;
			}
			if (!strcmp(tbuf, ")"))
				break;
		}
		subtilis_error_set_expected(err, ", or )", tbuf,
					    p->l->stream->name, p->l->line);
		return;
	}

	if (strcmp(tbuf, ")")) {
		subtilis_error_set_expected(err, ")", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	for (id = id + 1; id < rec->num_fields; id++) {
		field_type = &rec->field_types[id];
		offset = subtilis_type_rec_field_offset_id(rec, id);
		subtilis_rec_type_init_field(p, field_type, &rec->fields[id],
					     mem_reg, offset + loc, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (push && subtilis_type_rec_need_deref(type)) {
		subtilis_reference_type_push_reference(p, type, mem_reg, loc,
						       err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

static void prv_rec_init_check_tmp(subtilis_parser_t *p, subtilis_token_t *t,
				   const subtilis_type_t *type,
				   const char *var_name, bool push,
				   subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_coerce_type(p, e, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (e->temporary) {
		(void)subtilis_symbol_table_promote_tmp(
		    p->local_st, type, e->temporary, var_name, err);
	} else {
		s = subtilis_symbol_table_insert(p->local_st, var_name, type,
						 err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_rec_type_copy(p, type, SUBTILIS_IR_REG_LOCAL, s->loc,
				       e->exp.ir_op.reg, true, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (push && subtilis_type_rec_need_deref(type))
			subtilis_reference_type_push_reference(
			    p, type, SUBTILIS_IR_REG_LOCAL, s->loc, err);
	}

cleanup:
	subtilis_exp_delete(e);
}

void subtilis_parser_rec_init(subtilis_parser_t *p, subtilis_token_t *t,
			      const subtilis_type_t *type, const char *var_name,
			      bool local, bool push, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Special case here.  If we're creating a new local variable from a
	 * temporary REC returned from a function we can just promote the
	 * temporary and avoid the copy.
	 */

	tbuf = subtilis_token_get_text(t);
	if (local &&
	    !((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "("))) {
		prv_rec_init_check_tmp(p, t, type, var_name, push, err);
		return;
	}
	s = subtilis_symbol_table_insert(local ? p->local_st : p->st, var_name,
					 type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_rec_init(p, t, type,
		     local ? SUBTILIS_IR_REG_LOCAL : SUBTILIS_IR_REG_GLOBAL,
		     s->loc, push, err);
}

static void prv_rec_reset(subtilis_parser_t *p, subtilis_token_t *t,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_error_t *err)
{
	const char *tbuf;
	size_t id;
	const subtilis_type_rec_t *rec;
	const subtilis_type_t *field_type;
	uint32_t offset;
	subtilis_exp_t *e;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")")) {
		subtilis_rec_type_deref(p, type, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_rec_type_zero(p, type, mem_reg, loc, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_lexer_get(p->l, t, err);
		return;
	}

	rec = &type->params.rec;

	for (id = 0; id < rec->num_fields; id++) {
		field_type = &rec->field_types[id];
		offset = subtilis_type_rec_field_offset_id(rec, id);
		if (subtilis_type_if_is_numeric(field_type) ||
		    field_type->type == SUBTILIS_TYPE_FN) {
			e = subtilis_parser_priority7(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			e = subtilis_type_if_coerce_type(p, e, field_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_to_mem(p, mem_reg, loc + offset,
						       e, err);
		} else if (field_type->type == SUBTILIS_TYPE_STRING) {
			e = subtilis_parser_priority7(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_to_mem(p, mem_reg, loc + offset,
						       e, err);
		} else if (subtilis_type_if_is_array(field_type) ||
			   subtilis_type_if_is_vector(field_type)) {
			prv_array_field_reset(p, t, &rec->fields[id],
					      field_type, mem_reg, loc + offset,
					      err);
		} else if (field_type->type == SUBTILIS_TYPE_REC) {
			tbuf = subtilis_token_get_text(t);
			if ((t->type != SUBTILIS_TOKEN_OPERATOR) ||
			    strcmp(tbuf, "(")) {
				subtilis_error_set_expected(err, "(", tbuf,
							    p->l->stream->name,
							    p->l->line);
				return;
			}
			subtilis_lexer_get(p->l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;

			prv_rec_reset(p, t, field_type, mem_reg, loc + offset,
				      err);
		} else {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_OPERATOR) {
			if (!strcmp(tbuf, ",")) {
				if (id < rec->num_fields - 1) {
					subtilis_lexer_get(p->l, t, err);
					if (err->type != SUBTILIS_ERROR_OK)
						return;
				}
				continue;
			}
			if (!strcmp(tbuf, ")"))
				break;
		}
		subtilis_error_set_expected(err, ", or )", tbuf,
					    p->l->stream->name, p->l->line);
		return;
	}

	if (strcmp(tbuf, ")")) {
		subtilis_error_set_expected(err, ")", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	/*
	 * I could add a type_if interface for this, a reset method.
	 * The main motivation would be to avoid de-allocing and re-allocing
	 * arrays, or vectors when their size hasn't changed, that have a
	 * single owner.  We'd have to do a ref count check though and since
	 * we know the heap will have a block of the correct size, allocation
	 * should be fast, so it's not worth the effort.
	 */

	for (id = id + 1; id < rec->num_fields; id++) {
		field_type = &rec->field_types[id];
		offset = subtilis_type_rec_field_offset_id(rec, id);
		if (subtilis_type_if_is_reference(field_type)) {
			subtilis_reference_type_deref(p, mem_reg, offset + loc,
						      err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (field_type->type == SUBTILIS_TYPE_REC) {
			subtilis_rec_type_deref(p, field_type, mem_reg,
						offset + loc, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		subtilis_rec_type_init_field(p, field_type, &rec->fields[id],
					     mem_reg, offset + loc, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_lexer_get(p->l, t, err);
}

void subtilis_parser_rec_reset(subtilis_parser_t *p, subtilis_token_t *t,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_rec_reset(p, t, type, mem_reg, loc, err);
}
