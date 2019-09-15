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
					   "builtin", 0, p->error_offset, err);
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

	if (p->current->in_error_handler) {
		/*
		 * We're in an error handler.  We ignore any error and continue.
		 */

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_CLEARE, err);
		return;
	}

	op1.reg = subtilis_ir_section_add_instr1(p->current,
						 SUBTILIS_OP_INSTR_TESTE, err);
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
				      size_t num_params, subtilis_error_t *err)
{
	size_t reg;
	size_t call_site;
	subtilis_exp_t *e = NULL;
	subtilis_parser_call_t *call = NULL;

	if (fn_type->type == SUBTILIS_TYPE_VOID) {
		subtilis_ir_section_add_call(p->current, num_params, args, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = NULL;
	} else {
		if (fn_type->type == SUBTILIS_TYPE_INTEGER)
			reg = subtilis_ir_section_add_i32_call(
			    p->current, num_params, args, err);
		else if (fn_type->type == SUBTILIS_TYPE_REAL)
			reg = subtilis_ir_section_add_real_call(
			    p->current, num_params, args, err);
		else
			subtilis_error_set_assertion_failed(err);

		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = NULL;
		e = subtilis_exp_new_var(fn_type, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	/* For calls made inside handlers we will add the offset from the start
	 * of the handler section and fix it up later on when checking the
	 * calls.
	 */

	call_site = p->current->in_error_handler ? p->current->error_len
						 : p->current->len;
	call_site--;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

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

subtilis_exp_t *subtilis_exp_coerce_type(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 const subtilis_type_t *type,
					 subtilis_error_t *err)
{
	switch (type->type) {
	case SUBTILIS_TYPE_REAL:
		e = subtilis_type_if_to_float64(p, e, err);
		break;
	case SUBTILIS_TYPE_INTEGER:
		e = subtilis_type_if_to_int(p, e, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		subtilis_exp_delete(e);
		e = NULL;
	}
	return e;
}

/* Swap the arguments if necessary to ensure that the constant comes last
 * Returns true if arguments have been swapped.
 */

static bool prv_order_expressions(subtilis_exp_t **a1, subtilis_exp_t **a2)
{
	subtilis_exp_t *e1 = *a1;
	subtilis_exp_t *e2 = *a2;

	if ((e2->type.type == SUBTILIS_TYPE_INTEGER ||
	     e2->type.type == SUBTILIS_TYPE_REAL ||
	     e2->type.type == SUBTILIS_TYPE_STRING) &&
	    (e1->type.type == SUBTILIS_TYPE_CONST_INTEGER ||
	     e1->type.type == SUBTILIS_TYPE_CONST_REAL ||
	     e1->type.type == SUBTILIS_TYPE_CONST_STRING)) {
		*a1 = e2;
		*a2 = e1;
		return true;
	}

	return false;
}

subtilis_exp_t *subtilis_exp_new_var(const subtilis_type_t *type,
				     unsigned int reg, subtilis_error_t *err)
{
	subtilis_exp_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = *type;
	e->exp.ir_op.reg = reg;

	return e;
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
	subtilis_buffer_init(&e->exp.str, str->granularity);
	subtilis_buffer_append(&e->exp.str, str->buffer->data,
			       subtilis_buffer_get_size(str), err);

	return e;
}

static subtilis_exp_t *prv_subtilis_exp_add_str(subtilis_parser_t *p,
						subtilis_exp_t *a1,
						subtilis_exp_t *a2,
						subtilis_error_t *err)
{
	size_t len;

	(void)prv_order_expressions(&a1, &a2);

	if (a1->type.type == SUBTILIS_TYPE_CONST_STRING) {
		len = subtilis_buffer_get_size(&a2->exp.str);
		subtilis_buffer_remove_terminator(&a1->exp.str);
		subtilis_buffer_append(&a1->exp.str, a2->exp.str.buffer->data,
				       len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		return a1;
	} else if (a2->type.type == SUBTILIS_TYPE_CONST_STRING) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	subtilis_error_set_assertion_failed(err);
	return NULL;
}

subtilis_exp_t *subtilis_exp_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a1->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a1->type.type == SUBTILIS_TYPE_STRING) &&
	    (a2->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a2->type.type == SUBTILIS_TYPE_STRING)) {
		return prv_subtilis_exp_add_str(p, a1, a2, err);
	}

	return subtilis_type_if_add(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a1->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a1->type.type == SUBTILIS_TYPE_STRING) &&
	    (a2->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a2->type.type == SUBTILIS_TYPE_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return subtilis_type_if_gt(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a1->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a1->type.type == SUBTILIS_TYPE_STRING) &&
	    (a2->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a2->type.type == SUBTILIS_TYPE_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return subtilis_type_if_lte(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a1->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a1->type.type == SUBTILIS_TYPE_STRING) &&
	    (a2->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a2->type.type == SUBTILIS_TYPE_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return subtilis_type_if_lt(p, a1, a2, err);
}

subtilis_exp_t *subtilis_exp_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err)
{
	if ((a1->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a1->type.type == SUBTILIS_TYPE_STRING) &&
	    (a2->type.type == SUBTILIS_TYPE_CONST_STRING ||
	     a2->type.type == SUBTILIS_TYPE_STRING)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return subtilis_type_if_gte(p, a1, a2, err);
}

void subtilis_exp_delete(subtilis_exp_t *e)
{
	if (!e)
		return;
	if (e->type.type == SUBTILIS_TYPE_CONST_STRING)
		subtilis_buffer_free(&e->exp.str);
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

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_SETE, err);
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

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_SETE, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_exp_return_default_value(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}
