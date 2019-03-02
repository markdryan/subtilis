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

#include "builtins_ir.h"

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
