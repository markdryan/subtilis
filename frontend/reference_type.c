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

#include "reference_type.h"

void subtilis_reference_type_init_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t copy;

	op0.reg = source_reg;

	op1.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_REFERENCE_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, copy, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = SUBTIILIS_REFERENCE_DATA_OFF;
	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_REFERENCE_DATA_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, copy, op1, op2, err);
}

void subtilis_reference_type_assign_ref(subtilis_parser_t *p,
					size_t dest_mem_reg,
					size_t dest_loc, size_t source_reg,
					subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t copy;

	op0.reg = dest_mem_reg;
	op1.integer = dest_loc + SUBTIILIS_REFERENCE_DATA_OFF;

	copy.reg = subtilis_ir_section_add_instr(
		p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current,
					     SUBTILIS_OP_INSTR_DEREF, copy,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_init_ref(p, dest_mem_reg, dest_loc, source_reg,
					 err);
}

size_t subtilis_reference_get_pointer(subtilis_parser_t *p, size_t reg,
				      size_t offset, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t  dest;

	if (offset == 0)
		return reg;

	op0.reg = reg;
	op1.integer = (int32_t) offset;
	dest = subtilis_ir_section_add_instr(p->current,
					     SUBTILIS_OP_INSTR_ADDI_I32, op0,
					     op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return dest;
}
