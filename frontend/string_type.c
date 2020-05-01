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

void subtilis_string_type_set_size(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, size_t size_reg,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op0.reg = size_reg;
	op1.reg = mem_reg;
	op2.integer = loc + SUBTIILIS_STRING_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);
}

void subtilis_string_type_zero_ref(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t mem_reg,
				   size_t loc, subtilis_error_t *err)
{
	subtilis_exp_t *zero = NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_set_size(p, mem_reg, loc, zero->exp.ir_op.reg,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_push_reference(p, type, mem_reg, loc, err);

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

	subtilis_string_type_set_size(p, mem_reg, loc, size_reg, err);
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
					      loc, err);
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
				     args, &subtilis_type_integer, 3, err);
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
				     args, &subtilis_type_integer, 4, err);
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

	subtilis_string_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
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

subtilis_exp_t *subtilis_string_type_left(subtilis_parser_t *p,
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

	subtilis_string_type_set_size(p, SUBTILIS_IR_REG_LOCAL, s->loc,
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

subtilis_exp_t *subtilis_string_type_right(subtilis_parser_t *p,
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
