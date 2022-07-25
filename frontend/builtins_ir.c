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

#include "../common/error_codes.h"
#include "array_type.h"
#include "builtins_ir.h"
#include "globals.h"
#include "parser_exp.h"
#include "rec_type.h"
#include "reference_type.h"
#include "type_if.h"
#include "variable.h"

static subtilis_ir_section_t *prv_add_args(subtilis_parser_t *p,
					   const char *name, size_t arg_count,
					   const subtilis_type_t **ptype,
					   const subtilis_type_t *rtype,
					   subtilis_error_t *err);

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

void subtilis_builtins_ir_fp_to_str(subtilis_parser_t *p,
				    subtilis_ir_section_t *current,
				    subtilis_error_t *err)
{
	subtilis_exp_t *sizee;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t op1i;
	subtilis_ir_operand_t digit;
	subtilis_ir_operand_t number;
	subtilis_ir_operand_t cond;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t loop_label;
	subtilis_ir_operand_t end_loop_label;
	subtilis_ir_operand_t first_not_zero_label;
	subtilis_ir_operand_t second_not_zero_label;
	subtilis_ir_operand_t digit_zero_label;
	subtilis_ir_operand_t digit_not_zero_label;
	subtilis_ir_operand_t end_label;
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t fp_left;
	subtilis_symbol_table_t *old_st;
	subtilis_ir_operand_t arg_val;
	subtilis_ir_operand_t arg_buf;
	subtilis_ir_operand_t last_non_zero;
	subtilis_ir_operand_t buf_start;
	size_t size_reg;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 *
	 * Note this function has an additional problem as it calls
	 * subtilis_type_if_print which reserves stack space for a local buffer,
	 * so we also need to swap out the symbol table.
	 *
	 * There really needs to be some generic set up and tear down code for
	 * defining builtins like this.
	 */

	/*
	 * TODO.  This doesn't really work properly for big or small numbers.
	 */

	old_current = p->current;
	p->current = current;
	old_st = p->local_st;
	p->local_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * arg_buf is expected to be >= 22 bytes.
	 */

	arg_buf.reg = SUBTILIS_IR_REG_TEMP_START;
	arg_val.reg = 0;

	loop_label.label = subtilis_ir_section_new_label(current);
	end_label.label = current->end_label;
	end_loop_label.label = subtilis_ir_section_new_label(current);
	first_not_zero_label.label = subtilis_ir_section_new_label(current);
	second_not_zero_label.label = subtilis_ir_section_new_label(current);
	digit_not_zero_label.label = subtilis_ir_section_new_label(current);
	digit_zero_label.label = subtilis_ir_section_new_label(current);

	buf_start.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, arg_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (p->backend.caps & SUBTILIS_BACKEND_HAVE_I32_TO_DEC) {
		size_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_I32TODEC, op1i, arg_buf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		sizee = subtilis_builtin_ir_call_dec_to_str(p, op1i.reg,
							    arg_buf.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		size_reg = sizee->exp.ir_op.reg;
		subtilis_exp_delete(sizee);
	}

	op2.reg = size_reg;
	op1.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADD_I32, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	last_non_zero.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_ABSR, arg_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_I32_FP, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	number.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUB_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.real = 0.0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_NEQI_REAL, number, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  first_not_zero_label, end_loop_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, first_not_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = '.';
	op1i.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  op1i, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 10;
	counter.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.real = 10.0;
	op1.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_MULI_REAL, number, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	digit.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_FP_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV_I32_FP, digit, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	fp_left.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUB_REAL, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* TODO:
	 * We can't use the subtraction operation to write directly into the
	 * destinationr register as the register allocator cannot currently
	 * handle the case when the destination register  is the same as one
	 * of the operands.  In the long run the move gets removed by the
	 * peephole optimizer, but we shouldn't need to add it in the first
	 * place.  This needs to be fixed.
	 */

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOVFP,
					      number, fp_left, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = '0';
	op1i.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADDI_I32, digit, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  op1i, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADDI_I32, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  digit, digit_not_zero_label,
					  digit_zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, digit_not_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      last_non_zero, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, digit_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      arg_buf, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.real = 0.0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_NEQI_REAL, fp_left, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, cond,
					  second_not_zero_label, end_loop_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, second_not_zero_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 1;
	op1i.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUBI_I32, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      counter, op1i, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	cond.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_EQI_I32, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC_NF,
					  cond, end_loop_label, loop_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = current->ret_reg;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_SUB_I32,
					  op0, last_non_zero, buf_start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	current->locals = p->local_st->max_allocated;

cleanup:

	subtilis_symbol_table_delete(p->local_st);
	p->local_st = old_st;

	p->current = old_current;
}

static void prv_builtins_ir_dec_to_str(subtilis_parser_t *p,
				       subtilis_ir_section_t *current,
				       subtilis_error_t *err)
{
	subtilis_ir_operand_t arg_int;
	subtilis_ir_operand_t arg_buf;
	subtilis_ir_operand_t arg_buf_copy;
	subtilis_ir_operand_t reverse_start;
	subtilis_ir_operand_t count;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t v1;
	subtilis_ir_operand_t v2;
	subtilis_ir_operand_t less_zero;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t zero_label;
	subtilis_ir_operand_t not_zero_label;
	subtilis_ir_operand_t main_loop_label;
	subtilis_ir_operand_t neg_label;
	subtilis_ir_operand_t swap_label;
	subtilis_ir_operand_t reverse_label;
	subtilis_ir_operand_t minus;
	subtilis_ir_section_t *old_current;
	subtilis_exp_t *zero_asc = NULL;
	subtilis_exp_t *ten = NULL;
	subtilis_exp_t *val = NULL;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 */

	old_current = p->current;
	p->current = current;

	end_label.label = current->end_label;

	neg_label.label = subtilis_ir_section_new_label(current);
	not_zero_label.label = subtilis_ir_section_new_label(current);
	main_loop_label.label = subtilis_ir_section_new_label(current);
	zero_label.label = subtilis_ir_section_new_label(current);
	swap_label.label = subtilis_ir_section_new_label(current);
	reverse_label.label = subtilis_ir_section_new_label(current);

	count.reg = current->ret_reg;

	arg_int.reg = SUBTILIS_IR_REG_TEMP_START;
	arg_buf.reg = SUBTILIS_IR_REG_TEMP_START + 1;

	arg_buf_copy.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	less_zero.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LTI_I32, arg_int, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  less_zero, neg_label, not_zero_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, neg_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = '-';
	minus.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * TODO: We could write a rule to combine these two instructions in
	 * these backend on ARM.
	 */

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  minus, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, not_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reverse_start.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ten = subtilis_exp_new_int32(10, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_exp_new_int32_var(SUBTILIS_IR_REG_TEMP_START, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, main_loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_type_if_mod(p, val, ten, err);
	ten = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_type_if_abs(p, val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero_asc = subtilis_exp_new_int32('0', err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_type_if_add(p, val, zero_asc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  val->exp.ir_op, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(val);
	val = NULL;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ten = subtilis_exp_new_int32(10, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_exp_new_int32_var(SUBTILIS_IR_REG_TEMP_START, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val = subtilis_type_if_div(p, val, ten, err);
	ten = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = SUBTILIS_IR_REG_TEMP_START;
	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      op0, val->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, op0,
					  main_loop_label, zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_SUB_I32,
					  count, arg_buf, arg_buf_copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * Now we need to reverse the digits.
	 */

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_SUBI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, reverse_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	less_zero.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LT_I32, reverse_start, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  less_zero, swap_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, swap_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	v1.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LOADO_I8, reverse_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	v2.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LOADO_I8, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  v1, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_SUBI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  v2, reverse_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  reverse_start, reverse_start, op1,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_JMP,
					     reverse_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     count, err);

cleanup:

	p->current = old_current;

	subtilis_exp_delete(ten);
	subtilis_exp_delete(val);
}

static void prv_builtins_ir_hex_to_str(subtilis_parser_t *p,
				       subtilis_ir_section_t *current,
				       subtilis_error_t *err)
{
	subtilis_ir_operand_t arg_int;
	subtilis_ir_operand_t arg_buf;
	subtilis_ir_operand_t arg_buf_copy;
	subtilis_ir_operand_t shift;
	subtilis_ir_operand_t count;
	subtilis_ir_operand_t chr;
	subtilis_ir_operand_t nibble;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t have_written;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t main_loop_label;
	subtilis_ir_operand_t not_zero_label;
	subtilis_ir_operand_t all_zero_label;
	subtilis_ir_operand_t not_all_zero_label;
	subtilis_ir_operand_t zero_label;
	subtilis_ir_section_t *old_current;

	/*
	 * This is a bit nasty.  The expression functions take a parser
	 * and not a section.  As we don't want to add to the current section
	 * here we need to temporarily replace the current section.  We should
	 * probably update the expression functions to take an explicit section.
	 */

	old_current = p->current;
	p->current = current;

	end_label.label = current->end_label;

	all_zero_label.label = subtilis_ir_section_new_label(current);
	not_all_zero_label.label = subtilis_ir_section_new_label(current);
	main_loop_label.label = subtilis_ir_section_new_label(current);
	zero_label.label = subtilis_ir_section_new_label(current);
	not_zero_label.label = subtilis_ir_section_new_label(current);

	count.reg = current->ret_reg;

	arg_int.reg = SUBTILIS_IR_REG_TEMP_START;
	arg_buf.reg = SUBTILIS_IR_REG_TEMP_START + 1;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  arg_int, not_all_zero_label,
					  all_zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, all_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = '0';
	chr.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  chr, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	subtilis_ir_section_add_instr_no_reg(
	    current, SUBTILIS_OP_INSTR_RETI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, not_all_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arg_buf_copy.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOV, arg_buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 32;
	shift.reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, main_loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 4;
	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_SUBI_I32, shift, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg2(current, SUBTILIS_OP_INSTR_MOV,
					      shift, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_LSR_I32, arg_int, shift, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 15;
	nibble.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ANDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	have_written.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GT_I32, arg_buf, arg_buf_copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_OR_I32, nibble, have_written, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC, op0,
					  not_zero_label, zero_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, not_zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = '0';
	chr.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADDI_I32, nibble, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 10;
	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GTEI_I32, nibble, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = ('0' - 'A') + 10;
	op0.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_MULI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	chr.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_ADD_I32, op0, chr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_STOREO_I8,
					  chr, arg_buf, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_ADDI_I32,
					  arg_buf, arg_buf, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, zero_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  shift, main_loop_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_SUB_I32,
					  count, arg_buf, arg_buf_copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     count, err);

cleanup:

	p->current = old_current;
}

/* clang-format off */
static size_t prv_find_first_stop_prolog(subtilis_parser_t *p,
					 subtilis_ir_operand_t str_data,
					 subtilis_ir_operand_t str_len,
					 subtilis_ir_operand_t max_digit,
					 bool immediate_digit,
					 subtilis_ir_operand_t end_str,
					 subtilis_ir_operand_t *byte,
					 subtilis_ir_operand_t skip_sign_label,
					 subtilis_ir_operand_t first_end_label,
					 subtilis_ir_operand_t end_label,
					 subtilis_ir_operand_t *minus,
					 subtilis_ir_operand_t *loop_cond,
					 subtilis_error_t *err)
/* clang-format on */

{
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t le_9;
	subtilis_ir_operand_t have_sign_label;
	subtilis_ir_operand_t check_for_minus_label;
	subtilis_ir_operand_t negative_label;
	subtilis_ir_operand_t whole_number_start_label;
	subtilis_ir_operand_t keep_looking_label;
	subtilis_ir_operand_t condee;

	check_for_minus_label.label = subtilis_ir_section_new_label(p->current);
	negative_label.label = subtilis_ir_section_new_label(p->current);
	have_sign_label.label = subtilis_ir_section_new_label(p->current);
	whole_number_start_label.label =
	    subtilis_ir_section_new_label(p->current);
	keep_looking_label.label = subtilis_ir_section_new_label(p->current);

	ptr.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, str_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 1;
	minus->reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 0;
	byte->reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = '+';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_NEQI_I32, *byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, check_for_minus_label,
					  have_sign_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, check_for_minus_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = '-';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, *byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, negative_label,
					  whole_number_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, negative_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 0;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_RSUBI_I32, *minus, *minus, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, have_sign_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADDI_I32, str_data,
					  str_data, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, skip_sign_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr, end_str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, keep_looking_label,
					  first_end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, keep_looking_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 0;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, *byte, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current,
				      whole_number_start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = '0';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, *byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	le_9.reg = subtilis_ir_section_add_instr(
	    p->current, immediate_digit ? SUBTILIS_OP_INSTR_GTI_I32
					: SUBTILIS_OP_INSTR_GT_I32,
	    *byte, max_digit, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	loop_cond->reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, condee, le_9, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return ptr.reg;
}

static size_t
prv_find_first_stop(subtilis_parser_t *p, subtilis_ir_operand_t str_data,
		    subtilis_ir_operand_t str_len,
		    subtilis_ir_operand_t max_digit, bool immediate_digit,
		    subtilis_ir_operand_t end_str,
		    subtilis_ir_operand_t end_label,
		    subtilis_ir_operand_t *minus, subtilis_error_t *err)
{
	subtilis_ir_operand_t skip_sign_label;
	subtilis_ir_operand_t first_end_label;
	subtilis_ir_operand_t condee;
	size_t ptr;
	subtilis_ir_operand_t byte;

	skip_sign_label.label = subtilis_ir_section_new_label(p->current);
	first_end_label.label = subtilis_ir_section_new_label(p->current);

	ptr = prv_find_first_stop_prolog(
	    p, str_data, str_len, max_digit, immediate_digit, end_str, &byte,
	    skip_sign_label, first_end_label, end_label, minus, &condee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  condee, first_end_label,
					  skip_sign_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, first_end_label.label, err);

	return ptr;
}

static void
prv_compute_whole_part(subtilis_parser_t *p, subtilis_ir_operand_t str_data,
		       subtilis_ir_operand_t ptr, subtilis_ir_operand_t ret_val,
		       subtilis_ir_operand_t factor, subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t byte;
	subtilis_ir_operand_t bytef;
	subtilis_ir_operand_t product;
	subtilis_ir_operand_t new_ptr;
	subtilis_ir_operand_t whole_start_label;
	subtilis_ir_operand_t process_whole_label;
	subtilis_ir_operand_t skip_whole_label;

	whole_start_label.label = subtilis_ir_section_new_label(p->current);
	process_whole_label.label = subtilis_ir_section_new_label(p->current);
	skip_whole_label.label = subtilis_ir_section_new_label(p->current);

	factor.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVFP, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	new_ptr.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, whole_start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, new_ptr, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTE_I32, new_ptr, str_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, process_whole_label,
					  skip_whole_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, process_whole_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '0';
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, byte, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	bytef.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, byte, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	product.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_MUL_REAL, bytef, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.real = 10.0;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_MULI_REAL, factor, factor, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADD_REAL, ret_val,
					  ret_val, product, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     whole_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, skip_whole_label.label, err);
}

static void prv_compute_fractional_part(subtilis_parser_t *p,
					subtilis_ir_operand_t end_str,
					subtilis_ir_operand_t ptr,
					subtilis_ir_operand_t ret_val,
					subtilis_ir_operand_t factor,
					subtilis_error_t *err)
{
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t byte;
	subtilis_ir_operand_t bytef;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t le_9;
	subtilis_ir_operand_t product;
	subtilis_ir_operand_t have_fractional_label;
	subtilis_ir_operand_t no_fractional_label;
	subtilis_ir_operand_t decimal_point_label;
	subtilis_ir_operand_t process_digit_label;
	subtilis_ir_operand_t keep_looking_label;
	subtilis_ir_operand_t loop_label;

	have_fractional_label.label = subtilis_ir_section_new_label(p->current);
	no_fractional_label.label = subtilis_ir_section_new_label(p->current);
	decimal_point_label.label = subtilis_ir_section_new_label(p->current);
	process_digit_label.label = subtilis_ir_section_new_label(p->current);
	keep_looking_label.label = subtilis_ir_section_new_label(p->current);
	loop_label.label = subtilis_ir_section_new_label(p->current);

	op1.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr, end_str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, have_fractional_label,
					  no_fractional_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, have_fractional_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '.';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, decimal_point_label,
					  no_fractional_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, decimal_point_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.real = 0.1;
	factor.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_MULI_REAL, factor, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, loop_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr, end_str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, keep_looking_label,
					  no_fractional_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, keep_looking_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, byte, ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '0';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTEI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '9';
	le_9.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTEI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_AND_I32, condee, le_9, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, process_digit_label,
					  no_fractional_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, process_digit_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '0';
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, byte, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	bytef.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, byte, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	product.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_MUL_REAL, bytef, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.real = 0.1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_MULI_REAL, factor, factor, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADD_REAL, ret_val,
					  ret_val, product, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     loop_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, no_fractional_label.label,
				      err);
}

void subtilis_builtins_ir_str_to_fp(subtilis_parser_t *p,
				    subtilis_ir_section_t *current,
				    subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t str;
	subtilis_ir_operand_t str_data;
	subtilis_ir_operand_t str_len;
	subtilis_ir_operand_t end_str;
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t ret_val;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t factor;
	subtilis_ir_operand_t plus_minus_label;
	subtilis_ir_operand_t inf_label;
	subtilis_ir_operand_t finish_label;
	subtilis_exp_t *e = NULL;

	old_current = p->current;
	p->current = current;

	str.reg = SUBTILIS_IR_REG_TEMP_START;
	end_label.label = current->end_label;
	ret_val.reg = current->ret_reg;
	plus_minus_label.label = subtilis_ir_section_new_label(current);
	inf_label.label = subtilis_ir_section_new_label(current);
	finish_label.label = subtilis_ir_section_new_label(current);

	op1.real = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, ret_val, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_len.reg = subtilis_reference_type_get_size(p, str.reg, 0, err);
	;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GTI_I32, str_len, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  condee, plus_minus_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, plus_minus_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_data.reg = subtilis_reference_get_data(p, str.reg, 0, err);
	;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	end_str.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, str_data, str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = '9';
	ptr.reg = prv_find_first_stop(p, str_data, str_len, op1, true, end_str,
				      finish_label, &factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	factor.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV_I32_FP, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compute_whole_part(p, str_data, ptr, ret_val, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_compute_fractional_part(p, end_str, ptr, ret_val, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, finish_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_new_real_var(ret_val.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_is_inf(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  e->exp.ir_op, inf_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, inf_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);
	e = subtilis_exp_new_int32(SUBTILIS_ERROR_CODE_NUMBER_TOO_BIG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_generate_error(p, e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(
	    current, SUBTILIS_OP_INSTR_RET_REAL, ret_val, err);

cleanup:
	subtilis_exp_delete(e);
	p->current = old_current;
}

/* clang-format off */
static void prv_compute_whole_int_part(subtilis_parser_t *p,
				       subtilis_ir_operand_t str_data,
				       subtilis_ir_operand_t base,
				       subtilis_ir_operand_t ptr,
				       subtilis_ir_operand_t ret_val,
				       subtilis_ir_operand_t overflow_label,
				       subtilis_ir_operand_t end_label,
				       subtilis_ir_operand_t factor,
				       subtilis_error_t *err)
/* clang-format on */

{
	subtilis_ir_operand_t sign;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t byte;
	subtilis_ir_operand_t product;
	subtilis_ir_operand_t new_ptr;
	subtilis_ir_operand_t overflow;
	subtilis_ir_operand_t whole_start_label;
	subtilis_ir_operand_t process_whole_label;
	subtilis_ir_operand_t check_overflow_label;

	whole_start_label.label = subtilis_ir_section_new_label(p->current);
	process_whole_label.label = subtilis_ir_section_new_label(p->current);
	check_overflow_label.label = subtilis_ir_section_new_label(p->current);

	sign.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	new_ptr.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, whole_start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, new_ptr, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTE_I32, new_ptr, str_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, process_whole_label,
					  end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, process_whole_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '0';
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, byte, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	product.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_MUL_I32, byte, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_MUL_I32,
					  factor, factor, base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_ADD_I32,
					  ret_val, ret_val, product, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  ret_val, check_overflow_label,
					  whole_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, check_overflow_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Now check for overflow.  We EOR the sign and the sum
	 * together.  If the top bit is set we've overflowed.
	 */

	overflow.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, ret_val, sign, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0x80000000;
	overflow.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, overflow, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  overflow, overflow_label,
					  whole_start_label, err);
}

static void prv_builtins_ir_str_to_int32(subtilis_parser_t *p,
					 subtilis_ir_section_t *current,
					 subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t str;
	subtilis_ir_operand_t str_data;
	subtilis_ir_operand_t base;
	subtilis_ir_operand_t max_digit;
	subtilis_ir_operand_t str_len;
	subtilis_ir_operand_t end_str;
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t ret_val;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t factor;
	subtilis_ir_operand_t plus_minus_label;
	subtilis_ir_operand_t overflow_label;
	subtilis_exp_t *e = NULL;

	old_current = p->current;
	p->current = current;

	str.reg = SUBTILIS_IR_REG_TEMP_START;
	base.reg = SUBTILIS_IR_REG_TEMP_START + 1;
	end_label.label = current->end_label;
	ret_val.reg = current->ret_reg;
	plus_minus_label.label = subtilis_ir_section_new_label(current);
	overflow_label.label = subtilis_ir_section_new_label(current);

	op1.integer = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, ret_val, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_len.reg = subtilis_reference_type_get_size(p, str.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GTI_I32, str_len, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  condee, plus_minus_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, plus_minus_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_data.reg = subtilis_reference_get_data(p, str.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	end_str.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, str_data, str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = '0';
	max_digit.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, base, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ptr.reg = prv_find_first_stop(p, str_data, str_len, max_digit, false,
				      end_str, end_label, &factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_compute_whole_int_part(p, str_data, base, ptr, ret_val,
				   overflow_label, end_label, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     ret_val, err);

	subtilis_ir_section_add_label(current, overflow_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_new_int32(SUBTILIS_ERROR_CODE_NUMBER_TOO_BIG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_generate_error(p, e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_deref_array_els(subtilis_parser_t *p,
					    subtilis_ir_section_t *current,
					    subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t no_destruct_label;
	subtilis_ir_operand_t destruct_label;
	subtilis_ir_operand_t getref_label;
	subtilis_ir_operand_t deref_label;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t ref_count;
	subtilis_ir_operand_t el_start;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t orig_size;

	old_current = p->current;
	p->current = current;

	getref_label.label = subtilis_ir_section_new_label(p->current);
	destruct_label.label = subtilis_ir_section_new_label(p->current);
	deref_label.label = subtilis_ir_section_new_label(p->current);
	no_destruct_label.label = subtilis_ir_section_new_label(p->current);

	el_start.reg = SUBTILIS_IR_REG_TEMP_START;

	op2.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = SUBTIILIS_REFERENCE_HEAP_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, getref_label, no_destruct_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, getref_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ref_count.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_GETREF, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	op2.integer = 1;
	ref_count.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, ref_count, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  ref_count, destruct_label,
					  deref_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_ir_section_add_label(p->current, destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = SUBTIILIS_REFERENCE_ORIG_SIZE_OFF;
	orig_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_deref_els(p, offset.reg, orig_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, deref_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reference_type_call_deref(p, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, no_destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_deref_array_recs_gen(subtilis_parser_t *p,
						 subtilis_ir_section_t *current,
						 subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t no_destruct_label;
	subtilis_ir_operand_t destruct_label;
	subtilis_ir_operand_t getref_label;
	subtilis_ir_operand_t deref_label;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t ref_count;
	subtilis_ir_operand_t el_start;
	subtilis_ir_operand_t el_size;
	subtilis_ir_operand_t el_destructor;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t orig_size;

	old_current = p->current;
	p->current = current;

	getref_label.label = subtilis_ir_section_new_label(p->current);
	destruct_label.label = subtilis_ir_section_new_label(p->current);
	deref_label.label = subtilis_ir_section_new_label(p->current);
	no_destruct_label.label = subtilis_ir_section_new_label(p->current);

	el_start.reg = SUBTILIS_IR_REG_TEMP_START;
	el_size.reg = SUBTILIS_IR_REG_TEMP_START + 1;
	el_destructor.reg = SUBTILIS_IR_REG_TEMP_START + 2;

	op2.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = SUBTIILIS_REFERENCE_HEAP_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, getref_label, no_destruct_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, getref_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ref_count.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_GETREF, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = 1;
	ref_count.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EQI_I32, ref_count, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  ref_count, destruct_label,
					  deref_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = SUBTIILIS_REFERENCE_ORIG_SIZE_OFF;
	orig_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_deref_recs(p, offset.reg, orig_size.reg,
				       el_size.reg, el_destructor.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, deref_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reference_type_call_deref(p, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, no_destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

void subtilis_builtin_ir_call_deref_array_recs_gen(subtilis_parser_t *p,
						   size_t el_start_reg,
						   size_t el_size_reg,
						   size_t el_destruct_reg,
						   subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[3];
	subtilis_ir_arg_t *args;
	char *name_copy;
	const char *name = "_deref_array_recs";

	ptype[0] = &subtilis_type_integer;
	ptype[1] = &subtilis_type_integer;
	ptype[2] = &subtilis_type_integer;

	fn = prv_add_args(p, name, 3, ptype, &subtilis_type_void, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_deref_array_recs_gen(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	name_copy = malloc(strlen(name) + 1);
	if (!name_copy) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name_copy, name);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name_copy);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = el_start_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = el_size_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = el_destruct_reg;

	(void)subtilis_exp_add_call(p, name_copy, SUBTILIS_BUILTINS_MAX, NULL,
				    args, &subtilis_type_void, 3, false, err);
}

static void prv_builtins_ir_deref_array_recs(subtilis_parser_t *p,
					     subtilis_ir_section_t *current,
					     const subtilis_type_t *el_type,
					     subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	size_t el_size;
	size_t els_call_index;
	subtilis_ir_operand_t op2;
	size_t base_reg;
	size_t el_size_reg;
	size_t el_destruct_reg;

	old_current = p->current;
	p->current = current;

	base_reg = SUBTILIS_IR_REG_TEMP_START;
	els_call_index = subtilis_type_if_destructor(p, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	el_size = subtilis_type_rec_size(el_type);

	op2.integer = (int32_t)el_size;
	;
	el_size_reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = (int32_t)els_call_index;
	;
	el_destruct_reg = subtilis_ir_section_add_instr2(
	    current, SUBTILIS_OP_INSTR_MOVI_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_builtin_ir_call_deref_array_recs_gen(p, base_reg, el_size_reg,
						      el_destruct_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_call_deref(subtilis_parser_t *p,
				       subtilis_ir_section_t *current,
				       subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t el_start;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t no_destruct_label;
	subtilis_ir_operand_t destruct_label;

	old_current = p->current;
	p->current = current;

	el_start.reg = SUBTILIS_IR_REG_TEMP_START;

	destruct_label.label = subtilis_ir_section_new_label(p->current);
	no_destruct_label.label = subtilis_ir_section_new_label(p->current);

	op2.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, destruct_label,
					  no_destruct_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2.integer = SUBTIILIS_REFERENCE_HEAP_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reference_type_call_deref(p, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, no_destruct_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

static size_t prv_find_first_stop_hex(subtilis_parser_t *p,
				      subtilis_ir_operand_t str_data,
				      subtilis_ir_operand_t str_len,
				      subtilis_ir_operand_t end_str,
				      subtilis_ir_operand_t end_label,
				      subtilis_ir_operand_t *minus,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t skip_sign_label;
	subtilis_ir_operand_t first_end_label;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t max_digit;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t condee2;
	subtilis_ir_operand_t lta;
	subtilis_ir_operand_t gtf;
	size_t ptr;
	subtilis_ir_operand_t byte;

	skip_sign_label.label = subtilis_ir_section_new_label(p->current);
	first_end_label.label = subtilis_ir_section_new_label(p->current);

	max_digit.integer = '9';
	ptr = prv_find_first_stop_prolog(
	    p, str_data, str_len, max_digit, true, end_str, &byte,
	    skip_sign_label, first_end_label, end_label, minus, &condee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 32;
	byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ORI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 'a';
	lta.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = 'f';
	gtf.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	condee2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_OR_I32, lta, gtf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, condee, condee2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC_NF,
					  condee, skip_sign_label,
					  first_end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, first_end_label.label, err);

	return ptr;
}

/* clang-format off */
static void prv_compute_whole_hex_part(subtilis_parser_t *p,
				       subtilis_ir_operand_t str_data,
				       subtilis_ir_operand_t base,
				       subtilis_ir_operand_t ptr,
				       subtilis_ir_operand_t ret_val,
				       subtilis_ir_operand_t overflow_label,
				       subtilis_ir_operand_t end_label,
				       subtilis_ir_operand_t factor,
				       subtilis_error_t *err)
/* clang-format off */

{
	subtilis_ir_operand_t sign;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t byte;
	subtilis_ir_operand_t ored_byte;
	subtilis_ir_operand_t product;
	subtilis_ir_operand_t new_ptr;
	subtilis_ir_operand_t overflow;
	subtilis_ir_operand_t whole_start_label;
	subtilis_ir_operand_t process_whole_label;
	subtilis_ir_operand_t hex_digit_label;
	subtilis_ir_operand_t skip_dec_label;
	subtilis_ir_operand_t dec_digit_label;
	subtilis_ir_operand_t check_overflow_label;

	whole_start_label.label = subtilis_ir_section_new_label(p->current);
	process_whole_label.label = subtilis_ir_section_new_label(p->current);
	hex_digit_label.label = subtilis_ir_section_new_label(p->current);
	dec_digit_label.label = subtilis_ir_section_new_label(p->current);
	skip_dec_label.label = subtilis_ir_section_new_label(p->current);
	check_overflow_label.label = subtilis_ir_section_new_label(p->current);

	sign.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	new_ptr.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, whole_start_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 1;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, new_ptr, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTE_I32, new_ptr, str_data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, process_whole_label,
					  end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, process_whole_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0;
	byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I8, new_ptr, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '9';
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, hex_digit_label,
					  dec_digit_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, hex_digit_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 32;
	ored_byte.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ORI_I32, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 'a' - 10;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, byte, ored_byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     skip_dec_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, dec_digit_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = '0';
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, byte, byte, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, skip_dec_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	product.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_MUL_I32, byte, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 4;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_LSLI_I32, factor, factor, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_ADD_I32,
					  ret_val, ret_val, product, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  ret_val, check_overflow_label,
					  whole_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, check_overflow_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Now check for overflow.  We EOR the sign and the sum
	 * together.  If the top bit is set we've overflowed.
	 */

	overflow.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_EOR_I32, ret_val, sign, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = 0x80000000;
	overflow.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ANDI_I32, overflow, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  overflow, overflow_label,
					  whole_start_label, err);
}

static void prv_builtins_ir_hexstr_to_int32(subtilis_parser_t *p,
					    subtilis_ir_section_t *current,
					    subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_ir_operand_t str;
	subtilis_ir_operand_t str_data;
	subtilis_ir_operand_t base;
	subtilis_ir_operand_t str_len;
	subtilis_ir_operand_t end_str;
	subtilis_ir_operand_t ptr;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t ret_val;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t factor;
	subtilis_ir_operand_t plus_minus_label;
	subtilis_ir_operand_t overflow_label;
	subtilis_exp_t *e = NULL;

	old_current = p->current;
	p->current = current;

	str.reg = SUBTILIS_IR_REG_TEMP_START;
	base.reg = SUBTILIS_IR_REG_TEMP_START + 1;
	end_label.label = current->end_label;
	ret_val.reg = current->ret_reg;
	plus_minus_label.label = subtilis_ir_section_new_label(current);
	overflow_label.label = subtilis_ir_section_new_label(current);

	op1.integer = 0;
	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, ret_val, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_len.reg = subtilis_reference_type_get_size(p, str.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    current, SUBTILIS_OP_INSTR_GTI_I32, str_len, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(current, SUBTILIS_OP_INSTR_JMPC,
					  condee, plus_minus_label, end_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, plus_minus_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	str_data.reg = subtilis_reference_get_data(p, str.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	end_str.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, str_data, str_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ptr.reg = prv_find_first_stop_hex(p, str_data, str_len, end_str,
					  end_label, &factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_compute_whole_hex_part(p, str_data, base, ptr, ret_val,
				   overflow_label, end_label, factor, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(current, SUBTILIS_OP_INSTR_RET_I32,
					     ret_val, err);

	subtilis_ir_section_add_label(current, overflow_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_exp_new_int32(SUBTILIS_ERROR_CODE_NUMBER_TOO_BIG, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_generate_error(p, e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_gen_rec_deref(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_ir_section_t *current,
					  subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;
	subtilis_type_t *field;
	size_t i;
	size_t call_index;
	subtilis_ir_operand_t base;
	subtilis_ir_operand_t reg;
	subtilis_ir_operand_t offset;
	const subtilis_type_rec_t *rec = &type->params.rec;

	old_current = p->current;
	p->current = current;
	base.reg = SUBTILIS_IR_REG_TEMP_START;

	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		call_index = subtilis_type_if_destructor(p, field, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (call_index == SIZE_MAX)
			continue;

		if (i == 0) {
			reg.reg = base.reg;
		} else {
			offset.integer =
				subtilis_type_rec_field_offset_id(rec, i);
			reg.reg = subtilis_ir_section_add_instr(
				p->current, SUBTILIS_OP_INSTR_ADDI_I32, base,
				offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		}

		(void)subtilis_parser_call_1_arg_fn(
			p, p->prog->string_pool->strings[call_index], reg.reg,
			SUBTILIS_BUILTINS_MAX, SUBTILIS_IR_REG_TYPE_INTEGER,
			&subtilis_type_void, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_gen_rec_zero(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 subtilis_ir_section_t *current,
					 subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;

	old_current = p->current;
	p->current = current;

	subtilis_rec_type_zero_body(p, type, SUBTILIS_IR_REG_TEMP_START, 0,
				    subtilis_type_rec_zero_fill_size(type),
				    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, p->current->end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_gen_index_error_code(p, err);

cleanup:
	p->current = old_current;
}

static void prv_builtins_ir_gen_rec_copy(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 subtilis_ir_section_t *current,
					 bool new_rec, subtilis_error_t *err)
{
	subtilis_ir_section_t *old_current;

	old_current = p->current;
	p->current = current;

	subtilis_type_rec_copy_ref(p, type, SUBTILIS_IR_REG_TEMP_START,
				   0, SUBTILIS_IR_REG_TEMP_START + 1,
				   new_rec, err);

	subtilis_ir_section_add_label(p->current, p->current->end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_RET,
					     err);

cleanup:
	p->current = old_current;
}

static subtilis_ir_section_t *prv_add_args_ci(subtilis_parser_t *p,
					      const char *name,
					      size_t arg_count,
					      const subtilis_type_t **ptype,
					      const subtilis_type_t *rtype,
					      size_t *call_index,
					      subtilis_error_t *err)
{
	subtilis_type_section_t *ts;
	subtilis_type_t *params;
	subtilis_ir_section_t *current = NULL;
	size_t j;
	size_t i = 0;

	params = malloc(sizeof(subtilis_type_t) * arg_count);
	if (!params) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	for (; i < arg_count; i++) {
		subtilis_type_init_copy(&params[i], ptype[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	ts = subtilis_type_section_new(rtype, arg_count, params, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	current = subtilis_ir_prog_section_new(
	    p->prog, name, 0, ts, SUBTILIS_BUILTINS_MAX, "builtin", 0,
	    p->eflag_offset, p->error_offset, call_index, err);

cleanup:

	for (j = 0; j < i; j++)
		subtilis_type_free(&params[j]);
	free(params);

	return current;
}

static subtilis_ir_section_t *prv_add_args(subtilis_parser_t *p,
					   const char *name, size_t arg_count,
					   const subtilis_type_t **ptype,
					   const subtilis_type_t *rtype,
					   subtilis_error_t *err)
{
	return prv_add_args_ci(p, name, arg_count, ptype, rtype, NULL, err);
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

/*
 * TODO: we should do this for the other builtin ir functions.
 */

subtilis_exp_t *subtilis_builtin_ir_call_dec_to_str(subtilis_parser_t *p,
						    size_t val_reg,
						    size_t buf_reg,
						    subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;

	const subtilis_type_t *ptype[2];

	ptype[0] = &subtilis_type_integer;
	ptype[1] = &subtilis_type_integer;

	fn = prv_add_args(p, "_dec_to_str", 2, ptype, &subtilis_type_integer,
			  err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_dec_to_str(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_2_arg_fn(
	    p, "_dec_to_str", val_reg, buf_reg, SUBTILIS_IR_REG_TYPE_INTEGER,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, false, err);
}

subtilis_exp_t *subtilis_builtin_ir_call_hex_to_str(subtilis_parser_t *p,
						    size_t val_reg,
						    size_t buf_reg,
						    subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;

	const subtilis_type_t *ptype[2];

	ptype[0] = &subtilis_type_integer;
	ptype[1] = &subtilis_type_integer;

	fn = prv_add_args(p, "_hex_to_str", 2, ptype, &subtilis_type_integer,
			  err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_hex_to_str(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_2_arg_fn(
	    p, "_hex_to_str", val_reg, buf_reg, SUBTILIS_IR_REG_TYPE_INTEGER,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, false, err);
}

subtilis_exp_t *subtilis_builtin_ir_call_fp_to_str(subtilis_parser_t *p,
						   size_t val_reg,
						   size_t buf_reg,
						   subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;

	const subtilis_type_t *ptype[2];

	ptype[0] = &subtilis_type_real;
	ptype[1] = &subtilis_type_integer;

	fn = prv_add_args(p, "_fp_to_str", 2, ptype, &subtilis_type_integer,
			  err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_fp_to_str(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_2_arg_fn(
	    p, "_fp_to_str", val_reg, buf_reg, SUBTILIS_IR_REG_TYPE_REAL,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, false, err);
}

subtilis_exp_t *subtilis_builtin_ir_call_str_to_fp(subtilis_parser_t *p,
						   size_t str_reg,
						   subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;

	const subtilis_type_t *ptype[1];

	ptype[0] = &subtilis_type_integer;

	fn = prv_add_args(p, "_str_to_fp", 1, ptype, &subtilis_type_real, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		subtilis_builtins_ir_str_to_fp(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_1_arg_fn(
	    p, "_str_to_fp", str_reg, SUBTILIS_BUILTINS_MAX,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_real, true, err);
}

subtilis_exp_t *subtilis_builtin_ir_call_hexstr_to_int32(subtilis_parser_t *p,
							 size_t str_reg,
							 subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[1];

	ptype[0] = &subtilis_type_integer;

	fn = prv_add_args(p, "_hexstr_to_int32", 1, ptype,
			  &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_hexstr_to_int32(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_1_arg_fn(
	    p, "_hexstr_to_int32", str_reg, SUBTILIS_BUILTINS_MAX,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, true, err);
}

subtilis_exp_t *subtilis_builtin_ir_call_str_to_int32(subtilis_parser_t *p,
						      size_t str_reg,
						      size_t base_reg,
						      subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[2];

	ptype[0] = &subtilis_type_integer;
	ptype[1] = &subtilis_type_integer;

	fn = prv_add_args(p, "_str_to_int32", 2, ptype, &subtilis_type_integer,
			  err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return NULL;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_str_to_int32(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return subtilis_parser_call_2_arg_fn(
	    p, "_str_to_int32", str_reg, base_reg, SUBTILIS_IR_REG_TYPE_INTEGER,
	    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, true, err);
}

size_t subtilis_builtin_ir_deref_array_els(subtilis_parser_t *p,
					   subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[2];
	size_t call_index;
	const char *name = "_deref_array";

	ptype[0] = &subtilis_type_integer;

	fn = prv_add_args_ci(p, name, 1, ptype, &subtilis_type_void,
			     &call_index, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return SIZE_MAX;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_deref_array_els(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	return call_index;
}

size_t subtilis_builtin_ir_deref_array_recs(subtilis_parser_t *p,
					    const subtilis_type_t *el_type,
					    subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[2];
	char *name;
	size_t call_index = SIZE_MAX;
	const char *base_name = "_deref_array_";
	const char *el_type_name = subtilis_type_name(el_type);

	name = malloc(strlen(base_name) + strlen(el_type_name) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return SIZE_MAX;
	}

	sprintf(name, "%s%s", base_name, el_type_name);

	ptype[0] = &subtilis_type_integer;
	fn = prv_add_args_ci(p, name, 1, ptype, &subtilis_type_void,
			     &call_index, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_deref_array_recs(p, fn, el_type, err);
	}

cleanup:

	free(name);

	return call_index;
}

size_t subtilis_builtin_ir_call_deref(subtilis_parser_t *p,
				      subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[1];
	size_t call_index;
	const char *name = "_call_deref";

	ptype[0] = &subtilis_type_integer;

	fn = prv_add_args_ci(p, name, 1, ptype, &subtilis_type_void,
			     &call_index, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			return SIZE_MAX;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_call_deref(p, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	return call_index;
}

size_t subtilis_builtin_ir_rec_deref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[1];
	size_t call_index;
	char *name;

	name = subtilis_rec_type_deref_fn_name(type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	ptype[0] = &subtilis_type_integer;

	fn = prv_add_args_ci(p, name, 1, ptype, &subtilis_type_void,
			     &call_index, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto on_error;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_gen_rec_deref(p, type, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	free(name);
	return call_index;

on_error:

	free(name);
	return SIZE_MAX;
}

void subtilis_builtin_ir_rec_zero(subtilis_parser_t *p,
				  const subtilis_type_t *type,
				  size_t base_reg, subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[1];
	char *name;
	bool check_errors;
	const char * const zero = "_zero";

	name = malloc(strlen(type->params.rec.name) + strlen(zero)
		      + 1 + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	sprintf(name, "_%s%s", type->params.rec.name, zero);
	ptype[0] = &subtilis_type_integer;
	fn = prv_add_args(p, name, 1, ptype, &subtilis_type_void, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_gen_rec_zero(p, type, fn, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	check_errors = subtilis_type_rec_need_zero_alloc(type);
	(void)subtilis_parser_call_1_arg_fn(
		p, name, base_reg, SUBTILIS_BUILTINS_MAX,
		SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_void, check_errors,
		err);

cleanup:

	free(name);
}

void subtilis_builtin_ir_rec_copy(subtilis_parser_t *p,
				  const subtilis_type_t *type,
				  size_t dest_reg, size_t src_reg,
				  bool new_rec, subtilis_error_t *err)
{
	subtilis_ir_section_t *fn;
	const subtilis_type_t *ptype[2];
	const char *copy = "_copy";
	char *name;

	if (subtilis_type_rec_need_deref(type) && new_rec)
		/*
		 * Then we want to do an init rather than a copy.
		 * As these are two different actions we'll need
		 * separate functions with different names.
		 */

		copy = "_new";

	/*
	 * If new_rec is true but the structure contains no references
	 * we can just leave it set to true as it will be ignored.
	 */

	name = malloc(strlen(type->params.rec.name) + strlen(copy)
		      + 1 + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	sprintf(name, "_%s%s", type->params.rec.name, copy);

	ptype[0] = &subtilis_type_integer;
	ptype[1] = &subtilis_type_integer;

	fn = prv_add_args(p, name, 2, ptype, &subtilis_type_void, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		if (err->type != SUBTILIS_ERROR_ALREADY_DEFINED)
			goto cleanup;
		subtilis_error_init(err);
	} else {
		prv_builtins_ir_gen_rec_copy(p, type, fn, new_rec, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	(void)subtilis_parser_call_2_arg_fn(
		p, name, dest_reg, src_reg, SUBTILIS_IR_REG_TYPE_INTEGER,
		SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_void, true, err);

cleanup:

	free(name);
}

