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

#include <stdlib.h>
#include <string.h>

#include "array_rec_type.h"
#include "array_type.h"
#include "builtins_helper.h"
#include "builtins_ir.h"
#include "collection.h"
#include "rec_type.h"
#include "reference_type.h"

static size_t prv_size(const subtilis_type_t *type)
{
	return subtilis_array_type_size(type);
}

static size_t prv_align(const subtilis_type_t *type)
{
	return SUBTILIS_CONFIG_POINTER_SIZE;
}

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_exp_t *el_size;
	subtilis_type_t typ;
	int32_t data_size;

	typ.type = SUBTILIS_TYPE_VOID;
	subtilis_type_copy_from_rec(&typ, &type->params.array.params.rec, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	data_size = (int32_t)subtilis_type_rec_size(&typ);
	el_size = subtilis_exp_new_int32(data_size, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return subtilis_type_if_mul(p, e, el_size, err);
}

static subtilis_exp_t *prv_zero(subtilis_parser_t *p,
				const subtilis_type_t *type,
				subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "zero on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static void prv_zero_reg(subtilis_parser_t *p, const subtilis_type_t *type,
			 size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = reg;
	op1.integer = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op0, op1, err);
}

static void prv_element_type(const subtilis_type_t *type,
			     subtilis_type_t *element_type,
			     subtilis_error_t *err)
{
	subtilis_type_copy_from_rec(element_type,
				    &type->params.array.params.rec, err);
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_copy_col(subtilis_parser_t *p, subtilis_exp_t *e1,
			 subtilis_exp_t *e2, subtilis_error_t *err)
{
	bool scalar;
	subtilis_type_t typ;

	typ.type = SUBTILIS_TYPE_VOID;
	prv_element_type(&e1->type, &typ, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	scalar = subtilis_type_rec_is_scalar(&typ);
	subtilis_type_free(&typ);

	if (!scalar) {
		subtilis_collection_copy_ref(p, e1, e2, err);
		return;
	}
	subtilis_collection_copy_scalar(p, e1, e2, false, err);
}

static subtilis_exp_t *
prv_indexed_read(subtilis_parser_t *p, const char *var_name,
		 const subtilis_type_t *type, size_t mem_reg, size_t loc,
		 subtilis_exp_t **indices, size_t index_count,
		 subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_array_index_calc(p, var_name, type, mem_reg, loc, indices,
				      index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_type_if_element_type(p, type, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static void prv_set(subtilis_parser_t *p, const char *var_name,
		    const subtilis_type_t *type, size_t mem_reg, size_t loc,
		    subtilis_exp_t *e, bool check_size, subtilis_error_t *err)
{
	subtilis_type_t el_type;
	size_t sizee;
	//	size_t ptr;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t eq_zero;
	subtilis_ir_operand_t op0;

	eq_zero.label = SIZE_MAX;

	el_type.type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_e;

	e = subtilis_type_if_coerce_type(p, e, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_reference_type_get_size(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (check_size) {
		gt_zero.label = subtilis_ir_section_new_label(p->current);
		eq_zero.label = subtilis_ir_section_new_label(p->current);
		op0.reg = sizee;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, op0,
						  gt_zero, eq_zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_label(p->current, gt_zero.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	//	ptr = subtilis_reference_get_data(p, mem_reg, loc, err);
	//	if (err->type != SUBTILIS_ERROR_OK)
	//		goto cleanup;

	subtilis_error_set_assertion_failed(err);
	goto cleanup;

	if (check_size)
		subtilis_ir_section_add_label(p->current, eq_zero.label, err);

cleanup:
	subtilis_type_free(&el_type);

free_e:
	subtilis_exp_delete(e);
}

static void prv_array_set(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_exp_t *e, subtilis_error_t *err)
{
	prv_set(p, var_name, type, mem_reg, loc, e, false, err);
}

static void prv_zero_non_scalar(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t data_reg,
				size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t el_addr;
	subtilis_ir_operand_t last_addr;
	subtilis_ir_operand_t loop_label;
	subtilis_ir_operand_t end_label;

	loop_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	op2.reg = data_reg;
	el_addr.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.reg = size_reg;
	op1.reg = data_reg;
	last_addr.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rec_type_zero(p, type, el_addr.reg, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = (int32_t)subtilis_type_rec_size(type);
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, el_addr, el_addr, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, el_addr, last_addr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, loop_label, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end_label.label, err);
}

static void prv_zero_buf(subtilis_parser_t *p, const subtilis_type_t *type,
			 size_t data_reg, size_t size_reg,
			 subtilis_error_t *err)
{
	subtilis_type_t el_type;

	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (subtilis_type_rec_can_zero_fill(&el_type))
		subtilis_builtin_bzero_reg(p, data_reg, 0, size_reg, err);
	else
		prv_zero_non_scalar(p, &el_type, data_reg, size_reg, err);

	subtilis_type_free(&el_type);
}

static subtilis_exp_t *
prv_indexed_address(subtilis_parser_t *p, const char *var_name,
		    const subtilis_type_t *type, size_t mem_reg, size_t loc,
		    subtilis_exp_t **indices, size_t index_count,
		    subtilis_error_t *err)
{
	return subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					 indices, index_count, err);
}

static void prv_append(subtilis_parser_t *p, subtilis_exp_t *a1,
		       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_type_t el_type;

	if (a2->type.type == SUBTILIS_TYPE_REC) {
		subtilis_array_append_scalar(p, a1, a2, err);
		return;
	}

	prv_element_type(&a1->type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		return;
	}

	if (!subtilis_type_rec_need_deref(&el_type)) {
		subtilis_array_append_scalar_array(p, a1, a2, err);
		return;
	}

	subtilis_array_append_ref_array(p, a1, a2, err);
}

static void prv_indexed_write(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t *e,
			      subtilis_exp_t **indices, size_t index_count,
			      subtilis_error_t *err)
{
	subtilis_type_t rec_type;

	subtilis_type_init_copy_from_rec(&rec_type,
					 &type->params.array.params.rec, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_coerce_type(p, e, &rec_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_type;

	subtilis_array_write(p, var_name, type, &rec_type, mem_reg, loc, e,
			     indices, index_count, err);

free_type:
	subtilis_type_free(&rec_type);

cleanup:
	subtilis_exp_delete(e);
}

static void prv_indexed_add(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "+= on collections of REC",
					 p->l->stream->name, p->l->line);
}

static void prv_indexed_sub(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "-= on collections of REC",
					 p->l->stream->name, p->l->line);
}

static subtilis_exp_t *prv_unary_minus(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "unary - on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "+ on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "* on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "AND on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "OR on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "EOR on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "NOT on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "= on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<> on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "- on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "DIV on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "MOD on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "pow on on collections of RECs",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<< on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">> on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">>> on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "ABS on collections of REC",
					 p->l->stream->name, p->l->line);
	return NULL;
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

static subtilis_exp_t *prv_call_ptr(subtilis_parser_t *p,
				    const subtilis_type_t *type,
				    subtilis_ir_arg_t *args, size_t num_args,
				    size_t ptr, subtilis_error_t *err)
{
	size_t reg;

	reg = subtilis_ir_section_add_i32_call_ptr(p->current, num_args, args,
						   ptr, err);
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

static size_t prv_destructor(subtilis_parser_t *p, const subtilis_type_t *type,
			     subtilis_error_t *err)
{
	subtilis_type_t el_type;
	size_t call_index = SIZE_MAX;

	el_type.type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (!subtilis_type_rec_need_deref(&el_type)) {
		call_index = subtilis_type_if_destruct_deref(p, type, err);
		goto cleanup;
	}

	call_index = subtilis_builtin_ir_deref_array_recs(p, &el_type, err);

cleanup:
	subtilis_type_free(&el_type);

	return call_index;
}

/* clang-format off */
subtilis_type_if subtilis_type_array_rec = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = true,
	.is_vector = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.alignment = prv_align,
	.data_size = prv_data_size,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = subtilis_array_type_assign_ref_exp,
	.assign_ref_no_rc = NULL,
	.zero_reg = prv_zero_reg,
	.copy_ret = subtlis_array_type_copy_ret,
	.array_of = NULL,
	.vector_of = NULL,
	.element_type = prv_element_type,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = subtilis_array_type_dup,
	.copy_col = prv_copy_col,
	.assign_reg = subtilis_array_type_assign_to_reg,
	.assign_mem = NULL,
	.assign_new_mem = NULL,
	.indexed_write = prv_indexed_write,
	.indexed_add = prv_indexed_add,
	.indexed_sub = prv_indexed_sub,
	.indexed_read = prv_indexed_read,
	.append = NULL,
	.set = prv_array_set,
	.zero_buf = prv_zero_buf,
	.indexed_address = prv_indexed_address,
	.load_mem = NULL,
	.to_int32 = NULL,
	.zerox = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.unary_minus = prv_unary_minus,
	.add = prv_add,
	.mul = prv_mul,
	.and = prv_and,
	.or = prv_or,
	.eor = prv_eor,
	.not = prv_not,
	.eq = prv_eq,
	.neq = prv_neq,
	.sub = prv_sub,
	.div = prv_div,
	.mod = prv_mod,
	.gt = NULL,
	.lte = NULL,
	.lt = NULL,
	.gte = NULL,
	.pow = prv_pow,
	.lsl = prv_lsl,
	.lsr = prv_lsr,
	.asr = prv_asr,
	.abs = prv_abs,
	.is_inf = NULL,
	.call = prv_call,
	.call_ptr = prv_call_ptr,
	.ret = prv_ret,
	.destructor = prv_destructor,
	.swap_reg_reg = NULL,
	.swap_reg_mem = NULL,
	.swap_mem_mem = subtilis_array_type_swap,
};

/* clang-format on */

static void prv_vector_set(subtilis_parser_t *p, const char *var_name,
			   const subtilis_type_t *type, size_t mem_reg,
			   size_t loc, subtilis_exp_t *e, subtilis_error_t *err)
{
	prv_set(p, var_name, type, mem_reg, loc, e, true, err);
}

/* clang-format off */

subtilis_type_if subtilis_type_vector_rec = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = false,
	.is_vector = true,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.alignment = prv_align,
	.data_size = prv_data_size,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = subtilis_array_type_assign_ref_exp,
	.assign_ref_no_rc = NULL,
	.zero_reg = prv_zero_reg,
	.copy_ret = subtlis_array_type_copy_ret,
	.array_of = NULL,
	.vector_of = NULL,
	.element_type = prv_element_type,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = subtilis_array_type_dup,
	.copy_col = prv_copy_col,
	.assign_reg = subtilis_array_type_assign_to_reg,
	.assign_mem = NULL,
	.assign_new_mem = NULL,
	.indexed_write = prv_indexed_write,
	.indexed_add = prv_indexed_add,
	.indexed_sub = prv_indexed_sub,
	.indexed_read = prv_indexed_read,
	.append = prv_append,
	.set = prv_vector_set,
	.zero_buf = prv_zero_buf,
	.indexed_address = prv_indexed_address,
	.load_mem = NULL,
	.to_int32 = NULL,
	.zerox = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.unary_minus = prv_unary_minus,
	.add = prv_add,
	.mul = prv_mul,
	.and = prv_and,
	.or = prv_or,
	.eor = prv_eor,
	.not = prv_not,
	.eq = prv_eq,
	.neq = prv_neq,
	.sub = prv_sub,
	.div = prv_div,
	.mod = prv_mod,
	.gt = NULL,
	.lte = NULL,
	.lt = NULL,
	.gte = NULL,
	.pow = prv_pow,
	.lsl = prv_lsl,
	.lsr = prv_lsr,
	.asr = prv_asr,
	.abs = prv_abs,
	.is_inf = NULL,
	.call = prv_call,
	.call_ptr = prv_call_ptr,
	.ret = prv_ret,
	.destructor = prv_destructor,
	.swap_reg_reg = NULL,
	.swap_reg_mem = NULL,
	.swap_mem_mem = subtilis_array_type_swap,
};

/* clang-format on */
