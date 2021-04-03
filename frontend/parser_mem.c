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

#include <string.h>

#include "parser_exp.h"
#include "parser_mem.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_mem_heap_free(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_HEAP_FREE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

void subtilis_parser_copy(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t size1;
	subtilis_ir_operand_t size2;
	subtilis_ir_operand_t skip_label;
	subtilis_ir_operand_t no_skip_label;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t final_size;
	size_t ptr1;
	size_t ptr2;
	size_t buf_size;
	subtilis_exp_t *obj1 = NULL;
	subtilis_exp_t *obj2 = NULL;

	skip_label.label = subtilis_ir_section_new_label(p->current);
	no_skip_label.label = subtilis_ir_section_new_label(p->current);

	obj1 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!subtilis_type_if_is_scalar_ref(&obj1->type)) {
		subtilis_error_set_expected(err,
					    "string or array of numeric types",
					    subtilis_type_name(&obj1->type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	obj2 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (obj2->type.type == SUBTILIS_TYPE_CONST_STRING) {
		buf_size = subtilis_buffer_get_size(&obj2->exp.str);
		if (buf_size < 2)
			goto cleanup;
		buf_size--;
		ptr2 = subtilis_string_type_lca_const(
		    p, subtilis_buffer_get_string(&obj2->exp.str), buf_size,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		op1.integer = buf_size;
		size2.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	} else if (subtilis_type_if_is_scalar_ref(&obj2->type)) {
		size2.reg = subtilis_reference_type_get_size(
		    p, obj2->exp.ir_op.reg, 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		ptr2 =
		    subtilis_reference_get_data(p, obj2->exp.ir_op.reg, 0, err);
	} else {
		subtilis_error_set_expected(
		    err, "string, constant string or array of numeric types",
		    subtilis_type_name(&obj2->type), p->l->stream->name,
		    p->l->line);
		goto cleanup;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size1.reg =
	    subtilis_reference_type_get_size(p, obj1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTE_I32, size1, size2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	final_size.reg = p->current->reg_counter++;
	subtilis_ir_section_add_instr4(p->current, SUBTILIS_OP_INSTR_CMOV_I32,
				       final_size, condee, size1, size2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  final_size, no_skip_label, skip_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, no_skip_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (obj1->type.type == SUBTILIS_TYPE_STRING)
		ptr1 = subtilis_reference_type_copy_on_write(
		    p, obj1->exp.ir_op.reg, 0, size1.reg, err);
	else
		ptr1 =
		    subtilis_reference_get_data(p, obj1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_memcpy_dest(p, ptr1, ptr2, final_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, skip_label.label, err);

cleanup:

	subtilis_exp_delete(obj2);
	subtilis_exp_delete(obj1);
}
