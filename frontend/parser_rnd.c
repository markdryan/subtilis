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

#include <string.h>

#include "builtins_ir.h"
#include "globals.h"
#include "parser_exp.h"
#include "parser_rnd.h"
#include "type_if.h"
#include "variable.h"

static subtilis_exp_t *prv_rnd_var(subtilis_parser_t *p, subtilis_exp_t *e,
				   subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_section_t *fn;
	size_t zero;
	size_t one;
	size_t cond;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t else_label;
	subtilis_ir_operand_t zero_one_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t result;

	zero_one_label.label = subtilis_ir_section_new_label(p->current);
	else_label.label = subtilis_ir_section_new_label(p->current);
	end_label.label = subtilis_ir_section_new_label(p->current);

	result.reg = p->current->freg_counter++;
	reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);
	e = NULL;

	fn = subtilis_builtins_ir_add_1_arg_int(p, "_rnd_int",
						&subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_rnd_int(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	fn = subtilis_builtins_ir_add_1_arg_int(p, "_rnd_real",
						&subtilis_type_real, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_rnd_real(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = reg;
	op2.integer = 0;
	zero = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 1;
	one = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = zero;
	op2.reg = one;
	cond = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = cond;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op1, zero_one_label, else_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, zero_one_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_parser_call_1_arg_fn(p, "_rnd_real", reg,
					  SUBTILIS_IR_REG_TYPE_INTEGER,
					  &subtilis_type_real, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVFP, result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);
	e = NULL;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, else_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_parser_call_1_arg_fn(p, "_rnd_int", reg,
					  SUBTILIS_IR_REG_TYPE_INTEGER,
					  &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(p->current,
					      SUBTILIS_OP_INSTR_MOV_I32_FP,
					      result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);
	e = NULL;

	subtilis_ir_section_add_label(p->current, end_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_exp_new_real_var(result.reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

static subtilis_exp_t *prv_rnd_const(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	int32_t val = e->exp.ir_op.integer;

	subtilis_exp_delete(e);
	if (val == 0)
		return subtilis_builtins_ir_rnd_0(p, err);
	else if (val == 1)
		return subtilis_builtins_ir_rnd_1(p, err);
	else if (val < 0)
		return subtilis_builtins_ir_rnd_neg(p, val, err);
	else
		return subtilis_builtins_ir_rnd_pos(p, val, err);
}

subtilis_exp_t *subtilis_parser_rnd(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e_dup = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "(")) {
			e = subtilis_parser_bracketed_exp_internal(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			subtilis_lexer_get(p->l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			e = subtilis_type_if_to_int(p, e, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER)
				return prv_rnd_const(p, e, err);
			return prv_rnd_var(p, e, err);
		}
	}

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	subtilis_exp_delete(e_dup);

	return NULL;
}
