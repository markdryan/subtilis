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
	uint8_t *buf;
	subtilis_ir_operand_t op1;
	size_t dest_reg;
	subtilis_exp_t *sizee = NULL;
	size_t buf_size = subtilis_buffer_get_size(str);

	if (buf_size == 1) {
		subtilis_string_type_zero_ref(p, NULL, mem_reg, loc, err);
		return;
	}

	buf_size--;

	buf = malloc(buf_size);
	if (!buf) {
		subtilis_error_set_oom(err);
		return;
	}
	memcpy(buf, subtilis_buffer_get_string(str), buf_size);

	id = subtilis_constant_pool_add(p->prog->constant_pool, buf, buf_size,
					false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	buf = NULL;

	sizee = subtilis_exp_new_int32((int32_t)buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

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
	free(buf);
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
