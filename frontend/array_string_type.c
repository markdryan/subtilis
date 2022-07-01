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

#include "array_string_type.h"
#include "array_type.h"
#include "builtins_ir.h"
#include "collection.h"
#include "reference_type.h"
#include "string_type.h"

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
	subtilis_exp_t *sizee;
	size_t string_size = subtilis_string_type_size(&subtilis_type_string);

	sizee = subtilis_exp_new_int32(string_size, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return subtilis_type_if_mul(p, e, sizee, err);
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
	element_type->type = SUBTILIS_TYPE_STRING;
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_copy_col(subtilis_parser_t *p, subtilis_exp_t *e1,
			 subtilis_exp_t *e2, subtilis_error_t *err)
{
	subtilis_collection_copy_ref(p, e1, e2, err);
}

static subtilis_exp_t *
prv_indexed_read(subtilis_parser_t *p, const char *var_name,
		 const subtilis_type_t *type, size_t mem_reg, size_t loc,
		 subtilis_exp_t **indices, size_t index_count,
		 subtilis_error_t *err)
{
	return subtilis_array_read(p, var_name, type, &subtilis_type_string,
				   mem_reg, loc, indices, index_count, err);
}

static void prv_set(subtilis_parser_t *p, const char *var_name,
		    const subtilis_type_t *type, size_t mem_reg, size_t loc,
		    subtilis_exp_t *e, bool check_size, subtilis_error_t *err)
{
	subtilis_ir_operand_t sizee;
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t end_ptr;
	subtilis_ir_operand_t string_size;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t loop_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t condee;
	subtilis_exp_t *e_dup;

	start_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);
	loop_label.label = subtilis_ir_section_new_label(p->current);

	e = subtilis_type_if_coerce_type(p, e, &subtilis_type_string, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee.reg = subtilis_reference_type_get_size(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (check_size) {
		gt_zero.label = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, sizee,
						  gt_zero, end_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, gt_zero.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	ptr.reg = subtilis_reference_get_data(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	string_size.integer = subtilis_type_if_size(&subtilis_type_string, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * TODO: Pointer arithmetic here.
	 */

	end_ptr.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, ptr, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr, end_ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, loop_label, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_assign_ref(p, &subtilis_type_string, ptr.reg, 0,
					e_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr, ptr, string_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);

cleanup:

	subtilis_exp_delete(e);
}

static void prv_array_set(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_exp_t *e, subtilis_error_t *err)
{
	prv_set(p, var_name, type, mem_reg, loc, e, false, err);
}

static void prv_append(subtilis_parser_t *p, subtilis_exp_t *a1,
		       subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a2->type.type == SUBTILIS_TYPE_ARRAY_STRING) ||
	    (a2->type.type == SUBTILIS_TYPE_VECTOR_STRING)) {
		subtilis_array_append_ref_array(p, a1, a2, err);
		return;
	} else if ((a2->type.type == SUBTILIS_TYPE_STRING) ||
		   (a2->type.type == SUBTILIS_TYPE_CONST_STRING)) {
		subtilis_array_append_scalar(p, a1, a2, err);
		return;
	}
	subtilis_error_set_expected(err, "string or array of strings",
				    subtilis_type_name(&a2->type),
				    p->l->stream->name, p->l->line);
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
}

static void prv_indexed_write(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t *e,
			      subtilis_exp_t **indices, size_t index_count,
			      subtilis_error_t *err)
{
	subtilis_array_write(p, var_name, type, &subtilis_type_string, mem_reg,
			     loc, e, indices, index_count, err);
}

static void prv_indexed_add(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_exp_t *offset;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_string_type_add_eq(p, offset->exp.ir_op.reg, 0, e, err);

	subtilis_exp_delete(offset);
}

static void prv_indexed_sub(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "-= on string arrays",
					 p->l->stream->name, p->l->line);
}

static subtilis_exp_t *prv_unary_minus(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "unary - on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "+ on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "* on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "AND on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "OR on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "EOR on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "NOT on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "= on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "- on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "DIV on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "MOD on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "pow on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<< on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">>> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "ABS on arrays",
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
	return subtilis_builtin_ir_deref_array_els(p, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_array_string = {
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
	.zero_ref = subtilis_array_create_1el,
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
	.set = prv_array_set,
	.zero_buf = subtilis_array_zero_buf_i32,
	.append = NULL,
	.indexed_address = NULL,
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

static void prv_vector_zero_ref(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t mem_reg,
				size_t loc, bool push, subtilis_error_t *err)
{
	subtilis_array_type_vector_zero_ref(p, type, loc, mem_reg, push, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_vector_string = {
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
	.zero_ref = prv_vector_zero_ref,
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
	.set = prv_vector_set,
	.zero_buf = subtilis_array_zero_buf_i32,
	.append = prv_append,
	.indexed_address = NULL,
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
