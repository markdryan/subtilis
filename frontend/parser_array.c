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

static uint8_t *prv_append_const_el(subtilis_parser_t *p, subtilis_exp_t *e,
				    subtilis_type_t *el_type,
				    size_t element_size, uint8_t *buffer,
				    size_t el, size_t *max_els,
				    subtilis_error_t *err)
{
	uint8_t *new_buf;
	size_t new_max;

	if (el == *max_els) {
		new_max = *max_els + SUBTILIS_CONFIG_CONSTANT_ARRAY_GRAN;
		new_buf = realloc(buffer, new_max * element_size);
		if (!new_buf) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		buffer = new_buf;
		*max_els = new_max;
	}

	if (!subtilis_type_if_is_const(&e->type)) {
		subtilis_error_set_const_expression_expected(
		    err, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_type_if_coerce_type(p, e, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * This is only going to work for scalar types.  How do we
	 * handle strings?
	 */

	memcpy(&buffer[el * element_size], &e->exp.ir_op, element_size);

cleanup:

	subtilis_exp_delete(e);

	return buffer;
}

static void prv_check_initialiser_count(subtilis_parser_t *p, size_t entries,
					size_t mem_reg,
					const subtilis_symbol_t *s,
					subtilis_error_t *err)
{
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *maxe = NULL;
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;

	error_label = subtilis_array_type_error_label(p);
	ok_label.label = subtilis_ir_section_new_label(p->current);

	sizee = subtilis_exp_new_int32(entries, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	maxe = subtilis_array_type_dynamic_size(p, mem_reg, s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_lte(p, sizee, maxe, err);
	maxe = NULL;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  sizee->exp.ir_op, ok_label,
					  error_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, ok_label.label, err);

cleanup:
	subtilis_exp_delete(maxe);
	subtilis_exp_delete(sizee);
}

static void prv_parse_initialiser(subtilis_parser_t *p, subtilis_token_t *t,
				  subtilis_exp_t *e, size_t mem_reg,
				  const subtilis_symbol_t *s,
				  subtilis_error_t *err)
{
	const char *tbuf;
	size_t id;
	int32_t dim;
	subtilis_exp_t *sizee = NULL;
	const subtilis_type_t *type = &s->t;
	bool dynamic = false;
	size_t entries = 1;
	subtilis_type_t el_type;
	bool el_type_dbl;
	size_t max_elements = 1;
	size_t element_size;
	size_t reg;
	subtilis_ir_operand_t op1;
	uint8_t *buffer = NULL;
	size_t i = 0;

	e = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	do {
		dim = type->params.array.dims[i];
		if (dim == SUBTILIS_DYNAMIC_DIMENSION) {
			max_elements = SUBTILIS_CONFIG_CONSTANT_ARRAY_GRAN;
			dynamic = true;
			break;
		}
		max_elements *= (dim + 1);
		i++;
	} while (i < type->params.array.num_dims);

	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_const_of(&el_type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	element_size = subtilis_type_if_size(&el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	buffer = malloc(max_elements * element_size);
	if (!buffer) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	buffer = prv_append_const_el(p, e, &el_type, element_size, buffer, 0,
				     &max_elements, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);

	while ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		if (!dynamic && (entries >= max_elements)) {
			subtilis_error_bad_element_count(
			    err, p->l->stream->name, p->l->line);
			goto cleanup;
		}

		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		buffer =
		    prv_append_const_el(p, e, &el_type, element_size, buffer,
					entries, &max_elements, err);
		e = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		entries++;

		tbuf = subtilis_token_get_text(t);
	}

	if (dynamic) {
		prv_check_initialiser_count(p, entries * element_size, mem_reg,
					    s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	el_type_dbl = el_type.type == SUBTILIS_TYPE_CONST_REAL;
	id = subtilis_constant_pool_add(p->prog->constant_pool, buffer,
					entries * element_size, el_type_dbl,
					err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	buffer = NULL;

	sizee = subtilis_exp_new_int32(entries * element_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = (int32_t)id;
	reg = subtilis_ir_section_add_instr2(p->current, SUBTILIS_OP_INSTR_LCA,
					     op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_memcpy(p, mem_reg, s->loc, reg,
				   sizee->exp.ir_op.reg, err);

cleanup:
	subtilis_exp_delete(sizee);
	subtilis_exp_delete(e);
	free(buffer);
}

void subtilis_parser_array_assign_reference(subtilis_parser_t *p,
					    subtilis_token_t *t, size_t mem_reg,
					    const subtilis_symbol_t *s,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	if (subtilis_type_eq(&s->t, &e->type))
		subtilis_array_type_assign_ref(p, &s->t.params.array, mem_reg,
					       s->loc, e->exp.ir_op.reg, err);
	else if (subtilis_type_if_is_numeric(&e->type))
		prv_parse_initialiser(p, t, e, mem_reg, s, err);
	else
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);

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
