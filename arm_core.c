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

#include "arm_core.h"

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
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, int32_t op2,
				 subtilis_error_t *err)
{
	subtilis_arm_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	size_t label = p->label_counter++;

	prv_add_constant(p, label, (uint32_t)op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_program_add_instr(p, SUBTILIS_ARM_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ldrc = &instr->operands.ldrc;
	ldrc->ccode = ccode;
	ldrc->dest.type = SUBTILIS_ARM_REG_FLOATING;
	ldrc->dest.num = p->reg_counter++;
	ldrc->label = label;

	instr = subtilis_arm_program_add_instr(p, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.op.reg.num = ldrc->dest.num;
}

void subtilis_arm_add_data_imm(subtilis_arm_program_t *p,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_instr_type_t alt_type,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err)
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
			return prv_add_data_imm_ldr(p, itype, ccode, status,
						    dest, op1, op2, err);
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
		return prv_add_data_imm_ldr(p, itype, ccode, status, dest, op1,
					    op2, err);
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
