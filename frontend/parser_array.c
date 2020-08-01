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

	memcpy(&buffer[el * element_size], &e->exp.ir_op, element_size);

cleanup:

	subtilis_exp_delete(e);

	return buffer;
}

static void prv_check_initialiser_count(subtilis_parser_t *p, size_t entries,
					size_t mem_reg, subtilis_exp_t *maxe,
					const subtilis_symbol_t *s,
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
		maxe =
		    subtilis_array_type_dynamic_size(p, mem_reg, s->loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		prv_check_initialiser_count(p, entries * element_size, mem_reg,
					    maxe, s, err);
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

	subtilis_reference_type_memcpy(p, mem_reg, s->loc, reg,
				       sizee->exp.ir_op.reg, err);

cleanup:
	subtilis_exp_delete(sizee);
	subtilis_exp_delete(e);
	free(buffer);
}

static subtilis_exp_t **prv_parse_const_string_list(subtilis_parser_t *p,
						    subtilis_exp_t *e,
						    subtilis_token_t *t,
						    size_t *len,
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

		if (e->type.type != SUBTILIS_TYPE_CONST_STRING) {
			subtilis_error_set_const_string_expected(
			    err, p->l->stream->name, p->l->line);
			if (err->type != SUBTILIS_ERROR_OK)
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
	subtilis_exp_delete(e);

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
	subtilis_ir_operand_t tmp;
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
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, array_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      array_ptr, tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = sizeof(int32_t) * 2;
	tmp.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, table_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      table_ptr, tmp, err);
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

static void prv_parse_string_initialiser(subtilis_parser_t *p,
					 subtilis_token_t *t, subtilis_exp_t *e,
					 size_t mem_reg,
					 const subtilis_symbol_t *s,
					 subtilis_error_t *err)
{
	int32_t dim;
	size_t element_size;
	size_t data_reg;
	subtilis_exp_t **ee;
	size_t table_base;
	subtilis_exp_t *maxe = NULL;
	subtilis_exp_t *maxe_dup = NULL;
	const subtilis_type_t *type = &s->t;
	bool dynamic = false;
	size_t max_elements = 1;
	size_t entries;
	size_t i = 0;

	ee = prv_parse_const_string_list(p, e, t, &entries, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * If there's only one string in the list it's not worth setting up tge
	 * loop.
	 */

	if (entries == 1) {
		data_reg = subtilis_reference_get_data(p, mem_reg, s->loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_string_type_new_owned_ref_from_const(p, data_reg, 0,
							      ee[0], err);
		ee[0] = NULL;
		goto cleanup;
	}

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

	element_size = subtilis_type_if_size(&subtilis_type_string, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!dynamic) {
		if (entries > max_elements) {
			subtilis_error_bad_element_count(
			    err, p->l->stream->name, p->l->line);
			goto cleanup;
		}
	} else {
		maxe =
		    subtilis_array_type_dynamic_size(p, mem_reg, s->loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		maxe_dup = subtilis_type_if_dup(maxe, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		prv_check_initialiser_count(p, entries * element_size, mem_reg,
					    maxe_dup, s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	table_base = prv_init_lca_table(p, ee, entries, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_get_data(p, mem_reg, s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_read_string_table(p, data_reg, table_base, entries, element_size,
			      err);

cleanup:

	for (i = 0; i < entries; i++)
		subtilis_exp_delete(ee[i]);
	free(ee);

	subtilis_exp_delete(maxe);
}

void subtilis_parser_array_assign_reference(subtilis_parser_t *p,
					    subtilis_token_t *t, size_t mem_reg,
					    const subtilis_symbol_t *s,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	subtilis_type_t el_type;

	if (subtilis_type_eq(&s->t, &e->type)) {
		subtilis_array_type_assign_ref(p, &s->t.params.array, mem_reg,
					       s->loc, e->exp.ir_op.reg, err);
	} else {
		subtilis_type_if_element_type(p, &s->t, &el_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (subtilis_type_if_is_numeric(&el_type) &&
		    subtilis_type_if_is_numeric(&e->type))
			prv_parse_numeric_initialiser(p, t, e, mem_reg, s, err);
		else if ((s->t.type == SUBTILIS_TYPE_ARRAY_STRING) &&
			 (e->type.type == SUBTILIS_TYPE_CONST_STRING))
			prv_parse_string_initialiser(p, t, e, mem_reg, s, err);
		else
			subtilis_error_set_array_type_mismatch(
			    err, p->l->stream->name, p->l->line);
	}

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

	if ((ar->type.type != SUBTILIS_TYPE_ARRAY_REAL) &&
	    (ar->type.type != SUBTILIS_TYPE_ARRAY_INTEGER) &&
	    (ar->type.type != SUBTILIS_TYPE_ARRAY_STRING)) {
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
