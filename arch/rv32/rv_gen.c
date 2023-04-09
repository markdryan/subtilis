/*
 * Copyright (c) 2023 Mark Ryan
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

#include <limits.h>

#include "rv_gen.h"

void subtilis_rv_gen_mov(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t op2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	op2 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	subtilis_rv_section_add_mv(rv_s, dest, op2, err);
}

void subtilis_rv_gen_movii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[1].integer;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);

	subtilis_rv_section_add_li(rv_s, dest, op2, err);
}

static void prv_itype_gen(subtilis_ir_section_t *s,
			  subtilis_rv_instr_type_t itype,
			  subtilis_rv_instr_type_t rtype,
			  size_t start, void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t tmp;
	int32_t imm;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);
	imm = instr->operands[2].integer;

	if ((imm >= -2048) && (imm < 2048)) {
		subtilis_rv_section_add_itype(rv_s, itype, dest, rs1, imm, err);
		return;
	}

	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);
	subtilis_rv_section_add_li(rv_s, tmp, imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_rv_section_add_rtype(rv_s, rtype, dest, rs1, tmp, err);
}

void subtilis_rv_gen_addii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_ADDI, SUBTILIS_RV_ADD, start, user_data,
		      err);
}

void subtilis_rv_gen_subii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t tmp;
	int32_t imm;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);
	imm = -instr->operands[2].integer;

	if ((imm >= -2048) && (imm < 2048)) {
		subtilis_rv_section_add_itype(rv_s, SUBTILIS_RV_ADDI, dest, rs1,
					      imm, err);
		return;
	}

	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);
	subtilis_rv_section_add_li(rv_s, tmp, imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_rv_section_add_itype(rv_s, SUBTILIS_RV_ADDI, dest, rs1, tmp,
				      err);
}

void subtilis_rv_gen_rsubii32(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t tmp;
	int32_t imm;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);
	imm = instr->operands[2].integer;
	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);

	subtilis_rv_section_add_li(rv_s, tmp, imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_rtype(rv_s, SUBTILIS_RV_SUB, dest, tmp, rs1,
				      err);
}

static void prv_gen_rtype_imm(subtilis_ir_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t tmp;
	int32_t imm;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);
	imm = instr->operands[2].integer;
	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);

	subtilis_rv_section_add_li(rv_s, tmp, imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_rtype(rv_s, itype, dest, rs1, tmp,
				      err);
}

void subtilis_rv_gen_mulii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_gen_rtype_imm(s, SUBTILIS_RV_MUL, start, user_data, err);
}

static void prv_rtype_gen(subtilis_ir_section_t *s,
			  subtilis_rv_instr_type_t itype,
			  size_t start, void *user_data,
			  subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);
	rs2 = subtilis_rv_ir_to_rv_reg(instr->operands[2].reg);

	subtilis_rv_section_add_rtype(rv_s, itype, dest, rs1, rs2, err);
}

void subtilis_rv_gen_divi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_DIV, start, user_data, err);
}

void subtilis_rv_gen_divii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_gen_rtype_imm(s, SUBTILIS_RV_DIV, start, user_data, err);
}

void subtilis_rv_gen_modi32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_REM, start, user_data, err);
}

void subtilis_rv_gen_addi32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_ADD, start, user_data, err);
}

void subtilis_rv_gen_subi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_SUB, start, user_data, err);
}

void subtilis_rv_gen_muli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_MUL, start, user_data, err);
}

void subtilis_rv_gen_storeoi8(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_rv_reg_t val;
	subtilis_rv_reg_t base;

	val = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	base = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	/*
	 * Now this will have to be modified to support offsets larger
	 * than 12 bits.
	 */

	if (offset < -2048 || offset > 2047) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_rv_section_add_sb(rv_s, base, val, offset, err);
}

void subtilis_rv_gen_loadoi8(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t base;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	base = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	/*
	 * Now this will have to be modified to support offsets larger
	 * than 12 bits.
	 */

	if (offset < -2048 || offset > 2047) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_rv_section_add_lb(rv_s, dest, base, offset, err);
}

void subtilis_rv_gen_storeoi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t base;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	base = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	/*
	 * Now this will have to be modified to support offsets larger
	 * than 12 bits.
	 */

	if (offset < -2048 || offset > 2047) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_rv_section_add_sw(rv_s, base, dest, offset, err);
}

void subtilis_rv_gen_loadoi32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t base;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	base = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	/*
	 * Now this will have to be modified to support offsets larger
	 * than 12 bits.
	 */

	if (offset < -2048 || offset > 2047) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_rv_section_add_lw(rv_s, dest, base, offset, err);
}

void subtilis_rv_gen_label(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	size_t label = s->ops[start]->op.label;

	subtilis_rv_section_add_label(rv_s, label, err);
}

static void prv_stack_args(subtilis_rv_section_t *rv_s,
			   subtilis_ir_call_t *call, size_t int_args_left,
			   size_t real_args_left, size_t *int_arg_ops,
			   size_t *real_arg_ops, subtilis_error_t *err)
{
	size_t offset;
	size_t i;
	size_t reg_num;
	size_t arg;
	size_t stack_ops;
	subtilis_rv_reg_t arg_src;
	subtilis_rv_reg_t arg_dest;

	/* TODO: There should be an argument limit here. */

	/*
	 * We shove the remaining arguments on the stack.  Then we just need
	 * to prime the register allocator to think it has previously spilled
	 * these values to the stack.
	 */

	offset = 0;
	stack_ops = 0;
	for (i = call->arg_count; i > 0 && int_args_left > 8; i--) {
		if (call->args[i - 1].type == SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		offset += 4;
		reg_num = call->args[i - 1].reg;
		subtilis_rv_section_add_sw(rv_s,
					   SUBTILIS_RV_REG_STACK,
					   subtilis_rv_ir_to_rv_reg(reg_num),
					   -offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		int_arg_ops[stack_ops++] = rv_s->last_op;
		int_args_left--;
	}

/*
	stack_ops = 0;
	for (i = call->arg_count; i > 0 && real_args_left > 8; i--) {
		if (call->args[i - 1].type != SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		offset += 8;
		reg_num = call->args[i - 1].reg;

		arm_s->fp_if->store_dbl_fn(
		    arm_s, subtilis_arm_ir_to_real_reg(arm_s, reg_num), op0,
		    offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		real_arg_ops[stack_ops++] = arm_s->last_op;
		real_args_left--;
	}
*/
	for (i = 0, arg = 0; i < call->arg_count && arg < int_args_left; i++) {
		if (call->args[i].type != SUBTILIS_IR_REG_TYPE_INTEGER)
			continue;
		arg_dest = arg + SUBTILIS_RV_REG_A0;
		arg_src = subtilis_rv_ir_to_rv_reg(call->args[i].reg);
		subtilis_rv_section_add_mv(rv_s, arg_dest, arg_src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		arg++;
	}
/*
	for (i = 0, arg = 0; i < call->arg_count && arg < real_args_left; i++) {
		if (call->args[i].type != SUBTILIS_IR_REG_TYPE_REAL)
			continue;
		arg_dest = arg;
		arg_src = subtilis_arm_ir_to_real_reg(arm_s, call->args[i].reg);
		arm_s->fp_if->mov_reg_fn(arm_s, arg_dest, arg_src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		arg++;
	}
*/
}

void subtilis_rv_gen_call_gen(subtilis_ir_section_t *s, size_t start,
			      void *user_data, bool indirect,
			      subtilis_rv_jal_link_type_t link_type,
			      subtilis_error_t *err)
{
	int i;
	subtilis_rv_reg_t jump_reg;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;
	subtilis_rv_section_t *rv_s = user_data;
	size_t int_args = 0;
	size_t real_args = 0;
	size_t int_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];
	size_t real_arg_ops[SUBTILIS_IR_MAX_ARGS_PER_TYPE - 4];

	for (i = 0; i < call->arg_count; i++) {
		if (call->args[i].type == SUBTILIS_IR_REG_TYPE_REAL)
			real_args++;
		else
			int_args++;
	}

	if (real_args > SUBTILIS_IR_MAX_ARGS_PER_TYPE ||
	    int_args > SUBTILIS_IR_MAX_ARGS_PER_TYPE) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prv_stack_args(rv_s, call, int_args, real_args, int_arg_ops,
		       real_arg_ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_sw(rv_s, SUBTILIS_RV_REG_STACK,
				   SUBTILIS_RV_REG_LINK, -4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_rv_section_add_sw(rv_s, SUBTILIS_RV_REG_STACK,
				   SUBTILIS_RV_REG_LOCAL, -8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_addi(rv_s, SUBTILIS_RV_REG_STACK,
				     SUBTILIS_RV_REG_STACK, -8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!indirect) {
		subtilis_rv_section_add_jal(rv_s, SUBTILIS_RV_REG_LINK,
					    call->proc_id, link_type,
					    err);
	} else {
		jump_reg = subtilis_rv_ir_to_rv_reg(call->proc_id);
		subtilis_rv_section_add_jalr(rv_s, SUBTILIS_RV_REG_LINK,
					     jump_reg, 0, link_type, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_addi(rv_s, SUBTILIS_RV_REG_STACK,
				     SUBTILIS_RV_REG_STACK, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_lw(rv_s, SUBTILIS_RV_REG_LOCAL,
				   SUBTILIS_RV_REG_STACK, -8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_lw(rv_s, SUBTILIS_RV_REG_LINK,
				   SUBTILIS_RV_REG_STACK, -4, err);

}

void subtilis_rv_gen_call(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_rv_gen_call_gen(s, start, user_data, false,
				 SUBTILIS_RV_JAL_LINK_VOID, err);

}

void subtilis_rv_gen_calli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_call_t *call = &s->ops[start]->op.call;

	subtilis_rv_gen_call_gen(s, start, user_data, false,
				 SUBTILIS_RV_JAL_LINK_INT, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_rv_ir_to_rv_reg(call->reg);

	subtilis_rv_section_add_mv(rv_s, dest, SUBTILIS_RV_REG_A0, err);
}

static void prv_cmp_jmp_imm(subtilis_ir_section_t *s, size_t start,
			    void *user_data,
			    subtilis_rv_instr_type_t itype,
			    subtilis_error_t *err)
{
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	/*
	 * This is messy.  We have to allow for the fact that our
	 * target will be more than -+ 4096 away from the current
	 * instruction, so we'll need to insert a nop in case
	 * we need to switch to a long jump when we resolve the
	 * local function addresses.
	 */

	rs1 = subtilis_rv_ir_to_rv_reg(cmp->operands[1].reg);
	rs2 = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);

	subtilis_rv_section_add_li(rv_s, rs2, cmp->operands[2].integer,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_btype(rv_s, itype, rs1, rs2,
				      jmp->operands[2].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

static void prv_cmp_jmp_imm_rev(subtilis_ir_section_t *s, size_t start,
				void *user_data,
				subtilis_rv_instr_type_t itype,
				subtilis_error_t *err)
{
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	/*
	 * This is messy.  We have to allow for the fact that our
	 * target will be more than -+ 4096 away from the current
	 * instruction, so we'll need to insert a nop in case
	 * we need to switch to a long jump when we resolve the
	 * local function addresses.
	 */

	rs1 = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);
	rs2 = subtilis_rv_ir_to_rv_reg(cmp->operands[1].reg);

	subtilis_rv_section_add_li(rv_s, rs1, cmp->operands[2].integer,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_btype(rv_s, itype, rs1, rs2,
				      jmp->operands[2].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

static void prv_cmp_jmp(subtilis_ir_section_t *s, size_t start,
			void *user_data,
			subtilis_rv_instr_type_t itype,
			subtilis_error_t *err)
{
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	/*
	 * This is messy.  We have to allow for the fact that our
	 * target will be more than -+ 4096 away from the current
	 * instruction, so we'll need to insert a nop in case
	 * we need to switch to a long jump when we resolve the
	 * local function addresses.
	 */

	rs1 = subtilis_rv_ir_to_rv_reg(cmp->operands[1].reg);
	rs2 = subtilis_rv_ir_to_rv_reg(cmp->operands[2].reg);

	subtilis_rv_section_add_btype(rv_s, itype, rs1, rs2,
				      jmp->operands[2].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

static void prv_cmp_jmp_rev(subtilis_ir_section_t *s, size_t start,
			    void *user_data,
			    subtilis_rv_instr_type_t itype,
			    subtilis_error_t *err)
{
	subtilis_rv_reg_t rs1;
	subtilis_rv_reg_t rs2;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *cmp = &s->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &s->ops[start + 1]->op.instr;

	/*
	 * This is messy.  We have to allow for the fact that our
	 * target will be more than -+ 4096 away from the current
	 * instruction, so we'll need to insert a nop in case
	 * we need to switch to a long jump when we resolve the
	 * local function addresses.
	 */

	rs1 = subtilis_rv_ir_to_rv_reg(cmp->operands[2].reg);
	rs2 = subtilis_rv_ir_to_rv_reg(cmp->operands[1].reg);

	subtilis_rv_section_add_btype(rv_s, itype, rs1, rs2,
				      jmp->operands[2].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

void subtilis_rv_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_RV_BGE, err);
}

void subtilis_rv_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm_rev(s, start, user_data, SUBTILIS_RV_BLT, err);
}

void subtilis_rv_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_RV_BEQ, err);
}

void subtilis_rv_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_RV_BNE, err);
}

void subtilis_rv_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm_rev(s, start, user_data, SUBTILIS_RV_BGE, err);
}

void subtilis_rv_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(s, start, user_data, SUBTILIS_RV_BLT, err);
}

void subtilis_rv_gen_jump(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;

	subtilis_rv_section_add_jal(rv_s, 0, jmp->operands[2].label,
				    SUBTILIS_RV_JAL_LINK_VOID, err);
}

void subtilis_rv_gen_jmpc(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;
	subtilis_rv_reg_t op1;

	op1 = subtilis_rv_ir_to_rv_reg(jmp->operands[0].reg);

	subtilis_rv_section_add_beq(rv_s, op1, 0, jmp->operands[2].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

void subtilis_rv_gen_jmpc_rev(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *jmp = &s->ops[start]->op.instr;
	subtilis_rv_reg_t op1;

	op1 = subtilis_rv_ir_to_rv_reg(jmp->operands[0].reg);

	subtilis_rv_section_add_bne(rv_s, op1, 0, jmp->operands[1].label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We add the NOP if we need to do the long jump.
	 */

	subtilis_rv_section_add_nop(rv_s, err);
}

void subtilis_rv_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_RV_BGE, err);
}

void subtilis_rv_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_rev(s, start, user_data, SUBTILIS_RV_BLT, err);
}

void subtilis_rv_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_RV_BNE, err);
}

void subtilis_rv_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_RV_BEQ, err);
}

void subtilis_rv_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_rev(s, start, user_data, SUBTILIS_RV_BGE, err);
}

void subtilis_rv_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(s, start, user_data, SUBTILIS_RV_BLT, err);
}

void subtilis_rv_gen_eori32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_XOR, start, user_data, err);
}

void subtilis_rv_gen_ori32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_OR, start, user_data, err);
}

void subtilis_rv_gen_andi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_AND, start, user_data, err);
}

void subtilis_rv_gen_eorii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_XORI, SUBTILIS_RV_XOR, start,
		      user_data, err);
}

void subtilis_rv_gen_orii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_ORI, SUBTILIS_RV_OR, start,
		      user_data, err);
}

void subtilis_rv_gen_andii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_ANDI, SUBTILIS_RV_AND, start,
		      user_data, err);
}

void subtilis_rv_gen_lsli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_SLL, start, user_data, err);
}

void subtilis_rv_gen_lslii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_SLLI, SUBTILIS_RV_SLL, start,
		      user_data, err);
}

void subtilis_rv_gen_lsri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_SRL, start, user_data, err);
}

void subtilis_rv_gen_lsrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_SRLI, SUBTILIS_RV_SRL, start,
		      user_data, err);
}

void subtilis_rv_gen_asri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_rtype_gen(s, SUBTILIS_RV_SRA, start, user_data, err);
}

void subtilis_rv_gen_asrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_itype_gen(s, SUBTILIS_RV_SRAI, SUBTILIS_RV_SRA, start,
		      user_data, err);
}

void subtilis_rv_gen_mvni32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t rs1;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	subtilis_rv_section_add_itype(rv_s, SUBTILIS_RV_XOR, dest, rs1, -1,
				      err);
}

void subtilis_rv_gen_ret(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t tmp;
	subtilis_rv_section_t *rv_s = user_data;

	tmp = subtilis_rv_ir_to_rv_reg(rv_s->reg_counter++);

	subtilis_rv_section_add_lui(rv_s, tmp, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_ret_site(rv_s, rv_s->last_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_addi(rv_s, tmp, tmp, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_add(rv_s, SUBTILIS_RV_REG_STACK,
				    SUBTILIS_RV_REG_STACK, tmp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_section_add_jalr(rv_s, 0, 1, 0, SUBTILIS_RV_JAL_LINK_VOID,
				     err);
}

void subtilis_rv_gen_reti32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t rs1;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	rs1 = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);

	subtilis_rv_section_add_mv(rv_s, 10, rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_gen_ret(s, start, user_data, err);
}

void subtilis_rv_gen_retii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;

	subtilis_rv_section_add_li(rv_s, 10, instr->operands[0].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_rv_gen_ret(s, start, user_data, err);
}

void subtilis_rv_restore_stack(subtilis_rv_section_t *rv_s,
			       size_t stack_space, subtilis_error_t *err)
{
	size_t i;
	size_t rs;
	rv_ujtype_t *lui;
	subtilis_rv_op_pool_t *pool = rv_s->op_pool;
	rv_itype_t *addi = NULL;
	rv_itype_t *itype = NULL;
	rv_rtype_t *add = NULL;
	rv_rtype_t *rtype = NULL;
	subtilis_rv_instr_t *lui_instr;
	subtilis_rv_instr_t *add_instr;

	for (i = 0; i < rv_s->ret_sites.len; i++) {

		rs = rv_s->ret_sites.vals[i];
		lui_instr = &pool->ops[rs].op.instr;
		lui = &lui_instr->operands.uj;

		/*
		 * addi should be the next instruction but best be sure
		 */

		for (rs = rs + 1; rs < pool->len; rs++)
			if (pool->ops[rs].op.instr.itype == SUBTILIS_RV_ADDI) {
				itype = &pool->ops[rs].op.instr.operands.i;
				if (pool->ops[rs].op.instr.operands.i.imm == 0)
					addi = itype;
				break;
			}
		if (!addi) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		/*
		 * add should be the next instruction but best be sure
		 */

		for (rs = rs + 1; rs < pool->len; rs++)
			if (pool->ops[rs].op.instr.itype == SUBTILIS_RV_ADD) {
				rtype = &pool->ops[rs].op.instr.operands.r;
				if ((rtype->rd == SUBTILIS_RV_REG_STACK) &&
				    (rtype->rs1 == SUBTILIS_RV_REG_STACK) &&
				    (rtype->rs2 == addi->rd)) {
					add = rtype;
					add_instr = &pool->ops[rs].op.instr;
					break;
				    }
			}
		if (!add) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		if (stack_space < 2048) {

			/*
			 * Then we just need to make the lui and the add
			 * instructions into nops and the add into an addi.
			 */

			addi->rd = addi->rs1 = SUBTILIS_RV_REG_STACK;
			addi->imm = stack_space;

			subtilis_rv_section_nopify_instr(lui_instr);
			subtilis_rv_section_nopify_instr(add_instr);
			continue;
		}

		lui->op.imm = stack_space >> 12;
		addi->imm = ((int32_t) stack_space) & 0xfff;
	}
}

void subtilis_rv_gen_lca(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *ir_op = &s->ops[start]->op.instr;
	int32_t const_id = ir_op->operands[1].integer;
	subtilis_rv_reg_t dest =
		subtilis_rv_ir_to_rv_reg(ir_op->operands[0].reg);

	subtilis_rv_section_add_la(rv_s, dest, const_id, err);
}
