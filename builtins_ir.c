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

#include "builtins_ir.h"
#include "variable.h"

const char *subtilis_rnd_hidden_var = "_RND";

void subtilis_builtins_ir_inkey(subtilis_ir_section_t *current,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t true_label;
	subtilis_ir_operand_t false_label;
	subtilis_ir_operand_t end_label;

	op0.reg = current->reg_counter++;

	op1.reg = SUBTILIS_IR_REG_TEMP_START;
	op2.integer = 0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GTEI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(current);
	false_label.reg = subtilis_ir_section_new_label(current);
	end_label.reg = subtilis_ir_section_new_label(current);

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_GET_TO,
					      op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = -256;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	true_label.reg = subtilis_ir_section_new_label(current);
	false_label.reg = subtilis_ir_section_new_label(current);
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  true_label, false_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, true_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    current, SUBTILIS_OP_INSTR_OS_BYTE_ID, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, false_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_INKEY,
					      op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, end_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     op0, err);
}

/*
 * TODO: This needs to be moved to a better location.
 */

subtilis_exp_t *subtilis_builtins_ir_basic_rnd(subtilis_parser_t *p,
					       subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *a = NULL;
	subtilis_exp_t *c = NULL;

	a = subtilis_exp_new_int32(1664525, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	c = subtilis_exp_new_int32(1013904223, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_var_lookup_var(p, subtilis_rnd_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_mul(p, a, e, err);
	a = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_exp_add(p, e, c, err);

cleanup:

	subtilis_exp_delete(e);
	subtilis_exp_delete(a);
	subtilis_exp_delete(c);

	return NULL;
}

void subtilis_builtins_ir_rnd(subtilis_parser_t *p,
			      subtilis_ir_section_t *current,
			      subtilis_error_t *err)
{
	subtilis_exp_t *m = NULL;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e_dup = NULL;
	subtilis_exp_t *top_bit = NULL;
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t op0;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 */

	old_current = p->current;
	p->current = current;

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_exp_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	m = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER,
				 SUBTILIS_IR_REG_TEMP_START, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_and(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_mod(p, e, m, err);
	m = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_add(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   SUBTILIS_TYPE_INTEGER, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = e->exp.ir_op.reg;
	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     op0, err);

cleanup:

	p->current = old_current;

	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(e);
	subtilis_exp_delete(m);
}

subtilis_ir_section_t *subtilis_builtins_ir_add_1_arg_int(subtilis_parser_t *p,
							  const char *name,
							  subtilis_error_t *err)
{
	subtilis_type_section_t *ts;
	subtilis_type_t *params;
	subtilis_ir_section_t *current;

	params = malloc(sizeof(subtilis_type_t) * 1);
	if (!params) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	params[0] = SUBTILIS_TYPE_INTEGER;
	ts = subtilis_type_section_new(SUBTILIS_TYPE_INTEGER, 1, params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	current = subtilis_ir_prog_section_new(
	    p->prog, name, 0, ts, SUBTILIS_BUILTINS_MAX, "builtin", 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		subtilis_type_section_delete(ts);

	return current;
}
