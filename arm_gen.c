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

#include "arm_gen.h"

#include "arm_core.h"

void subtilis_arm_gen_movii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[1].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);

	subtilis_arm_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
			     err);
}

void subtilis_arm_gen_addii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_add_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

static void prv_stran_instr(subtilis_arm_instr_type_t itype,
			    subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	base = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);
	subtilis_arm_stran_imm(arm_p, itype, SUBTILIS_ARM_CCODE_AL, dest, base,
			       offset, err);
}

void subtilis_arm_gen_storeoi32(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_ARM_INSTR_STR, p, start, user_data, err);
}

void subtilis_arm_gen_loadoi32(subtilis_ir_program_t *p, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	prv_stran_instr(SUBTILIS_ARM_INSTR_LDR, p, start, user_data, err);
}

void subtilis_arm_gen_label(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_program_t *arm_p = user_data;
	size_t label = p->ops[start]->op.label;

	subtilis_arm_program_add_label(arm_p, label, err);
}

void subtilis_arm_gen_if_lt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *cmp = &p->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &p->ops[start + 1]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(cmp->operands[1].reg);

	subtilis_arm_cmp_imm(arm_p, SUBTILIS_ARM_CCODE_AL, op1,
			     cmp->operands[2].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->label = jmp->operands[2].label;
}

void subtilis_arm_gen_jump(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *jmp = &p->ops[start]->op.instr;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->label = jmp->operands[0].label;
}

void subtilis_arm_gen_jmpc(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *jmp = &p->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(jmp->operands[0].reg);

	subtilis_arm_cmp_imm(arm_p, SUBTILIS_ARM_CCODE_AL, op1, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->label = jmp->operands[2].label;
}
