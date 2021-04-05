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

#include "parser_os.h"

#include "parser_exp.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

static subtilis_exp_t **prv_get_args(subtilis_parser_t *p, subtilis_token_t *t,
				     size_t *no_args, subtilis_error_t *err)
{
	subtilis_exp_t **tmp;
	const char *tbuf;
	size_t i;
	subtilis_exp_t **args = NULL;
	size_t count = 0;
	size_t max_count = 0;
	subtilis_exp_t *e;

	do {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(t);
		if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    (!strcmp(tbuf, ","))) {
			e = NULL;
		} else {
			e = subtilis_parser_priority7(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}

		if (count == max_count) {
			if (args) {
				subtilis_error_set_sys_max_too_many_args(
				    err, p->l->stream->name, p->l->line);
				goto cleanup;
			}
			max_count += 16;
			tmp = realloc(args, sizeof(*args) * max_count);
			if (!tmp) {
				subtilis_error_set_oom(err);
				goto cleanup;
			}
			args = tmp;
		}
		args[count++] = e;
	} while ((t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, ",")));

	*no_args = count;
	return args;

cleanup:

	for (i = 0; i < count; i++)
		subtilis_exp_delete(args[i]);
	free(args);
	*no_args = 0;

	return NULL;
}

/*
 * Assumes ownership of args.
 */

static size_t *prv_process_input_args(subtilis_parser_t *p,
				      subtilis_exp_t **args, size_t no_args,
				      uint32_t *reg_mask, subtilis_error_t *err)
{
	size_t ptr;
	subtilis_exp_t *e;
	size_t *in_regs = NULL;
	size_t i = 0;

	*reg_mask = 0;

	for (i = 0; i < no_args; i++) {
		e = args[i];
		if (!e)
			continue;
		*reg_mask |= 1 << i;
		if (subtilis_type_if_is_numeric(&e->type)) {
			args[i] = subtilis_type_if_to_int(p, args[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			args[i] = subtilis_type_if_exp_to_var(p, args[i], err);
		} else if (e->type.type == SUBTILIS_TYPE_STRING) {
			args[i] = subtilis_string_zt_non_const(p, e, err);
		} else if (e->type.type == SUBTILIS_TYPE_CONST_STRING) {
			args[i] = subtilis_string_lca_const_zt(p, e, err);
		} else if ((e->type.type == SUBTILIS_TYPE_ARRAY_INTEGER) ||
			   (e->type.type == SUBTILIS_TYPE_ARRAY_INTEGER)) {
			ptr = subtilis_reference_get_data(p, e->exp.ir_op.reg,
							  0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			subtilis_exp_delete(e);
			args[i] = subtilis_exp_new_int32_var(ptr, err);
		} else {
			subtilis_error_set_sys_bad_args(err, p->l->stream->name,
							p->l->line);
			goto cleanup;
		}
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	in_regs = calloc(no_args, sizeof(*in_regs));
	if (!in_regs) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	for (i = 0; i < no_args; i++)
		if (!args[i])
			in_regs[i] = SIZE_MAX;
		else
			in_regs[i] = args[i]->exp.ir_op.reg;

cleanup:

	for (i = 0; i < no_args; i++)
		subtilis_exp_delete(args[i]);
	free(args);

	return in_regs;
}

static subtilis_ir_sys_out_reg_t *
prv_get_output_args(subtilis_parser_t *p, subtilis_token_t *t, size_t *no_args,
		    uint32_t *reg_mask, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;
	bool local;
	subtilis_type_t type;
	subtilis_ir_sys_out_reg_t *args = NULL;
	size_t count = 0;
	size_t max_count = 0;

	*reg_mask = 0;

	do {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(t);
		if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    (!strcmp(tbuf, ","))) {
			e = NULL;
		} else {
			*reg_mask |= 1 << count;
			e = subtilis_var_lookup_ref(p, t, &local, &type, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			if (e->type.type != SUBTILIS_TYPE_INTEGER) {
				subtilis_error_set_sys_bad_args(
				    err, p->l->stream->name, p->l->line);
				goto cleanup;
			}
		}

		if (count == max_count) {
			if (args) {
				subtilis_error_set_sys_max_too_many_args(
				    err, p->l->stream->name, p->l->line);
				goto cleanup;
			}
			max_count += 16;
			args = malloc(sizeof(*args) * max_count);
			if (!args) {
				subtilis_error_set_oom(err);
				goto cleanup;
			}
		}

		if (e) {
			args[count].reg = e->exp.ir_op.reg;
			args[count].local = local;
			subtilis_exp_delete(e);
			e = NULL;
		} else {
			args[count].reg = SIZE_MAX;
		}
		count++;
	} while ((t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, ",")));

	*no_args = count;
	return args;

cleanup:

	subtilis_exp_delete(e);
	free(args);
	*no_args = 0;

	return NULL;
}

static size_t prv_handle_flags(subtilis_parser_t *p, subtilis_token_t *t,
			       bool *is_local, subtilis_error_t *err)
{
	subtilis_exp_t *e;
	bool local;
	subtilis_type_t type;
	size_t ret_val = SIZE_MAX;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	e = subtilis_var_lookup_ref(p, t, &local, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (e->type.type != SUBTILIS_TYPE_INTEGER) {
		subtilis_error_set_sys_bad_args(err, p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	*is_local = local;
	ret_val = e->exp.ir_op.reg;

cleanup:

	subtilis_exp_delete(e);
	return ret_val;
}

void subtilis_parser_sys(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	subtilis_exp_t *call_name;
	const char *tbuf;
	size_t i;
	const char *swi_name;
	size_t translated;
	bool handle_errors = false;
	uint32_t expected_in_regs = 0;
	uint32_t expected_out_regs = 0;
	uint32_t found_in_regs = 0;
	uint32_t found_out_regs = 0;
	bool flags_local = false;
	size_t flags_reg = SIZE_MAX;
	size_t no_inputs = 0;
	size_t no_outputs = 0;
	subtilis_exp_t **input_args = NULL;
	size_t *in_regs = NULL;
	subtilis_ir_sys_out_reg_t *out_regs = NULL;

	if (!p->backend.sys_check || !p->backend.sys_trans) {
		subtilis_error_set_not_supported(
		    err, "SYS on this target", p->l->stream->name, p->l->line);
		return;
	}

	call_name = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (subtilis_type_if_is_numeric(&call_name->type)) {
		call_name = subtilis_type_if_to_int(p, call_name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if (call_name->type.type == SUBTILIS_TYPE_CONST_STRING) {
		swi_name = subtilis_buffer_get_string(&call_name->exp.str);
		translated = p->backend.sys_trans(swi_name);
		if (translated == SIZE_MAX) {
			subtilis_error_set_sys_call_unknown(
			    err, swi_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		subtilis_exp_delete(call_name);
		call_name = subtilis_exp_new_int32((int32_t)translated, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if (call_name->type.type != SUBTILIS_TYPE_CONST_INTEGER) {
		subtilis_error_set_expected(
		    err, "String or Integer constant expected",
		    subtilis_type_name(&call_name->type), p->l->stream->name,
		    p->l->line);
		goto cleanup;
	}

	/*
	 * At this point call_name must be a constant integer.
	 */

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		input_args = prv_get_args(p, t, &no_inputs, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (no_inputs == 0) {
			subtilis_error_set_sys_bad_args(err, p->l->stream->name,
							p->l->line);
			goto cleanup;
		}

		in_regs = prv_process_input_args(p, input_args, no_inputs,
						 &found_in_regs, err);
		input_args = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if ((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	    (t->tok.keyword.type == SUBTILIS_KEYWORD_TO)) {
		out_regs = prv_get_output_args(p, t, &no_outputs,
					       &found_out_regs, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (no_outputs == 0) {
			subtilis_error_set_sys_bad_args(err, p->l->stream->name,
							p->l->line);
			goto cleanup;
		}
	}

	if (!p->backend.sys_check(call_name->exp.ir_op.integer,
				  &expected_in_regs, &expected_out_regs,
				  &handle_errors)) {
		subtilis_error_set_not_supported(err, "SWI", p->l->stream->name,
						 p->l->line);
		goto cleanup;
	}

	if ((found_in_regs | expected_in_regs) != expected_in_regs) {
		subtilis_error_set_sys_bad_args(err, p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	if ((found_out_regs | expected_out_regs) != expected_out_regs) {
		subtilis_error_set_sys_bad_args(err, p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ";")) {
		flags_reg = prv_handle_flags(p, t, &flags_local, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		handle_errors = false;
	}

	/*
	 * TODO,
	 * 5. Allow unknown constant integer call_names
	 * 9. Add missing SWIs
	 */

	subtilis_ir_section_add_sys_call(
	    p->current, call_name->exp.ir_op.integer, in_regs, out_regs,
	    found_in_regs, found_out_regs, flags_reg, flags_local, err);
	in_regs = NULL;
	out_regs = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (handle_errors)
		subtilis_exp_handle_errors(p, err);

cleanup:

	subtilis_exp_delete(call_name);

	free(out_regs);
	free(in_regs);

	if (input_args) {
		for (i = 0; i < no_inputs; i++)
			subtilis_exp_delete(input_args[i]);
		free(input_args);
	}
}

void subtilis_parser_oscli(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_exp_t *command;

	command = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (command->type.type == SUBTILIS_TYPE_STRING) {
		command = subtilis_string_zt_non_const(p, command, err);
	} else if (command->type.type == SUBTILIS_TYPE_CONST_STRING) {
		command = subtilis_string_lca_const_zt(p, command, err);
	} else {
		subtilis_error_set_string_expected(err, p->l->stream->name,
						   p->l->line);
		goto cleanup;
	}

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_OSCLI, command->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);

cleanup:

	subtilis_exp_delete(command);
}
