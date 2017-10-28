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

	return p;
}

void subtilis_arm_program_delete(subtilis_arm_program_t *p)
{
	if (!p)
		return;

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

	switch (i) {
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
		*encoded = imm >> (24 - ((i - 4) * 2));
		break;
	}

	*encoded |= i << 8;

	return true;
}
