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

#include "builtins_helper.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

/*
 * Each string variable occupies  a fixed amount of space either on the stack or
 * in global memory.  This memory is laid out as follows.
 *
 *  ----------------------------------
 * | Size in Bytes     |           0 |
 *  ----------------------------------
 * | Pointer to Data   |           4 |
 *  ----------------------------------
 * | Destructor        |           8 |
 *  ----------------------------------
 */

#define SUBTIILIS_STRING_SIZE_OFF SUBTIILIS_REFERENCE_SIZE_OFF
#define SUBTIILIS_STRING_DATA_OFF SUBTIILIS_REFERENCE_DATA_OFF
#define SUBTIILIS_STRING_DESTRUCTOR_OFF SUBTIILIS_REFERENCE_DESTRUCTOR_OFF

static size_t prv_add_string_constant(subtilis_parser_t *p, const char *str,
				      size_t len, subtilis_error_t *err)
{
	uint8_t *buf;
	size_t id;

	buf = malloc(len);
	if (!buf) {
		subtilis_error_set_oom(err);
		return SIZE_MAX;
	}
	memcpy(buf, str, len);

	id = subtilis_constant_pool_add(p->prog->constant_pool, buf, len, false,
					err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return id;

cleanup:

	free(buf);
	return SIZE_MAX;
}

size_t subtilis_string_type_lca_const(subtilis_parser_t *p, const char *str,
				      size_t len, subtilis_error_t *err)
{
	size_t id;
	subtilis_ir_operand_t op1;

	id = prv_add_string_constant(p, str, len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op1.integer = (int32_t)id;
	return subtilis_ir_section_add_instr2(p->current, SUBTILIS_OP_INSTR_LCA,
					      op1, err);
}

size_t subtilis_string_type_size(const subtilis_type_t *type)
{
	size_t size;

	/*
	 * TODO: Size appropriately for 64 bit builds.
	 */

	/*
	 * We need, on 32 bit builds,
	 * 4 bytes for the pointer
	 * 4 bytes for the size
	 * 4 bytes for the destructor
	 */

	size = SUBTIILIS_REFERENCE_SIZE;

	return size;
}

void subtilis_string_type_zero_ref(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t mem_reg,
				   size_t loc, bool push, subtilis_error_t *err)
{
	subtilis_exp_t *zero = NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_size(p, mem_reg, loc, zero->exp.ir_op.reg,
					 err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (push)
		subtilis_reference_type_push_reference(p, type, mem_reg, loc,
						       err);

cleanup:

	subtilis_exp_delete(zero);
}

void subtilis_string_init_from_ptr(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, size_t lca_reg, size_t size_reg,
				   bool push, subtilis_error_t *err)

{
	size_t dest_reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t empty;
	subtilis_ir_operand_t not_empty;

	empty.label = subtilis_ir_section_new_label(p->current);
	not_empty.label = subtilis_ir_section_new_label(p->current);

	subtilis_reference_type_set_size(p, mem_reg, loc, size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op0, not_empty, empty, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, not_empty.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest_reg = subtilis_reference_type_alloc(p, &subtilis_type_string, loc,
						 mem_reg, size_reg, push, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_memcpy_dest(p, dest_reg, lca_reg, size_reg,
					    err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, empty.label, err);
}

void subtilis_string_init_from_lca(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, size_t lca_reg, size_t size_reg,
				   bool push, subtilis_error_t *err)

{
	size_t dest_reg;

	dest_reg = subtilis_reference_type_alloc(p, &subtilis_type_string, loc,
						 mem_reg, size_reg, push, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_memcpy_dest(p, dest_reg, lca_reg, size_reg,
					    err);
}

static void prv_init_string_from_const(subtilis_parser_t *p, size_t mem_reg,
				       size_t loc, const subtilis_buffer_t *str,
				       bool push, subtilis_error_t *err)
{
	size_t id;
	size_t src_reg;
	subtilis_ir_operand_t op1;
	subtilis_exp_t *sizee = NULL;
	size_t buf_size = subtilis_buffer_get_size(str);

	if (buf_size == 1) {
		subtilis_string_type_zero_ref(p, &subtilis_type_string, mem_reg,
					      loc, push, err);
		return;
	}

	buf_size--;
	id = prv_add_string_constant(p, subtilis_buffer_get_string(str),
				     buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_exp_new_int32((int32_t)buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = (int32_t)id;
	src_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_init_from_lca(p, mem_reg, loc, src_reg,
				      sizee->exp.ir_op.reg, push, err);
cleanup:

	subtilis_exp_delete(sizee);
}

subtilis_exp_t *subtilis_string_type_new_tmp_from_char(subtilis_parser_t *p,
						       subtilis_exp_t *e,
						       subtilis_error_t *err)
{
	subtilis_type_t type;
	const subtilis_symbol_t *s;
	size_t reg;

	type.type = SUBTILIS_TYPE_STRING;
	s = subtilis_symbol_table_insert_tmp(p->local_st, &type, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_new_ref_from_char(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					       e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&s->t, reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

void subtilis_string_type_new_ref_from_char(subtilis_parser_t *p,
					    size_t mem_reg, size_t loc,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t dest_reg;
	subtilis_ir_operand_t dest_op;
	subtilis_ir_operand_t zero_op;
	subtilis_exp_t *sizee = NULL;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_reg = subtilis_reference_type_alloc(p, &subtilis_type_string, loc,
						 mem_reg, sizee->exp.ir_op.reg,
						 true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_op.reg = dest_reg;
	zero_op.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I8,
					  e->exp.ir_op, dest_op, zero_op, err);

cleanup:

	subtilis_exp_delete(sizee);
	subtilis_exp_delete(e);
}

subtilis_exp_t *subtilis_string_lca_const_zt(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *ret_val = NULL;

	subtilis_buffer_zero_terminate(&e->exp.str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_string_type_lca_const(
	    p, subtilis_buffer_get_string(&e->exp.str),
	    subtilis_buffer_get_size(&e->exp.str), err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ret_val = subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(e);

	return ret_val;
}

static subtilis_exp_t *prv_zt_non_const_tmp(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t reg;
	size_t old_size_reg;
	size_t delta_reg;
	size_t new_size_reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_exp_t *ret_val = NULL;

	reg = subtilis_reference_get_data(p, e->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	old_size_reg =
	    subtilis_reference_type_get_size(p, e->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	delta_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = old_size_reg;
	new_size_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_reference_type_realloc(p, 0, e->exp.ir_op.reg, reg,
					      old_size_reg, new_size_reg,
					      delta_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.integer = 0;
	op0.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = reg;
	op2.reg = old_size_reg;
	op1.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I8, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ret_val = subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(e);
	return ret_val;
}

static subtilis_exp_t *prv_zt_non_const(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err)
{
	subtilis_buffer_t buf;
	size_t reg;
	subtilis_exp_t *null_str = NULL;
	subtilis_exp_t *new_str = NULL;

	subtilis_buffer_init(&buf, 2);

	/*
	 * The string addition code will discard one of the null terminators
	 * so we need two.
	 */

	subtilis_buffer_append(&buf, "\0\0", 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	null_str = subtilis_exp_new_str(&buf, err);
	subtilis_buffer_free(&buf);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	new_str = subtilis_string_type_add(p, e, null_str, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	reg = subtilis_reference_get_data(p, new_str->exp.ir_op.reg, 0, err);
	subtilis_exp_delete(new_str);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(null_str);
	subtilis_exp_delete(e);

	return NULL;
}

/*
 * Copies a variable string appending a null character to
 * the end of the new string.  This is needed for SYS calls.
 */

subtilis_exp_t *subtilis_string_zt_non_const(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err)
{
	if (e->type.type != SUBTILIS_TYPE_STRING) {
		subtilis_error_set_string_variable_expected(
		    err, p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	if (e->temporary)
		return prv_zt_non_const_tmp(p, e, err);

	/*
	 * We need to create a brand new temporary string and copy
	 * the contents of the old string appending a 0 to the end
	 */

	return prv_zt_non_const(p, e, err);
}

/*
 * Used to initialise a string variable from a constant.  The string
 * variable is owned by some container and so does not need to be
 * pushed.
 */

void subtilis_string_type_new_owned_ref_from_const(subtilis_parser_t *p,
						   size_t mem_reg, size_t loc,
						   subtilis_exp_t *e,
						   subtilis_error_t *err)
{
	prv_init_string_from_const(p, mem_reg, loc, &e->exp.str, false, err);
	subtilis_exp_delete(e);
}

void subtilis_string_type_new_ref(subtilis_parser_t *p,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_error_t *err)
{
	switch (e->type.type) {
	case SUBTILIS_TYPE_STRING:
		subtilis_reference_type_new_ref(p, type, mem_reg, loc,
						e->exp.ir_op.reg, true, err);
		break;
	case SUBTILIS_TYPE_CONST_STRING:
		prv_init_string_from_const(p, mem_reg, loc, &e->exp.str, true,
					   err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

	subtilis_exp_delete(e);
}

void subtilis_string_type_assign_ref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     size_t mem_reg, size_t loc,
				     subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_reference_type_deref(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (e->type.type) {
	case SUBTILIS_TYPE_STRING:
		subtilis_reference_type_init_ref(p, mem_reg, loc,
						 e->exp.ir_op.reg, true, err);
		break;
	case SUBTILIS_TYPE_CONST_STRING:
		prv_init_string_from_const(p, mem_reg, loc, &e->exp.str, false,
					   err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_string_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
					subtilis_exp_t *e,
					subtilis_error_t *err)
{
	subtilis_reference_type_assign_to_reg(p, reg, e, true, err);
}

subtilis_exp_t *subtilis_string_type_len(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t offset;
	size_t size;
	int32_t bytes;
	subtilis_exp_t *len = NULL;

	switch (e->type.type) {
	case SUBTILIS_TYPE_STRING:
		offset.integer = SUBTIILIS_STRING_SIZE_OFF;
		size = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, e->exp.ir_op,
		    offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		len = subtilis_exp_new_int32_var(size, err);
		break;
	case SUBTILIS_TYPE_CONST_STRING:
		bytes = (int32_t)subtilis_buffer_get_size(&e->exp.str);
		if (bytes > 0)
			bytes--;
		len = subtilis_exp_new_int32(bytes, err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(e);

	return len;
}

static subtilis_exp_t *prv_asc_var(subtilis_parser_t *p,
				   subtilis_ir_operand_t str,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t data_reg;
	subtilis_ir_operand_t byte_reg;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t neg1;
	subtilis_ir_operand_t gtzero;
	subtilis_ir_operand_t offset;

	offset.integer = SUBTIILIS_STRING_SIZE_OFF;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, str, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	zero.label = subtilis_ir_section_new_label(p->current);
	gtzero.label = subtilis_ir_section_new_label(p->current);

	neg1.integer = -1;
	byte_reg.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, neg1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, gtzero, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, gtzero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	offset.integer = SUBTIILIS_STRING_DATA_OFF;
	data_reg.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, str, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	offset.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_LOADO_I8, byte_reg,
					  data_reg, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_int32_var(byte_reg.reg, err);
}

subtilis_exp_t *subtilis_string_type_asc(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err)
{
	int32_t byte;
	int32_t bytes;
	size_t start;
	subtilis_exp_t *retval = NULL;

	switch (e->type.type) {
	case SUBTILIS_TYPE_STRING:
		retval = prv_asc_var(p, e->exp.ir_op, err);
		break;
	case SUBTILIS_TYPE_CONST_STRING:
		bytes = (int32_t)subtilis_buffer_get_size(&e->exp.str);
		if (bytes <= 1) {
			byte = -1;
		} else {
			start = e->exp.str.buffer->start;
			byte = e->exp.str.buffer->data[start];
		}
		retval = subtilis_exp_new_int32(byte, err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

	subtilis_exp_delete(e);

	return retval;
}

void subtilis_string_type_print(subtilis_parser_t *p, subtilis_exp_t *e,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t gtzero;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t str;
	subtilis_ir_operand_t size;

	offset.integer = SUBTIILIS_STRING_SIZE_OFF;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, e->exp.ir_op, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero.label = subtilis_ir_section_new_label(p->current);
	gtzero.label = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, gtzero, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, gtzero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	offset.integer = SUBTIILIS_STRING_DATA_OFF;
	str.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, e->exp.ir_op, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_PRINT_STR, str, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, zero.label, err);

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_string_type_print_const(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	size_t id;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t str;
	size_t buf_size = subtilis_buffer_get_size(&e->exp.str);

	if (buf_size == 1)
		goto cleanup;

	buf_size--;
	id = prv_add_string_constant(p, subtilis_buffer_get_string(&e->exp.str),
				     buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = (int32_t)id;
	str.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = buf_size;
	op1.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_PRINT_STR, str, op1, err);

cleanup:

	subtilis_exp_delete(e);
}

subtilis_exp_t *subtilis_string_type_eq(subtilis_parser_t *p, size_t a_reg,
					size_t b_reg, size_t len,
					subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memcpy[] = "_memcmp";

	name = malloc(sizeof(memcpy));
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(name, memcpy);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return NULL;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = a_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = b_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = len;

	return subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMCMP, NULL,
				     args, &subtilis_type_integer, 3, true,
				     err);
}

subtilis_exp_t *subtilis_string_type_compare(subtilis_parser_t *p, size_t a_reg,
					     size_t a_len_reg, size_t b_reg,
					     size_t b_len_reg,
					     subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memcpy[] = "_compare";

	name = malloc(sizeof(memcpy));
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(name, memcpy);

	args = malloc(sizeof(*args) * 4);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return NULL;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = a_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = a_len_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = b_reg;
	args[3].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[3].reg = b_len_reg;

	return subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_COMPARE, NULL,
				     args, &subtilis_type_integer, 4, true,
				     err);
}

static size_t prv_left_right_const(subtilis_parser_t *p,
				   const subtilis_buffer_t *str,
				   size_t size_reg,
				   const subtilis_symbol_t **sym,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t gt_len;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t full_string;
	subtilis_ir_operand_t partial;
	const subtilis_symbol_t *s;
	size_t id;
	size_t buf_size = subtilis_buffer_get_size(str) - 1;

	full_string.label = subtilis_ir_section_new_label(p->current);
	partial.label = subtilis_ir_section_new_label(p->current);

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op0.reg = size_reg;
	op1.integer = 0;
	less_zero.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op1.integer = buf_size;
	gt_len.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTEI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, less_zero, gt_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, full_string, partial, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	subtilis_ir_section_add_label(p->current, full_string.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op1.integer = buf_size;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	subtilis_ir_section_add_label(p->current, partial.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	id = prv_add_string_constant(p, subtilis_buffer_get_string(str),
				     buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	*sym = s;
	return id;
}

static subtilis_exp_t *prv_left_const(subtilis_parser_t *p,
				      const subtilis_buffer_t *str,
				      size_t size_reg, subtilis_error_t *err)
{
	size_t id;
	size_t ptr;
	size_t src_reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t zero_len;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t getp;
	const subtilis_symbol_t *s;

	zero_len.label = subtilis_ir_section_new_label(p->current);
	gt_zero.label = subtilis_ir_section_new_label(p->current);
	getp.label = subtilis_ir_section_new_label(p->current);

	id = prv_left_right_const(p, str, size_reg, &s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.reg = size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, gt_zero, zero_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, zero_len.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     getp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, gt_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.integer = (int32_t)id;
	src_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_lca(p, SUBTILIS_IR_REG_LOCAL, s->loc, src_reg,
				      size_reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, getp.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr, err);
}

static size_t prv_left_right_non_const(subtilis_parser_t *p, size_t str_reg,
				       size_t arg2_reg, bool arg2_const,
				       subtilis_ir_operand_t end,
				       size_t ptr_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t gt_len;
	subtilis_ir_operand_t str_len_zero;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t full_string;
	subtilis_ir_operand_t partial;
	size_t len_reg;

	op0.reg = str_reg;
	op1.integer = SUBTIILIS_STRING_SIZE_OFF;
	len_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	full_string.label = subtilis_ir_section_new_label(p->current);
	partial.label = subtilis_ir_section_new_label(p->current);

	if (!arg2_const) {
		op0.reg = arg2_reg;
		op1.integer = 0;
		less_zero.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LTI_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		op0.reg = arg2_reg;
		op1.reg = len_reg;
		gt_len.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_GTE_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_OR_I32, less_zero, gt_len,
		    err);
	} else {
		op0.reg = arg2_reg;
		op1.reg = len_reg;
		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_GTE_I32, op0, op1, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op0.reg = len_reg;
	op1.integer = 0;
	str_len_zero.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, condee, str_len_zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, full_string, partial, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, full_string.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op0.reg = ptr_reg;
	op1.reg = str_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, partial.label, err);

	return len_reg;
}

static subtilis_exp_t *prv_left_non_const(subtilis_parser_t *p, size_t str_reg,
					  size_t left_reg, bool left_const,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t end;
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t ptr_reg = p->current->reg_counter++;

	end.label = subtilis_ir_section_new_label(p->current);
	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	(void)prv_left_right_non_const(p, str_reg, left_reg, left_const, end,
				       ptr_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	str_reg = subtilis_reference_get_data(p, str_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_ptr(p, SUBTILIS_IR_REG_LOCAL, s->loc, str_reg,
				      left_reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = ptr_reg;
	op1.reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						 s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr_reg, err);
}

static subtilis_exp_t *prv_left_right_const_str(subtilis_exp_t *str,
						subtilis_exp_t *len,
						size_t start,
						subtilis_error_t *err)
{
	size_t const_str_len;
	size_t const_int_val;

	const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;
	const_int_val = len->exp.ir_op.integer;
	if (const_int_val < const_str_len) {
		subtilis_buffer_delete(&str->exp.str, start,
				       const_str_len - const_int_val, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_buffer_zero_terminate(&str->exp.str, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_exp_delete(len);
	return str;

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(len);

	return NULL;
}

subtilis_exp_t *subtilis_string_type_left_exp(subtilis_parser_t *p,
					      subtilis_exp_t *str,
					      subtilis_exp_t *len,
					      subtilis_error_t *err)
{
	size_t const_str_len;
	size_t const_int_val;
	subtilis_exp_t *ret;
	bool left_const = false;

	len = subtilis_type_if_to_int(p, len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (str->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;
		if (const_str_len == 0) {
			subtilis_exp_delete(len);
			return str;
		}
		if (len->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			const_int_val = len->exp.ir_op.integer;
			return prv_left_right_const_str(str, len, const_int_val,
							err);
		}
		ret = prv_left_const(p, &str->exp.str, len->exp.ir_op.reg, err);
		subtilis_exp_delete(str);
		subtilis_exp_delete(len);
		return ret;
	case SUBTILIS_TYPE_STRING:
		if (len->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			left_const = true;
			if (len->exp.ir_op.integer < 0) {
				subtilis_exp_delete(len);
				return str;
			}
			if (len->exp.ir_op.integer == 0) {
				str->type.type = SUBTILIS_TYPE_CONST_STRING;
				subtilis_buffer_init(&str->exp.str, 1);
				subtilis_buffer_zero_terminate(&str->exp.str,
							       err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				subtilis_exp_delete(len);
				return str;
			}
			len = subtilis_type_if_exp_to_var(p, len, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		ret = prv_left_non_const(p, str->exp.ir_op.reg,
					 len->exp.ir_op.reg, left_const, err);
		subtilis_exp_delete(str);
		subtilis_exp_delete(len);
		return ret;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(len);

	return NULL;
}

static subtilis_exp_t *prv_right_const(subtilis_parser_t *p,
				       const subtilis_buffer_t *str,
				       size_t size_reg, subtilis_error_t *err)
{
	size_t id;
	size_t ptr;
	size_t src_reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t zero_len;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t getp;
	size_t buf_size = subtilis_buffer_get_size(str) - 1;

	zero_len.label = subtilis_ir_section_new_label(p->current);
	gt_zero.label = subtilis_ir_section_new_label(p->current);
	getp.label = subtilis_ir_section_new_label(p->current);

	id = prv_left_right_const(p, str, size_reg, &s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.reg = size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, gt_zero, zero_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, zero_len.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     getp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, gt_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.integer = (int32_t)id;
	src_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = src_reg;
	op1.integer = (int32_t)buf_size;
	src_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = src_reg;
	op1.reg = size_reg;
	src_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_SUB_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_lca(p, SUBTILIS_IR_REG_LOCAL, s->loc, src_reg,
				      size_reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, getp.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr, err);
}

static subtilis_exp_t *prv_right_non_const(subtilis_parser_t *p, size_t str_reg,
					   size_t right_reg, bool right_const,
					   subtilis_error_t *err)
{
	subtilis_ir_operand_t end;
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t len_reg;
	size_t ptr_reg = p->current->reg_counter++;

	end.label = subtilis_ir_section_new_label(p->current);
	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	len_reg = prv_left_right_non_const(p, str_reg, right_reg, right_const,
					   end, ptr_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	str_reg = subtilis_reference_get_data(p, str_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = str_reg;
	op1.reg = len_reg;
	op0.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.reg = right_reg;
	str_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_SUB_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_ptr(p, SUBTILIS_IR_REG_LOCAL, s->loc, str_reg,
				      right_reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = ptr_reg;
	op1.reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						 s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr_reg, err);
}

subtilis_exp_t *subtilis_string_type_right_exp(subtilis_parser_t *p,
					       subtilis_exp_t *str,
					       subtilis_exp_t *len,
					       subtilis_error_t *err)
{
	size_t const_str_len;
	subtilis_exp_t *ret = NULL;
	bool right_const = false;

	len = subtilis_type_if_to_int(p, len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (str->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;
		if (const_str_len == 0) {
			subtilis_exp_delete(len);
			return str;
		}
		if (len->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			return prv_left_right_const_str(str, len, 0, err);

		ret =
		    prv_right_const(p, &str->exp.str, len->exp.ir_op.reg, err);
		subtilis_exp_delete(str);
		subtilis_exp_delete(len);
		return ret;
	case SUBTILIS_TYPE_STRING:
		if (len->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			right_const = true;
			if (len->exp.ir_op.integer < 0) {
				subtilis_exp_delete(len);
				return str;
			}
			if (len->exp.ir_op.integer == 0) {
				str->type.type = SUBTILIS_TYPE_CONST_STRING;
				subtilis_buffer_init(&str->exp.str, 1);
				subtilis_buffer_zero_terminate(&str->exp.str,
							       err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				subtilis_exp_delete(len);
				return str;
			}
			len = subtilis_type_if_exp_to_var(p, len, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		ret = prv_right_non_const(p, str->exp.ir_op.reg,
					  len->exp.ir_op.reg, right_const, err);
		subtilis_exp_delete(str);
		subtilis_exp_delete(len);
		return ret;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(len);

	return NULL;
}

static subtilis_exp_t *prv_mid_str_ccv(subtilis_parser_t *p,
				       subtilis_exp_t *str, size_t len_reg,
				       subtilis_error_t *err)
{
	size_t const_str_len;
	subtilis_ir_operand_t len_zero;
	subtilis_ir_operand_t len_not_zero;
	subtilis_ir_operand_t len_too_big;
	subtilis_ir_operand_t len_below_zero;
	subtilis_ir_operand_t len_ok;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t end;
	subtilis_ir_operand_t condee;
	const subtilis_symbol_t *s;
	size_t ptr;
	size_t id;
	size_t src_reg;
	subtilis_ir_operand_t to_copy;

	to_copy.reg = p->current->reg_counter++;
	len_zero.label = subtilis_ir_section_new_label(p->current);
	len_not_zero.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);
	len_too_big.label = subtilis_ir_section_new_label(p->current);
	len_ok.label = subtilis_ir_section_new_label(p->current);
	const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = len_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op0, len_not_zero, len_zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, len_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, len_not_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.integer = 0;
	len_below_zero.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.integer = const_str_len;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, condee, len_below_zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, len_too_big, len_ok, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, len_too_big.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = len_reg;
	op1.integer = const_str_len;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, len_ok.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.reg = len_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      to_copy, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	id = prv_add_string_constant(
	    p, subtilis_buffer_get_string(&str->exp.str), const_str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op1.integer = (int32_t)id;
	src_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_lca(p, SUBTILIS_IR_REG_LOCAL, s->loc, src_reg,
				      to_copy.reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr, err);
}

static subtilis_exp_t *prv_mid_str_cc(subtilis_parser_t *p, subtilis_exp_t *str,
				      subtilis_exp_t *start,
				      subtilis_exp_t *len,
				      subtilis_error_t *err)
{
	int32_t len_const;
	int32_t start_const;
	int32_t const_str_len =
	    (int32_t)subtilis_buffer_get_size(&str->exp.str) - 1;
	subtilis_exp_t *ret = NULL;

	start_const = start->exp.ir_op.integer - 1;
	if (start_const == -1)
		start_const = 0;
	if (start_const < 0 || start_const >= const_str_len) {
		subtilis_buffer_reset(&str->exp.str);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_buffer_zero_terminate(&str->exp.str, err);
		if (err->type == SUBTILIS_ERROR_OK) {
			ret = str;
			str = NULL;
		}
		goto cleanup;
	}

	if (start_const > 0) {
		subtilis_buffer_delete(&str->exp.str, 0, start_const, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		const_str_len -= start_const;
		start_const = 0;
	}

	if (!len || (len->type.type == SUBTILIS_TYPE_CONST_INTEGER)) {
		if (!len) {
			len_const = const_str_len;
		} else {
			len_const = len->exp.ir_op.integer;
			if (len_const > const_str_len)
				len_const = const_str_len;
		}

		if (len_const < const_str_len)
			subtilis_buffer_delete(&str->exp.str, len_const,
					       const_str_len - len_const, err);
		if (err->type == SUBTILIS_ERROR_OK) {
			ret = str;
			str = NULL;
		}
		goto cleanup;
	}

	/* len has been specified and is non_const */

	ret = prv_mid_str_ccv(p, str, len->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(start);
	subtilis_exp_delete(len);

	return ret;
}

static size_t prv_mid_str_check(subtilis_parser_t *p, size_t start_reg,
				size_t len_reg, subtilis_ir_operand_t to_copy,
				subtilis_ir_operand_t end,
				subtilis_exp_t *str_len,
				const subtilis_symbol_t **s_ret,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t start_reg_zero;
	subtilis_ir_operand_t start_reg_not_zero;
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t len_too_big;
	subtilis_ir_operand_t len_below_zero;
	subtilis_ir_operand_t len_ok;
	subtilis_ir_operand_t len_zero;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t empty;
	subtilis_ir_operand_t non_empty;
	subtilis_ir_operand_t zero_reg;
	subtilis_ir_operand_t condee;
	const subtilis_symbol_t *s;
	size_t start_reg_copy;
	subtilis_exp_t *a1 = NULL;
	subtilis_exp_t *a2 = NULL;

	start_reg_zero.label = subtilis_ir_section_new_label(p->current);
	start_reg_not_zero.label = subtilis_ir_section_new_label(p->current);
	empty.label = subtilis_ir_section_new_label(p->current);
	non_empty.label = subtilis_ir_section_new_label(p->current);
	len_too_big.label = subtilis_ir_section_new_label(p->current);
	len_ok.label = subtilis_ir_section_new_label(p->current);

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = start_reg;
	start_reg_copy = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	condee.reg = start_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, start_reg_not_zero,
					  start_reg_zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, start_reg_not_zero.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = start_reg_copy;
	op1.reg = start_reg;
	op2.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	start_reg = start_reg_copy;

	subtilis_ir_section_add_label(p->current, start_reg_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = start_reg;
	op1.integer = 0;
	less_zero.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1 = subtilis_exp_new_int32_var(start_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	a2 = subtilis_type_if_dup(str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1 = subtilis_type_if_gte(p, a1, a2, err);
	a2 = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	condee.reg =
	    subtilis_ir_section_add_instr(p->current, SUBTILIS_OP_INSTR_OR_I32,
					  less_zero, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_exp_delete(a1);
	a1 = NULL;

	if (len_reg != SIZE_MAX) {
		op0.reg = len_reg;
		op1.integer = 0;
		len_zero.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_EQI_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_OR_I32, condee, len_zero,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, empty, non_empty, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, empty.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	zero_reg.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 zero_reg.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * No need to push to the stack as this is a temporary reference
	 * which cannot be assigned to and that owns no data, i.e., it can
	 * never own any data.
	 */

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, non_empty.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1 = subtilis_exp_new_int32_var(start_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1 = subtilis_type_if_sub(p, str_len, a1, err);
	str_len = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      to_copy, a1->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_exp_delete(a1);
	a1 = NULL;

	if (len_reg != SIZE_MAX) {
		op0.reg = len_reg;
		op1.integer = 0;
		len_below_zero.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LTI_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_GT_I32, op0, to_copy, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_OR_I32, condee,
		    len_below_zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, len_too_big,
		    len_ok, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, len_too_big.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op0.reg = len_reg;
		subtilis_ir_section_add_instr_no_reg2(
		    p->current, SUBTILIS_OP_INSTR_MOV, op0, to_copy, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, len_ok.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op1.reg = len_reg;
		subtilis_ir_section_add_instr_no_reg2(
		    p->current, SUBTILIS_OP_INSTR_MOV, to_copy, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_exp_delete(str_len);
	*s_ret = s;
	return start_reg;

cleanup:

	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(str_len);

	return 0;
}

static subtilis_exp_t *prv_mid_str_cv(subtilis_parser_t *p, subtilis_exp_t *str,
				      size_t start_reg, size_t len_reg,
				      subtilis_error_t *err)
{
	size_t const_str_len;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t end;
	const subtilis_symbol_t *s;
	size_t ptr;
	size_t id;
	size_t src_reg;
	subtilis_ir_operand_t to_copy;
	subtilis_exp_t *str_len = NULL;

	to_copy.reg = p->current->reg_counter++;
	const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;
	end.label = subtilis_ir_section_new_label(p->current);

	str_len = subtilis_exp_new_int32(const_str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	start_reg = prv_mid_str_check(p, start_reg, len_reg, to_copy, end,
				      str_len, &s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	id = prv_add_string_constant(
	    p, subtilis_buffer_get_string(&str->exp.str), const_str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op1.integer = (int32_t)id;
	src_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_LCA, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = src_reg;
	op1.reg = start_reg;
	src_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_lca(p, SUBTILIS_IR_REG_LOCAL, s->loc, src_reg,
				      to_copy.reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr, err);
}

static subtilis_exp_t *prv_mid_str_vvv(subtilis_parser_t *p,
				       subtilis_exp_t *str, size_t start_reg,
				       size_t len_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t end;
	const subtilis_symbol_t *s;
	size_t ptr;
	size_t src_reg;
	subtilis_ir_operand_t to_copy;
	subtilis_exp_t *str_len = NULL;

	to_copy.reg = p->current->reg_counter++;
	end.label = subtilis_ir_section_new_label(p->current);

	op1.integer = SUBTIILIS_STRING_SIZE_OFF;
	op1.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, str->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	str_len = subtilis_exp_new_int32_var(op1.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	start_reg = prv_mid_str_check(p, start_reg, len_reg, to_copy, end,
				      str_len, &s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_data(p, str->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = ptr;
	op1.reg = start_reg;
	src_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_string_init_from_ptr(p, SUBTILIS_IR_REG_LOCAL, s->loc, src_reg,
				      to_copy.reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	ptr = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, ptr, err);
}

subtilis_exp_t *subtilis_string_type_mid_exp(subtilis_parser_t *p,
					     subtilis_exp_t *str,
					     subtilis_exp_t *start,
					     subtilis_exp_t *len,
					     subtilis_error_t *err)
{
	size_t const_str_len;
	subtilis_exp_t *ret = NULL;
	size_t len_reg;

	start = subtilis_type_if_to_int(p, start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (len) {
		len = subtilis_type_if_to_int(p, len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	switch (str->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		const_str_len = subtilis_buffer_get_size(&str->exp.str) - 1;
		if (const_str_len == 0) {
			ret = str;
			str = NULL;
			goto cleanup;
		}
		if (len && len->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			if (len->exp.ir_op.integer == 0) {
				subtilis_buffer_reset(&str->exp.str);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				subtilis_buffer_zero_terminate(&str->exp.str,
							       err);
				ret = str;
				str = NULL;
				goto cleanup;
			}
			if (len->exp.ir_op.integer < 0)
				len->exp.ir_op.integer = const_str_len;
		}

		if (start->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			return prv_mid_str_cc(p, str, start, len, err);
		if (len) {
			len = subtilis_type_if_exp_to_var(p, len, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			len_reg = len->exp.ir_op.reg;
		} else {
			len_reg = SIZE_MAX;
		}
		ret =
		    prv_mid_str_cv(p, str, start->exp.ir_op.reg, len_reg, err);
		break;
	case SUBTILIS_TYPE_STRING:
		start = subtilis_type_if_exp_to_var(p, start, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (len) {
			len = subtilis_type_if_exp_to_var(p, len, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			len_reg = len->exp.ir_op.reg;
		} else {
			len_reg = SIZE_MAX;
		}
		ret =
		    prv_mid_str_vvv(p, str, start->exp.ir_op.reg, len_reg, err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(start);
	subtilis_exp_delete(len);

	return ret;
}

static size_t prv_left_right_calc_len(subtilis_parser_t *p, subtilis_exp_t *len,
				      subtilis_exp_t *value, size_t str_len_reg,
				      size_t *value_len_reg,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t len_op;
	subtilis_ir_operand_t str_len;
	subtilis_ir_operand_t gt_len;
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t value_len;
	size_t to_comp_reg;
	size_t to_copy_reg = p->current->reg_counter++;

	str_len.reg = str_len_reg;
	if ((len->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (len->exp.ir_op.integer < 0)) {
		to_comp_reg = str_len.reg;
	} else {
		to_comp_reg = p->current->reg_counter++;
		if (len->type.type == SUBTILIS_TYPE_INTEGER) {
			op1.integer = 0;
			less_zero.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_LTI_I32,
			    len->exp.ir_op, op1, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return SIZE_MAX;

			gt_len.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_GTE_I32,
			    len->exp.ir_op, str_len, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return SIZE_MAX;

			condee.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_OR_I32, less_zero,
			    gt_len, err);
			len_op = len->exp.ir_op;
		} else {
			len_op.reg = subtilis_ir_section_add_instr2(
			    p->current, SUBTILIS_OP_INSTR_MOVI_I32,
			    len->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return SIZE_MAX;
			condee.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_LT_I32, str_len,
			    len_op, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		op0.reg = to_comp_reg;
		subtilis_ir_section_add_instr4(p->current,
					       SUBTILIS_OP_INSTR_CMOV_I32, op0,
					       condee, str_len, len_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	if (value->type.type == SUBTILIS_TYPE_CONST_STRING) {
		op1.integer = subtilis_buffer_get_size(&value->exp.str) - 1;
		value_len.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	} else {
		op1.integer = SUBTIILIS_STRING_SIZE_OFF;
		value_len.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, value->exp.ir_op,
		    op1, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.reg = to_comp_reg;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GT_I32, value_len, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op0.reg = to_copy_reg;
	op1.reg = to_comp_reg;

	subtilis_ir_section_add_instr4(p->current, SUBTILIS_OP_INSTR_CMOV_I32,
				       op0, condee, op1, value_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	*value_len_reg = value_len.reg;

	return to_copy_reg;
}

/*
 * Takes ownership of len.
 */

static subtilis_exp_t *prv_left_right_check_args(subtilis_parser_t *p,
						 subtilis_exp_t *str,
						 subtilis_exp_t *len,
						 subtilis_exp_t *value,
						 subtilis_error_t *err)
{
	size_t buf_size;

	if (str->type.type != SUBTILIS_TYPE_STRING) {
		subtilis_error_set_string_variable_expected(
		    err, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	switch (value->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		buf_size = subtilis_buffer_get_size(&value->exp.str);

		/*
		 * No error here.  We just don't do anything.
		 */

		if (buf_size == 1)
			goto cleanup;
		break;
	case SUBTILIS_TYPE_STRING:
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	len = subtilis_type_if_to_int(p, len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (len->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		/*
		 * No error here.  We just don't do anything.
		 */

		if (len->exp.ir_op.integer == 0)
			goto cleanup;
	}

	return len;

cleanup:

	subtilis_exp_delete(len);
	return NULL;
}

static void prv_left_right_copy(subtilis_parser_t *p, size_t data_reg,
				size_t str_len_reg, size_t value_len_reg,
				subtilis_ir_operand_t to_copy,
				subtilis_exp_t *value, bool left,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t data;
	subtilis_ir_operand_t str_len;
	subtilis_ir_operand_t value_len;
	subtilis_ir_operand_t source_str;

	if (value->type.type == SUBTILIS_TYPE_CONST_STRING)
		source_str.reg = subtilis_string_type_lca_const(
		    p, subtilis_buffer_get_string(&value->exp.str),
		    subtilis_buffer_get_size(&value->exp.str) - 1, err);
	else
		source_str.reg = subtilis_reference_get_data(
		    p, value->exp.ir_op.reg, 0, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!left) {
		data.reg = data_reg;
		str_len.reg = str_len_reg;
		data.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADD_I32, data, str_len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		data_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_SUB_I32, data, to_copy, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		value_len.reg = value_len_reg;
		source_str.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADD_I32, source_str,
		    value_len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		source_str.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_SUB_I32, source_str, to_copy,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_reference_type_memcpy_dest(p, data_reg, source_str.reg,
					    to_copy.reg, err);
}

static void prv_left_right_statement(subtilis_parser_t *p, subtilis_exp_t *str,
				     subtilis_exp_t *len, subtilis_exp_t *value,
				     bool left, subtilis_error_t *err)
{
	size_t data_reg;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t strlen_gt0_label;
	subtilis_ir_operand_t to_copy;
	subtilis_ir_operand_t op1;
	size_t str_len_reg;
	size_t value_len_reg;

	end_label.label = subtilis_ir_section_new_label(p->current);
	strlen_gt0_label.label = subtilis_ir_section_new_label(p->current);

	len = prv_left_right_check_args(p, str, len, value, err);
	if (err->type != SUBTILIS_ERROR_OK || !len)
		goto cleanup;

	op1.integer = SUBTIILIS_STRING_SIZE_OFF;
	str_len_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, str->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_copy.reg = prv_left_right_calc_len(p, len, value, str_len_reg,
					      &value_len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * If to_copy is 0 there's nothing to do.
	 */

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  to_copy, strlen_gt0_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, strlen_gt0_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_type_copy_on_write(p, str->exp.ir_op.reg,
							 0, str_len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_left_right_copy(p, data_reg, str_len_reg, value_len_reg, to_copy,
			    value, left, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(len);
	subtilis_exp_delete(value);
}

void subtilis_string_type_left(subtilis_parser_t *p, subtilis_exp_t *str,
			       subtilis_exp_t *len, subtilis_exp_t *value,
			       subtilis_error_t *err)
{
	prv_left_right_statement(p, str, len, value, true, err);
}

void subtilis_string_type_right(subtilis_parser_t *p, subtilis_exp_t *str,
				subtilis_exp_t *len, subtilis_exp_t *value,
				subtilis_error_t *err)
{
	prv_left_right_statement(p, str, len, value, false, err);
}

void subtilis_string_type_mid(subtilis_parser_t *p, subtilis_exp_t *str,
			      subtilis_exp_t *start, subtilis_exp_t *len,
			      subtilis_exp_t *value, subtilis_error_t *err)
{
	size_t data_reg;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t start_gt0_label;
	subtilis_ir_operand_t start_lt_str_len_label;
	subtilis_ir_operand_t strlen_gt0_label;
	subtilis_ir_operand_t to_copy;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t condee;
	size_t str_len_reg;
	size_t value_len_reg;

	end_label.label = subtilis_ir_section_new_label(p->current);
	start_gt0_label.label = subtilis_ir_section_new_label(p->current);
	start_lt_str_len_label.label =
	    subtilis_ir_section_new_label(p->current);
	strlen_gt0_label.label = subtilis_ir_section_new_label(p->current);

	len = prv_left_right_check_args(p, str, len, value, err);
	if (err->type != SUBTILIS_ERROR_OK || !len)
		goto cleanup;

	start = subtilis_type_if_to_int(p, start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (start->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		if (start->exp.ir_op.integer < 0)
			goto cleanup;
	} else {
		op1.integer = 0;
		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_GTEI_I32, start->exp.ir_op,
		    op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, start_gt0_label,
		    end_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, start_gt0_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.integer = SUBTIILIS_STRING_SIZE_OFF;
	str_len_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, str->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = str_len_reg;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, (start->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			    ? SUBTILIS_OP_INSTR_GTI_I32
			    : SUBTILIS_OP_INSTR_GT_I32,
	    op1, start->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, start_lt_str_len_label,
					  end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, start_lt_str_len_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = str_len_reg;
	str_len_reg = subtilis_ir_section_add_instr(
	    p->current, (start->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			    ? SUBTILIS_OP_INSTR_SUBI_I32
			    : SUBTILIS_OP_INSTR_SUB_I32,
	    op1, start->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_copy.reg = prv_left_right_calc_len(p, len, value, str_len_reg,
					      &value_len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * If to_copy is 0 there's nothing to do.
	 */

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  to_copy, strlen_gt0_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, strlen_gt0_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_type_copy_on_write(p, str->exp.ir_op.reg,
							 0, str_len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = data_reg;
	data_reg = subtilis_ir_section_add_instr(
	    p->current, (start->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			    ? SUBTILIS_OP_INSTR_ADDI_I32
			    : SUBTILIS_OP_INSTR_ADD_I32,
	    op1, start->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_left_right_copy(p, data_reg, str_len_reg, value_len_reg, to_copy,
			    value, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);

cleanup:

	subtilis_exp_delete(str);
	subtilis_exp_delete(start);
	subtilis_exp_delete(len);
	subtilis_exp_delete(value);
}

static subtilis_exp_t *prv_string_str_const(subtilis_parser_t *p,
					    subtilis_exp_t *count,
					    subtilis_exp_t *str,
					    subtilis_error_t *err)
{
	int32_t i;
	char *copy = NULL;

	if (count->exp.ir_op.integer <= 0) {
		subtilis_buffer_reset(&str->exp.str);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_buffer_zero_terminate(&str->exp.str, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(count);
		return str;
	}

	copy = malloc(subtilis_buffer_get_size(&str->exp.str) + 1);
	if (!copy) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(copy, subtilis_buffer_get_string(&str->exp.str));
	subtilis_buffer_remove_terminator(&str->exp.str);
	for (i = 1; i < count->exp.ir_op.integer; i++) {
		subtilis_buffer_append_string(&str->exp.str, copy, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	subtilis_buffer_zero_terminate(&str->exp.str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	free(copy);
	subtilis_exp_delete(count);
	return str;

cleanup:

	free(copy);
	subtilis_exp_delete(count);
	subtilis_exp_delete(str);

	return NULL;
}

static subtilis_exp_t *prv_string_from_const_char(subtilis_parser_t *p,
						  subtilis_exp_t *count,
						  subtilis_exp_t *str,
						  subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	size_t dest_reg;
	size_t val_reg;
	size_t str_reg;
	subtilis_ir_operand_t op1;

	op1.integer = (int32_t)subtilis_buffer_get_string(&str->exp.str)[0];
	val_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	count = subtilis_type_if_exp_to_var(p, count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_reg = subtilis_reference_type_alloc(
	    p, &subtilis_type_string, s->loc, SUBTILIS_IR_REG_LOCAL,
	    count->exp.ir_op.reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_builtin_memset_i8(p, dest_reg, count->exp.ir_op.reg, val_reg,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						 s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(count);
	subtilis_exp_delete(str);

	return subtilis_exp_new_var(&subtilis_type_string, str_reg, err);

cleanup:

	subtilis_exp_delete(count);
	subtilis_exp_delete(str);

	return NULL;
}

/* clang-format off */
static void prv_string_str_non_const_common(subtilis_parser_t *p,
					    const subtilis_symbol_t *s,
					    subtilis_exp_t *count,
					    size_t str_data,
					    subtilis_exp_t *str_len,
					    bool check_for_one,
					    subtilis_error_t *err)
/* clang-format on */
{
	subtilis_ir_operand_t lt_one;
	subtilis_ir_operand_t gte_one;
	subtilis_ir_operand_t end;
	subtilis_ir_operand_t memcpy_loop;
	size_t dest_reg;
	size_t val_reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t end_ptr;
	subtilis_ir_operand_t zero_reg;
	subtilis_ir_operand_t eq_one_label;
	subtilis_ir_operand_t not_eq_one_label;
	subtilis_exp_t *dup = NULL;
	subtilis_exp_t *one = NULL;

	not_eq_one_label.label = SIZE_MAX;
	eq_one_label.label = SIZE_MAX;

	lt_one.label = subtilis_ir_section_new_label(p->current);
	gte_one.label = subtilis_ir_section_new_label(p->current);
	if (check_for_one) {
		eq_one_label.label = subtilis_ir_section_new_label(p->current);
		not_eq_one_label.label =
		    subtilis_ir_section_new_label(p->current);
	}
	memcpy_loop.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);

	one = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dup = subtilis_type_if_dup(count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dup = subtilis_type_if_lt(p, dup, one, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	one = NULL;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  dup->exp.ir_op, lt_one, gte_one, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_exp_delete(dup);
	dup = NULL;

	subtilis_ir_section_add_label(p->current, lt_one.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	zero_reg.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 zero_reg.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, gte_one.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dup = subtilis_type_if_dup(str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	count = subtilis_type_if_mul(p, count, dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	dup = NULL;

	dest_reg = subtilis_reference_type_alloc(
	    p, &subtilis_type_string, s->loc, SUBTILIS_IR_REG_LOCAL,
	    count->exp.ir_op.reg, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (check_for_one && str_len->type.type == SUBTILIS_TYPE_INTEGER) {
		op2.integer = 1;
		op1.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_EQI_I32, str_len->exp.ir_op,
		    op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, op1, eq_one_label,
		    not_eq_one_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, eq_one_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op1.reg = str_data;
		op2.integer = 0;
		val_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I8, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_builtin_memset_i8(p, dest_reg, count->exp.ir_op.reg,
					   val_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current,
					      not_eq_one_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = dest_reg;
	end_ptr.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op1, count->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_len = subtilis_type_if_exp_to_var(p, str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, memcpy_loop.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_memcpy_dest(p, dest_reg, str_data,
					    str_len->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = dest_reg;
	op2.reg =
	    subtilis_ir_section_add_instr(p->current, SUBTILIS_OP_INSTR_ADD_I32,
					  op1, str_len->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = dest_reg;
	op2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQ_I32, op1, end_ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op2, end, memcpy_loop, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(count);
	subtilis_exp_delete(str_len);

	return;

cleanup:

	subtilis_exp_delete(one);
	subtilis_exp_delete(dup);
	subtilis_exp_delete(count);
	subtilis_exp_delete(str_len);
}

static subtilis_exp_t *prv_string_str_non_const_lengt1(subtilis_parser_t *p,
						       subtilis_exp_t *count,
						       size_t str_data,
						       subtilis_exp_t *str_len,
						       subtilis_error_t *err)
{
	const subtilis_symbol_t *s;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_string_str_non_const_common(p, s, count, str_data, str_len, false,
					err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	str_data = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						  s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, str_data, err);

cleanup:

	subtilis_exp_delete(count);
	subtilis_exp_delete(str_len);

	return NULL;
}

static subtilis_exp_t *prv_string_str_non_const(subtilis_parser_t *p,
						subtilis_exp_t *count,
						subtilis_exp_t *str,
						subtilis_error_t *err)
{
	subtilis_ir_operand_t zero_label;
	subtilis_ir_operand_t gt_zero_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t zero_reg;
	size_t size_reg;
	const subtilis_symbol_t *s;
	size_t str_data;
	subtilis_exp_t *str_len;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &subtilis_type_string,
					     NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero_label.label = subtilis_ir_section_new_label(p->current);
	gt_zero_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	op1.reg = str->exp.ir_op.reg;
	op2.integer = 0;
	size_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, gt_zero_label, zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	zero_reg.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					 zero_reg.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, gt_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_data = subtilis_reference_get_data(p, str->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(str);
	str = NULL;

	str_len = subtilis_exp_new_int32_var(size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_string_str_non_const_common(p, s, count, str_data, str_len, true,
					err);
	count = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_ir_section_add_label(p->current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_data = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						  s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&subtilis_type_string, str_data, err);

cleanup:

	subtilis_exp_delete(count);
	subtilis_exp_delete(str);

	return NULL;
}

subtilis_exp_t *subtilis_string_type_string(subtilis_parser_t *p,
					    subtilis_exp_t *count,
					    subtilis_exp_t *str,
					    subtilis_error_t *err)
{
	size_t buf_size;
	size_t ptr;
	const char *cstr;
	subtilis_exp_t *str_len;
	subtilis_exp_t *ret;
	subtilis_buffer_t dummy;

	switch (str->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		buf_size = subtilis_buffer_get_size(&str->exp.str);
		if (buf_size == 1) {
			subtilis_exp_delete(count);
			return str;
		}
		if (count->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			return prv_string_str_const(p, count, str, err);

		buf_size--;

		if (buf_size == 1)
			return prv_string_from_const_char(p, count, str, err);

		cstr = subtilis_buffer_get_string(&str->exp.str);
		ptr = subtilis_string_type_lca_const(p, cstr, buf_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		str_len = subtilis_exp_new_int32(buf_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		ret = prv_string_str_non_const_lengt1(p, count, ptr, str_len,
						      err);
		subtilis_exp_delete(str);
		return ret;
	case SUBTILIS_TYPE_STRING:
		if (count->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			if (count->exp.ir_op.integer == 1) {
				subtilis_exp_delete(count);
				return str;
			} else if (count->exp.ir_op.integer <= 1) {
				subtilis_buffer_init(&dummy, 128);
				subtilis_buffer_zero_terminate(&dummy, err);
				if (err->type != SUBTILIS_ERROR_OK) {
					subtilis_buffer_free(&dummy);
					goto cleanup;
				}
				subtilis_exp_delete(count);
				subtilis_exp_delete(str);
				str = subtilis_exp_new_str(&dummy, err);
				subtilis_buffer_free(&dummy);
				return str;
			}
			count = subtilis_type_if_exp_to_var(p, count, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}

		return prv_string_str_non_const(p, count, str, err);

	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		break;
	}

cleanup:

	subtilis_exp_delete(count);
	subtilis_exp_delete(str);

	return NULL;
}

static bool prv_get_string_details(subtilis_parser_t *p, subtilis_exp_t *a2,
				   subtilis_ir_operand_t *a2_size,
				   subtilis_ir_operand_t *a2_data,
				   char **a2_tmp, subtilis_error_t *err)
{
	size_t len;
	bool retval = true;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		len = subtilis_buffer_get_size(&a2->exp.str) - 1;
		if (len == 0) {
			retval = false;
			goto cleanup;
		}

		op1.integer = len;
		a2_size->reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		*a2_tmp = a2->temporary;
		a2_data->reg = subtilis_string_type_lca_const(
		    p, subtilis_buffer_get_string(&a2->exp.str), len, err);
		break;
	case SUBTILIS_TYPE_STRING:
		op2.integer = SUBTIILIS_STRING_SIZE_OFF;
		a2_size->reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, a2->exp.ir_op, op2,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		*a2_tmp = a2->temporary;
		a2_data->reg =
		    subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
		break;
	default:
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
	}

cleanup:

	subtilis_exp_delete(a2);

	return retval;
}

subtilis_exp_t *subtilis_string_type_add(subtilis_parser_t *p,
					 subtilis_exp_t *a1, subtilis_exp_t *a2,
					 bool swapped, subtilis_error_t *err)
{
	subtilis_ir_operand_t a1_size;
	subtilis_ir_operand_t a2_size;
	subtilis_ir_operand_t new_size;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t a1_data;
	subtilis_ir_operand_t a2_data;
	const subtilis_symbol_t *s;
	size_t tmp;
	bool a2_gt_0;
	char *a1_tmp = NULL;
	char *a2_tmp = NULL;
	size_t dest_reg = SIZE_MAX;
	char *tmp_name = NULL;

	a2_data.reg = SIZE_MAX;
	a2_size.reg = SIZE_MAX;

	op2.integer = SUBTIILIS_STRING_SIZE_OFF;
	a1_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, a1->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_data.reg = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_gt_0 =
	    prv_get_string_details(p, a2, &a2_size, &a2_data, &a2_tmp, err);
	a2 = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!a2_gt_0)
		return a1;

	new_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a1_size, a2_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (swapped) {
		tmp = a1_size.reg;
		a1_size.reg = a2_size.reg;
		a2_size.reg = tmp;
		tmp = a1_data.reg;
		a1_data.reg = a2_data.reg;
		a2_data.reg = tmp;
		a1_tmp = a2_tmp;
	} else {
		a1_tmp = a1->temporary;
	}

	if (a1_tmp) {
		s = subtilis_symbol_table_lookup(p->local_st, a1_tmp);
		if (!s) {
			subtilis_error_set_assertion_failed(err);
			goto cleanup;
		}
		dest_reg = subtilis_reference_type_realloc(
		    p, s->loc, SUBTILIS_IR_REG_LOCAL, a1_data.reg, a1_size.reg,
		    new_size.reg, a2_size.reg, err);
	} else {
		s = subtilis_symbol_table_insert_tmp(
		    p->local_st, &subtilis_type_string, &tmp_name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		a1->temporary = tmp_name;

		dest_reg = subtilis_reference_type_alloc(
		    p, &subtilis_type_string, s->loc, SUBTILIS_IR_REG_LOCAL,
		    new_size.reg, true, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_reference_type_memcpy_dest(p, dest_reg, a1_data.reg,
						    a1_size.reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = dest_reg;
	dest_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op1, a1_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_memcpy_dest(p, dest_reg, a2_data.reg,
					    a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1->exp.ir_op.reg = subtilis_reference_get_pointer(
	    p, SUBTILIS_IR_REG_LOCAL, s->loc, err);

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return a1;

cleanup:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

void subtilis_string_type_add_eq(subtilis_parser_t *p, size_t store_reg,
				 size_t loc, subtilis_exp_t *a2,
				 subtilis_error_t *err)
{
	size_t dest_reg;
	char *a2_tmp;
	subtilis_ir_operand_t a1_size;
	subtilis_ir_operand_t a2_size;
	subtilis_ir_operand_t new_size;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t a1_data;
	subtilis_ir_operand_t a2_data;
	subtilis_ir_operand_t store;
	subtilis_ir_operand_t ref_count;
	bool a2_gt_0;
	subtilis_ir_operand_t a1_gt_zero_label;
	subtilis_ir_operand_t malloc_label;
	subtilis_ir_operand_t realloc_label;
	subtilis_ir_operand_t copy_label;
	subtilis_ir_operand_t ptr;

	a2_data.reg = SIZE_MAX;
	a2_size.reg = SIZE_MAX;
	ptr.reg = p->current->reg_counter++;

	a1_gt_zero_label.label = subtilis_ir_section_new_label(p->current);
	malloc_label.label = subtilis_ir_section_new_label(p->current);
	realloc_label.label = subtilis_ir_section_new_label(p->current);
	copy_label.label = subtilis_ir_section_new_label(p->current);

	store.reg = store_reg;
	a2_gt_0 =
	    prv_get_string_details(p, a2, &a2_size, &a2_data, &a2_tmp, err);
	if (err->type != SUBTILIS_ERROR_OK || !a2_gt_0)
		return;

	op2.integer = loc + SUBTIILIS_STRING_SIZE_OFF;
	a1_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, store, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	new_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a1_size, a2_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  a1_size, a1_gt_zero_label,
					  malloc_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, a1_gt_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = loc + SUBTIILIS_STRING_DATA_OFF;
	a1_data.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, store, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * If our string only has one reference we can realloc it.  Otherwise
	 * we need to malloc and copy.
	 */

	ref_count.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_GETREF, a1_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = 1;
	ref_count.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, ref_count, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  ref_count, realloc_label,
					  malloc_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, malloc_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest_reg = subtilis_reference_type_re_malloc(p, store_reg, loc,
						     a1_data.reg, a1_size.reg,
						     new_size.reg, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     copy_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, realloc_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest_reg = subtilis_reference_type_realloc(
	    p, loc, store_reg, a1_data.reg, a1_size.reg, new_size.reg,
	    a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, copy_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, ptr, a1_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_memcpy_dest(p, dest_reg, a2_data.reg,
					    a2_size.reg, err);
}
