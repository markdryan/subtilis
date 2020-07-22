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

#include <stddef.h>

#include "builtins_ir.h"
#include "parser_exp.h"
#include "parser_input.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_get(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e;

	reg = subtilis_ir_section_add_instr1(p->current, SUBTILIS_OP_INSTR_GET,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

static subtilis_exp_t *prv_inkey_const(subtilis_parser_t *p,
				       subtilis_token_t *t, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t itype;
	subtilis_ir_operand_t op1;
	int32_t inkey_value = e->exp.ir_op.integer;

	if (inkey_value == -256) {
		reg = subtilis_ir_section_add_instr1(
		    p->current, SUBTILIS_OP_INSTR_OS_BYTE_ID, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		if (inkey_value >= 0)
			itype = SUBTILIS_OP_INSTR_GET_TO;
		else
			itype = SUBTILIS_OP_INSTR_INKEY;

		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		op1.reg = e->exp.ir_op.reg;
		;
		reg =
		    subtilis_ir_section_add_instr2(p->current, itype, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_exp_delete(e);

	return subtilis_exp_new_int32_var(reg, err);

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_inkey(subtilis_parser_t *p, subtilis_token_t *t,
				      subtilis_error_t *err)
{
	subtilis_exp_t *e;
	size_t reg;
	subtilis_ir_section_t *current;

	e = subtilis_parser_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER)
		return prv_inkey_const(p, t, e, err);

	reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	current = subtilis_builtins_ir_add_1_arg_int(
	    p, "_inkey", &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_inkey(current, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_1_arg_fn(
	    p, "_inkey", reg, SUBTILIS_BUILTINS_MAX,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, true, err);
}
