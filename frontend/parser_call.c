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
#include "call.h"
#include "expression.h"
#include "parser_array.h"
#include "parser_call.h"
#include "parser_compound.h"
#include "parser_error.h"
#include "parser_exp.h"
#include "type_if.h"

struct subtilis_parser_param_t_ {
	subtilis_type_t type;
	size_t reg;
	size_t nop;
};

typedef struct subtilis_parser_param_t_ subtilis_parser_param_t;

static subtilis_ir_arg_t *prv_parser_to_ir_args(subtilis_parser_param_t *params,
						size_t num_params,
						subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_arg_t *args = malloc(sizeof(*args) * num_params);

	if (!args) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	for (i = 0; i < num_params; i++) {
		switch (params[i].type.type) {
		case SUBTILIS_TYPE_REAL:
			args[i].type = SUBTILIS_IR_REG_TYPE_REAL;
			break;
		case SUBTILIS_TYPE_INTEGER:
			args[i].type = SUBTILIS_IR_REG_TYPE_INTEGER;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			free(args);
			return NULL;
		}
		args[i].reg = params[i].reg;
		args[i].nop = params[i].nop;
	}

	return args;
}

static subtilis_type_t *prv_parser_to_ptypes(subtilis_parser_param_t *params,
					     size_t num_params,
					     subtilis_error_t *err)
{
	size_t i;
	subtilis_type_t *ptypes = malloc(sizeof(*ptypes) * num_params);

	if (!ptypes) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	for (i = 0; i < num_params; i++)
		ptypes[i] = params[i].type;

	return ptypes;
}

static subtilis_parser_param_t *prv_call_parameters(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    size_t *num_parameters,
						    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_parser_param_t *new_params;
	size_t nop;
	size_t new_max;
	size_t max_params = 0;
	size_t num_params = 0;
	subtilis_parser_param_t *params = NULL;
	subtilis_exp_t *e = NULL;

	for (;;) {
		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		if (num_params == max_params) {
			new_max = max_params + 16;
			new_params = realloc(params, new_max * sizeof(*params));
			if (!new_params) {
				subtilis_error_set_oom(err);
				goto on_error;
			}
			max_params = new_max;
			params = new_params;
		}

		tbuf = subtilis_token_get_text(t);

		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		params[num_params].type = e->type;
		nop = subtilis_ir_section_add_nop(p->current, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		params[num_params].reg = e->exp.ir_op.reg;
		params[num_params].nop = nop;
		num_params++;
		subtilis_exp_delete(e);
		e = NULL;

		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		if (!strcmp(tbuf, ")"))
			goto read_next;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}
	}

read_next:

	*num_parameters = num_params;
	subtilis_lexer_get(p->l, t, err);
	return params;

on_error:

	subtilis_exp_delete(e);
	free(params);
	return NULL;
}

static char *prv_proc_name_and_type(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_type_t *type,
				    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t id_type;
	char *name = NULL;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_expected(err, "PROC or FN", tbuf,
					    p->l->stream->name, p->l->line);
		return NULL;
	}

	if (t->tok.keyword.type == SUBTILIS_KEYWORD_PROC)
		tbuf += 4;
	else if (t->tok.keyword.type == SUBTILIS_KEYWORD_FN)
		tbuf += 2;
	id_type = t->tok.keyword.id_type;

	name = malloc(strlen(tbuf) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	*type = id_type;
	strcpy(name, tbuf);

	return name;
}

subtilis_exp_t *subtilis_parser_call(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t fn_type;
	subtilis_type_section_t *stype = NULL;
	subtilis_type_t *ptypes = NULL;
	char *name = NULL;
	subtilis_parser_param_t *params = NULL;
	size_t num_params = 0;
	subtilis_ir_arg_t *args = NULL;

	name = prv_proc_name_and_type(p, t, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		params = prv_call_parameters(p, t, &num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		args = prv_parser_to_ir_args(params, num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		ptypes = prv_parser_to_ptypes(params, num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	free(params);

	stype = subtilis_type_section_new(&fn_type, num_params, ptypes, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	/* Ownership of stypes, args and name passed to this function. */

	return subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MAX, stype,
				     args, &fn_type, num_params, err);

on_error:

	free(params);
	free(ptypes);
	free(args);
	free(name);

	return NULL;
}

static subtilis_type_t *prv_def_parameters(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   size_t *num_parameters,
					   subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t *new_params;
	size_t new_max;
	size_t max_params = 0;
	size_t num_params = 0;
	size_t num_iparams = 0;
	size_t num_fparams = 0;
	subtilis_type_t *params = NULL;
	size_t reg_num;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")"))
		goto read_next;

	for (;;) {
		if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
			subtilis_error_set_id_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		/* TODO: maybe these checks should go in symbol table insert */

		if (subtilis_symbol_table_lookup(p->local_st, tbuf)) {
			subtilis_error_set_already_defined(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		switch (t->tok.id_type.type) {
		case SUBTILIS_TYPE_INTEGER:
			reg_num = SUBTILIS_IR_REG_TEMP_START + num_iparams;
			num_iparams++;
			break;
		case SUBTILIS_TYPE_REAL:
			reg_num = num_fparams++;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			goto on_error;
		}

		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, tbuf, &t->tok.id_type, reg_num, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		if (num_params == max_params) {
			new_max = max_params + 16;
			new_params = realloc(params, new_max * sizeof(*params));
			if (!new_params) {
				subtilis_error_set_oom(err);
				goto on_error;
			}
			max_params = new_max;
			params = new_params;
		}
		params[num_params++] = t->tok.id_type;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		tbuf = subtilis_token_get_text(t);

		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		if (!strcmp(tbuf, ")"))
			goto read_next;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
		tbuf = subtilis_token_get_text(t);
	}

read_next:

	*num_parameters = num_params;
	subtilis_lexer_get(p->l, t, err);
	return params;

on_error:

	free(params);
	return NULL;
}

static void prv_set_fn_retval(subtilis_parser_t *p, subtilis_exp_t *e,
			      const subtilis_type_t *fn_type,
			      subtilis_error_t *err)
{
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t ret_reg;

	/* Ownership of e is passed to prv_coerce_type */

	e = subtilis_exp_coerce_type(p, e, fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (e->type.type) {
	case SUBTILIS_TYPE_CONST_INTEGER:
		type = SUBTILIS_OP_INSTR_MOVI_I32;
		break;
	case SUBTILIS_TYPE_INTEGER:
		type = SUBTILIS_OP_INSTR_MOV;
		break;
	case SUBTILIS_TYPE_CONST_REAL:
		type = SUBTILIS_OP_INSTR_MOVI_REAL;
		break;
	case SUBTILIS_TYPE_REAL:
		type = SUBTILIS_OP_INSTR_MOVFP;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	ret_reg.reg = p->current->ret_reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, type, ret_reg,
					      e->exp.ir_op, err);

cleanup:

	subtilis_exp_delete(e);
}

static void prv_add_fn_ret(subtilis_parser_t *p, const subtilis_type_t *fn_type,
			   subtilis_error_t *err)
{
	subtilis_op_instr_type_t type;
	subtilis_ir_operand_t ret_reg;

	switch (fn_type->type) {
	case SUBTILIS_TYPE_INTEGER:
		type = SUBTILIS_OP_INSTR_RET_I32;
		break;
	case SUBTILIS_TYPE_REAL:
		type = SUBTILIS_OP_INSTR_RET_REAL;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ret_reg.reg = p->current->ret_reg;
	subtilis_ir_section_add_instr_no_reg(p->current, type, ret_reg, err);
}

static void prv_fn_compound(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	unsigned int start;
	const char *tbuf;

	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		tbuf = subtilis_token_get_text(t);
		if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    !strcmp(tbuf, "<-") && (p->level == 0))
			break;

		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);
	//	p->current->endproc = false;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
}

static void prv_proc_compound(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	unsigned int start;

	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
		    (t->tok.keyword.type == SUBTILIS_KEYWORD_ENDPROC))
			break;

		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);
	//	p->current->endproc = false;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
}

void subtilis_parser_def(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_section_t *stype;
	subtilis_type_t fn_type;
	subtilis_exp_t *e;
	size_t num_params = 0;
	subtilis_type_t *params = NULL;
	subtilis_symbol_table_t *local_st = NULL;
	char *name = NULL;

	if (p->current != p->main) {
		subtilis_error_set_nested_procedure(err, p->l->stream->name,
						    p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	name = prv_proc_name_and_type(p, t, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	local_st = subtilis_symbol_table_new(err);
	p->local_st = local_st;
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		params = prv_def_parameters(p, t, &num_params, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	stype = subtilis_type_section_new(&fn_type, num_params, params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	params = NULL;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, name, 0, stype, SUBTILIS_BUILTINS_MAX, p->l->stream->name,
	    p->l->line, p->eflag_offset, p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_type_section_delete(stype);
		goto on_error;
	}

	subtilis_parser_handle_escape(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current->cleanup_stack_nop =
	    subtilis_ir_section_add_nop(p->current, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	if (fn_type.type != SUBTILIS_TYPE_VOID) {
		prv_fn_compound(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		e = subtilis_parser_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		/* Ownership of e is passed to add_fn_ret */

		if (p->current->endproc) {
			subtilis_exp_delete(e);
		} else {
			prv_set_fn_retval(p, e, &fn_type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
		}
		subtilis_ir_section_add_label(p->current, p->current->end_label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_parser_unwind(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		prv_add_fn_ret(p, &fn_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	} else {
		prv_proc_compound(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_ir_section_add_label(p->current, p->current->end_label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_parser_unwind(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_RET, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		subtilis_lexer_get(p->l, t, err);
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_ir_merge_errors(p->current, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_array_gen_index_error_code(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current->locals = p->local_st->max_allocated;

on_error:

	free(params);
	subtilis_symbol_table_delete(local_st);
	free(name);
	p->local_st = p->main_st;
	p->current = p->main;
}

void subtilis_parser_unwind(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t start_label;
	subtilis_ir_operand_t start_inner_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t counter;

	if (p->current->cleanup_stack == SIZE_MAX)
		return;

	start_label.label = subtilis_ir_section_new_label(p->current);
	start_inner_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);
	counter.reg = p->current->cleanup_stack;

	subtilis_ir_section_add_label(p->current, start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  counter, start_inner_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_label(p->current, start_inner_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_pop_and_deref(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end_label.label, err);
}

void subtilis_parser_return(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_type_t fn_type;
	subtilis_ir_operand_t end_label;

	if (p->current == p->main) {
		subtilis_error_set_return_in_main(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	fn_type = p->current->type->return_type;
	if (fn_type.type == SUBTILIS_TYPE_VOID) {
		subtilis_error_set_return_in_proc(err, p->l->stream->name,
						  p->l->line);
		return;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Ownership of e is passed to add_fn_ret */

	prv_set_fn_retval(p, e, &fn_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	end_label.label = p->current->end_label;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}

void subtilis_parser_endproc(subtilis_parser_t *p, subtilis_token_t *t,
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

void subtilis_parser_check_calls(subtilis_parser_t *p, subtilis_error_t *err)
{
	size_t i;

	for (i = 0; i < p->num_calls; i++) {
		prv_check_call(p, p->calls[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}
