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
#include "string_type_if.h"
#include "symbol_table.h"

static subtilis_exp_t *prv_exp_to_var_const(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t reg;
	const subtilis_symbol_t *s;
	subtilis_type_t type;

	type.type = SUBTILIS_TYPE_STRING;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &type, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_new_ref(p, &e->type, SUBTILIS_IR_REG_LOCAL, s->loc,
				     e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&s->t, reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

static subtilis_exp_t *prv_coerce_type_const(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     const subtilis_type_t *type,
					     subtilis_error_t *err)
{
	if (type->type != SUBTILIS_TYPE_STRING) {
		subtilis_error_set_bad_conversion(
		    err, subtilis_type_name(&e->type), subtilis_type_name(type),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return prv_exp_to_var_const(p, e, err);
}

static subtilis_exp_t *prv_add_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	size_t len;

	len = subtilis_buffer_get_size(&a2->exp.str);
	subtilis_buffer_remove_terminator(&a1->exp.str);
	subtilis_buffer_append(&a1->exp.str, a2->exp.str.buffer->data, len,
			       err);
	subtilis_exp_delete(a2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}
	return a1;
}

static void prv_assign_to_mem_const(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	subtilis_string_type_assign_ref(p, &subtilis_type_const_string, mem_reg,
					loc, e, err);
}

static void prv_dup_const(subtilis_exp_t *e1, subtilis_exp_t *e2,
			  subtilis_error_t *err)
{
	subtilis_buffer_init(&e2->exp.str, e1->exp.str.granularity);
	subtilis_buffer_append(&e2->exp.str,
			       subtilis_buffer_get_string(&e1->exp.str),
			       subtilis_buffer_get_size(&e1->exp.str), err);
}

static subtilis_exp_t *prv_eq_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	int32_t res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str)) == 0
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_neq_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	int32_t res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str))
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_gt_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, bool swapped,
				    subtilis_error_t *err)
{
	int res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str)) > 0
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_lte_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	int res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str)) <= 0
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_lt_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, bool swapped,
				    subtilis_error_t *err)
{
	int res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str)) < 0
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

static subtilis_exp_t *prv_gte_const(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, bool swapped,
				     subtilis_error_t *err)
{
	int res;

	/* a2 must be const */

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		res = strcmp(subtilis_buffer_get_string(&a1->exp.str),
			     subtilis_buffer_get_string(&a2->exp.str)) >= 0
			  ? -1
			  : 0;
		subtilis_buffer_free(&a1->exp.str);
		a1->exp.ir_op.integer = res;
		a1->type.type = SUBTILIS_TYPE_CONST_INTEGER;
		break;
	default:
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		return NULL;
	}

	subtilis_exp_delete(a2);
	return a1;
}

/* clang-format off */
subtilis_type_if subtilis_type_if_const_string = {
	.is_const = true,
	.is_numeric = false,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = NULL,
	.data_size = NULL,
	.zero = NULL,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.zero_reg = NULL,
	.copy_ret = NULL,
	.array_of = NULL,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var_const,
	.copy_var = NULL,
	.dup = prv_dup_const,
	.assign_reg = NULL,
	.assign_mem = prv_assign_to_mem_const,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.load_mem = NULL,
	.to_int32 = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.coerce = prv_coerce_type_const,
	.unary_minus = NULL,
	.add = prv_add_const,
	.mul = NULL,
	.and = NULL,
	.or = NULL,
	.eor = NULL,
	.not = NULL,
	.eq = prv_eq_const,
	.neq = prv_neq_const,
	.sub = NULL,
	.div = NULL,
	.mod = NULL,
	.gt = prv_gt_const,
	.lte = prv_lte_const,
	.lt = prv_lt_const,
	.gte = prv_gte_const,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = NULL,
	.call = NULL,
	.ret = NULL,
	.print = subtilis_string_type_print_const,
	.destructor = NULL,
};

/* clang-format on */

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_zero_reg(subtilis_parser_t *p, size_t reg,
			 subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = reg;
	op1.integer = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, op1, err);
}

static subtilis_exp_t *prv_call(subtilis_parser_t *p,
				const subtilis_type_t *type,
				subtilis_ir_arg_t *args, size_t num_args,
				subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_i32_call(p->current, num_args, args, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(type, reg, err);
}

static void prv_ret(subtilis_parser_t *p, size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t ret_reg;

	ret_reg.reg = reg;
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_RET_I32, ret_reg, err);
}

static void prv_array_of(const subtilis_type_t *element_type,
			 subtilis_type_t *type)
{
	type->type = SUBTILIS_TYPE_ARRAY_STRING;
}

static subtilis_exp_t *prv_load_from_mem(subtilis_parser_t *p, size_t mem_reg,
					 size_t loc, subtilis_error_t *err)
{
	size_t reg;

	if (loc > 0) {
		reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	} else {
		reg = mem_reg;
	}

	return subtilis_exp_new_var(&subtilis_type_string, reg, err);
}

static void prv_assign_to_mem(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_string_type_assign_ref(p, &subtilis_type_string, mem_reg, loc,
					e, err);
}

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static subtilis_exp_t *prv_compare_fixed_len(subtilis_parser_t *p, size_t a1,
					     size_t a2,
					     subtilis_op_instr_type_t iload,
					     subtilis_op_instr_type_t icmp,
					     subtilis_error_t *err)
{
	subtilis_ir_operand_t a1_ptr;
	subtilis_ir_operand_t a2_ptr;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t op0;

	offset.integer = 0;
	op0.reg = a1;
	a1_ptr.reg =
	    subtilis_ir_section_add_instr(p->current, iload, op0, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op0.reg = a2;
	a2_ptr.reg =
	    subtilis_ir_section_add_instr(p->current, iload, op0, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	a1 = subtilis_ir_section_add_instr(p->current, icmp, a1_ptr, a2_ptr,
					   err);
	return subtilis_exp_new_var(&subtilis_type_integer, a1, err);
}

static subtilis_exp_t *prv_eq_non_const_const(subtilis_parser_t *p,
					      subtilis_exp_t *a1,
					      subtilis_exp_t *a2,
					      subtilis_error_t *err)
{
	size_t const_ptr;
	subtilis_ir_operand_t len_eq;
	subtilis_ir_operand_t len_neq;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;
	size_t a2_len;
	size_t a1_ptr;
	size_t dest_reg = p->current->reg_counter++;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *eq = NULL;

	len_eq.label = subtilis_ir_section_new_label(p->current);
	len_neq.label = subtilis_ir_section_new_label(p->current);

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	op0.reg = dest_reg;
	a2_len = subtilis_buffer_get_size(&a2->exp.str) - 1;
	op2.integer = (int32_t)a2_len;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_EQI_I32,
					  op0, len1->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, len_eq, len_neq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, len_eq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	const_ptr = subtilis_string_type_lca_const(
	    p, subtilis_buffer_get_string(&a2->exp.str), op2.integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (a2_len == 1)
		eq = prv_compare_fixed_len(p, a1_ptr, const_ptr,
					   SUBTILIS_OP_INSTR_LOADO_I8,
					   SUBTILIS_OP_INSTR_EQI_I32, err);
	else if (a2_len == 4)
		eq = prv_compare_fixed_len(p, a1_ptr, const_ptr,
					   SUBTILIS_OP_INSTR_LOADO_I32,
					   SUBTILIS_OP_INSTR_EQI_I32, err);
	else
		eq = subtilis_string_type_eq(p, a1_ptr, const_ptr,
					     len1->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, eq->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, len_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1->exp.ir_op.reg = dest_reg;
	a1->type.type = SUBTILIS_TYPE_INTEGER;

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a2);
	return a1;

cleanup:

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_eq_non_const_non_const(subtilis_parser_t *p,
						  subtilis_exp_t *a1,
						  subtilis_exp_t *a2,
						  subtilis_error_t *err)
{
	subtilis_ir_operand_t len_eq;
	subtilis_ir_operand_t len_neq;
	subtilis_ir_operand_t ptr_neq;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t a1_ptr;
	size_t a2_ptr;
	size_t dest_reg = p->current->reg_counter++;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *len2 = NULL;
	subtilis_exp_t *eq = NULL;

	len_eq.label = subtilis_ir_section_new_label(p->current);
	len_neq.label = subtilis_ir_section_new_label(p->current);
	ptr_neq.label = subtilis_ir_section_new_label(p->current);

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	len2 = subtilis_type_if_dup(a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len2 = subtilis_string_type_len(p, len2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = dest_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_EQ_I32,
					  op0, len1->exp.ir_op, len2->exp.ir_op,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, len_eq, len_neq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, len_eq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_ptr = subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = dest_reg;
	op1.reg = a1_ptr;
	op2.reg = a2_ptr;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_EQ_I32,
					  op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, len_neq, ptr_neq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, ptr_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eq = subtilis_string_type_eq(p, a1_ptr, a2_ptr, len1->exp.ir_op.reg,
				     err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, eq->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, len_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1->exp.ir_op.reg = dest_reg;
	a1->type.type = SUBTILIS_TYPE_INTEGER;

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(len2);
	subtilis_exp_delete(a2);
	return a1;

cleanup:

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(len2);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		return prv_eq_non_const_const(p, a1, a2, err);
	case SUBTILIS_TYPE_STRING:
		return prv_eq_non_const_non_const(p, a1, a2, err);
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
	}

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_neq_non_const_const(subtilis_parser_t *p,
					       subtilis_exp_t *a1,
					       subtilis_exp_t *a2,
					       subtilis_error_t *err)
{
	size_t const_ptr;
	subtilis_ir_operand_t len_eq;
	subtilis_ir_operand_t len_neq;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;
	size_t a2_len;
	size_t a1_ptr;
	size_t dest_reg = p->current->reg_counter++;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *eq = NULL;

	len_eq.label = subtilis_ir_section_new_label(p->current);
	len_neq.label = subtilis_ir_section_new_label(p->current);

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	op0.reg = dest_reg;
	a2_len = subtilis_buffer_get_size(&a2->exp.str) - 1;
	op2.integer = a2_len;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_NEQI_I32, op0,
					  len1->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, len_neq, len_eq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, len_eq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	const_ptr = subtilis_string_type_lca_const(
	    p, subtilis_buffer_get_string(&a2->exp.str), op2.integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (a2_len == 1)
		eq = prv_compare_fixed_len(p, a1_ptr, const_ptr,
					   SUBTILIS_OP_INSTR_LOADO_I8,
					   SUBTILIS_OP_INSTR_EQI_I32, err);
	else if (a2_len == 4)
		eq = prv_compare_fixed_len(p, a1_ptr, const_ptr,
					   SUBTILIS_OP_INSTR_LOADO_I32,
					   SUBTILIS_OP_INSTR_EQI_I32, err);
	else
		eq = subtilis_string_type_eq(p, a1_ptr, const_ptr,
					     len1->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_NOT_I32, op0, eq->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, len_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1->exp.ir_op.reg = dest_reg;
	a1->type.type = SUBTILIS_TYPE_INTEGER;

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a2);
	return a1;

cleanup:

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_neq_non_const_non_const(subtilis_parser_t *p,
						   subtilis_exp_t *a1,
						   subtilis_exp_t *a2,
						   subtilis_error_t *err)
{
	subtilis_ir_operand_t len_eq;
	subtilis_ir_operand_t len_neq;
	subtilis_ir_operand_t ptr_neq;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t a1_ptr;
	size_t a2_ptr;
	size_t dest_reg = p->current->reg_counter++;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *len2 = NULL;
	subtilis_exp_t *eq = NULL;

	len_eq.label = subtilis_ir_section_new_label(p->current);
	len_neq.label = subtilis_ir_section_new_label(p->current);
	ptr_neq.label = subtilis_ir_section_new_label(p->current);

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	len2 = subtilis_type_if_dup(a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len2 = subtilis_string_type_len(p, len2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = dest_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_NEQ_I32,
					  op0, len1->exp.ir_op, len2->exp.ir_op,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, len_neq, len_eq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, len_eq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_ptr = subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = dest_reg;
	op1.reg = a1_ptr;
	op2.reg = a2_ptr;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_NEQ_I32,
					  op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  op0, ptr_neq, len_neq, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, ptr_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eq = subtilis_string_type_eq(p, a1_ptr, a2_ptr, len1->exp.ir_op.reg,
				     err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_NOT_I32, op0, eq->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, len_neq.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1->exp.ir_op.reg = dest_reg;
	a1->type.type = SUBTILIS_TYPE_INTEGER;

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(len2);
	subtilis_exp_delete(a2);
	return a1;

cleanup:

	subtilis_exp_delete(eq);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(len2);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		return prv_neq_non_const_const(p, a1, a2, err);
	case SUBTILIS_TYPE_STRING:
		return prv_neq_non_const_non_const(p, a1, a2, err);
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
	}

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_compare_non_const_const(subtilis_parser_t *p,
						   subtilis_exp_t *a1,
						   subtilis_exp_t *a2,
						   bool swapped,
						   subtilis_error_t *err)
{
	size_t tmp_ptr;
	size_t a1_ptr;
	size_t a2_ptr;
	subtilis_ir_operand_t op2;
	subtilis_exp_t *tmp;
	int32_t a2_len;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *len2 = NULL;
	subtilis_exp_t *res = NULL;

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_len = subtilis_buffer_get_size(&a2->exp.str) - 1;
	len2 = subtilis_exp_new_int32(a2_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	len2 = subtilis_type_if_exp_to_var(p, len2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = a2_len;
	a2_ptr = subtilis_string_type_lca_const(
	    p, subtilis_buffer_get_string(&a2->exp.str), op2.integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (swapped) {
		tmp_ptr = a1_ptr;
		a1_ptr = a2_ptr;
		a2_ptr = tmp_ptr;
		tmp = len1;
		len1 = len2;
		len2 = tmp;
	}

	res = subtilis_string_type_compare(p, a1_ptr, len1->exp.ir_op.reg,
					   a2_ptr, len2->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(len2);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);

	return res;
}

static subtilis_exp_t *prv_compare_non_const_non_const(subtilis_parser_t *p,
						       subtilis_exp_t *a1,
						       subtilis_exp_t *a2,
						       subtilis_error_t *err)
{
	size_t a1_ptr;
	size_t a2_ptr;
	subtilis_exp_t *len1 = NULL;
	subtilis_exp_t *len2 = NULL;
	subtilis_exp_t *res = NULL;

	len1 = subtilis_type_if_dup(a1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len1 = subtilis_string_type_len(p, len1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	len2 = subtilis_type_if_dup(a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	len2 = subtilis_string_type_len(p, len2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_ptr = subtilis_reference_get_data(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_ptr = subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	res = subtilis_string_type_compare(p, a1_ptr, len1->exp.ir_op.reg,
					   a2_ptr, len2->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(len2);
	subtilis_exp_delete(len1);
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);

	return res;
}

static subtilis_exp_t *prv_compare(subtilis_parser_t *p, subtilis_exp_t *a1,
				   subtilis_exp_t *a2, bool swapped,
				   subtilis_op_instr_type_t itype,
				   subtilis_error_t *err)
{
	subtilis_exp_t *zero;
	size_t res;

	switch (a2->type.type) {
	case SUBTILIS_TYPE_CONST_STRING:
		a1 = prv_compare_non_const_const(p, a1, a2, swapped, err);
		break;
	case SUBTILIS_TYPE_STRING:
		a1 = prv_compare_non_const_non_const(p, a1, a2, err);
		break;
	default:
		subtilis_error_set_bad_expression(err, p->l->stream->name,
						  p->l->line);
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		return NULL;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	res = subtilis_ir_section_add_instr(p->current, itype, a1->exp.ir_op,
					    zero->exp.ir_op, err);
	subtilis_exp_delete(a1);
	subtilis_exp_delete(zero);

	return subtilis_exp_new_int32_var(res, err);
}

static subtilis_exp_t *prv_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, swapped, SUBTILIS_OP_INSTR_GTI_I32, err);
}

static subtilis_exp_t *prv_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, swapped, SUBTILIS_OP_INSTR_LTEI_I32, err);
}

static subtilis_exp_t *prv_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, swapped, SUBTILIS_OP_INSTR_LTI_I32, err);
}

static subtilis_exp_t *prv_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	return prv_compare(p, a1, a2, swapped, SUBTILIS_OP_INSTR_GTEI_I32, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_if_string = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = subtilis_string_type_size,
	.data_size = prv_data_size,
	.zero = NULL,
	.zero_ref = subtilis_string_type_zero_ref,
	.new_ref = subtilis_string_type_new_ref,
	.assign_ref = subtilis_string_type_assign_ref,
	.zero_reg = prv_zero_reg,
	.copy_ret = subtilis_reference_type_copy_ret,
	.array_of = prv_array_of,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = prv_dup,
	.assign_reg = subtilis_string_type_assign_to_reg,
	.assign_mem = prv_assign_to_mem,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.load_mem = prv_load_from_mem,
	.to_int32 = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.unary_minus = NULL,
	.add = subtilis_string_type_add,
	.mul = NULL,
	.and = NULL,
	.or = NULL,
	.eor = NULL,
	.not = NULL,
	.eq = prv_eq,
	.neq = prv_neq,
	.sub = NULL,
	.div = NULL,
	.mod = NULL,
	.gt = prv_gt,
	.lte = prv_lte,
	.lt = prv_lt,
	.gte = prv_gte,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = NULL,
	.call = prv_call,
	.ret = prv_ret,
	.print = subtilis_string_type_print,
	.destructor = NULL,
};

/* clang-format on */
