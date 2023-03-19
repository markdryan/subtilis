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

#include "rv_peephole.h"

/*
 * The world's worst peephole optimizer with a window size of 1!
 * You've got to start somwhere.
 */

static size_t prv_remove_op(subtilis_rv_section_t *rv_s, size_t ptr,
			    subtilis_rv_op_t *op)
{
	size_t next;
	size_t prev;

	if (rv_s->first_op == rv_s->last_op) {
		rv_s->first_op = SIZE_MAX;
		rv_s->last_op = SIZE_MAX;
		return SIZE_MAX;
	}

	if (rv_s->first_op == ptr) {
		rv_s->first_op = op->next;
		return op->next;
	}

	if (rv_s->last_op == ptr) {
		rv_s->last_op = op->prev;
		return SIZE_MAX;
	}

	next = op->next;
	prev = op->prev;
	rv_s->op_pool->ops[prev].next = next;
	rv_s->op_pool->ops[next].prev = prev;
	return next;
}

void subtilis_rv_peephole(subtilis_rv_section_t *rv_s, subtilis_error_t *err)
{

	size_t ptr;
	size_t prev;
	subtilis_rv_instr_t *instr;
	subtilis_rv_op_t *prev_op;
	subtilis_rv_op_t *op = NULL;

	prev = SIZE_MAX;
	ptr = rv_s->first_op;
	while (ptr != SIZE_MAX) {
		op = &rv_s->op_pool->ops[ptr];

		if (op->type != SUBTILIS_RV_OP_INSTR) {
			ptr = op->next;
			prev = ptr;
			continue;
		}

		instr = &op->op.instr;
		switch (instr->itype) {
		case SUBTILIS_RV_ADDI:
			if (subtilis_rv_section_is_nop(op)) {
				if (prev == SIZE_MAX) {
					ptr = prv_remove_op(rv_s, ptr, op);
					prev = SIZE_MAX;
					continue;
				}
				prev_op = &rv_s->op_pool->ops[prev];
				if ((prev_op->type != SUBTILIS_RV_OP_INSTR) ||
				    (prev_op->op.instr.etype !=
				     SUBTILIS_RV_B_TYPE)) {
					ptr = prv_remove_op(rv_s, ptr, op);
					prev = SIZE_MAX;
					continue;
				}
			} else {
				if ((instr->operands.i.rd ==
				     instr->operands.i.rs1) &&
				    (instr->operands.i.imm == 0)) {
					ptr = prv_remove_op(rv_s, ptr, op);
					prev = SIZE_MAX;
				}
			}
			break;
		default:
			break;
		}

		prev = ptr;
		ptr = op->next;
	}
}
