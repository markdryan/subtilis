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

#include "arm_peephole.h"

/*
 * The world's worst peephole optimizer with a window size of 1!
 * You've got to start somwhere.
 */

static size_t prv_remove_op(subtilis_arm_section_t *arm_s, size_t ptr,
			    subtilis_arm_op_t *op)
{
	size_t next;
	size_t prev;

	if (arm_s->first_op == arm_s->last_op) {
		arm_s->first_op = SIZE_MAX;
		arm_s->last_op = SIZE_MAX;
		return SIZE_MAX;
	}

	if (arm_s->first_op == ptr) {
		arm_s->first_op = op->next;
		return op->next;
	}

	if (arm_s->last_op == ptr) {
		arm_s->last_op = op->prev;
		return SIZE_MAX;
	}

	next = op->next;
	prev = op->prev;
	arm_s->op_pool->ops[prev].next = next;
	arm_s->op_pool->ops[next].prev = prev;
	return next;
}

void subtilis_arm_peephole(subtilis_arm_section_t *arm_s, subtilis_error_t *err)
{
	subtilis_arm_op_t *op = NULL;
	size_t ptr;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *data;
	subtilis_fpa_data_instr_t *fpa_data;
	subtilis_fpa_stran_instr_t *fpa_stran;

	ptr = arm_s->first_op;
	while (ptr != SIZE_MAX) {
		op = &arm_s->op_pool->ops[ptr];

		if (op->type != SUBTILIS_ARM_OP_INSTR) {
			ptr = op->next;
			continue;
		}

		instr = &op->op.instr;
		switch (instr->type) {
		case SUBTILIS_ARM_INSTR_MOV:
			data = &instr->operands.data;
			if ((data->op2.type == SUBTILIS_ARM_OP2_REG) &&
			    (data->op2.op.reg == data->dest) &&
			    (data->dest != 15)) {
				ptr = prv_remove_op(arm_s, ptr, op);
				continue;
			}
			break;
		case SUBTILIS_FPA_INSTR_MVF:
			fpa_data = &instr->operands.fpa_data;
			if (!fpa_data->immediate &&
			    fpa_data->op2.reg == fpa_data->dest) {
				ptr = prv_remove_op(arm_s, ptr, op);
				continue;
			}
			break;
		case SUBTILIS_FPA_INSTR_LDF:
		case SUBTILIS_FPA_INSTR_STF:
			fpa_stran = &instr->operands.fpa_stran;
			if (fpa_stran->ccode == SUBTILIS_ARM_CCODE_NV) {
				ptr = prv_remove_op(arm_s, ptr, op);
				continue;
			}
			break;
		default:
			break;
		}

		ptr = op->next;
	}
}
