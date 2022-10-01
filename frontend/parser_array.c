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
#include "parser_call.h"
#include "parser_exp.h"
#include "parser_rec.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

subtilis_exp_t *subtils_parser_read_array(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  const char *var_name,
					  subtilis_error_t *err)
{
	size_t mem_reg;
	const subtilis_symbol_t *s;

	s = subtilis_parser_get_symbol(p, var_name, &mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (!subtilis_type_if_is_array(&s->t)) {
		subtilis_error_not_array(err, var_name, p->l->stream->name,
					 p->l->line);
		return NULL;
	}

	return subtils_parser_read_array_with_type(p, t, &s->t, mem_reg, s->loc,
						   var_name, err);
}

static subtilis_exp_t *
prv_read_rec_el(subtilis_parser_t *p, subtilis_token_t *t,
		const subtilis_type_t *type, subtilis_exp_t **indices,
		size_t dims, size_t mem_reg, size_t loc, const char *var_name,
		subtilis_error_t *err)
{
	subtilis_type_t el_type;
	subtilis_exp_t *offset;
	subtilis_exp_t *e;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	el_type.type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(offset);
		return NULL;
	}

	e = subtilis_parser_rec_exp(p, t, &el_type, offset->exp.ir_op.reg, 0,
				    err);
	subtilis_exp_delete(offset);
	subtilis_type_free(&el_type);

	return e;
}

/* clang-format off */
subtilis_exp_t *subtils_parser_read_array_with_type(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    const subtilis_type_t *type,
						    size_t mem_reg, size_t loc,
						    const char *var_name,
						    subtilis_error_t *err)
/* clang-format on */

{
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	bool rec;
	const char *tbuf;
	subtilis_exp_t *e = NULL;
	size_t dims = 0;

	if (type->params.array.num_dims == 1)
		dims =
		    subtilis_round_bracketed_slice_have_b(p, t, indices, err);
	else
		dims = subtilis_var_bracketed_int_args_have_b(
		    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	rec = (t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ".");

	if (rec && (type->type != SUBTILIS_TYPE_ARRAY_REC)) {
		subtilis_error_set_expected(err, "Collection of RECs",
					    subtilis_type_name(type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (dims == 0) {
		if (rec) {
			subtilis_error_set_expected(
			    err, "Array of RECs", "Array of RECs reference",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}
		/* What we have here is an array reference. */
		e = subtilis_exp_new_var_block(p, type, mem_reg, loc, err);
	} else if ((type->params.array.num_dims == 1) && (dims == 2)) {
		if (rec) {
			subtilis_error_set_expected(
			    err, "Array of RECs", "Slice of RECs",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}
		/* We have a slice. */
		e = subtilis_array_type_slice_array(
		    p, type, mem_reg, loc, indices[0], indices[1], err);
	} else {
		if (rec)
			e = prv_read_rec_el(p, t, type, &indices[0], dims,
					    mem_reg, loc, var_name, err);
		else
			e = subtilis_type_if_indexed_read(p, var_name, type,
							  mem_reg, loc, indices,
							  dims, err);
	}

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return e;
}

size_t subtils_parser_array_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
				   const subtilis_symbol_t *s, size_t mem_reg,
				   subtilis_type_t *type, subtilis_error_t *err)
{
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_exp_t *e;
	size_t dims;
	const char *tbuf;
	size_t ret_val = SIZE_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "("))) {
		subtilis_error_set_expected(err, "(", tbuf, p->l->stream->name,
					    p->l->line);
		return SIZE_MAX;
	}

	if (s->t.params.array.num_dims == 1)
		dims =
		    subtilis_round_bracketed_slice_have_b(p, t, indices, err);
	else
		dims = subtilis_var_bracketed_int_args_have_b(
		    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (dims == 0) {
		/* What we have here is an array reference. */

		subtilis_type_copy(type, &s->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		ret_val =
		    subtilis_reference_get_pointer(p, mem_reg, s->loc, err);
		goto cleanup;
	} else if ((s->t.params.array.num_dims == 1) && (dims == 2)) {
		subtilis_error_set_lvalue_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	/*
	 * Otherwise we have an array element.  We need to compute its
	 * address.
	 */

	subtilis_type_if_element_type(p, &s->t, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_array_index_calc(p, s->key, &s->t, mem_reg, s->loc,
				      indices, dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ret_val = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return ret_val;
}

subtilis_exp_t *subtils_parser_read_vector(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   const char *var_name,
					   subtilis_error_t *err)
{
	size_t mem_reg;
	const subtilis_symbol_t *s;

	s = subtilis_parser_get_symbol(p, var_name, &mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (!subtilis_type_if_is_vector(&s->t)) {
		subtilis_error_not_vector(err, var_name, p->l->stream->name,
					  p->l->line);
		return NULL;
	}

	return subtils_parser_read_vector_with_type(p, t, &s->t, mem_reg,
						    s->loc, var_name, err);
}

/* clang-format off */
subtilis_exp_t *subtils_parser_read_vector_with_type(subtilis_parser_t *p,
						     subtilis_token_t *t,
						     const subtilis_type_t *typ,
						     size_t mem_reg, size_t loc,
						     const char *var_name,
						     subtilis_error_t *err)
/* clang-format on */

{
	size_t indices_count;
	bool rec;
	const char *tbuf;
	subtilis_exp_t *indices[2] = {NULL, NULL};
	subtilis_exp_t *e = NULL;

	indices_count =
	    subtilis_curly_bracketed_slice_have_b(p, t, indices, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	rec = (t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ".");
	if (rec && (typ->type != SUBTILIS_TYPE_VECTOR_REC)) {
		subtilis_error_set_expected(err, "Collection of RECs",
					    subtilis_type_name(typ),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (indices_count == 0) {
		if (rec) {
			subtilis_error_set_expected(
			    err, "Vector of RECs", "Vector of RECs  reference",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}

		/* What we have here is an array reference. */
		e = subtilis_exp_new_var_block(p, typ, mem_reg, loc, err);
	} else if (indices_count == 1) {
		/* And here we're reading an individual entry. */
		if (rec)
			e = prv_read_rec_el(p, t, typ, &indices[0], 1, mem_reg,
					    loc, var_name, err);
		else
			e = subtilis_type_if_indexed_read(
			    p, var_name, typ, mem_reg, loc, indices, 1, err);
	} else {
		if (rec) {
			subtilis_error_set_expected(
			    err, "Vector of RECs", "Slice of RECs",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}

		/* We have a slice */
		e = subtilis_array_type_slice_vector(
		    p, typ, mem_reg, loc, indices[0], indices[1], err);
	}

cleanup:

	subtilis_exp_delete(indices[0]);
	subtilis_exp_delete(indices[1]);

	return e;
}

size_t subtils_parser_vector_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
				    const subtilis_symbol_t *s, size_t mem_reg,
				    subtilis_type_t *type,
				    subtilis_error_t *err)
{
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_exp_t *e;
	size_t dims;
	const char *tbuf;
	size_t ret_val = SIZE_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "{"))) {
		subtilis_error_set_expected(err, "{", tbuf, p->l->stream->name,
					    p->l->line);
		return SIZE_MAX;
	}

	dims = subtilis_curly_bracketed_slice_have_b(p, t, indices, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (dims > 1) {
		/* It's a slice which is not allowed. */

		subtilis_error_set_lvalue_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	if (dims == 0) {
		/* What we have here is a vector reference. */

		subtilis_type_copy(type, &s->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		ret_val =
		    subtilis_reference_get_pointer(p, mem_reg, s->loc, err);
		goto cleanup;
	}

	/*
	 * Otherwise we have an array element.  We need to compute its
	 * address.
	 */

	subtilis_type_if_element_type(p, &s->t, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_array_index_calc(p, s->key, &s->t, mem_reg, s->loc,
				      indices, dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ret_val = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return ret_val;
}

subtilis_exp_t *
subtils_parser_element_address(subtilis_parser_t *p, subtilis_token_t *t,
			       const subtilis_symbol_t *s, size_t mem_reg,
			       const char *var_name, subtilis_error_t *err)
{
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_exp_t *e = NULL;
	size_t dims = 0;

	dims = subtilis_var_bracketed_int_args_have_b(
	    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (dims == 0) {
		subtilis_error_bad_index_count(err, var_name,
					       p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_type_if_indexed_address(p, var_name, &s->t, mem_reg,
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

size_t parser_array_var_bracketed_int_args(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_exp_t **e, size_t max,
					   bool *vec, subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	tbuf = subtilis_token_get_text(t);
	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		if (!strcmp(tbuf, "(")) {
			*vec = false;
			return subtilis_var_bracketed_int_args_have_b(p, t, e,
								      max, err);
		} else if (!strcmp(tbuf, "{")) {
			e[0] = subtilis_curly_bracketed_arg_have_b(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return 0;

			*vec = true;
			return e[0] ? 1 : 0;
		}
	}

	subtilis_error_set_expected(err, "( or {", tbuf, p->l->stream->name,
				    p->l->line);

	return 0;
}

static uint8_t *prv_append_const_el(subtilis_parser_t *p, subtilis_exp_t *e,
				    const subtilis_type_t *el_type,
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

	memcpy(&buffer[el * element_size], &e->exp.ir_op, element_size);

cleanup:

	subtilis_exp_delete(e);

	return buffer;
}

static void prv_check_initialiser_count(subtilis_parser_t *p, size_t entries,
					size_t mem_reg, subtilis_exp_t *maxe,
					subtilis_error_t *err)
{
	subtilis_exp_t *sizee = NULL;
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;

	error_label = subtilis_array_type_error_label(p);
	ok_label.label = subtilis_ir_section_new_label(p->current);

	sizee = subtilis_exp_new_int32(entries, err);
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

static void prv_parse_numeric_initialiser(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_exp_t *e,
					  const subtilis_array_desc_t *d,
					  subtilis_error_t *err)
{
	const char *tbuf;
	size_t id;
	int32_t dim;
	subtilis_exp_t *sizee = NULL;
	const subtilis_type_t *type = d->t;
	bool dynamic = false;
	size_t entries = 1;
	subtilis_type_t el_type;
	subtilis_type_t const_el_type;
	bool el_type_dbl;
	size_t max_elements = 1;
	size_t element_size;
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_exp_t *maxe;
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
		goto free_e;

	subtilis_type_if_const_of(&el_type, &const_el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_el_type;

	element_size = subtilis_type_if_size(&const_el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	buffer = malloc(max_elements * element_size);
	if (!buffer) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	buffer = prv_append_const_el(p, e, &const_el_type, element_size, buffer,
				     0, &max_elements, err);
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
		    prv_append_const_el(p, e, &const_el_type, element_size,
					buffer, entries, &max_elements, err);
		e = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		entries++;

		tbuf = subtilis_token_get_text(t);
	}

	if (dynamic) {
		maxe = subtilis_array_type_dynamic_size(p, d->reg, d->loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		prv_check_initialiser_count(p, entries * element_size, d->reg,
					    maxe, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	el_type_dbl = const_el_type.type == SUBTILIS_TYPE_CONST_REAL;
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

	subtilis_reference_type_memcpy(p, d->reg, d->loc, reg,
				       sizee->exp.ir_op.reg, err);

free_e:
	subtilis_exp_delete(e);

free_el_type:
	subtilis_type_free(&el_type);

cleanup:
	subtilis_type_free(&const_el_type);
	subtilis_exp_delete(sizee);
	free(buffer);
}

typedef void (*subtilis_arr_ct_t)(subtilis_parser_t *p, subtilis_exp_t *e1,
				  const subtilis_type_t *type,
				  subtilis_error_t *err);

static subtilis_exp_t **
prv_parse_expression_list(subtilis_parser_t *p, subtilis_exp_t *e,
			  const subtilis_type_t *el_type, subtilis_token_t *t,
			  size_t *len, subtilis_arr_ct_t check_type,
			  subtilis_error_t *err)
{
	size_t i;
	const char *tbuf;
	subtilis_exp_t **ee;
	subtilis_exp_t **ee_tmp;
	size_t new_max;
	size_t max = SUBTILIS_CONFIG_CONSTANT_ARRAY_GRAN;
	size_t count = 0;

	ee = malloc(sizeof(*ee) * max);
	if (!ee) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	ee[count++] = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	e = NULL;

	tbuf = subtilis_token_get_text(t);
	while ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		if (count == max) {
			new_max = max + SUBTILIS_CONFIG_CONSTANT_ARRAY_GRAN;
			ee_tmp = realloc(ee, new_max * sizeof(*ee));
			if (!*ee_tmp) {
				subtilis_error_set_oom(err);
				goto cleanup;
			}
			ee = ee_tmp;
			max = new_max;
		}
		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		check_type(p, e, el_type, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(e);
			goto cleanup;
		}

		ee[count++] = e;
		e = NULL;
		tbuf = subtilis_token_get_text(t);
	}

	*len = count;
	return ee;

cleanup:

	for (i = 0; i < count; i++)
		subtilis_exp_delete(ee[i]);
	free(ee);

	return NULL;
}

static size_t prv_init_lca_table(subtilis_parser_t *p, subtilis_exp_t **ee,
				 size_t entries, subtilis_error_t *err)
{
	size_t i;
	size_t buf_size;
	size_t table_base;
	const subtilis_symbol_t *s;
	size_t local_block_size;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	const subtilis_buffer_t *str;

	/*
	 * TODO: pointer size is hardcoded to 4
	 */

	local_block_size = entries * sizeof(int32_t) * 2;
	s = subtilis_symbol_table_create_local_buf(p->local_st,
						   local_block_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op0.reg = SUBTILIS_IR_REG_LOCAL;
	op1.integer = s->loc;
	table_base = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	for (i = 0; i < entries; i++) {
		str = &ee[i]->exp.str;
		buf_size = subtilis_buffer_get_size(str) - 1;
		op1.integer = buf_size;
		op0.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
		op1.reg = table_base;
		op2.integer = i * sizeof(int32_t) * 2;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_STOREO_I32,
						  op0, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;

		if (buf_size == 0)
			continue;

		op0.reg = subtilis_string_type_lca_const(
		    p, subtilis_buffer_get_string(str), buf_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;

		op1.reg = table_base;
		op2.integer = (i * sizeof(int32_t) * 2) + sizeof(int32_t);
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_STOREO_I32,
						  op0, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
	}

	return table_base;
}

static void prv_read_string_table(subtilis_parser_t *p, size_t array_base,
				  size_t table_base, size_t entries,
				  size_t element_size, subtilis_error_t *err)
{
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t end;
	subtilis_ir_operand_t zero_len;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t table_end;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t skip;
	subtilis_ir_operand_t lca_offset;
	subtilis_ir_operand_t table_ptr;
	subtilis_ir_operand_t array_ptr;

	start.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);
	zero_len.label = subtilis_ir_section_new_label(p->current);
	gt_zero.label = subtilis_ir_section_new_label(p->current);
	skip.label = subtilis_ir_section_new_label(p->current);

	table_ptr.reg = table_base;
	array_ptr.reg = array_base;

	op1.integer = entries * sizeof(int32_t) * 2;
	table_end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, table_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, table_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, gt_zero, zero_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, gt_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = sizeof(int32_t);
	lca_offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, table_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_deref(p, array_ptr.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_string_init_from_lca(p, array_ptr.reg, 0, lca_offset.reg,
				      size.reg, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     skip, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, zero_len.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_set_size(p, array_ptr.reg, 0, size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, skip.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = (int32_t)element_size;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADDI_I32, array_ptr,
					  array_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = sizeof(int32_t) * 2;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADDI_I32, table_ptr,
					  table_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cond.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, table_ptr, table_end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, start, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end.label, err);
}

static void prv_check_expression_list_size(subtilis_parser_t *p,
					   const subtilis_array_desc_t *d,
					   subtilis_exp_t **ee, size_t entries,
					   size_t element_size,
					   subtilis_error_t *err)
{
	int32_t dim;
	bool dynamic = false;
	size_t max_elements = 1;
	subtilis_exp_t *maxe = NULL;
	subtilis_exp_t *maxe_dup = NULL;
	size_t i = 0;
	const subtilis_type_t *type = d->t;

	do {
		dim = type->params.array.dims[i];
		if (dim == SUBTILIS_DYNAMIC_DIMENSION) {
			max_elements = SIZE_MAX;
			dynamic = true;
			break;
		}
		max_elements *= (dim + 1);
		i++;
	} while (i < type->params.array.num_dims);

	if (!dynamic) {
		if (entries > max_elements)
			subtilis_error_bad_element_count(
			    err, p->l->stream->name, p->l->line);
	} else {
		maxe = subtilis_array_type_dynamic_size(p, d->reg, d->loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		maxe_dup = subtilis_type_if_dup(maxe, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		prv_check_initialiser_count(p, entries * element_size, d->reg,
					    maxe_dup, err);
	}

cleanup:

	subtilis_exp_delete(maxe);
}

static void prv_string_check_type(subtilis_parser_t *p, subtilis_exp_t *e1,
				  const subtilis_type_t *type,
				  subtilis_error_t *err)
{
	if (!subtilis_type_eq(&e1->type, type))
		subtilis_error_set_const_string_expected(
		    err, p->l->stream->name, p->l->line);
}

static void prv_parse_string_initialiser(subtilis_parser_t *p,
					 subtilis_token_t *t, subtilis_exp_t *e,
					 const subtilis_array_desc_t *d,
					 subtilis_error_t *err)
{
	size_t data_reg;
	subtilis_exp_t **ee;
	size_t table_base;
	size_t element_size;
	size_t entries = 0;
	size_t i = 0;

	ee = prv_parse_expression_list(p, e, &subtilis_type_const_string, t,
				       &entries, prv_string_check_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	element_size = subtilis_type_if_size(&subtilis_type_string, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_check_expression_list_size(p, d, ee, entries, element_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	table_base = prv_init_lca_table(p, ee, entries, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_get_data(p, d->reg, d->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_read_string_table(p, data_reg, table_base, entries, element_size,
			      err);

cleanup:

	for (i = 0; i < entries; i++)
		subtilis_exp_delete(ee[i]);
	free(ee);
}

static void prv_fn_check_type(subtilis_parser_t *p, subtilis_exp_t *e1,
			      const subtilis_type_t *type,
			      subtilis_error_t *err)
{
	if (e1->partial_name)
		subtilis_parser_call_add_addr(p, type, e1, err);
	else if (!subtilis_type_eq(&e1->type, type))
		subtilis_error_set_fn_type_mismatch(err, p->l->stream->name,
						    p->l->line);
}

static void prv_parse_fn_list(subtilis_parser_t *p, size_t array_base,
			      subtilis_exp_t **ee, size_t entries,
			      size_t element_size, subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op1.reg = array_base;
	op2.integer = (int32_t)element_size;

	for (i = 0; i < entries - 1; i++) {
		subtilis_type_if_assign_to_mem(p, op1.reg, 0, ee[i], err);
		ee[i] = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op1.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
	subtilis_type_if_assign_to_mem(p, op1.reg, 0, ee[i], err);
	ee[i] = NULL;
}

static void prv_parse_fn_initialiser(subtilis_parser_t *p, subtilis_token_t *t,
				     const subtilis_type_t *el_type,
				     subtilis_exp_t *e,
				     const subtilis_array_desc_t *d,
				     subtilis_error_t *err)
{
	subtilis_exp_t **ee;
	size_t element_size;
	size_t data_reg;
	size_t entries = 0;
	size_t i = 0;

	ee = prv_parse_expression_list(p, e, el_type, t, &entries,
				       prv_fn_check_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	element_size = subtilis_type_if_size(&e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_check_expression_list_size(p, d, ee, entries, element_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_get_data(p, d->reg, d->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_parse_fn_list(p, data_reg, ee, entries, element_size, err);

cleanup:

	for (i = 0; i < entries; i++)
		subtilis_exp_delete(ee[i]);
	free(ee);
}

void subtilis_parser_array_init_list(subtilis_parser_t *p, subtilis_token_t *t,
				     const subtilis_array_desc_t *d,
				     subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_type_t el_type;
	bool single_val;
	const char *tbuf;

	el_type.type = SUBTILIS_TYPE_VOID;

	subtilis_type_if_element_type(p, d->t, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	tbuf = subtilis_token_get_text(t);
	single_val = (t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",");
	if (subtilis_type_if_is_numeric(&el_type) &&
	    subtilis_type_if_is_numeric(&e->type)) {
		if (single_val) {
			subtilis_type_if_array_set(p, d->name, d->t, d->reg,
						   d->loc, e, err);
			e = NULL;
		} else {
			prv_parse_numeric_initialiser(p, t, e, d, err);
		}
	} else if (((d->t->type == SUBTILIS_TYPE_ARRAY_STRING) ||
		    (d->t->type == SUBTILIS_TYPE_VECTOR_STRING)) &&
		   ((e->type.type == SUBTILIS_TYPE_CONST_STRING) ||
		    (e->type.type == SUBTILIS_TYPE_STRING))) {
		if (single_val) {
			subtilis_type_if_array_set(p, d->name, d->t, d->reg,
						   d->loc, e, err);
			e = NULL;
		} else if (e->type.type == SUBTILIS_TYPE_CONST_STRING) {
			prv_parse_string_initialiser(p, t, e, d, err);
		} else {
			subtilis_error_set_const_string_expected(
			    err, p->l->stream->name, p->l->line);
		}
	} else if ((d->t->type == SUBTILIS_TYPE_ARRAY_FN) ||
		   (d->t->type == SUBTILIS_TYPE_VECTOR_FN)) {
		prv_fn_check_type(p, e, &el_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (single_val) {
			subtilis_type_if_array_set(p, d->name, d->t, d->reg,
						   d->loc, e, err);
			e = NULL;
		} else {
			prv_parse_fn_initialiser(p, t, &el_type, e, d, err);
		}
	} else {
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);
	}
cleanup:
	subtilis_type_free(&el_type);
	subtilis_exp_delete(e);
}

void subtilis_parser_array_assign_reference(subtilis_parser_t *p,
					    subtilis_token_t *t,
					    const subtilis_array_desc_t *d,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	if (subtilis_type_eq(d->t, &e->type))
		subtilis_type_if_assign_ref(p, d->t, d->reg, d->loc, e, err);
	else
		subtilis_parser_array_init_list(p, t, d, e, err);
}

static void prv_create_vector(subtilis_parser_t *p,
			      subtilis_ir_operand_t local_global,
			      const subtilis_type_t *element_type, size_t *dims,
			      subtilis_exp_t **e, const char *var_name,
			      bool local, subtilis_error_t *err)
{
	subtilis_type_t type;
	const subtilis_symbol_t *s;

	subtilis_array_type_vector_init(element_type, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * No dimensions means a vector with no
	 * elements.
	 */

	if (*dims == 0) {
		e[0] = subtilis_exp_new_int32(-1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		*dims = 1;
	}

	s = subtilis_symbol_table_insert(local ? p->local_st : p->st, var_name,
					 &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_array_type_vector_alloc(p, s->loc, &type, e[0], local_global,
					 true, err);

cleanup:
	subtilis_type_free(&type);
}

static void prv_create_array(subtilis_parser_t *p,
			     subtilis_ir_operand_t local_global,
			     const subtilis_type_t *element_type, size_t dims,
			     subtilis_exp_t **e, const char *var_name,
			     bool local, subtilis_error_t *err)
{
	subtilis_type_t type;
	const subtilis_symbol_t *s;

	if (dims == 0) {
		subtilis_error_set_integer_exp_expected(err, p->l->stream->name,
							p->l->line);
		return;
	}

	type.type = SUBTILIS_TYPE_VOID;
	subtilis_array_type_init(p, var_name, element_type, &type, &e[0], dims,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	s = subtilis_symbol_table_insert(local ? p->local_st : p->st, var_name,
					 &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_array_type_allocate(p, var_name, &type, s->loc, &e[0],
				     local_global, true, err);

cleanup:
	subtilis_type_free(&type);
}

void subtilis_parser_create_array(subtilis_parser_t *p, subtilis_token_t *t,
				  bool local, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e[SUBTILIS_MAX_DIMENSIONS];
	subtilis_type_t element_type;

	size_t i;
	subtilis_ir_operand_t local_global;
	bool vec = false;
	size_t dims = 0;
	char *var_name = NULL;

	local_global.reg =
	    local ? SUBTILIS_IR_REG_LOCAL : SUBTILIS_IR_REG_GLOBAL;

	do {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return;
		}

		if (!local && p->current->proc_called) {
			subtilis_error_set_global_after_proc(
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
		subtilis_type_init_copy(&element_type, &t->tok.id_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if ((element_type.type == SUBTILIS_TYPE_FN) ||
		    (element_type.type == SUBTILIS_TYPE_REC)) {
			subtilis_complete_custom_type(p, var_name,
						      &element_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}

		dims = parser_array_var_bracketed_int_args(
		    p, t, e, SUBTILIS_MAX_DIMENSIONS, &vec, err);
		if (err->type == SUBTILIS_ERROR_RIGHT_BKT_EXPECTED) {
			subtilis_error_too_many_dims(
			    err, var_name, p->l->stream->name, p->l->line);
			subtilis_type_free(&element_type);
			goto cleanup;
		}
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_type_free(&element_type);
			goto cleanup;
		}

		if (!local && (p->level != 0)) {
			subtilis_error_variable_bad_level(
			    err, var_name, p->l->stream->name, p->l->line);
			subtilis_type_free(&element_type);
			goto cleanup;
		}

		if (vec)
			prv_create_vector(p, local_global, &element_type, &dims,
					  e, var_name, local, err);
		else
			prv_create_array(p, local_global, &element_type, dims,
					 e, var_name, local, err);
		subtilis_type_free(&element_type);
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

subtilis_exp_t *subtilis_parser_get_dim(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *dim = NULL;
	subtilis_exp_t *ar = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "("))) {
		subtilis_error_set_expected(err, "(", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	ar = subtilis_parser_expression(p, t, err);
	if (err->type == SUBTILIS_ERROR_NUMERIC_EXPECTED) {
		subtilis_error_not_array(err, "First argument to dim",
					 p->l->stream->name, p->l->line);
		goto cleanup;
	} else if (err->type != SUBTILIS_ERROR_OK) {
		goto cleanup;
	}

	if (!subtilis_type_if_is_array(&ar->type) &&
	    !subtilis_type_if_is_vector(&ar->type)) {
		subtilis_error_not_array(err, "First argument to dim",
					 p->l->stream->name, p->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_expected(err, ") or ,", tbuf,
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, ",")) {
		dim = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		dim = subtilis_type_if_to_int(p, dim, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(t);
	}

	if (strcmp(tbuf, ")")) {
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	ar = subtilis_array_get_dim(p, ar, dim, err);
	dim = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return ar;

cleanup:

	subtilis_exp_delete(ar);
	subtilis_exp_delete(dim);

	return NULL;
}
