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
	if ((e->type.type != SUBTILIS_TYPE_CONST_INTEGER) &&
	    (e->type.type != SUBTILIS_TYPE_CONST_REAL) &&
	    (e->type.type != SUBTILIS_TYPE_INTEGER) &&
	    (e->type.type != SUBTILIS_TYPE_REAL)) {
		subtilis_exp_delete(e);
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
	switch (step->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		if (step->exp.ir_op.integer == 0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.integer < 0 ? SUBTILIS_STEP_CONST_DEC
						   : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_TYPE_CONST_REAL:
		if (step->exp.ir_op.real == 0.0)
			subtilis_error_set_zero_step(err, p->l->stream->name,
						     p->l->line);
		return step->exp.ir_op.real < 0.0 ? SUBTILIS_STEP_CONST_DEC
						  : SUBTILIS_STEP_CONST_INC;
	case SUBTILIS_TYPE_INTEGER:
	case SUBTILIS_TYPE_REAL:
		return SUBTILIS_STEP_VAR;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}

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
					 subtilis_for_context_t *for_ctx,
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

	e = subtilis_exp_coerce_type(p, e, &for_ctx->type, err);
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

static void prv_for_assignment(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_for_context_t *for_ctx,
			       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_type_t type;
	bool new_var = false;
	char *var_name = NULL;
	subtilis_exp_t *e = NULL;

	var_name = subtilis_parser_lookup_assignment_var(
	    p, t, &type, &s, &op1.reg, &new_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, "(")) {
		if (new_var) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}

		prv_for_assign_array(p, var_name, t, s, op1.reg, for_ctx, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		if (new_var) {
			s = subtilis_symbol_table_insert(p->st, var_name, &type,
							 err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
		for_ctx->is_reg = s->is_reg;
		for_ctx->type = s->t;
		for_ctx->reg = op1.reg;
		for_ctx->loc = s->loc;
	}

	if (strcmp(tbuf, "=")) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* Ownership of e is passed to the following functions. */

	e = subtilis_exp_coerce_type(p, e, &for_ctx->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (for_ctx->is_reg)
		subtilis_type_if_assign_to_reg(p, for_ctx->loc, e, err);
	else
		subtilis_type_if_assign_to_mem(p, for_ctx->reg, for_ctx->loc, e,
					       err);

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

	subtilis_parser_compound(p, t, SUBTILIS_KEYWORD_NEXT, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label->reg = subtilis_ir_section_new_label(p->current);
}

static void prv_for_step_real_var(subtilis_parser_t *p, subtilis_token_t *t,
				  subtilis_exp_t *to, subtilis_exp_t *step,
				  subtilis_for_context_t *for_ctx,
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

	if (var->type.type != SUBTILIS_TYPE_REAL) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	var_dup = subtilis_type_if_dup(var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	to_dup = subtilis_type_if_dup(to, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
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
				 subtilis_for_context_t *for_ctx,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t final_label;
	subtilis_ir_operand_t op2;
	size_t eor_reg;
	size_t step_dir_reg = SIZE_MAX;
	subtilis_exp_t *conde = NULL;
	subtilis_exp_t *var = NULL;
	subtilis_exp_t *eor_var = NULL;
	subtilis_exp_t *eor_var_dup = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *sub = NULL;
	subtilis_exp_t *top_bit = NULL;

	/*
	 * TODO: Add a runtime check for step variable of 0.
	 * and generate an error.
	 */

	op2.integer = (int32_t)0x80000000;
	step_dir_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, step->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_for_loop_start(p, t, &start_label, &true_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	var = prv_increment_var(p, step, for_ctx, err);
	step = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (var->type.type != SUBTILIS_TYPE_INTEGER) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	sub = subtilis_type_if_sub(p, to, var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	var = NULL;
	to = NULL;

	op2.reg = step_dir_reg;
	eor_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, sub->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	eor_var = subtilis_exp_new_int32_var(eor_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	eor_var_dup = subtilis_type_if_dup(eor_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_exp_new_int32(0, err);
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

	top_bit = subtilis_exp_new_int32_var(step_dir_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(conde);
	conde = subtilis_type_if_neq(p, eor_var_dup, top_bit, err);
	eor_var_dup = NULL;
	top_bit = NULL;
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
	subtilis_exp_delete(step);
}

static void prv_for_step_const(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_exp_t *to, subtilis_exp_t *step,
			       subtilis_step_type_t step_type,
			       subtilis_for_context_t *for_ctx,
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
			    subtilis_exp_t *to, subtilis_for_context_t *for_ctx,
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
	subtilis_step_type_t step_type = SUBTILIS_STEP_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	var_type = t->tok.id_type;
	if ((var_type.type != SUBTILIS_TYPE_REAL) &&
	    (var_type.type != SUBTILIS_TYPE_INTEGER)) {
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

	prv_for_assignment(p, t, &for_ctx, err);
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
			if (var_type.type == SUBTILIS_TYPE_INTEGER)
				prv_for_step_int_var(p, t, to, step, &for_ctx,
						     err);
			else
				prv_for_step_real_var(p, t, to, step, &for_ctx,
						      err);
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
