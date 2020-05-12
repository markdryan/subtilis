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
#include "globals.h"
#include "type_if.h"
#include "variable.h"

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

	e = subtilis_type_if_mul(p, a, e, err);
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

subtilis_exp_t *subtilis_builtins_ir_rnd_0(subtilis_parser_t *p,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *top_bit = NULL;
	subtilis_exp_t *top_bit_dup = NULL;

	e = subtilis_var_lookup_var(p, subtilis_rnd_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit_dup = subtilis_type_if_dup(top_bit, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_and(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_divr(p, e, top_bit_dup, err);
	top_bit_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(top_bit_dup);

	return NULL;
}

subtilis_exp_t *subtilis_builtins_ir_rnd_1(subtilis_parser_t *p,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_exp_t *e_dup = NULL;
	subtilis_exp_t *top_bit = NULL;

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_and(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_divr(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_builtins_ir_rnd_pos(subtilis_parser_t *p, int32_t val,
					     subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_exp_t *m;
	subtilis_exp_t *e_dup = NULL;
	subtilis_exp_t *top_bit = NULL;

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_and(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	m = subtilis_exp_new_int32(val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_mod(p, e, m, err);
	m = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	m = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_exp_add(p, e, m, err);

cleanup:

	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_builtins_ir_rnd_neg(subtilis_parser_t *p, int32_t val,
					     subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_exp_t *e_dup = NULL;

	e = subtilis_exp_new_int32(-val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(e);

	return NULL;
}

void subtilis_builtins_ir_rnd_int(subtilis_parser_t *p,
				  subtilis_ir_section_t *current,
				  subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t neg_label;
	subtilis_ir_operand_t zero_pos_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t result;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t cond;
	subtilis_exp_t *m = NULL;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e_dup = NULL;
	subtilis_exp_t *top_bit = NULL;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 */

	old_current = p->current;
	p->current = current;

	neg_label.label = subtilis_ir_section_new_label(current);
	zero_pos_label.label = subtilis_ir_section_new_label(current);
	end_label.label = current->end_label;

	result.reg = current->ret_reg;

	op1.reg = SUBTILIS_IR_REG_TEMP_START;
	op2.integer = 0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LTI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  neg_label, zero_pos_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, neg_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_RSUBI_I32,
					  result, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_exp_new_int32_var(result.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, zero_pos_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_builtins_ir_basic_rnd(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e_dup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	m = subtilis_exp_new_int32_var(SUBTILIS_IR_REG_TEMP_START, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	top_bit = subtilis_exp_new_int32((int32_t)0x7fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_and(p, e, top_bit, err);
	top_bit = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_mod(p, e, m, err);
	m = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  result, e->exp.ir_op, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, e_dup, err);
	e_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     result, err);

cleanup:

	p->current = old_current;

	subtilis_exp_delete(top_bit);
	subtilis_exp_delete(e_dup);
	subtilis_exp_delete(e);
	subtilis_exp_delete(m);
}

void subtilis_builtins_ir_rnd_real(subtilis_parser_t *p,
				   subtilis_ir_section_t *current,
				   subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t one_label;
	subtilis_ir_operand_t zero_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t result;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t cond;
	subtilis_exp_t *e = NULL;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 */

	old_current = p->current;
	p->current = current;

	one_label.label = subtilis_ir_section_new_label(current);
	zero_label.label = subtilis_ir_section_new_label(current);
	end_label.label = current->end_label;

	result.reg = current->ret_reg;

	op1.reg = SUBTILIS_IR_REG_TEMP_START;
	op2.integer = 1;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_EQI_I32, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  one_label, zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, one_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_builtins_ir_rnd_1(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOVFP,
					      result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_exp_delete(e);
	e = NULL;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, zero_label.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_builtins_ir_rnd_0(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOVFP,
					      result, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(
	    current, SUBTILIS_OP_INSTR_RET_REAL, result, err);

cleanup:

	p->current = old_current;

	subtilis_exp_delete(e);
}

void subtilis_builtins_ir_print_fp(subtilis_parser_t *p,
				   subtilis_ir_section_t *current,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t op1i;
	subtilis_ir_operand_t number;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t loop_label;
	subtilis_ir_operand_t first_not_zero_label;
	subtilis_ir_operand_t second_not_zero_label;
	subtilis_ir_operand_t end_label;

	loop_label.label = subtilis_ir_section_new_label(current);
	end_label.label = subtilis_ir_section_new_label(current);
	first_not_zero_label.label = subtilis_ir_section_new_label(current);
	second_not_zero_label.label = subtilis_ir_section_new_label(current);

	op1.reg = 0;
	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    current, SUBTILIS_OP_INSTR_PRINT_I32, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_ABSR, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_I32_FP, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	number.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUB_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.real = 0.0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_NEQI_REAL, number, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  first_not_zero_label, end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, first_not_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1i.integer = '.';
	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_VDUI,
					     op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = 10;
	counter.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.real = 10.0;
	op1.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_MULI_REAL, number, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_I32_FP, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUB_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* TODO:
	 * We can't use the subtraction operation to write directly into the
	 * destinationr register as the register allocator cannot currently
	 * handle the case when the destination register  is the same as one
	 * of the operands.  In the long run the move gets removed by the
	 * peephole optimizer, but we shouldn't need to add it in the first
	 * place.  This needs to be fixed.
	 */

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOVFP,
					      number, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = '0';
	op1i.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADDI_I32, op1i, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_VDU,
					     op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.real = 0.0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_NEQI_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  second_not_zero_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, second_not_zero_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = 1;
	op1i.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUBI_I32, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      counter, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = 0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_EQI_I32, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC_NF,
					  cond, end_label, loop_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_arg(current, SUBTILIS_OP_INSTR_RET,
					     err);
}

static subtilis_ir_section_t *prv_add_args(subtilis_parser_t *p,
					   const char *name, size_t arg_count,
					   const subtilis_type_t **ptype,
					   const subtilis_type_t *rtype,
					   subtilis_error_t *err)
{
	subtilis_type_section_t *ts;
	subtilis_type_t *params;
	subtilis_ir_section_t *current;
	size_t i;

	params = malloc(sizeof(subtilis_type_t) * arg_count);
	if (!params) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	for (i = 0; i < arg_count; i++)
		params[i] = *ptype[i];
	ts = subtilis_type_section_new(rtype, arg_count, params, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	current = subtilis_ir_prog_section_new(
	    p->prog, name, 0, ts, SUBTILIS_BUILTINS_MAX, "builtin", 0,
	    p->eflag_offset, p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		subtilis_type_section_delete(ts);

	return current;
}

subtilis_ir_section_t *
subtilis_builtins_ir_add_1_arg_int(subtilis_parser_t *p, const char *name,
				   const subtilis_type_t *rtype,
				   subtilis_error_t *err)
{
	const subtilis_type_t *ptype[1];

	ptype[0] = &subtilis_type_integer;
	return prv_add_args(p, name, 1, ptype, rtype, err);
}

subtilis_ir_section_t *
subtilis_builtins_ir_add_1_arg_real(subtilis_parser_t *p, const char *name,
				    const subtilis_type_t *rtype,
				    subtilis_error_t *err)
{
	const subtilis_type_t *ptype[1];

	ptype[0] = &subtilis_type_real;
	return prv_add_args(p, name, 1, ptype, rtype, err);
}
