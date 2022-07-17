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

#include "array_type.h"
#include "parser_assignment.h"
#include "parser_compound.h"
#include "parser_cond.h"
#include "parser_error.h"
#include "parser_exp.h"
#include "parser_loops.h"
#include "rec_type.h"
#include "reference_type.h"
#include "type_if.h"

static subtilis_exp_t *prv_numeric_expression(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	if (!subtilis_type_if_is_numeric(&e->type)) {
		subtilis_exp_delete(e);
		subtilis_error_set_numeric_exp_expected(err, p->l->stream->name,
							p->l->line);
		return NULL;
	}

	return e;
}

typedef enum {
	SUBTILIS_STEP_CONST_INC,
	SUBTILIS_STEP_CONST_DEC,
	SUBTILIS_STEP_VAR,
	SUBTILIS_STEP_MAX
} subtilis_step_type_t;

static subtilis_step_type_t prv_compute_step_type(subtilis_parser_t *p,
						  subtilis_exp_t *step,
						  subtilis_error_t *err)
{
	subtilis_step_type_t retval;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *step_dup = NULL;

	if (!subtilis_type_if_is_numeric(&step->type)) {
		subtilis_error_set_numeric_exp_expected(err, p->l->stream->name,
							p->l->line);
		return SUBTILIS_STEP_MAX;
	}

	if (!subtilis_type_if_is_const(&step->type))
		return SUBTILIS_STEP_VAR;

	e = subtilis_type_if_zero(p, &step->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SUBTILIS_STEP_MAX;

	step_dup = subtilis_type_if_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	e = subtilis_type_if_eq(p, step_dup, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	if (!subtilis_type_if_is_const(&e->type)) {
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	if (e->exp.ir_op.integer == -1) {
		subtilis_error_set_zero_step(err, p->l->stream->name,
					     p->l->line);
		goto on_error;
	}

	subtilis_exp_delete(e);
	e = subtilis_type_if_zero(p, &step->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	step_dup = subtilis_type_if_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	e = subtilis_type_if_lt(p, step_dup, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	if (!subtilis_type_if_is_const(&e->type)) {
		subtilis_error_set_assertion_failed(err);
		goto on_error;
	}

	retval = (e->exp.ir_op.integer == -1) ? SUBTILIS_STEP_CONST_DEC
					      : SUBTILIS_STEP_CONST_INC;
	subtilis_exp_delete(e);

	return retval;

on_error:

	subtilis_exp_delete(e);

	return SUBTILIS_STEP_MAX;
}

struct subtilis_for_context_t_ {
	bool is_reg;
	size_t reg;
	size_t loc;
	subtilis_type_t type;
};

typedef struct subtilis_for_context_t_ subtilis_for_context_t;

static subtilis_exp_t *prv_increment_var(subtilis_parser_t *p,
					 subtilis_exp_t *inc,
					 const subtilis_for_context_t *for_ctx,
					 subtilis_error_t *err)
{
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *e = NULL;

	if (for_ctx->is_reg)
		var = subtilis_exp_new_var(&for_ctx->type, for_ctx->loc, err);
	else
		var = subtilis_type_if_load_from_mem(
		    p, &for_ctx->type, for_ctx->reg, for_ctx->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_add(p, var, inc, err);
	var = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_coerce_type(p, e, &for_ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (for_ctx->is_reg) {
		subtilis_type_if_assign_to_reg(p, for_ctx->loc, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		var = subtilis_exp_new_var(&for_ctx->type, for_ctx->loc, err);
	} else {
		var = subtilis_type_if_dup(e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_type_if_assign_to_mem(p, for_ctx->reg, for_ctx->loc, e,
					       err);
	}

	return var;

cleanup:

	subtilis_exp_delete(var);
	subtilis_exp_delete(inc);

	return NULL;
}

static void prv_for_assign_array(subtilis_parser_t *p, const char *var_name,
				 subtilis_token_t *t,
				 const subtilis_symbol_t *s, size_t mem_reg,
				 subtilis_for_context_t *ctx,
				 subtilis_error_t *err)
{
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t i;
	subtilis_exp_t *e = NULL;
	size_t dims = 0;

	dims = subtilis_var_bracketed_int_args_have_b(
	    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_element_type(p, &s->t, &ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_array_index_calc(p, var_name, &s->t, mem_reg, s->loc,
				      indices, dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ctx->is_reg = false;
	ctx->reg = e->exp.ir_op.reg;
	ctx->loc = 0;

cleanup:

	subtilis_exp_delete(e);

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);
}

static void prv_for_assign_vector(subtilis_parser_t *p, const char *var_name,
				  subtilis_token_t *t,
				  const subtilis_symbol_t *s, size_t mem_reg,
				  subtilis_for_context_t *ctx,
				  subtilis_error_t *err)
{
	subtilis_exp_t *index[1];
	subtilis_exp_t *e = NULL;

	index[0] = subtilis_curly_bracketed_arg_have_b(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_element_type(p, &s->t, &ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_array_index_calc(p, var_name, &s->t, mem_reg, s->loc,
				      index, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ctx->is_reg = false;
	ctx->reg = e->exp.ir_op.reg;
	ctx->loc = 0;

cleanup:

	subtilis_exp_delete(e);
	subtilis_exp_delete(index[0]);
}

static void prv_init_array_var(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_for_context_t *for_ctx,
			       const char *var_name, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	bool new_global = false;

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (new_global) {
		subtilis_error_set_unknown_variable(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	prv_for_assign_array(p, var_name, t, s, op1.reg, for_ctx, err);
}

static void prv_init_vector_var(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_for_context_t *for_ctx,
				const char *var_name, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	bool new_global = false;

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (new_global) {
		subtilis_error_set_unknown_variable(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	prv_for_assign_vector(p, var_name, t, s, op1.reg, for_ctx, err);
}

static void prv_init_scalar_var(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_for_context_t *for_ctx,
				const char *var_name, bool new_local,
				const subtilis_type_t *type,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	bool new_global = false;

	if (!new_local) {
		subtilis_parser_lookup_assignment_var(
		    p, t, var_name, &s, &op1.reg, &new_global, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (new_global) {
			s = subtilis_symbol_table_insert(p->st, var_name, type,
							 err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		for_ctx->is_reg = s->is_reg;
		subtilis_type_copy(&for_ctx->type, &s->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		for_ctx->reg = op1.reg;
		for_ctx->loc = s->loc;
		return;
	}

	if (subtilis_symbol_table_lookup(p->local_st, var_name)) {
		subtilis_error_set_already_defined(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	for_ctx->is_reg = true;
	for_ctx->type = *type;
	if (subtilis_type_if_reg_type(&for_ctx->type) ==
	    SUBTILIS_IR_REG_TYPE_INTEGER)
		for_ctx->loc = p->current->reg_counter++;
	else
		for_ctx->loc = p->current->freg_counter++;
}

static char *prv_init_for_var(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_for_context_t *for_ctx, bool *new_local,
			      subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t type;
	char *var_name = NULL;

	type.type = SUBTILIS_TYPE_VOID;

	var_name = subtilis_parser_get_assignment_var(p, t, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, "(")) {
		prv_init_array_var(p, t, for_ctx, var_name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if (!strcmp(tbuf, "{")) {
		prv_init_vector_var(p, t, for_ctx, var_name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		if (!strcmp(tbuf, ":=")) {
			if (*new_local) {
				subtilis_error_set_expected(err, "=", tbuf,
							    p->l->stream->name,
							    p->l->line);
				goto cleanup;
			}
			*new_local = true;
		}

		prv_init_scalar_var(p, t, for_ctx, var_name, *new_local, &type,
				    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return var_name;

cleanup:
	subtilis_type_free(&type);
	free(var_name);

	return NULL;
}

static void prv_for_assignment(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_for_context_t *for_ctx, bool new_local,
			       subtilis_error_t *err)
{
	char *var_name;
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	var_name = prv_init_for_var(p, t, for_ctx, &new_local, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "=") && strcmp(tbuf, ":=")) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* Ownership of e is passed to the following functions. */

	e = subtilis_type_if_coerce_type(p, e, &for_ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (new_local) {
		p->level++;
		subtilis_symbol_table_level_up(p->local_st, p->l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_type_if_assign_to_reg(p, for_ctx->loc, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, &for_ctx->type, for_ctx->loc, err);
	} else {
		if (for_ctx->is_reg) {
			subtilis_type_if_assign_to_reg(p, for_ctx->loc, e, err);
		} else {
			subtilis_type_if_assign_to_mem(p, for_ctx->reg,
						       for_ctx->loc, e, err);
		}
		p->level++;
		subtilis_symbol_table_level_up(p->local_st, p->l, err);
	}

cleanup:

	free(var_name);
}

static void prv_for_loop_start(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_ir_operand_t *start_label,
			       subtilis_ir_operand_t *true_label,
			       subtilis_error_t *err)
{
	start_label->reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label->reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_compound_at_sym_level(p, t, SUBTILIS_KEYWORD_NEXT, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label->reg = subtilis_ir_section_new_label(p->current);
}

static void prv_for_step_generic_var(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_exp_t *to, subtilis_exp_t *step,
				     const subtilis_for_context_t *for_ctx,
				     subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t true_label2;
	subtilis_ir_operand_t inc_label;
	subtilis_ir_operand_t dec_label;
	subtilis_exp_t *step_dup = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *var_dup = NULL;
	subtilis_exp_t *to_dup = NULL;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *zero = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	step_dup = subtilis_type_if_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, step, for_ctx, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!subtilis_type_if_is_numeric(&var->type)) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	var_dup = subtilis_type_if_dup(var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_dup = subtilis_type_if_dup(to, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_type_if_zero(p, &step_dup->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	conde = subtilis_exp_gt(p, step_dup, zero, err);
	step_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	inc_label.reg = subtilis_ir_section_new_label(p->current);
	dec_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, inc_label,
					  dec_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, inc_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_exp_gt(p, var, to, err);
	var = NULL;
	to = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, dec_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_exp_lt(p, var_dup, to_dup, err);
	var_dup = NULL;
	to_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	true_label2.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label2,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label2.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var_dup);
	subtilis_exp_delete(to_dup);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step_dup);
	subtilis_exp_delete(step);
}

static void prv_for_step_int_var(subtilis_parser_t *p, subtilis_token_t *t,
				 subtilis_exp_t *to, subtilis_exp_t *step,
				 const subtilis_for_context_t *for_ctx,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t final_label;
	subtilis_type_t step_type_const;
	subtilis_exp_t *step_dir = NULL;
	subtilis_exp_t *step_dup = NULL;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *eor_var = NULL;
	subtilis_exp_t *eor_var_dup = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *sub = NULL;

	step_type_const.type = SUBTILIS_TYPE_VOID;

	/*
	 * TODO: Add a runtime check for step variable of 0.
	 * and generate an error.
	 */

	step = subtilis_type_if_coerce_type(p, step, &for_ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	step_dup = subtilis_type_if_dup(step, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_const_of(&step->type, &step_type_const, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	step_dir = subtilis_type_if_top_bit(p, &step_type_const, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	step_dir = subtilis_type_if_and(p, step_dup, step_dir, err);
	step_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, step, for_ctx, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!subtilis_type_if_is_integer(&var->type)) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	sub = subtilis_type_if_sub(p, to, var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	var = NULL;
	to = NULL;

	step_dup = subtilis_type_if_dup(step_dir, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eor_var = subtilis_type_if_eor(p, sub, step_dup, err);
	step_dup = NULL;
	sub = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eor_var_dup = subtilis_type_if_dup(eor_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_type_if_zero(p, &step_type_const, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	conde = subtilis_exp_lt(p, eor_var, zero, err);
	eor_var = NULL;

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_type_if_neq(p, eor_var_dup, step_dir, err);
	eor_var_dup = NULL;
	step_dir = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	final_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, final_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, final_label.reg, err);

cleanup:

	subtilis_exp_delete(sub);
	subtilis_exp_delete(eor_var_dup);
	subtilis_exp_delete(eor_var);
	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step_dir);
	subtilis_exp_delete(step_dup);
	subtilis_exp_delete(step);
	subtilis_type_free(&step_type_const);
}

static void prv_for_step_const(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_exp_t *to, subtilis_exp_t *step,
			       subtilis_step_type_t step_type,
			       const subtilis_for_context_t *for_ctx,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *var = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, step, for_ctx, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (step_type == SUBTILIS_STEP_CONST_INC) {
		conde = subtilis_exp_gt(p, var, to, err);
		var = NULL;
		to = NULL;
	} else {
		conde = subtilis_exp_lt(p, var, to, err);
		var = NULL;
		to = NULL;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
	subtilis_exp_delete(step);
}

static void prv_for_no_step(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_exp_t *to,
			    const subtilis_for_context_t *for_ctx,
			    subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_exp_t *inc = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *conde = NULL;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	inc = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, inc, for_ctx, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	conde = subtilis_exp_gt(p, var, to, err);
	var = NULL;
	to = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde->exp.ir_op, true_label,
					  start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);

cleanup:

	subtilis_exp_delete(conde);
	subtilis_exp_delete(var);
	subtilis_exp_delete(to);
}

void subtilis_parser_for(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t var_type;
	subtilis_for_context_t for_ctx;
	char *var_name = NULL;
	subtilis_exp_t *to = NULL;
	subtilis_exp_t *step = NULL;
	bool new_local = false;
	subtilis_step_type_t step_type = SUBTILIS_STEP_MAX;

	var_type.type = SUBTILIS_TYPE_VOID;
	for_ctx.type.type = SUBTILIS_TYPE_VOID;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (t->type == SUBTILIS_TOKEN_KEYWORD) {
		if (t->tok.keyword.type != SUBTILIS_KEYWORD_LOCAL) {
			subtilis_error_set_expected(
			    err, "LOCAL", tbuf, p->l->stream->name, p->l->line);
			return;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		new_local = true;
		tbuf = subtilis_token_get_text_with_err(t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	var_type = t->tok.id_type;
	if (!subtilis_type_if_is_numeric(&var_type)) {
		subtilis_error_set_numeric_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	var_name = malloc(subtilis_buffer_get_size(&t->buf));
	if (!var_name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(var_name, tbuf);

	prv_for_assignment(p, t, &for_ctx, new_local, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (t->tok.keyword.type != SUBTILIS_KEYWORD_TO)) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_expected(err, "TO", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	to = prv_numeric_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_STEP)) {
		step = prv_numeric_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		step_type = prv_compute_step_type(p, step, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		/* Takes ownership of to and step */

		if (step_type == SUBTILIS_STEP_VAR) {
			step = subtilis_type_if_copy_var(p, step, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			if (subtilis_type_if_is_integer(&var_type))
				prv_for_step_int_var(p, t, to, step, &for_ctx,
						     err);
			else
				prv_for_step_generic_var(p, t, to, step,
							 &for_ctx, err);
		} else {
			prv_for_step_const(p, t, to, step, step_type, &for_ctx,
					   err);
		}
		step = NULL;
	} else {
		/* Takes ownership of to */

		prv_for_no_step(p, t, to, &for_ctx, err);
	}
	to = NULL;

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);

cleanup:

	subtilis_type_free(&var_type);
	subtilis_type_free(&for_ctx.type);
	subtilis_exp_delete(step);
	free(var_name);
	subtilis_exp_delete(to);
}

void subtilis_parser_while(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;

	start_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_parser_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_compound(p, t, SUBTILIS_KEYWORD_ENDWHILE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

void subtilis_parser_repeat(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;

	start_label.reg = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, start_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_compound(p, t, SUBTILIS_KEYWORD_UNTIL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_parser_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

struct subtilis_range_var_t_ {
	char *name;
	subtilis_type_t type;
	subtilis_for_context_t for_ctx;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t end_reg;
};

typedef struct subtilis_range_var_t_ subtilis_range_var_t;

static subtilis_range_var_t *prv_get_varnames(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      size_t *count,
					      subtilis_error_t *err)
{
	size_t i;
	subtilis_range_var_t *range_vars;
	size_t var_count = 0;
	const char *tbuf;

	range_vars = calloc(SUBTILIS_MAX_DIMENSIONS + 1, sizeof(*range_vars));
	if (!range_vars) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	for (i = 0; i < SUBTILIS_MAX_DIMENSIONS + 1; i++) {
		range_vars[i].type.type = SUBTILIS_TYPE_VOID;
		range_vars[i].for_ctx.type.type = SUBTILIS_TYPE_VOID;
	}

	tbuf = subtilis_token_get_text(t);
	if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
		range_vars[0].name = subtilis_parser_get_assignment_var(
		    p, t, &range_vars[0].type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if ((range_vars[0].type.type == SUBTILIS_TYPE_REC) ||
		    (range_vars[0].type.type == SUBTILIS_TYPE_FN)) {
			subtilis_complete_custom_type(p, range_vars[0].name,
						      &range_vars[0].type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "~")) {
		/*
		 * We can't set the type of the range variable until we've
		 * evaluated the collection.
		 */

		range_vars[0].name = NULL;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		goto cleanup;
	}

	var_count = 1;
	tbuf = subtilis_token_get_text(t);
	while ((t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, ","))) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		range_vars[var_count].name = subtilis_parser_get_assignment_var(
		    p, t, &range_vars[var_count].type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		var_count++;
		if (range_vars[var_count - 1].type.type !=
		    SUBTILIS_TYPE_INTEGER) {
			subtilis_error_set_integer_variable_expected(
			    err,
			    subtilis_type_name(&range_vars[var_count - 1].type),
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}

		tbuf = subtilis_token_get_text(t);
	}

	if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
	    (!strcmp(tbuf, "{") || !strcmp(tbuf, "("))) {
		subtilis_error_set_array_in_range(
		    err, range_vars[var_count - 1].name, p->l->stream->name,
		    p->l->line);
		goto cleanup;
	}

	*count = var_count;
	return range_vars;

cleanup:

	for (i = 0; i < var_count; i++) {
		subtilis_type_free(&range_vars[i].type);
		subtilis_type_free(&range_vars[i].for_ctx.type);
		free(range_vars[i].name);
	}
	free(range_vars);
	return NULL;
}

static subtilis_range_var_t *
prv_get_range_vars(subtilis_parser_t *p, subtilis_token_t *t, size_t *count,
		   subtilis_exp_t **col, bool *locals, subtilis_error_t *err)
{
	const char *tbuf;
	size_t i;
	subtilis_type_t el_type;
	subtilis_buffer_t buf;
	subtilis_error_t err2;
	const char *error_type_name;
	subtilis_exp_t *e = NULL;
	size_t var_count = 0;
	subtilis_range_var_t *range_vars = NULL;
	bool new_locals = false;

	el_type.type = SUBTILIS_TYPE_VOID;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (t->type == SUBTILIS_TOKEN_KEYWORD) {
		tbuf = subtilis_token_get_text(t);
		if (t->tok.keyword.type != SUBTILIS_KEYWORD_LOCAL) {
			subtilis_error_set_expected(
			    err, "LOCAL", tbuf, p->l->stream->name, p->l->line);
			return NULL;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		new_locals = true;
	}

	range_vars = prv_get_varnames(p, t, &var_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_expected(err, (new_locals) ? "=" : "= or :=",
					    tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, ":=")) {
		if (new_locals) {
			subtilis_error_set_expected(
			    err, "=", tbuf, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		new_locals = true;
	} else if (strcmp(tbuf, "=")) {
		subtilis_error_set_expected(err, (new_locals) ? "=" : "= or :=",
					    tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!subtilis_type_if_is_array(&e->type) &&
	    !subtilis_type_if_is_vector(&e->type)) {
		subtilis_error_not_array_or_vector(
		    err, subtilis_type_name(&e->type), p->l->stream->name,
		    p->l->line);
		goto cleanup;
	}

	subtilis_type_if_element_type(p, &e->type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (range_vars[0].name) {
		if (!subtilis_type_eq(&el_type, &range_vars[0].type)) {
			subtilis_error_init(&err2);
			subtilis_buffer_init(&buf, 64);
			subtilis_full_type_name(&e->type, &buf, &err2);
			if (err2.type == SUBTILIS_ERROR_OK)
				subtilis_buffer_zero_terminate(&buf, &err2);
			error_type_name =
			    (err2.type != SUBTILIS_ERROR_OK)
				? subtilis_type_name(&e->type)
				: subtilis_buffer_get_string(&buf);
			subtilis_error_set_range_type_mismatch(
			    err, range_vars[0].name, error_type_name,
			    p->l->stream->name, p->l->line);
			subtilis_buffer_free(&buf);
			subtilis_type_free(&el_type);
			goto cleanup;
		}
	} else {
		/*
		 * If we're ignoring the range variable we can simply
		 * copy it's type from that of the collection.
		 */

		subtilis_type_copy(&range_vars[0].type, &el_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	subtilis_type_free(&el_type);

	if ((var_count > 1) &&
	    (e->type.params.array.num_dims != var_count - 1)) {
		subtilis_error_set_range_bad_var_count(
		    err, e->type.params.array.num_dims + 1, var_count,
		    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	*col = e;
	*count = var_count;
	*locals = new_locals;
	return range_vars;

cleanup:

	subtilis_exp_delete(e);
	for (i = 0; i < var_count; i++) {
		subtilis_type_free(&range_vars[i].type);
		subtilis_type_free(&range_vars[i].for_ctx.type);
		free(range_vars[i].name);
	}
	free(range_vars);

	return NULL;
}

static void prv_assign_range_var(subtilis_parser_t *p,
				 subtilis_ir_operand_t ptr,
				 const subtilis_range_var_t *var,
				 bool new_locals, subtilis_error_t *err)
{
	subtilis_exp_t *val;

	if (var->for_ctx.type.type == SUBTILIS_TYPE_REC) {
		if (new_locals)
			/*
			 * Special handling for RECs.  We just memcpy.
			 */

			subtilis_rec_type_tmp_copy(
			    p, &var->for_ctx.type, var->for_ctx.reg,
			    var->for_ctx.loc, ptr.reg, err);
		else
			subtilis_rec_type_copy(p, &var->for_ctx.type,
					       var->for_ctx.reg,
					       var->for_ctx.loc, ptr.reg, err);
		return;
	}

	val = subtilis_type_if_load_from_mem(p, &var->for_ctx.type, ptr.reg, 0,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!subtilis_type_if_is_numeric(&var->for_ctx.type) &&
	    var->for_ctx.type.type != SUBTILIS_TYPE_FN) {
		/*
		 * If we have a reference type and it's a new local, there's
		 * no need to go through all the reference counting.  We know
		 * that the array has references to every element we're
		 * iterating through, so this saves some code.
		 */

		if (!new_locals)
			subtilis_type_if_assign_ref(p, &var->type,
						    var->for_ctx.reg,
						    var->for_ctx.loc, val, err);
		else
			subtilis_type_if_assign_ref_no_rc(
			    p, &var->type, var->for_ctx.reg, var->for_ctx.loc,
			    val, err);

	} else if (var->for_ctx.is_reg) {
		subtilis_type_if_assign_to_reg(p, var->for_ctx.loc, val, err);
	} else {
		subtilis_type_if_assign_to_mem(p, var->for_ctx.reg,
					       var->for_ctx.loc, val, err);
	}
}

static size_t prv_range_loop_start(subtilis_parser_t *p, subtilis_exp_t *e,
				   const subtilis_range_var_t *range_vars,
				   subtilis_ir_operand_t start_label,
				   subtilis_ir_operand_t end_label,
				   bool new_locals, subtilis_error_t *err)
{
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t end;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t iter_label;
	subtilis_ir_operand_t ptr;

	size.reg =
	    subtilis_reference_type_get_size(p, e->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	ptr.reg = subtilis_reference_get_data(p, e->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, ptr, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	iter_label.label = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, iter_label, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, iter_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (range_vars[0].name) {
		prv_assign_range_var(p, ptr, &range_vars[0], new_locals, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	return ptr.reg;
}

static size_t prv_range_loop_start_index(subtilis_parser_t *p,
					 subtilis_exp_t *e, size_t var_count,
					 subtilis_range_var_t *range_vars,
					 bool new_locals, subtilis_error_t *err)
{
	subtilis_exp_t *zero;
	subtilis_exp_t *dim;
	subtilis_exp_t *e_dup;
	subtilis_ir_operand_t ok_label;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t counter;
	size_t i;

	ptr.reg = subtilis_reference_get_data(p, e->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	for (i = 1; i < var_count; i++) {
		range_vars[i].start_label.label =
		    subtilis_ir_section_new_label(p->current);
		range_vars[i].end_label.label =
		    subtilis_ir_section_new_label(p->current);
		e_dup = subtilis_type_if_dup(e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		dim = subtilis_exp_new_int32(i, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(dim);
			return SIZE_MAX;
		}
		dim = subtilis_array_get_dim(p, e_dup, dim, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		dim = subtilis_type_if_exp_to_var(p, dim, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		range_vars[i].end_reg = dim->exp.ir_op;
		subtilis_exp_delete(dim);
	}

	for (i = 1; i < var_count; i++) {
		zero = subtilis_exp_new_int32(0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		if (range_vars[i].for_ctx.is_reg)
			subtilis_type_if_assign_to_reg(
			    p, range_vars[i].for_ctx.loc, zero, err);
		else
			subtilis_type_if_assign_to_mem(
			    p, range_vars[i].for_ctx.reg,
			    range_vars[i].for_ctx.loc, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_label(
		    p->current, range_vars[i].start_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		if (range_vars[i].for_ctx.is_reg) {
			counter.reg = range_vars[i].for_ctx.loc;
		} else {
			op1.reg = range_vars[i].for_ctx.reg;
			op2.integer = range_vars[i].for_ctx.loc;
			counter.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2,
			    err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LTE_I32, counter,
		    range_vars[i].end_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		ok_label.label = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, ok_label,
		    range_vars[i].end_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		subtilis_ir_section_add_label(p->current, ok_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (range_vars[0].name) {
		prv_assign_range_var(p, ptr, &range_vars[0], new_locals, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	return ptr.reg;
}

static void
prv_range_loop_end(subtilis_parser_t *p, const subtilis_range_var_t *range_vars,
		   subtilis_ir_operand_t ptr, subtilis_ir_operand_t start_label,
		   subtilis_ir_operand_t end_label, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;

	op1.integer = subtilis_type_if_size(&range_vars[0].for_ctx.type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end_label.label, err);
}

static void prv_range_loop_end_index(subtilis_parser_t *p, size_t var_count,
				     const subtilis_range_var_t *range_vars,
				     subtilis_ir_operand_t ptr,
				     subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t i;
	subtilis_exp_t *inc;

	op1.integer = subtilis_type_if_size(&range_vars[0].for_ctx.type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = var_count - 1; i > 0; i--) {
		inc = subtilis_exp_new_int32(1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		inc = prv_increment_var(p, inc, &range_vars[i].for_ctx, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_exp_delete(inc);

		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP,
		    range_vars[i].start_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(
		    p->current, range_vars[i].end_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_init_tilde_var(subtilis_for_context_t *for_ctx,
			       const subtilis_type_t *type,
			       subtilis_error_t *err)
{
	subtilis_type_copy(&for_ctx->type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for_ctx->is_reg = false;
	for_ctx->reg = SIZE_MAX;
	for_ctx->loc = SIZE_MAX;
}

static void prv_init_range_var(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_for_context_t *for_ctx,
			       const char *var_name,
			       const subtilis_type_t *type,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e;
	bool new_global = false;

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (new_global) {
		subtilis_type_copy(&for_ctx->type, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		s = subtilis_symbol_table_insert(p->st, var_name,
						 &for_ctx->type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (type->type == SUBTILIS_TYPE_REC)
			subtilis_rec_type_zero(p, &for_ctx->type,
					       SUBTILIS_IR_REG_GLOBAL, s->loc,
					       true, err);
		else if (subtilis_type_if_is_numeric(&for_ctx->type) ||
			 for_ctx->type.type == SUBTILIS_TYPE_FN) {
			e = subtilis_type_if_zero(p, &for_ctx->type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_to_mem(
			    p, SUBTILIS_IR_REG_GLOBAL, s->loc, e, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (!subtilis_type_if_is_numeric(&for_ctx->type))
			subtilis_type_if_zero_ref(p, &for_ctx->type,
						  SUBTILIS_IR_REG_GLOBAL,
						  s->loc, true, err);
	} else {
		subtilis_type_copy(&for_ctx->type, &s->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	for_ctx->is_reg = s->is_reg;
	for_ctx->reg = op1.reg;
	for_ctx->loc = s->loc;
}

static void prv_init_local_range_var(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_for_context_t *for_ctx,
				     const char *var_name,
				     const subtilis_type_t *type,
				     subtilis_error_t *err)
{
	const subtilis_symbol_t *s;

	if (subtilis_symbol_table_lookup(p->local_st, var_name)) {
		subtilis_error_set_already_defined(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	if (subtilis_type_if_is_numeric(type) ||
	    type->type == SUBTILIS_TYPE_FN) {
		for_ctx->is_reg = true;

		subtilis_type_copy(&for_ctx->type, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (subtilis_type_if_reg_type(&for_ctx->type) ==
		    SUBTILIS_IR_REG_TYPE_INTEGER)
			for_ctx->loc = p->current->reg_counter++;
		else
			for_ctx->loc = p->current->freg_counter++;
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, &for_ctx->type, for_ctx->loc, err);
		return;
	} else if (type->type == SUBTILIS_TYPE_REC) {
		/*
		 * TODO: So this isn't ideal.  The range loop value
		 * variable has copy semantics, so we need to
		 * allocate a block of memory large enough to copy the
		 * loop variable.  We only really need to do this
		 * if we know that the range variable will be modified.
		 */

		subtilis_type_copy(&for_ctx->type, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		(void)subtilis_symbol_table_insert_no_rc(p->local_st, var_name,
							 &for_ctx->type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		subtilis_type_copy(&for_ctx->type, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/*
	 * There's no need to initialise the range var as its lifetime is
	 * restricted to the lifetime of the loop and it's value will be
	 * set during the first iteration.
	 */

	s = subtilis_symbol_table_insert_no_rc(p->local_st, var_name, type,
					       err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	for_ctx->is_reg = s->is_reg;
	for_ctx->reg = SUBTILIS_IR_REG_LOCAL;
	for_ctx->loc = s->loc;
}

static void prv_range_compound(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_exp_t *e, size_t var_count,
			       bool new_locals,
			       subtilis_range_var_t *range_vars,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t var_reg;
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t ptr;
	size_t i;
	unsigned int start;

	/*
	 * If they're global variables we need to create them at
	 * the top level, assuming we're at the top level of the
	 * main function. If we're declaring local variables,
	 * let's declare them one level down.  If we're just
	 * using existing local variables then we can do that here.
	 */

	if (!new_locals) {
		if (range_vars[0].name)
			prv_init_range_var(p, t, &range_vars[0].for_ctx,
					   range_vars[0].name,
					   &range_vars[0].type, err);

		else
			prv_init_tilde_var(&range_vars[0].for_ctx,
					   &range_vars[0].type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		for (i = 1; i < var_count; i++) {
			prv_init_range_var(p, t, &range_vars[i].for_ctx,
					   range_vars[i].name,
					   &range_vars[i].type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	p->level++;

	subtilis_symbol_table_level_up(p->local_st, p->l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (new_locals) {
		if (range_vars[0].name)
			prv_init_local_range_var(p, t, &range_vars[0].for_ctx,
						 range_vars[0].name,
						 &range_vars[0].type, err);
		else
			prv_init_tilde_var(&range_vars[0].for_ctx,
					   &range_vars[0].type, err);
		for (i = 1; i < var_count; i++) {
			prv_init_local_range_var(p, t, &range_vars[i].for_ctx,
						 range_vars[i].name,
						 &range_vars[i].type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	start_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	if (var_count == 1)
		ptr.reg = prv_range_loop_start(p, e, range_vars, start_label,
					       end_label, new_locals, err);
	else
		ptr.reg = prv_range_loop_start_index(
		    p, e, var_count, range_vars, new_locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_ENDRANGE))
			break;
		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type == SUBTILIS_TOKEN_EOF) {
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);
		return;
	}

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_reference_deallocate_refs(p, var_reg, p->local_st, p->level,
					   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (var_count == 1)
		prv_range_loop_end(p, range_vars, ptr, start_label, end_label,
				   err);
	else
		prv_range_loop_end_index(p, var_count, range_vars, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->level--;
	subtilis_symbol_table_level_down(p->local_st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);

	subtilis_lexer_get(p->l, t, err);
}

void subtilis_parser_range(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)

{
	size_t i;
	subtilis_exp_t *e;
	size_t var_count;
	bool new_locals;
	subtilis_range_var_t *range_vars = NULL;

	range_vars = prv_get_range_vars(p, t, &var_count, &e, &new_locals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_range_compound(p, t, e, var_count, new_locals, range_vars, err);

	subtilis_exp_delete(e);
	for (i = 0; i < var_count; i++) {
		subtilis_type_free(&range_vars[i].type);
		subtilis_type_free(&range_vars[i].for_ctx.type);
		free(range_vars[i].name);
	}
	free(range_vars);
}
