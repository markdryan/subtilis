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

#include "array_float64_type.h"
#include "array_type.h"
#include "builtins_helper.h"
#include "collection.h"
#include "reference_type.h"

static size_t prv_size(const subtilis_type_t *type)
{
	return subtilis_array_type_size(type);
}

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	subtilis_exp_t *three;

	three = subtilis_exp_new_int32(3, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return subtilis_type_if_lsl(p, e, three, err);
}

static subtilis_exp_t *prv_zero(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "zero on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
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

static void prv_element_type(const subtilis_type_t *type,
			     subtilis_type_t *element_type)
{
	element_type->type = SUBTILIS_TYPE_REAL;
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_copy_col(subtilis_parser_t *p, subtilis_exp_t *e1,
			 subtilis_exp_t *e2, subtilis_error_t *err)
{
	subtilis_collection_copy_scalar(p, e1, e2, false, err);
}

static subtilis_exp_t *
prv_indexed_read(subtilis_parser_t *p, const char *var_name,
		 const subtilis_type_t *type, size_t mem_reg, size_t loc,
		 subtilis_exp_t **indices, size_t index_count,
		 subtilis_error_t *err)
{
	return subtilis_array_read(p, var_name, type, &subtilis_type_real,
				   mem_reg, loc, indices, index_count, err);
}

static void prv_set(subtilis_parser_t *p, const char *var_name,
		    const subtilis_type_t *type, size_t mem_reg, size_t loc,
		    subtilis_exp_t *e, bool check_size, subtilis_error_t *err)
{
	subtilis_type_t el_type;
	size_t ptr;
	size_t sizee;
	subtilis_ir_operand_t gt_zero;
	subtilis_ir_operand_t eq_zero;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	subtilis_type_if_element_type(p, type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_coerce_type(p, e, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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

	ptr = subtilis_reference_get_data(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = p->current->reg_counter++;
	op1.reg = p->current->reg_counter++;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_MOV_FP_I32_I32, op0,
					  op1, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_builtin_memset_i64(p, ptr, sizee, op0.reg, op1.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (check_size)
		subtilis_ir_section_add_label(p->current, eq_zero.label, err);

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
	if ((a2->type.type == SUBTILIS_TYPE_ARRAY_REAL) ||
	    (a2->type.type == SUBTILIS_TYPE_VECTOR_REAL)) {
		subtilis_array_append_scalar_array(p, a1, a2, err);
		return;
	} else if (subtilis_type_if_is_numeric(&a2->type)) {
		a2 = subtilis_type_if_to_float64(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_array_append_scalar(p, a1, a2, err);
		return;
	}
	subtilis_error_set_expected(err, "real or array of reals",
				    subtilis_type_name(&a2->type),
				    p->l->stream->name, p->l->line);

cleanup:
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
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

static void prv_indexed_write(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t *e,
			      subtilis_exp_t **indices, size_t index_count,
			      subtilis_error_t *err)
{
	subtilis_array_write(p, var_name, type, &subtilis_type_real, mem_reg,
			     loc, e, indices, index_count, err);
}

static void prv_indexed_add(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_array_add(p, var_name, type, &subtilis_type_real, mem_reg, loc,
			   e, indices, index_count, err);
}

static void prv_indexed_sub(subtilis_parser_t *p, const char *var_name,
			    const subtilis_type_t *type, size_t mem_reg,
			    size_t loc, subtilis_exp_t *e,
			    subtilis_exp_t **indices, size_t index_count,
			    subtilis_error_t *err)
{
	subtilis_array_sub(p, var_name, type, &subtilis_type_real, mem_reg, loc,
			   e, indices, index_count, err);
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

static void prv_ret(subtilis_parser_t *p, size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t ret_reg;

	ret_reg.reg = reg;
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_RET_I32, ret_reg, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_array_float64 = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = true,
	.is_vector = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = prv_data_size,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
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
	.append = NULL,
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
	.ret = prv_ret,
	.destructor = NULL,
};

/* clang-format on */

static void prv_vector_set(subtilis_parser_t *p, const char *var_name,
			   const subtilis_type_t *type, size_t mem_reg,
			   size_t loc, subtilis_exp_t *e, subtilis_error_t *err)
{
	prv_set(p, var_name, type, mem_reg, loc, e, true, err);
}

/* clang-format off */
subtilis_type_if subtilis_type_vector_float64 = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = false,
	.is_vector = true,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = prv_data_size,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
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
	.append = prv_append,
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
	.ret = prv_ret,
	.destructor = NULL,
};

/* clang-format on */
