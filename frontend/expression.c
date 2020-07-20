/*
 * Copyright (c) 2017 Mark Ryan
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

#include "../common/ir.h"
#include "array_type.h"
#include "builtins_ir.h"
#include "expression.h"
#include "globals.h"
#include "type_if.h"
#include "variable.h"

static void prv_add_call(subtilis_parser_t *p, subtilis_parser_call_t *call,
			 subtilis_error_t *err)
{
	subtilis_parser_call_t **new_calls;
	size_t new_max;

	if (p->num_calls == p->max_calls) {
		new_max = p->max_calls + SUBTILIS_CONFIG_PROC_GRAN;
		new_calls = realloc(p->calls, new_max * sizeof(*new_calls));
		if (!new_calls) {
			subtilis_error_set_oom(err);
			return;
		}
		p->calls = new_calls;
		p->max_calls = new_max;
	}
	p->calls[p->num_calls++] = call;
}

static void prv_add_builtin(subtilis_parser_t *p, char *name,
			    subtilis_builtin_type_t ftype,
			    subtilis_error_t *err)
{
	subtilis_type_section_t *ts;

	ts = subtilis_builtin_ts(ftype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	(void)subtilis_ir_prog_section_new(p->prog, name, 0, ts, ftype,
					   "builtin", 0, p->eflag_offset,
					   p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto on_error;
		subtilis_error_init(err);
		subtilis_type_section_delete(ts);
	}

	return;

on_error:

	subtilis_type_section_delete(ts);
}

void subtilis_exp_return_default_value(subtilis_parser_t *p,
				       subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	end_label.label = p->current->end_label;
	if ((p->current == p->main) ||
	    (p->current->type->return_type.type == SUBTILIS_TYPE_VOID)) {
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
		return;
	}

	subtilis_type_if_zero_reg(p, &p->current->type->return_type,
				  p->current->ret_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
}

void subtilis_exp_handle_errors(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;
	subtilis_ir_operand_t op1;
	subtilis_exp_t *ecode;

	if (p->current->in_error_handler) {
		/*
		 * We're in an error handler.  We ignore any error and continue.
		 */

		subtilis_var_set_eflag(p, false, err);
		return;
	}

	ecode = subtilis_var_lookup_var(p, subtilis_eflag_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op1.reg = ecode->exp.ir_op.reg;
	subtilis_exp_delete(ecode);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ok_label.label = subtilis_ir_section_new_label(p->current);

	if (p->current->handler_list) {
		/*
		 * We're not in an error handler but there is one.
		 * Let's transfer control to it.
		 */

		error_label.label = p->current->handler_list->label;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, op1,
						  error_label, ok_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else if ((p->current == p->main) ||
		   (p->current->type->return_type.type == SUBTILIS_TYPE_VOID)) {
		/*
		 * There's no error handler so we need to return.  We can jump
		 * straight to the exit code for the procedure as theres no
		 * return value to zero.
		 */

		error_label.label = p->current->end_label;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, op1,
						  error_label, ok_label, err);
	} else {
		/*
		 * There's no error handler so we need to return.
		 */

		error_label.label = subtilis_ir_section_new_label(p->current);

		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, op1,
						  error_label, ok_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_label(p->current, error_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_ir_section_add_label(p->current, ok_label.label, err);
}

static size_t prv_create_tmp_ref(subtilis_parser_t *p, size_t reg,
				 const subtilis_type_t *fn_type,
				 char **tmp_name, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t dest;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t source;
	char *name;

	s = subtilis_symbol_table_insert_tmp(p->local_st, fn_type, &name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	dest.reg = SUBTILIS_IR_REG_LOCAL;
	op2.integer = s->loc;

	if (s->loc > 0) {
		dest.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	source.reg = reg;
	subtilis_type_if_copy_ret(p, fn_type, dest.reg, source.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	*tmp_name = name;
	return dest.reg;

on_error:

	free(name);
	return 0;
}

/*
 * Takes ownership of name and args and stype. stype may be NULL if we're
 * calling a builtin function that's actually used to implement an operator
 * e.g., DIV. Passing NULL for stype means the compiler won't perform ANY
 * type checking when issuing the call, which is fine as it's already been
 * done by the expression parser.
 */

subtilis_exp_t *subtilis_exp_add_call(subtilis_parser_t *p, char *name,
				      subtilis_builtin_type_t ftype,
				      subtilis_type_section_t *stype,
				      subtilis_ir_arg_t *args,
				      const subtilis_type_t *fn_type,
				      size_t num_params, bool check_error,
				      subtilis_error_t *err)
{
	size_t reg;
	size_t call_site;
	subtilis_exp_t *e = NULL;
	subtilis_parser_call_t *call = NULL;
	char *tmp_name;

	if (fn_type->type == SUBTILIS_TYPE_VOID)
		subtilis_ir_section_add_call(p->current, num_params, args, err);
	else
		e = subtilis_type_if_call(p, fn_type, args, num_params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	args = NULL;

	/* For calls made inside handlers we will add the offset from the start
	 * of the handler section and fix it up later on when checking the
	 * calls.
	 */

	call_site = p->current->in_error_handler ? p->current->error_len
						 : p->current->len;
	call_site--;

	/*
	 * We don't check for errors after calls to builtin functions that do
	 * not generate errors.  Doing so is wasteful and can also cause problem
	 * when a builtin, like deref, is called in a place where we don't want
	 * to check for errors, e.g., when unwinding the stack.  IR builtins
	 * are indistinguishable from normal functions, but the check_error
	 * flag can be used to disable error checking for those functions.
	 */

	if (check_error && ((ftype == SUBTILIS_BUILTINS_MAX) ||
			    subtilis_builtin_list[ftype].generates_error)) {
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	if (fn_type->type != SUBTILIS_TYPE_VOID &&
	    !subtilis_type_if_is_numeric(fn_type)) {
		reg = prv_create_tmp_ref(p, e->exp.ir_op.reg, fn_type,
					 &tmp_name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		e->exp.ir_op.reg = reg;
		e->temporary = tmp_name;
	}

	if (ftype != SUBTILIS_BUILTINS_MAX) {
		prv_add_builtin(p, name, ftype, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	call = subtilis_parser_call_new(p->current, call_site,
					p->current->in_error_handler, name,
					stype, p->l->line, ftype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	name = NULL;
	stype = NULL;

	prv_add_call(p, call, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return e;

on_error:

	subtilis_exp_delete(e);
	free(args);
	subtilis_parser_call_delete(call);
	subtilis_type_section_delete(stype);
	free(name);

	return NULL;
}

subtilis_exp_t *subtilis_exp_new_empty(const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = *type;
	e->temporary = NULL;

	return e;
}

subtilis_exp_t *subtilis_exp_new_var(const subtilis_type_t *type,
				     unsigned int reg, subtilis_error_t *err)
{
	subtilis_exp_t *e = subtilis_exp_new_empty(type, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e->exp.ir_op.reg = reg;
	return e;
}

subtilis_exp_t *subtilis_exp_new_var_block(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   unsigned int mem_reg, size_t offset,
					   subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = *type;
	e->temporary = NULL;
	op0.reg = mem_reg;
	op1.integer = offset;
	e->exp.ir_op.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return e;

on_error:

	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_exp_new_int32_var(unsigned int reg,
					   subtilis_error_t *err)
{
	return subtilis_exp_new_var(&subtilis_type_integer, reg, err);
}

subtilis_exp_t *subtilis_exp_new_real_var(unsigned int reg,
					  subtilis_error_t *err)
{
	return subtilis_exp_new_var(&subtilis_type_real, reg, err);
}

subtilis_exp_t *subtilis_exp_new_int32(int32_t integer, subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type.type = SUBTILIS_TYPE_CONST_INTEGER;
	e->exp.ir_op.integer = integer;
	e->temporary = NULL;

	return e;
}

subtilis_exp_t *subtilis_exp_new_real(double real, subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type.type = SUBTILIS_TYPE_CONST_REAL;
	e->exp.ir_op.real = real;
	e->temporary = NULL;

	return e;
}

subtilis_exp_t *subtilis_exp_new_str(subtilis_buffer_t *str,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type.type = SUBTILIS_TYPE_CONST_STRING;
	e->temporary = NULL;
	subtilis_buffer_init(&e->exp.str, str->granularity);
	if (subtilis_buffer_get_size(str) > 0)
		subtilis_buffer_append(&e->exp.str, str->buffer->data,
				       subtilis_buffer_get_size(str), err);

	return e;
}

subtilis_exp_t *subtilis_exp_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return subtilis_type_if_add(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	return subtilis_type_if_gt(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return subtilis_type_if_lte(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	return subtilis_type_if_lt(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	return subtilis_type_if_gte(p, a1, a2, err);
}

void subtilis_exp_delete(subtilis_exp_t *e)
{
	if (!e)
		return;
	if (e->type.type == SUBTILIS_TYPE_CONST_STRING)
		subtilis_buffer_free(&e->exp.str);
	free(e->temporary);
	free(e);
}

/* Consumes e */

void subtilis_exp_generate_error(subtilis_parser_t *p, subtilis_exp_t *e,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t target_label;

	subtilis_var_assign_hidden(p, subtilis_err_hidden_var,
				   &subtilis_type_integer, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (p->current->in_error_handler) {
		/*
		 * We're in an error handler. Let's set the error flag and
		 * return the default value for the function type.
		 */

		subtilis_var_set_eflag(p, true, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else if (p->current->handler_list) {
		/*
		 * We're not in an error handler but one is defined.
		 * Let's jump there.
		 */

		target_label.label = p->current->handler_list->label;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, target_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		/*
		 * We're not in an error handler and none has been defined.
		 * Let's set the error flag and return the default value.
		 */

		subtilis_var_set_eflag(p, true, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}
