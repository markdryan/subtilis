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

static void prv_data_simple(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *ir_op = &p->ops[start]->op.instr;

	instr = subtilis_arm_program_add_instr(arm_p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	datai->op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = subtilis_arm_ir_to_arm_reg(ir_op->operands[2].reg);
}

static void prv_cmp_simple(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_arm_instr_type_t itype,
			   subtilis_arm_ccode_type_t ccode,
			   subtilis_error_t *err)
{
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *ir_op = &p->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);
	op2 = subtilis_arm_ir_to_arm_reg(ir_op->operands[2].reg);

	subtilis_arm_add_cmp(arm_p, itype, ccode, op1, op2, err);
}

void subtilis_arm_gen_movii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[1].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);

	subtilis_arm_add_mov_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
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

void subtilis_arm_gen_subii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_sub_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

void subtilis_arm_gen_rsubii32(subtilis_ir_program_t *p, size_t start,
			       void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_rsub_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest,
				  op1, op2, err);
}

void subtilis_arm_gen_mulii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	int32_t op2 = instr->operands[2].integer;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_mul_imm(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
				 op2, err);
}

void subtilis_arm_gen_muli32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[2].reg);

	subtilis_arm_add_mul(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op1,
			     op2, err);
}

void subtilis_arm_gen_addi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(p, start, user_data, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_subi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(p, start, user_data, SUBTILIS_ARM_INSTR_SUB,
			SUBTILIS_ARM_CCODE_AL, err);
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
	subtilis_arm_add_stran_imm(arm_p, itype, SUBTILIS_ARM_CCODE_AL, dest,
				   base, offset, err);
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

static void prv_cmp_jmp_imm(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *cmp = &p->ops[start]->op.instr;
	subtilis_ir_inst_t *jmp = &p->ops[start + 1]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(cmp->operands[1].reg);

	subtilis_arm_add_cmp_imm(arm_p, SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = false;
	br->label = jmp->operands[2].label;
}

void subtilis_arm_gen_if_lt_imm(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_if_lte_imm(subtilis_ir_program_t *p, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_if_neq_imm(subtilis_ir_program_t *p, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_arm_gen_if_eq_imm(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_if_gt_imm(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_if_gte_imm(subtilis_ir_program_t *p, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_LT, err);
}

static void prv_data_imm(subtilis_ir_program_t *p, size_t start,
			 void *user_data, subtilis_arm_instr_type_t itype,
			 subtilis_arm_ccode_type_t ccode, bool status,
			 subtilis_error_t *err)
{
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *ir_op = &p->ops[start]->op.instr;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;

	dest = subtilis_arm_ir_to_arm_reg(ir_op->operands[0].reg);
	op1 = subtilis_arm_ir_to_arm_reg(ir_op->operands[1].reg);

	subtilis_arm_add_data_imm(arm_p, itype, ccode, status, dest, op1,
				  ir_op->operands[2].integer, err);
}

static void prv_cmp_jmp(subtilis_ir_program_t *p, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ccode, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *jmp = &p->ops[start + 1]->op.instr;

	prv_cmp_simple(p, start, user_data, SUBTILIS_ARM_INSTR_CMP,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_program_add_instr(arm_p, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = false;
	br->label = jmp->operands[2].label;
}

void subtilis_arm_gen_if_lt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_if_lte(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_if_eq(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_if_neq(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_EQ, err);
}

void subtilis_arm_gen_if_gt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_if_gte(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_jmp(p, start, user_data, SUBTILIS_ARM_CCODE_LT, err);
}

static void prv_cmp_imm(subtilis_ir_program_t *p, size_t start, void *user_data,
			subtilis_arm_ccode_type_t ok,
			subtilis_arm_ccode_type_t nok, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *cmp = &p->ops[start]->op.instr;

	op1 = subtilis_arm_ir_to_arm_reg(cmp->operands[1].reg);
	subtilis_arm_add_cmp_imm(arm_p, SUBTILIS_ARM_CCODE_AL, op1,
				 cmp->operands[2].integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_p, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_p, nok, false, dest, 0, err);
}

void subtilis_arm_gen_gtii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_GT,
		    SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_ltii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_LT,
		    SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_gteii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_GE,
		    SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_arm_gen_lteii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_LE,
		    SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_eqii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		    SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_neqii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_cmp_imm(p, start, user_data, SUBTILIS_ARM_CCODE_NE,
		    SUBTILIS_ARM_CCODE_EQ, err);
}

static void prv_cmp(subtilis_ir_program_t *p, size_t start, void *user_data,
		    subtilis_arm_ccode_type_t ok, subtilis_arm_ccode_type_t nok,
		    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *cmp = &p->ops[start]->op.instr;

	prv_cmp_simple(p, start, user_data, SUBTILIS_ARM_INSTR_CMP,
		       SUBTILIS_ARM_CCODE_AL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = subtilis_arm_ir_to_arm_reg(cmp->operands[0].reg);
	subtilis_arm_add_mov_imm(arm_p, ok, false, dest, -1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_arm_add_mov_imm(arm_p, nok, false, dest, 0, err);
}

void subtilis_arm_gen_gti32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_GT,
		SUBTILIS_ARM_CCODE_LE, err);
}

void subtilis_arm_gen_lti32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_LT,
		SUBTILIS_ARM_CCODE_GE, err);
}

void subtilis_arm_gen_gtei32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_GE,
		SUBTILIS_ARM_CCODE_LT, err);
}

void subtilis_arm_gen_ltei32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_LE,
		SUBTILIS_ARM_CCODE_GT, err);
}

void subtilis_arm_gen_eqi32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_EQ,
		SUBTILIS_ARM_CCODE_NE, err);
}

void subtilis_arm_gen_neqi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_cmp(p, start, user_data, SUBTILIS_ARM_CCODE_NE,
		SUBTILIS_ARM_CCODE_EQ, err);
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

	subtilis_arm_add_cmp_imm(arm_p, SUBTILIS_ARM_CCODE_AL, op1, 0, err);
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

void subtilis_arm_gen_eorii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_data_imm(p, start, user_data, SUBTILIS_ARM_INSTR_EOR,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_orii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_imm(p, start, user_data, SUBTILIS_ARM_INSTR_ORR,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_andii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	prv_data_imm(p, start, user_data, SUBTILIS_ARM_INSTR_AND,
		     SUBTILIS_ARM_CCODE_AL, false, err);
}

void subtilis_arm_gen_eori32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(p, start, user_data, SUBTILIS_ARM_INSTR_EOR,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_ori32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	prv_data_simple(p, start, user_data, SUBTILIS_ARM_INSTR_ORR,
			SUBTILIS_ARM_CCODE_AL, err);
}

void subtilis_arm_gen_mvni32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_program_t *arm_p = user_data;
	subtilis_ir_inst_t *instr = &p->ops[start]->op.instr;
	subtilis_arm_reg_t op2;

	dest = subtilis_arm_ir_to_arm_reg(instr->operands[0].reg);
	op2 = subtilis_arm_ir_to_arm_reg(instr->operands[1].reg);

	subtilis_arm_add_mvn_reg(arm_p, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
}

void subtilis_arm_gen_andi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	prv_data_simple(p, start, user_data, SUBTILIS_ARM_INSTR_AND,
			SUBTILIS_ARM_CCODE_AL, err);
}
