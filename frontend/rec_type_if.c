/*
 * Copyright (c) 2022 Mark Ryan
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

#include "builtins_ir.h"
#include "rec_type.h"
#include "rec_type_if.h"
#include "reference_type.h"

/* clang-format on */

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

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_ret(subtilis_parser_t *p, size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t ret_reg;

	ret_reg.reg = reg;
	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_RET_I32, ret_reg, err);
}

static void prv_array_of(const subtilis_type_t *el_type, subtilis_type_t *type,
			 subtilis_error_t *err)
{
	subtilis_type_free(type);
	type->type = SUBTILIS_TYPE_ARRAY_REC;
	subtilis_type_init_to_from_rec(&type->params.array.params.rec, el_type,
				       err);
}

static void prv_vector_of(const subtilis_type_t *el_type, subtilis_type_t *type,
			  subtilis_error_t *err)
{
	subtilis_type_free(type);
	type->type = SUBTILIS_TYPE_VECTOR_REC;
	subtilis_type_init_to_from_rec(&type->params.array.params.rec, el_type,
				       err);
}

static subtilis_exp_t *prv_load_from_mem(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 size_t mem_reg, size_t loc,
					 subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
	return NULL;
}

static void prv_assign_to_mem(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_rec_type_copy(p, &e->type, mem_reg, loc, e->exp.ir_op.reg,
			       false, err);
	subtilis_exp_delete(e);
}

static void prv_assign_to_new_mem(subtilis_parser_t *p, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_error_t *err)
{
	subtilis_rec_type_copy(p, &e->type, mem_reg, loc, e->exp.ir_op.reg,
			       true, err);
	subtilis_exp_delete(e);
}

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static void prv_copy_col(subtilis_parser_t *p, subtilis_exp_t *e1,
			 subtilis_exp_t *e2, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "= on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<> on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "> on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<= on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, bool swapped,
			      subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "< on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">= on RECs", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static void prv_swap_mem_mem(subtilis_parser_t *p, const subtilis_type_t *type,
			     size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_rec_type_swap(p, type, reg1, reg2, err);
}

static size_t prv_destructor(subtilis_parser_t *p, const subtilis_type_t *type,
			     subtilis_error_t *err)
{
	size_t call_index = SIZE_MAX;

	if (!subtilis_type_rec_need_deref(type))
		return SIZE_MAX;

	call_index = subtilis_builtin_ir_rec_deref(p, type, err);

	return call_index;
}

/* clang-format off */
subtilis_type_if subtilis_type_rec = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = false,
	.is_vector = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = subtilis_type_rec_size,
	.alignment = subtilis_type_rec_align,
	.data_size = NULL,
	.zero = NULL,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.assign_ref_no_rc = NULL,
	.zero_reg = prv_zero_reg,
	.copy_ret = subtilis_rec_type_copy_ret,
	.array_of = prv_array_of,
	.vector_of = prv_vector_of,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = prv_dup,
	.copy_col = prv_copy_col,
	.assign_reg = subtilis_rec_type_assign_to_reg,
	.assign_mem = prv_assign_to_mem,
	.assign_new_mem = prv_assign_to_new_mem,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.set = NULL,
	.zero_buf = NULL,
	.append = NULL,
	.indexed_address = NULL,
	.load_mem = prv_load_from_mem,
	.to_int32 = NULL,
	.zerox = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.unary_minus = NULL,
	.add = NULL,
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
	.is_inf = NULL,
	.call = prv_call,
	.call_ptr = prv_call_ptr,
	.ret = prv_ret,
	.print = NULL,
	.destructor = prv_destructor,
	.swap_reg_reg = NULL,
	.swap_reg_mem = NULL,
	.swap_mem_mem = prv_swap_mem_mem,
};

/* clang-format on */
