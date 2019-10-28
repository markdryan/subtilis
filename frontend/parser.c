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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../common/error.h"
#include "../common/error_codes.h"
#include "array_type.h"
#include "builtins_ir.h"
#include "expression.h"
#include "globals.h"
#include "parser.h"
#include "parser_array.h"
#include "parser_assignment.h"
#include "parser_call.h"
#include "parser_compound.h"
#include "parser_error.h"
#include "parser_exp.h"
#include "parser_graphics.h"
#include "parser_output.h"
#include "type_if.h"
#include "variable.h"

#define SUBTILIS_MAIN_FN "subtilis_main"

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       subtilis_backend_caps_t caps,
				       subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_parser_t *p = calloc(1, sizeof(*p));
	subtilis_type_section_t *stype = NULL;

	if (!p) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	p->handle_escapes = true;

	p->st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->main_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->local_st = p->main_st;

	p->prog = subtilis_ir_prog_new(err, p->handle_escapes);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	stype = subtilis_type_section_new(&subtilis_type_void, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	/*
	 * We pre-seed the symbol table here to ensure that these variables
	 * have offsets 0 and 0 + sizeof(INT).
	 */

	s = subtilis_symbol_table_insert(p->st, subtilis_eflag_hidden_var,
					 &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->eflag_offset = (int32_t)s->loc;
	p->error_offset = s->size;
	s = subtilis_symbol_table_insert(p->st, subtilis_err_hidden_var,
					 &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, SUBTILIS_MAIN_FN, 0, stype, SUBTILIS_BUILTINS_MAX,
	    l->stream->name, l->line, p->eflag_offset, p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	stype = NULL;
	p->main = p->current;

	p->l = l;
	p->caps = caps;
	p->level = 0;

	subtilis_sizet_vector_init(&p->free_list);
	subtilis_sizet_vector_init(&p->local_free_list);
	subtilis_sizet_vector_init(&p->main_free_list);

	return p;

on_error:

	subtilis_type_section_delete(stype);
	subtilis_parser_delete(p);

	return NULL;
}

void subtilis_parser_delete(subtilis_parser_t *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < p->num_calls; i++)
		subtilis_parser_call_delete(p->calls[i]);
	free(p->calls);

	subtilis_ir_prog_delete(p->prog);
	subtilis_symbol_table_delete(p->main_st);
	subtilis_symbol_table_delete(p->st);
	subtilis_sizet_vector_free(&p->free_list);
	subtilis_sizet_vector_free(&p->local_free_list);
	subtilis_sizet_vector_free(&p->main_free_list);
	free(p);
}

typedef void (*subtilis_keyword_fn)(subtilis_parser_t *p, subtilis_token_t *,
				    subtilis_error_t *);

static const subtilis_keyword_fn keyword_fns[];

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

static void prv_dim(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	if (p->current != p->main) {
		subtilis_error_dim_in_proc(err, p->l->stream->name, p->l->line);
		return;
	}

	subtilis_parser_create_array(p, t, false, err);
}

static void prv_let(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	subtilis_parser_assignment(p, t, err);
}

static void prv_end(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	if (p->current != p->main) {
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_END, err);
	} else {
		end_label.label = p->current->end_label;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}

static subtilis_keyword_type_t
prv_if_compund(subtilis_parser_t *p, subtilis_token_t *t, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;
	subtilis_keyword_type_t key_type = SUBTILIS_KEYWORD_MAX;
	unsigned int start;

	p->level++;
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
			if (p->current->endproc) {
				subtilis_error_set_useless_statement(
				    err, p->l->stream->name, p->l->line);
				return key_type;
			}
			subtilis_parser_assignment(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, "<-")) {
			subtilis_parser_return(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}

		key_type = t->tok.keyword.type;
		if ((key_type == SUBTILIS_KEYWORD_ELSE) ||
		    (key_type == SUBTILIS_KEYWORD_ENDIF))
			break;

		if (p->current->endproc) {
			subtilis_error_set_useless_statement(
			    err, p->l->stream->name, p->l->line);
			return key_type;
		}

		fn = keyword_fns[key_type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}
		fn(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return key_type;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	p->current->endproc = false;
	p->level--;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
	return key_type;
}

void subtilis_parser_statement(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	subtilis_keyword_fn fn;
	const char *tbuf;
	subtilis_keyword_type_t key_type;

	if ((p->current->endproc) &&
	    !((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	      (t->tok.keyword.type == SUBTILIS_KEYWORD_DEF))) {
		subtilis_error_set_useless_statement(err, p->l->stream->name,
						     p->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(t);
	if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_parser_assignment(p, t, err);
		return;
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		   !strcmp(tbuf, "<-")) {
		subtilis_parser_return(p, t, err);
		return;
	} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_keyword_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	key_type = t->tok.keyword.type;
	fn = keyword_fns[key_type];
	if (!fn) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		return;
	}
	fn(p, t, err);
}

static subtilis_exp_t *prv_conditional_exp(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_ir_operand_t *cond,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		cond->reg = reg;
	} else if (e->type.type == SUBTILIS_TYPE_INTEGER) {
		cond->reg = e->exp.ir_op.reg;
	} else {
		subtilis_error_set_integer_exp_expected(err, p->l->stream->name,
							p->l->line);
		goto cleanup;
	}

	return e;

cleanup:
	subtilis_exp_delete(e);
	return NULL;
}

static void prv_if(subtilis_parser_t *p, subtilis_token_t *t,
		   subtilis_error_t *err)
{
	subtilis_exp_t *e;
	const char *tbuf;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;
	subtilis_ir_operand_t end_label;
	subtilis_keyword_type_t key_type;

	e = prv_conditional_exp(p, t, &cond, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (t->tok.keyword.type != SUBTILIS_KEYWORD_THEN)) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_expected(err, "THEN", tbuf,
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	true_label.reg = subtilis_ir_section_new_label(p->current);
	false_label.reg = subtilis_ir_section_new_label(p->current);

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  cond, true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	key_type = prv_if_compund(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (key_type == SUBTILIS_KEYWORD_ELSE) {
		end_label.reg = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_label(p->current, false_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_parser_compound(p, t, SUBTILIS_KEYWORD_ENDIF, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_ir_section_add_label(p->current, end_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		subtilis_ir_section_add_label(p->current, false_label.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	subtilis_exp_delete(e);
}

static void prv_while(subtilis_parser_t *p, subtilis_token_t *t,
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

	e = prv_conditional_exp(p, t, &cond, err);
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

static void prv_repeat(subtilis_parser_t *p, subtilis_token_t *t,
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

	e = prv_conditional_exp(p, t, &cond, err);
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

static void prv_for(subtilis_parser_t *p, subtilis_token_t *t,
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

static void prv_onerror(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_ir_operand_t handler_label;
	subtilis_ir_operand_t target_label;
	unsigned int start;

	if (p->current->in_error_handler) {
		subtilis_error_set_nested_handler(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	p->current->in_error_handler = true;
	handler_label.label = subtilis_ir_section_new_label(p->current);
	subtilis_ir_section_add_label(p->current, handler_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * The error flag needs to be clear when we're inside an error
	 * handler.  Otherwise and procedures or functions call in that
	 * handler may appear to have failed when they return.
	 */

	subtilis_var_set_eflag(p, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->level++;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_ENDERROR))
			break;
		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	if (!p->current->endproc) {
		if (!p->current->handler_list) {
			if (p->current != p->main) {
				subtilis_var_set_eflag(p, true, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
			}
			subtilis_exp_return_default_value(p, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else {
			target_label.label = p->current->handler_list->label;
			subtilis_ir_section_add_instr_no_reg(
			    p->current, SUBTILIS_OP_INSTR_JMP, target_label,
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}
	}

	p->current->handler_list = subtilis_handler_list_update(
	    p->current->handler_list, p->level - 1, handler_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->level--;
	p->current->endproc = false;

	subtilis_lexer_get(p->l, t, err);

cleanup:
	p->current->in_error_handler = false;
}

static void prv_error(subtilis_parser_t *p, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return;
	}

	subtilis_exp_generate_error(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}

static void prv_proc(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	(void)subtilis_parser_call(p, t, err);
}

static void prv_endproc(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	if (p->current == p->main) {
		subtilis_error_set_proc_in_main(err, p->l->stream->name,
						p->l->line);
		return;
	}

	if (p->current->type->return_type.type != SUBTILIS_TYPE_VOID) {
		subtilis_error_set_proc_in_fn(err, p->l->stream->name,
					      p->l->line);
		return;
	}

	p->current->endproc = true;
	end_label.label = p->current->end_label;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
}

static void prv_root(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_exp_t *seed;
	subtilis_ir_operand_t var_reg;

	seed = subtilis_exp_new_int32(0x3fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	seed = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_eflag_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	seed = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_err_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_locals(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	p->main->locals = p->main_st->allocated;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_ir_section_add_label(p->current, p->current->end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_parser_deallocate_arrays(p, var_reg, &p->main_free_list, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var_reg.reg = SUBTILIS_IR_REG_GLOBAL;
	subtilis_parser_deallocate_arrays(p, var_reg, &p->free_list, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_END,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_merge_errors(p->current, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_gen_index_error_code(p, err);
}

static void prv_check_call(subtilis_parser_t *p, subtilis_parser_call_t *call,
			   subtilis_error_t *err)
{
	size_t index;
	size_t call_index;
	size_t i;
	subtilis_type_section_t *st;
	const char *expected_typname;
	const char *got_typname;
	size_t new_reg;
	subtilis_op_instr_type_t itype;
	subtilis_ir_reg_type_t reg_type;
	subtilis_ir_call_t *call_site;
	subtilis_type_section_t *ct = call->call_type;

	if (!subtilis_string_pool_find(p->prog->string_pool, call->name,
				       &index)) {
		if (!ct)
			subtilis_error_set_assertion_failed(err);
		else if (ct->return_type.type == SUBTILIS_TYPE_VOID)
			subtilis_error_set_unknown_procedure(
			    err, call->name, p->l->stream->name, call->line);
		else
			subtilis_error_set_unknown_function(
			    err, call->name, p->l->stream->name, call->line);
		return;
	}

	/* We need to fix up the call site of procedures call in an error
	 * handler.
	 */

	call_index = call->index;
	if (call->in_error_handler)
		call_index += call->s->handler_offset;

	/*
	 * If the call has no type section that it is not a real function.
	 * It's an operator that is being implemented as a function call
	 * as the underlying CPU doesn't have an instruction that maps
	 * nicely onto the operator, e.g., DIV on ARM.  In these cases
	 * there's no need to perform the type checks as the parser has
	 * already done them.
	 */

	/*
	 * This assumes that builtins don't require any type promotions.
	 */

	if (!ct) {
		call->s->ops[call_index]->op.call.proc_id = index;
		return;
	}

	st = p->prog->sections[index]->type;

	if ((st->return_type.type == SUBTILIS_TYPE_VOID) &&
	    (ct->return_type.type != SUBTILIS_TYPE_VOID)) {
		subtilis_error_set_procedure_expected(
		    err, call->name, p->l->stream->name, call->line);
		return;
	} else if ((st->return_type.type != SUBTILIS_TYPE_VOID) &&
		   (ct->return_type.type == SUBTILIS_TYPE_VOID)) {
		subtilis_error_set_function_expected(
		    err, call->name, p->l->stream->name, call->line);
		return;
	}

	if (st->num_parameters != ct->num_parameters) {
		subtilis_error_set_bad_arg_count(
		    err, ct->num_parameters, st->num_parameters,
		    p->l->stream->name, call->line);
		return;
	}

	call_site = &call->s->ops[call_index]->op.call;
	for (i = 0; i < st->num_parameters; i++) {
		if (st->parameters[i].type == ct->parameters[i].type)
			continue;

		if ((st->parameters[i].type == SUBTILIS_TYPE_REAL) &&
		    (ct->parameters[i].type == SUBTILIS_TYPE_INTEGER)) {
			itype = SUBTILIS_OP_INSTR_MOV_I32_FP;
			reg_type = SUBTILIS_IR_REG_TYPE_REAL;
		} else if ((st->parameters[i].type == SUBTILIS_TYPE_INTEGER) &&
			   (ct->parameters[i].type == SUBTILIS_TYPE_REAL)) {
			itype = SUBTILIS_OP_INSTR_MOV_FP_I32;
			reg_type = SUBTILIS_IR_REG_TYPE_INTEGER;
		} else {
			expected_typname =
			    subtilis_type_name(&st->parameters[i]);
			got_typname = subtilis_type_name(&ct->parameters[i]);
			subtilis_error_set_bad_arg_type(
			    err, i + 1, expected_typname, got_typname,
			    p->l->stream->name, call->line, __FILE__, __LINE__);
			return;
		}

		switch (call->s->ops[call_index]->type) {
		case SUBTILIS_OP_CALL:
		case SUBTILIS_OP_CALLI32:
		case SUBTILIS_OP_CALLREAL:
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			return;
		}

		new_reg = subtilis_ir_section_promote_nop(
		    call->s, call_site->args[i].nop, itype,
		    call_site->args[i].reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		call_site->args[i].reg = new_reg;
		call_site->args[i].type = reg_type;
	}

	call_site->proc_id = index;
}

static void prv_check_calls(subtilis_parser_t *p, subtilis_error_t *err)
{
	size_t i;

	for (i = 0; i < p->num_calls; i++) {
		prv_check_call(p, p->calls[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_parse(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_token_t *t = NULL;

	t = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_root(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_check_calls(p, err);

cleanup:

	subtilis_token_delete(t);
}

/* The ordering of this table is very important.  The functions it
 * contains must correspond to the enumerated types in
 * subtilis_keyword_type_t
 */

/* clang-format off */
static const subtilis_keyword_fn keyword_fns[] = {
	NULL, /* SUBTILIS_KEYWORD_ABS */
	NULL, /* SUBTILIS_KEYWORD_ACS */
	NULL, /* SUBTILIS_KEYWORD_ADVAL */
	NULL, /* SUBTILIS_KEYWORD_AND */
	NULL, /* SUBTILIS_KEYWORD_APPEND */
	NULL, /* SUBTILIS_KEYWORD_ASC */
	NULL, /* SUBTILIS_KEYWORD_ASN */
	NULL, /* SUBTILIS_KEYWORD_ATN */
	NULL, /* SUBTILIS_KEYWORD_AUTO */
	NULL, /* SUBTILIS_KEYWORD_BEAT */
	NULL, /* SUBTILIS_KEYWORD_BEATS */
	NULL, /* SUBTILIS_KEYWORD_BGET_HASH */
	NULL, /* SUBTILIS_KEYWORD_BPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_BY */
	NULL, /* SUBTILIS_KEYWORD_CALL */
	NULL, /* SUBTILIS_KEYWORD_CASE */
	NULL, /* SUBTILIS_KEYWORD_CHAIN */
	NULL, /* SUBTILIS_KEYWORD_CHR_STR */
	subtilis_parser_circle, /* SUBTILIS_KEYWORD_CIRCLE */
	NULL, /* SUBTILIS_KEYWORD_CLEAR */
	subtilis_parser_clg, /* SUBTILIS_KEYWORD_CLG */
	NULL, /* SUBTILIS_KEYWORD_CLOSE_HASH */
	subtilis_parser_cls, /* SUBTILIS_KEYWORD_CLS */
	NULL, /* SUBTILIS_KEYWORD_COLOR */
	NULL, /* SUBTILIS_KEYWORD_COLOUR */
	NULL, /* SUBTILIS_KEYWORD_COS */
	NULL, /* SUBTILIS_KEYWORD_COUNT */
	NULL, /* SUBTILIS_KEYWORD_CRUNCH */
	NULL, /* SUBTILIS_KEYWORD_DATA */
	subtilis_parser_def, /* SUBTILIS_KEYWORD_DEF */
	NULL, /* SUBTILIS_KEYWORD_DEG */
	NULL, /* SUBTILIS_KEYWORD_DELETE */
	prv_dim, /* SUBTILIS_KEYWORD_DIM */
	NULL, /* SUBTILIS_KEYWORD_DIV */
	subtilis_parser_draw, /* SUBTILIS_KEYWORD_DRAW */
	NULL, /* SUBTILIS_KEYWORD_EDIT */
	NULL, /* SUBTILIS_KEYWORD_ELLIPSE */
	NULL, /* SUBTILIS_KEYWORD_ELSE */
	prv_end, /* SUBTILIS_KEYWORD_END */
	NULL, /* SUBTILIS_KEYWORD_ENDCASE */
	NULL, /* SUBTILIS_KEYWORD_ENDERROR */
	NULL, /* SUBTILIS_KEYWORD_ENDIF */
	prv_endproc, /* SUBTILIS_KEYWORD_ENDPROC */
	NULL, /* SUBTILIS_KEYWORD_ENDWHILE */
	NULL, /* SUBTILIS_KEYWORD_EOF_HASH */
	NULL, /* SUBTILIS_KEYWORD_EOR */
	NULL, /* SUBTILIS_KEYWORD_ERL */
	NULL, /* SUBTILIS_KEYWORD_ERR */
	prv_error, /* SUBTILIS_KEYWORD_ERROR */
	NULL, /* SUBTILIS_KEYWORD_EVAL */
	NULL, /* SUBTILIS_KEYWORD_EXP */
	NULL, /* SUBTILIS_KEYWORD_EXT_HASH */
	NULL, /* SUBTILIS_KEYWORD_FALSE */
	subtilis_parser_fill, /* SUBTILIS_KEYWORD_FILL */
	NULL, /* SUBTILIS_KEYWORD_FN */
	prv_for, /* SUBTILIS_KEYWORD_FOR */
	subtilis_parser_gcol, /* SUBTILIS_KEYWORD_GCOL */
	NULL, /* SUBTILIS_KEYWORD_GET */
	NULL, /* SUBTILIS_KEYWORD_GET_STR */
	NULL, /* SUBTILIS_KEYWORD_GET_STR_HASH */
	NULL, /* SUBTILIS_KEYWORD_GOSUB */
	NULL, /* SUBTILIS_KEYWORD_GOTO */
	NULL, /* SUBTILIS_KEYWORD_HELP */
	NULL, /* SUBTILIS_KEYWORD_HIMEM */
	prv_if, /* SUBTILIS_KEYWORD_IF */
	NULL, /* SUBTILIS_KEYWORD_INKEY */
	NULL, /* SUBTILIS_KEYWORD_INKEY_STR */
	NULL, /* SUBTILIS_KEYWORD_INPUT */
	NULL, /* SUBTILIS_KEYWORD_INPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_INSTALL */
	NULL, /* SUBTILIS_KEYWORD_INSTR */
	NULL, /* SUBTILIS_KEYWORD_INT */
	NULL, /* SUBTILIS_KEYWORD_LEFT_STR */
	NULL, /* SUBTILIS_KEYWORD_LEN */
	prv_let, /* SUBTILIS_KEYWORD_LET */
	NULL, /* SUBTILIS_KEYWORD_LIBRARY */
	subtilis_parser_line, /* SUBTILIS_KEYWORD_LINE */
	NULL, /* SUBTILIS_KEYWORD_LIST */
	NULL, /* SUBTILIS_KEYWORD_LISTO */
	NULL, /* SUBTILIS_KEYWORD_LN */
	NULL, /* SUBTILIS_KEYWORD_LOAD */
	NULL, /* SUBTILIS_KEYWORD_LOCAL */
	NULL, /* SUBTILIS_KEYWORD_LOG */
	NULL, /* SUBTILIS_KEYWORD_LOMEM */
	NULL, /* SUBTILIS_KEYWORD_LVAR */
	NULL, /* SUBTILIS_KEYWORD_MID_STR */
	NULL, /* SUBTILIS_KEYWORD_MOD */
	subtilis_parser_mode, /* SUBTILIS_KEYWORD_MODE */
	NULL, /* SUBTILIS_KEYWORD_MOUSE */
	subtilis_parser_move, /* SUBTILIS_KEYWORD_MOVE */
	NULL, /* SUBTILIS_KEYWORD_NEW */
	NULL, /* SUBTILIS_KEYWORD_NEXT */
	NULL, /* SUBTILIS_KEYWORD_NOT */
	NULL, /* SUBTILIS_KEYWORD_OF */
	subtilis_parser_off, /* SUBTILIS_KEYWORD_OFF */
	NULL, /* SUBTILIS_KEYWORD_OLD */
	subtilis_parser_on, /* SUBTILIS_KEYWORD_ON */
	prv_onerror, /* SUBTILIS_KEYWORD_ONERROR */
	NULL, /* SUBTILIS_KEYWORD_OPENIN */
	NULL, /* SUBTILIS_KEYWORD_OPENOUT */
	NULL, /* SUBTILIS_KEYWORD_OPENUP */
	NULL, /* SUBTILIS_KEYWORD_OR */
	subtilis_parser_origin, /* SUBTILIS_KEYWORD_ORIGIN */
	NULL, /* SUBTILIS_KEYWORD_OSCLI */
	NULL, /* SUBTILIS_KEYWORD_OTHERWISE */
	NULL, /* SUBTILIS_KEYWORD_OVERLAY */
	NULL, /* SUBTILIS_KEYWORD_PAGE */
	NULL, /* SUBTILIS_KEYWORD_PI */
	subtilis_parser_plot, /* SUBTILIS_KEYWORD_PLOT */
	subtilis_parser_point, /* SUBTILIS_KEYWORD_POINT */
	NULL, /* SUBTILIS_KEYWORD_POS */
	subtilis_parser_print, /* SUBTILIS_KEYWORD_PRINT */
	NULL, /* SUBTILIS_KEYWORD_PRINT_HASH */
	prv_proc, /* SUBTILIS_KEYWORD_PROC */
	NULL, /* SUBTILIS_KEYWORD_PTR_HASH */
	NULL, /* SUBTILIS_KEYWORD_QUIT */
	NULL, /* SUBTILIS_KEYWORD_RAD */
	NULL, /* SUBTILIS_KEYWORD_READ */
	subtilis_parser_rectangle, /* SUBTILIS_KEYWORD_RECTANGLE */
	NULL, /* SUBTILIS_KEYWORD_REM */
	NULL, /* SUBTILIS_KEYWORD_RENUMBER */
	prv_repeat, /* SUBTILIS_KEYWORD_REPEAT */
	NULL, /* SUBTILIS_KEYWORD_REPORT */
	NULL, /* SUBTILIS_KEYWORD_REPORT_STR */
	NULL, /* SUBTILIS_KEYWORD_RESTORE */
	NULL, /* SUBTILIS_KEYWORD_RETURN */
	NULL, /* SUBTILIS_KEYWORD_RIGHT_STR */
	NULL, /* SUBTILIS_KEYWORD_RND */
	NULL, /* SUBTILIS_KEYWORD_RUN */
	NULL, /* SUBTILIS_KEYWORD_SAVE */
	NULL, /* SUBTILIS_KEYWORD_SGN */
	NULL, /* SUBTILIS_KEYWORD_SIN */
	NULL, /* SUBTILIS_KEYWORD_SOUND */
	NULL, /* SUBTILIS_KEYWORD_SPC */
	NULL, /* SUBTILIS_KEYWORD_SQR */
	NULL, /* SUBTILIS_KEYWORD_STEP */
	NULL, /* SUBTILIS_KEYWORD_STEREO */
	NULL, /* SUBTILIS_KEYWORD_STOP */
	NULL, /* SUBTILIS_KEYWORD_STRING_STR */
	NULL, /* SUBTILIS_KEYWORD_STRS */
	NULL, /* SUBTILIS_KEYWORD_SUM */
	NULL, /* SUBTILIS_KEYWORD_SUMLEN */
	NULL, /* SUBTILIS_KEYWORD_SWAP */
	NULL, /* SUBTILIS_KEYWORD_SYS */
	NULL, /* SUBTILIS_KEYWORD_TAB */
	NULL, /* SUBTILIS_KEYWORD_TAN */
	NULL, /* SUBTILIS_KEYWORD_TEMPO */
	NULL, /* SUBTILIS_KEYWORD_TEXTLOAD */
	NULL, /* SUBTILIS_KEYWORD_TEXTSAVE */
	NULL, /* SUBTILIS_KEYWORD_THEN */
	NULL, /* SUBTILIS_KEYWORD_TIME */
	NULL, /* SUBTILIS_KEYWORD_TIME_STR */
	NULL, /* SUBTILIS_KEYWORD_TINT */
	NULL, /* SUBTILIS_KEYWORD_TO */
	NULL, /* SUBTILIS_KEYWORD_TOP */
	NULL, /* SUBTILIS_KEYWORD_TRACE */
	NULL, /* SUBTILIS_KEYWORD_TRUE */
	NULL, /* SUBTILIS_KEYWORD_TWIN */
	NULL, /* SUBTILIS_KEYWORD_UNTIL */
	NULL, /* SUBTILIS_KEYWORD_USR */
	NULL, /* SUBTILIS_KEYWORD_VAL */
	subtilis_parser_vdu, /* SUBTILIS_KEYWORD_VDU */
	NULL, /* SUBTILIS_KEYWORD_VOICES */
	NULL, /* SUBTILIS_KEYWORD_VPOS */
	subtilis_parser_wait, /* SUBTILIS_KEYWORD_WAIT */
	NULL, /* SUBTILIS_KEYWORD_WHEN */
	prv_while, /* SUBTILIS_KEYWORD_WHILE */
	NULL, /* SUBTILIS_KEYWORD_WIDTH */
};

/* clang-format on */
