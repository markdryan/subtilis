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
 */

#define SUBTIILIS_STRING_SIZE_OFF SUBTIILIS_REFERENCE_SIZE_OFF
#define SUBTIILIS_STRING_DATA_OFF SUBTIILIS_REFERENCE_DATA_OFF

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
	 */

	size = sizeof(int32_t) + sizeof(int32_t);

	return size;
}

void subtilis_string_type_zero_ref(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t mem_reg,
				   size_t loc, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_exp_t *zero = NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = mem_reg, op2.integer = loc + SUBTIILIS_STRING_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  zero->exp.ir_op, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_push_reference(p, mem_reg, loc, err);

cleanup:

	subtilis_exp_delete(zero);
}

static void prv_init_string_from_const(subtilis_parser_t *p, size_t mem_reg,
				       size_t loc, const subtilis_buffer_t *str,
				       bool push, subtilis_error_t *err)
{
	size_t id;
	size_t src_reg;
	subtilis_ir_operand_t op1;
	size_t dest_reg;
	subtilis_exp_t *sizee = NULL;
	size_t buf_size = subtilis_buffer_get_size(str);

	if (buf_size == 1) {
		subtilis_string_type_zero_ref(p, NULL, mem_reg, loc, err);
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

	dest_reg = subtilis_reference_type_alloc(
	    p, loc, mem_reg, sizee->exp.ir_op.reg, push, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_memcpy_dest(p, dest_reg, src_reg,
					    sizee->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(sizee);
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

	dest_reg = subtilis_reference_type_alloc(
	    p, loc, mem_reg, sizee->exp.ir_op.reg, true, err);
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

void subtilis_string_type_new_ref(subtilis_parser_t *p,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_error_t *err)
{
	switch (e->type.type) {
	case SUBTILIS_TYPE_STRING:
		subtilis_reference_type_new_ref(p, mem_reg, loc,
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
	subtilis_reference_type_deref(p, mem_reg, loc, true, err);
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
