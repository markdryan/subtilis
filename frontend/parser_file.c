/*
 * Copyright (c) 2021 Mark Ryan
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

#include <string.h>

#include "parser_file.h"

#include "parser_exp.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

static subtilis_exp_t *prv_open(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_op_instr_type_t itype,
				subtilis_error_t *err)
{
	subtilis_exp_t *file_name;
	size_t ret;
	subtilis_exp_t *handle = NULL;

	file_name = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (file_name->type.type == SUBTILIS_TYPE_STRING) {
		file_name = subtilis_string_zt_non_const(p, file_name, err);
	} else if (file_name->type.type == SUBTILIS_TYPE_CONST_STRING) {
		file_name = subtilis_string_lca_const_zt(p, file_name, err);
	} else {
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	ret = subtilis_ir_section_add_instr2(p->current, itype,
					     file_name->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	handle = subtilis_exp_new_int32_var(ret, err);

cleanup:

	subtilis_exp_delete(file_name);

	return handle;
}

subtilis_exp_t *subtilis_parser_openout(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	return prv_open(p, t, SUBTILIS_OP_INSTR_OPENOUT, err);
}

subtilis_exp_t *subtilis_parser_openin(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err)
{
	return prv_open(p, t, SUBTILIS_OP_INSTR_OPENIN, err);
}

subtilis_exp_t *subtilis_parser_openup(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err)
{
	return prv_open(p, t, SUBTILIS_OP_INSTR_OPENUP, err);
}

static subtilis_ir_operand_t prv_parse_handle(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	subtilis_exp_t *handle;
	subtilis_ir_operand_t ret;

	ret.reg = SIZE_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return ret;

	handle = subtilis_parser_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return ret;

	ret = handle->exp.ir_op;

	subtilis_exp_delete(handle);

	return ret;
}

void subtilis_parser_close(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;

	handle = prv_parse_handle(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_CLOSE, handle, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

static subtilis_exp_t *prv_2arg_file_op(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_op_instr_type_t itype,
					subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;
	size_t ret;

	handle = prv_parse_handle(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ret = subtilis_ir_section_add_instr2(p->current, itype, handle, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_int32_var(ret, err);
}

static subtilis_exp_t *parse_1arg_bkt_file_op(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_op_instr_type_t itype,
					      subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, p->l->stream->name,
					    p->l->line);
		return NULL;
	}

	e = prv_2arg_file_op(p, t, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_error_set_expected(err, ") ", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:
	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_bget(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	return parse_1arg_bkt_file_op(p, t, SUBTILIS_OP_INSTR_BGET, err);
}

void subtilis_parser_bput(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;
	const char *tbuf;
	subtilis_exp_t *val;

	handle = prv_parse_handle(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = subtilis_parser_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_BPUT, val->exp.ir_op, handle, err);
	subtilis_exp_delete(val);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

subtilis_exp_t *subtilis_parser_eof(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return parse_1arg_bkt_file_op(p, t, SUBTILIS_OP_INSTR_EOF, err);
}

subtilis_exp_t *subtilis_parser_ext(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return parse_1arg_bkt_file_op(p, t, SUBTILIS_OP_INSTR_EXT, err);
}

subtilis_exp_t *subtilis_parser_get_ptr(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	return parse_1arg_bkt_file_op(p, t, SUBTILIS_OP_INSTR_GET_PTR, err);
}

void subtilis_parser_set_ptr(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;
	const char *tbuf;
	subtilis_exp_t *val;

	handle = prv_parse_handle(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = subtilis_parser_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_SET_PTR, val->exp.ir_op, handle, err);
	subtilis_exp_delete(val);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}

static bool prv_block_operation_prep(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_ir_operand_t *handle,
				     subtilis_ir_operand_t *array_size,
				     size_t *val_reg, bool *cow, bool *arg1_tmp,
				     subtilis_error_t *err)

{
	const char *tbuf;
	bool check_dims = false;
	subtilis_exp_t *val = NULL;

	*handle = prv_parse_handle(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		return false;
	}

	val = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	if (!subtilis_type_if_is_scalar_ref(&val->type)) {
		subtilis_error_set_expected(err,
					    "string or array of numeric types",
					    subtilis_type_name(&val->type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	*arg1_tmp = val->temporary;
	*cow = val->type.type == SUBTILIS_TYPE_STRING;

	array_size->reg =
	    subtilis_reference_type_get_size(p, val->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	check_dims = (val->type.type == SUBTILIS_TYPE_STRING) ||
		     dynamic_1d_array(&val->type);

	*val_reg = val->exp.ir_op.reg;

cleanup:

	subtilis_exp_delete(val);

	return check_dims;
}

subtilis_exp_t *subtilis_parser_get_hash(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;
	subtilis_ir_operand_t ret;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t data;
	subtilis_ir_operand_t skip_label;
	subtilis_ir_operand_t array_size;
	subtilis_ir_operand_t get_label;
	bool check_dims;
	bool arg1_tmp;
	size_t val_reg;
	const char *tbuf;
	bool cow = false;

	skip_label.label = subtilis_ir_section_new_label(p->current);
	get_label.label = subtilis_ir_section_new_label(p->current);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "(", tbuf, p->l->stream->name,
					    p->l->line);
		return NULL;
	}

	check_dims = prv_block_operation_prep(p, t, &handle, &array_size,
					      &val_reg, &cow, &arg1_tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_error_set_expected(err, ")", tbuf, p->l->stream->name,
					    p->l->line);
		return NULL;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (arg1_tmp) {
		subtilis_error_set_temporary_not_allowed(
		    err, "second argument to get#", p->l->stream->name,
		    p->l->line);
		return NULL;
	}

	ret.reg = p->current->reg_counter++;
	if (check_dims) {
		zero.integer = 0;
		subtilis_ir_section_add_instr_no_reg2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, ret, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, array_size, get_label,
		    skip_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		subtilis_ir_section_add_label(p->current, get_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	if (cow)
		data.reg = subtilis_reference_type_copy_on_write(
		    p, val_reg, 0, array_size.reg, err);
	else
		data.reg = subtilis_reference_get_data(p, val_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr4(p->current, SUBTILIS_OP_INSTR_BLOCK_GET,
				       ret, handle, data, array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (check_dims)
		subtilis_ir_section_add_label(p->current, skip_label.label,
					      err);

	return subtilis_exp_new_int32_var(ret.reg, err);
}

void subtilis_parser_put_hash(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	subtilis_ir_operand_t handle;
	subtilis_ir_operand_t data;
	subtilis_ir_operand_t skip_label;
	subtilis_ir_operand_t array_size;
	subtilis_ir_operand_t put_label;
	bool check_dims;
	bool arg1_tmp;
	size_t val_reg;
	bool cow = false;

	skip_label.label = subtilis_ir_section_new_label(p->current);
	put_label.label = subtilis_ir_section_new_label(p->current);

	check_dims = prv_block_operation_prep(p, t, &handle, &array_size,
					      &val_reg, &cow, &arg1_tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (check_dims) {
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, array_size, put_label,
		    skip_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_label(p->current, put_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	data.reg = subtilis_reference_get_data(p, val_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_BLOCK_PUT, handle,
					  data, array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (check_dims)
		subtilis_ir_section_add_label(p->current, skip_label.label,
					      err);
}
