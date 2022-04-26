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

#include "../common/buffer.h"
#include "fn_type.h"
#include "parser_exp.h"
#include "reference_type.h"

static size_t prv_generate_zero_fn(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_error_t *err)
{
	subtilis_type_section_t *stype;
	subtilis_buffer_t name;
	subtilis_exp_t *e;
	const char *fn_name;
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t result;
	subtilis_ir_operand_t ret_reg;
	subtilis_symbol_table_t *local_st = NULL;
	size_t call_index = SIZE_MAX;
	subtilis_symbol_table_t *old_local_st = p->local_st;
	subtilis_ir_section_t *old_section = p->current;

	subtilis_buffer_init(&name, 64);

	if (type->type == SUBTILIS_TYPE_ARRAY_FN) {
		/* I need a loop here that generates functions recursively. */
		subtilis_error_set_assertion_failed(err);
		return SIZE_MAX;
	}

	local_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;
	p->local_st = local_st;

	stype = subtilis_type_section_new(type, 0, NULL, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_append_string(&name, "!", err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_full_type_name(type, &name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	fn_name = subtilis_buffer_get_string(&name);
	if (subtilis_string_pool_find(p->prog->string_pool, fn_name,
				      &call_index))
		goto cleanup;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, fn_name, 0, stype, SUBTILIS_BUILTINS_MAX,
	    p->l->stream->name, p->l->line, p->eflag_offset, p->error_offset,
	    &call_index, err);
	stype = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (type->type != SUBTILIS_TYPE_VOID) {
		if (subtilis_type_if_is_reference(type)) {
			s = subtilis_symbol_table_insert_tmp(local_st, type,
							     NULL, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			subtilis_type_if_zero_ref(
			    p, type, SUBTILIS_IR_REG_LOCAL, s->loc, false, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			result.reg = subtilis_reference_get_pointer(
			    p, SUBTILIS_IR_REG_LOCAL, s->loc, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			/*
			 * Arrays are trickier as they don't have a zero value
			 */

			if (subtilis_type_if_is_array(type)) {
				ret_reg.reg = p->current->ret_reg;
				subtilis_ir_section_add_instr_no_reg2(
				    p->current, SUBTILIS_OP_INSTR_MOV, ret_reg,
				    result, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;

				subtilis_ir_section_add_label(
				    p->current, p->current->end_label, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
			} else {
				ret_reg.reg = result.reg;
			}
			e = subtilis_exp_new_var(type, ret_reg.reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			p->current->locals = local_st->max_allocated;
		} else {
			e = subtilis_type_if_zero(p, type, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_type_if_ret(p, type, e->exp.ir_op.reg, err);
		subtilis_exp_delete(e);
	} else {
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_RET, err);
	}

cleanup:
	p->current = old_section;

	subtilis_type_section_delete(stype);
	p->local_st = old_local_st;
	subtilis_symbol_table_delete(local_st);
	subtilis_buffer_free(&name);

	return call_index;
}

static size_t prv_size(const subtilis_type_t *type) { return sizeof(int32_t); }

static subtilis_exp_t *prv_zero(subtilis_parser_t *p,
				const subtilis_type_t *type,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t reg_num;
	subtilis_exp_t *e = NULL;
	size_t call_index;

	call_index = prv_generate_zero_fn(p, type->params.fn.ret_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	op1.label = (int32_t)call_index;
	reg_num = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_GET_PROC_ADDR, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_exp_new_int32_var(reg_num, err);

	return e;
}

static void prv_zero_reg(subtilis_parser_t *p, const subtilis_type_t *type,
			 size_t reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t call_index;

	call_index = prv_generate_zero_fn(p, type->params.fn.ret_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op0.reg = reg;
	op1.label = (int32_t)call_index;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_GET_PROC_ADDR, op0, op1, err);
}

static void prv_array_of(const subtilis_type_t *el_type, subtilis_type_t *type,
			 subtilis_error_t *err)
{
	subtilis_type_free(type);
	type->type = SUBTILIS_TYPE_ARRAY_FN;
	subtilis_type_init_to_from_fn(&type->params.array.params.fn, el_type,
				      err);
}

static void prv_vector_of(const subtilis_type_t *el_type, subtilis_type_t *type,
			  subtilis_error_t *err)
{
	subtilis_type_free(type);
	type->type = SUBTILIS_TYPE_VECTOR_FN;
	subtilis_type_init_to_from_fn(&type->params.array.params.fn, el_type,
				      err);
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_dup(subtilis_exp_t *e1, subtilis_exp_t *e2,
		    subtilis_error_t *err)
{
	e2->exp.ir_op = e1->exp.ir_op;
}

static void prv_assign_to_reg(subtilis_parser_t *p, size_t reg,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_MOV,
					  op0, e->exp.ir_op, op2, err);
	subtilis_exp_delete(e);
}

static void prv_assign_to_mem(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			      subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  e->exp.ir_op, op1, op2, err);
	subtilis_exp_delete(e);
}

static subtilis_exp_t *prv_load_from_mem(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 size_t mem_reg, size_t loc,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t reg;

	op1.reg = mem_reg;
	op2.integer = (int32_t)loc;
	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(type, reg, err);
}

static subtilis_exp_t *prv_unary_minus(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "unary - on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "+ on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "* on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "AND on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "OR on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "EOR on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "NOT on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_logical(subtilis_parser_t *p,
				   subtilis_op_instr_type_t type,
				   subtilis_exp_t *a1, subtilis_exp_t *a2,
				   subtilis_error_t *err)
{
	size_t reg;

	if (!subtilis_type_eq(&a1->type, &a2->type)) {
		subtilis_error_set_fn_type_mismatch(err, p->l->stream->name,
						    p->l->line);
		goto cleanup;
	}

	reg = subtilis_ir_section_add_instr(p->current, type, a1->exp.ir_op,
					    a2->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	a1->exp.ir_op.reg = reg;
	subtilis_exp_delete(a2);
	return a1;

cleanup:

	subtilis_exp_delete(a1);
	subtilis_exp_delete(a2);

	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_logical(p, SUBTILIS_OP_INSTR_EQ_I32, a1, a2, err);
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	return prv_logical(p, SUBTILIS_OP_INSTR_NEQ_I32, a1, a2, err);
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "- on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "DIV on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "MOD on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "pow on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<< on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">> on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">>> on functions",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_abs(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "ABS on functions",
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

static subtilis_exp_t *prv_coerce_type(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	subtilis_type_compare_diag(p, &e->type, type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
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

static void prv_swap_reg_reg(subtilis_parser_t *p, const subtilis_type_t *type,
			     size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t tmp;

	op1.reg = reg1;
	op2.reg = reg2;

	tmp.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op2, tmp, err);
}

static void prv_swap_reg_mem(subtilis_parser_t *p, const subtilis_type_t *type,
			     size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t tmp;

	op1.reg = reg1;
	op2.reg = reg2;
	zero.integer = 0;

	tmp.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, tmp, op2, zero, err);
}

static void prv_swap_mem_mem(subtilis_parser_t *p, const subtilis_type_t *type,
			     size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_exp_swap_int32_mem(p, reg1, reg2, 0, err);
}

/* clang-format off */

subtilis_type_if subtilis_type_fn = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.is_array = false,
	.is_vector = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = prv_size,
	.data_size = NULL,
	.zero = prv_zero,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.assign_ref_no_rc = NULL,
	.zero_reg = prv_zero_reg,
	.copy_ret = NULL,
	.array_of = prv_array_of,
	.vector_of = prv_vector_of,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = prv_dup,
	.copy_col = NULL,
	.assign_reg = prv_assign_to_reg,
	.assign_mem = prv_assign_to_mem,
	.assign_new_mem = prv_assign_to_mem,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.append = NULL,
	.set = NULL,
	.indexed_address = NULL,
	.load_mem = prv_load_from_mem,
	.to_int32 = NULL,
	.zerox = NULL,
	.to_float64 = NULL,
	.to_string = NULL,
	.to_hex_string = NULL,
	.coerce = prv_coerce_type,
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
	.print = NULL,
	.destructor = NULL,
	.swap_reg_reg = prv_swap_reg_reg,
	.swap_reg_mem = prv_swap_reg_mem,
	.swap_mem_mem = prv_swap_mem_mem,
};

/* clang-format on */
