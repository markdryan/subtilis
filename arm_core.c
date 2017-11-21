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

#include <stdlib.h>
#include <string.h>

#include "arm_core.h"
#include "arm_walker.h"

/* clang-format off */
static const uint32_t subitlis_arm_imm_mask[] = {
	0x000000ff,
	0xc000003f,
	0xf000000f,
	0xfc000003,
	0xff000000,
	0xff000000 >> 2,
	0xff000000 >> 4,
	0xff000000 >> 6,
	0xff000000 >> 8,
	0xff000000 >> 10,
	0xff000000 >> 12,
	0xff000000 >> 14,
	0xff000000 >> 16,
	0xff000000 >> 18,
	0xff000000 >> 20,
	0xff000000 >> 22,
};

/* clang-format on */

subtilis_arm_program_t *subtilis_arm_program_new(size_t reg_counter,
						 size_t label_counter,
						 size_t globals,
						 subtilis_error_t *err)
{
	subtilis_arm_program_t *p = malloc(sizeof(*p));

	if (!p) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	p->max_len = 0;
	p->reg_counter = reg_counter;
	p->label_counter = label_counter;
	p->len = 0;
	p->ops = NULL;
	p->globals = globals;
	p->constants = NULL;
	p->constant_count = 0;
	p->max_constants = 0;

	return p;
}

void subtilis_arm_program_delete(subtilis_arm_program_t *p)
{
	if (!p)
		return;

	free(p->constants);
	free(p->ops);
	free(p);
}

subtilis_arm_reg_t subtilis_arm_ir_to_arm_reg(size_t ir_reg)
{
	subtilis_arm_reg_t arm_reg;

	switch (ir_reg) {
	case SUBTILIS_IR_REG_GLOBAL:
		arm_reg.num = 12;
		arm_reg.type = SUBTILIS_ARM_REG_FIXED;
		break;
	case SUBTILIS_IR_REG_LOCAL:
		arm_reg.num = 11;
		arm_reg.type = SUBTILIS_ARM_REG_FIXED;
		break;
	case SUBTILIS_IR_REG_STACK:
		arm_reg.num = 13;
		arm_reg.type = SUBTILIS_ARM_REG_FIXED;
		break;
	default:
		arm_reg.type = SUBTILIS_ARM_REG_FLOATING;
		ir_reg = ir_reg - SUBTILIS_IR_REG_TEMP_START + 4;
		if (ir_reg > 10 && ir_reg < 16)
			ir_reg += 6;
		arm_reg.num = ir_reg;
		break;
	}

	return arm_reg;
}

static subtilis_arm_reg_t prv_acquire_new_reg(subtilis_arm_program_t *p)
{
	return subtilis_arm_ir_to_arm_reg(p->reg_counter++);
}

static void prv_ensure_buffer(subtilis_arm_program_t *p, subtilis_error_t *err)
{
	subtilis_arm_op_t *new_ops;
	size_t new_max;

	if (p->len < p->max_len)
		return;

	new_max = p->max_len + SUBTILIS_CONFIG_PROGRAM_GRAN;
	new_ops = realloc(p->ops, new_max * sizeof(subtilis_arm_op_t));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return;
	}
	p->max_len = new_max;
	p->ops = new_ops;
}

void subtilis_arm_program_add_label(subtilis_arm_program_t *p, size_t label,
				    subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	prv_ensure_buffer(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op = &p->ops[p->len++];
	op->type = SUBTILIS_OP_LABEL;
	op->op.label = label;
}

static void prv_add_constant(subtilis_arm_program_t *p, size_t label,
			     uint32_t num, subtilis_error_t *err)
{
	subtilis_arm_constant_t *c;
	subtilis_arm_constant_t *new_constants;
	size_t new_max;

	if (p->constant_count == p->max_constants) {
		new_max = p->max_constants + 64;
		new_constants = realloc(
		    p->constants, new_max * sizeof(subtilis_arm_constant_t));
		if (!new_constants) {
			subtilis_error_set_oom(err);
			return;
		}
		p->max_constants = new_max;
		p->constants = new_constants;
	}
	c = &p->constants[p->constant_count++];
	c->integer = num;
	c->label = label;
}

subtilis_arm_instr_t *
subtilis_arm_program_add_instr(subtilis_arm_program_t *p,
			       subtilis_arm_instr_type_t type,
			       subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	prv_ensure_buffer(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	op = &p->ops[p->len++];
	memset(op, 0, sizeof(*op));
	op->type = SUBTILIS_OP_INSTR;
	op->op.instr.type = type;
	return &op->op.instr;
}

subtilis_arm_instr_t *subtilis_arm_program_dup_instr(subtilis_arm_program_t *p,
						     subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	if (p->len == 0) {
		subtilis_error_set_asssertion_failed(err);
		return NULL;
	}

	prv_ensure_buffer(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	p->ops[p->len] = p->ops[p->len - 1];
	op = &p->ops[p->len++];
	return &op->op.instr;
}

static void prv_encode_imm(uint32_t imm, size_t mask_index, uint32_t *encoded)
{
	switch (mask_index) {
	case 0:
		*encoded = imm;
		break;
	case 1:
		*encoded = (imm >> 30) | (((imm & 0x3F)) << 2);
		break;
	case 2:
		*encoded = (imm >> 28) | (((imm & 0xF)) << 4);
		break;
	case 3:
		*encoded = (imm >> 26) | (((imm & 0x3)) << 6);
		break;
	default:
		*encoded = imm >> (24 - ((mask_index - 4) * 2));
		break;
	}

	*encoded |= mask_index << 8;
}

bool subtilis_arm_encode_imm(int32_t num, uint32_t *encoded)
{
	size_t i;
	uint32_t imm = (uint32_t)num;

	if (imm == 0) {
		*encoded = 0;
		return true;
	}

	for (i = 0; i < sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t); i++) {
		if ((imm & ~subitlis_arm_imm_mask[i]) == 0)
			break;
	}
	if (i == sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t))
		return false;

	prv_encode_imm(imm, i, encoded);

	return true;
}

bool subtilis_arm_encode_lvl2_imm(int32_t num, uint32_t *encoded1,
				  uint32_t *encoded2)
{
	size_t i;
	uint32_t rem;
	uint32_t imm = (uint32_t)num;

	for (i = 0; i < sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t); i++) {
		rem = imm & ~subitlis_arm_imm_mask[i];
		if (subtilis_arm_encode_imm(rem, encoded2))
			break;
	}
	if (i == sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t))
		return false;

	prv_encode_imm(imm - rem, i, encoded1);

	return true;
}

static void prv_add_data_imm_ldr(subtilis_arm_program_t *p,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest, int32_t op2,
				 subtilis_error_t *err)
{
	subtilis_arm_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label = p->label_counter++;

	prv_add_constant(p, label, (uint32_t)op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_program_add_instr(p, SUBTILIS_ARM_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ldrc = &instr->operands.ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
}

static void prv_add_data_imm_ldr_datai(subtilis_arm_program_t *p,
				       subtilis_arm_instr_type_t itype,
				       subtilis_arm_ccode_type_t ccode,
				       bool status, subtilis_arm_reg_t dest,
				       subtilis_arm_reg_t op1, int32_t op2,
				       subtilis_error_t *err)
{
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t ldr_dest;

	ldr_dest = prv_acquire_new_reg(p);

	prv_add_data_imm_ldr(p, ccode, ldr_dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.op.reg.num = ldr_dest.num;
	datai->op2.op.reg.type = SUBTILIS_ARM_REG_FLOATING;
}

void subtilis_arm_add_addsub_imm(subtilis_arm_program_t *p,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, int32_t op2,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	bool can_encode;
	uint32_t encoded;
	uint32_t encoded2 = 0;

	can_encode = subtilis_arm_encode_imm(op2, &encoded);
	if ((!can_encode) && (op2 < 0)) {
		can_encode = subtilis_arm_encode_imm(-op2, &encoded);
		if (can_encode) {
			op2 = -op2;
			itype = alt_type;
		}
	}

	if (!can_encode) {
		if (status && (ccode != SUBTILIS_ARM_CCODE_AL)) {
			prv_add_data_imm_ldr_datai(p, itype, ccode, status,
						   dest, op1, op2, err);
			return;
		}

		can_encode =
		    subtilis_arm_encode_lvl2_imm(op2, &encoded, &encoded2);

		if ((!can_encode) && (op2 < 0)) {
			can_encode = subtilis_arm_encode_lvl2_imm(
			    -op2, &encoded, &encoded2);
			if (can_encode) {
				op2 = -op2;
				itype = alt_type;
			}
		}
	}

	if (!can_encode) {
		prv_add_data_imm_ldr_datai(p, itype, ccode, status, dest, op1,
					   op2, err);
		return;
	}

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.op.integer = encoded;

	if (encoded2 == 0)
		return;

	instr = subtilis_arm_program_dup_instr(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai->op2.op.integer = encoded2;
}

void subtilis_arm_add_mul_imm(subtilis_arm_program_t *p,
			      subtilis_arm_ccode_type_t ccode, bool status,
			      subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			      int32_t op2, subtilis_error_t *err)
{
	subtilis_arm_reg_t mov_dest;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	mov_dest = prv_acquire_new_reg(p);
	subtilis_arm_mov_imm(p, ccode, false, mov_dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* TODO: A whole pile of optimisations here */

	instr = subtilis_arm_program_add_instr(p, SUBTILIS_ARM_INSTR_MUL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = mov_dest;
}

void subtilis_arm_add_mul(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			  subtilis_arm_reg_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_reg_t tmp_reg;

	if (dest.num == op1.num) {
		if (dest.num == op2.num) {
			subtilis_error_set_asssertion_failed(err);
			return;
		}
		tmp_reg = op2;
		op2 = op1;
		op1 = tmp_reg;
	}

	instr = subtilis_arm_program_add_instr(p, SUBTILIS_ARM_INSTR_MUL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = op2;
}

void subtilis_arm_add_data_imm(subtilis_arm_program_t *p,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	uint32_t encoded;
	subtilis_arm_reg_t mov_dest;
	subtilis_arm_op2_t data_opt2;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		mov_dest = prv_acquire_new_reg(p);
		subtilis_arm_mov_imm(p, ccode, false, mov_dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		data_opt2.type = SUBTILIS_ARM_OP2_REG;
		data_opt2.op.reg = mov_dest;
	} else {
		data_opt2.type = SUBTILIS_ARM_OP2_I32;
		data_opt2.op.integer = encoded;
	}

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2 = data_opt2;
}

void subtilis_arm_mov_reg(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	instr = subtilis_arm_program_add_instr(p, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = op2;
}

void subtilis_arm_mov_imm(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, int32_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_instr_type_t itype;
	subtilis_arm_data_instr_t *datai;
	uint32_t encoded = 0;

	itype = SUBTILIS_ARM_INSTR_MOV;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		if (subtilis_arm_encode_imm(~op2, &encoded)) {
			itype = SUBTILIS_ARM_INSTR_MVN;
		} else {
			prv_add_data_imm_ldr(p, ccode, dest, op2, err);
			return;
		}
	}

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op2.op.integer = encoded;
}

void subtilis_arm_stran_imm(subtilis_arm_program_t *p,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			    int32_t offset, subtilis_error_t *err)
{
	subtilis_arm_op2_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	if (offset > 4095 || offset < -4095) {
		op2.op.reg = prv_acquire_new_reg(p);
		prv_add_data_imm_ldr(p, ccode, op2.op.reg, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op2.type = SUBTILIS_ARM_OP2_REG;
	} else {
		op2.type = SUBTILIS_ARM_OP2_I32;
		op2.op.integer = offset;
	}

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = op2;
	stran->pre_indexed = true;
	stran->write_back = false;
}

void subtilis_arm_cmp_imm(subtilis_arm_program_t *p,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t op1, int32_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_instr_type_t itype;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_reg_t dest;
	uint32_t encoded = 0;

	itype = SUBTILIS_ARM_INSTR_CMP;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		if (subtilis_arm_encode_imm(-op2, &encoded)) {
			itype = SUBTILIS_ARM_INSTR_CMN;
		} else {
			dest.type = SUBTILIS_ARM_REG_FIXED;
			dest.num = 0xffffffff;
			prv_add_data_imm_ldr_datai(p, itype, ccode, false, dest,
						   op1, op2, err);
			return;
		}
	}

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->op1 = op1;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = encoded;
}

/* clang-format off */

static const char *const ccode_desc[] = {
	"EQ", // SUBTILIS_ARM_CCODE_EQ
	"NE", // SUBTILIS_ARM_CCODE_NE
	"CS", // SUBTILIS_ARM_CCODE_CS
	"CC", // SUBTILIS_ARM_CCODE_CC
	"MI", // SUBTILIS_ARM_CCODE_MI
	"PL", // SUBTILIS_ARM_CCODE_PL
	"VS", // SUBTILIS_ARM_CCODE_VS
	"VC", // SUBTILIS_ARM_CCODE_VC
	"HI", // SUBTILIS_ARM_CCODE_HI
	"LS", // SUBTILIS_ARM_CCODE_LS
	"GE", // SUBTILIS_ARM_CCODE_GE
	"LT", // SUBTILIS_ARM_CCODE_LT
	"GT", // SUBTILIS_ARM_CCODE_GT
	"LE", // SUBTILIS_ARM_CCODE_LE
	"AL", // SUBTILIS_ARM_CCODE_AL
	"NV", // SUBTILIS_ARM_CCODE_NV
};

static const char *const instr_desc[] = {
	"AND", // SUBTILIS_ARM_INSTR_AND
	"EOR", // SUBTILIS_ARM_INSTR_EOR
	"SUB", // SUBTILIS_ARM_INSTR_SUB
	"RSB", // SUBTILIS_ARM_INSTR_RSB
	"ADD", // SUBTILIS_ARM_INSTR_ADD
	"ADC", // SUBTILIS_ARM_INSTR_ADC
	"SBC", // SUBTILIS_ARM_INSTR_SBC
	"RSC", // SUBTILIS_ARM_INSTR_RSC
	"TST", // SUBTILIS_ARM_INSTR_TST
	"TEQ", // SUBTILIS_ARM_INSTR_TEQ
	"CMP", // SUBTILIS_ARM_INSTR_CMP
	"CMN", // SUBTILIS_ARM_INSTR_CMN
	"ORR", // SUBTILIS_ARM_INSTR_ORR
	"MOV", // SUBTILIS_ARM_INSTR_MOV
	"BIC", // SUBTILIS_ARM_INSTR_BIC
	"MVN", // SUBTILIS_ARM_INSTR_MVN
	"MUL", // SUBTILIS_ARM_INSTR_MUL
	"MLA", // SUBTILIS_ARM_INSTR_MLA
	"LDR", // SUBTILIS_ARM_INSTR_LDR
	"STR", // SUBTILIS_ARM_INSTR_STR
	"LDM", // SUBTILIS_ARM_INSTR_LDM
	"STM", // SUBTILIS_ARM_INSTR_STM
	"B",   // SUBTILIS_ARM_INSTR_B
	"BL",  // SUBTILIS_ARM_INSTR_BL
	"SWI", // SUBTILIS_ARM_INSTR_SWI
	"LDR", //SUBTILIS_ARM_INSTR_LDRC
};

static const char *const shift_desc[] = {
	"LSL", // SUBTILIS_ARM_SHIFT_LSL
	"ASL", // SUBTILIS_ARM_SHIFT_ASL
	"LSR", // SUBTILIS_ARM_SHIFT_LSR
	"ASR", // SUBTILIS_ARM_SHIFT_ASR
	"ROR", // SUBTILIS_ARM_SHIFT_ROR
	"RRX", // SUBTILIS_ARM_SHIFT_RRX
};

/* clang-format on */

static void prv_dump_op2(subtilis_arm_op2_t *op2)
{
	switch (op2->type) {
	case SUBTILIS_ARM_OP2_REG:
		printf(", R%zu", op2->op.reg.num);
		break;
	case SUBTILIS_ARM_OP2_I32:
		printf(", #%d", op2->op.integer);
		break;
	default:
		printf(", R%zu, %s #%d", op2->op.shift.reg.num,
		       shift_desc[op2->op.shift.type], op2->op.shift.shift);
		break;
	}
}

static void prv_dump_mov_instr(void *user_data, subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->status)
		printf("S");
	printf(" R%zu", instr->dest.num);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_data_instr(void *user_data, subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->status)
		printf("S");
	printf(" R%zu, R%zu", instr->dest.num, instr->op1.num);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_stran_instr(void *user_data,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu", instr->dest.num);
	if (instr->pre_indexed) {
		printf(", [R%zu", instr->base.num);
		prv_dump_op2(&instr->offset);
		printf("]");
		if (instr->write_back)
			printf("!");
	} else {
		printf(", [R%zu]", instr->base.num);
		prv_dump_op2(&instr->offset);
	}
	printf("\n");
}

static void prv_dump_mtran_instr(void *user_data,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mtran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_error_set_asssertion_failed(err);
}

static void prv_dump_br_instr(void *user_data, subtilis_arm_instr_type_t type,
			      subtilis_arm_br_instr_t *instr,
			      subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" label_%zu\n", instr->label);
}

static void prv_dump_swi_instr(void *user_data, subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" &%zx\n", instr->code);
}

static void prv_dump_ldrc_instr(void *user_data, subtilis_arm_instr_type_t type,
				subtilis_arm_ldrc_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu, label_%zu\n", instr->dest.num, instr->label);
}

static void prv_dump_cmp_instr(void *user_data, subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu", instr->op1.num);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_label(void *user_data, size_t label, subtilis_error_t *err)
{
	printf(".label_%zu\n", label);
}

void subtilis_arm_program_dump(subtilis_arm_program_t *p)
{
	subtlis_arm_walker_t walker;
	subtilis_error_t err;
	size_t i;

	subtilis_error_init(&err);

	walker.user_data = NULL;
	walker.label_fn = prv_dump_label;
	walker.data_fn = prv_dump_data_instr;
	walker.cmp_fn = prv_dump_cmp_instr;
	walker.mov_fn = prv_dump_mov_instr;
	walker.stran_fn = prv_dump_stran_instr;
	walker.mtran_fn = prv_dump_mtran_instr;
	walker.br_fn = prv_dump_br_instr;
	walker.swi_fn = prv_dump_swi_instr;
	walker.ldrc_fn = prv_dump_ldrc_instr;

	subtilis_arm_walk(p, &walker, &err);

	for (i = 0; i < p->constant_count; i++) {
		printf(".label_%zu\n", p->constants[i].label);
		printf("\tEQUD &%x\n", p->constants[i].integer);
	}
}
