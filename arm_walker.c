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

#include <limits.h>

#include "arm_walker.h"

static void prv_walk_instr(subtlis_arm_walker_t *walker, subtilis_arm_op_t *op,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	switch (instr->type) {
	case SUBTILIS_ARM_INSTR_AND:
	case SUBTILIS_ARM_INSTR_EOR:
	case SUBTILIS_ARM_INSTR_SUB:
	case SUBTILIS_ARM_INSTR_RSB:
	case SUBTILIS_ARM_INSTR_ADD:
	case SUBTILIS_ARM_INSTR_ADC:
	case SUBTILIS_ARM_INSTR_SBC:
	case SUBTILIS_ARM_INSTR_RSC:
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
	case SUBTILIS_ARM_INSTR_ORR:
	case SUBTILIS_ARM_INSTR_BIC:
	case SUBTILIS_ARM_INSTR_MUL:
	case SUBTILIS_ARM_INSTR_MLA:
		walker->data_fn(walker->user_data, op, instr->type,
				&instr->operands.data, err);
		break;
	case SUBTILIS_ARM_INSTR_CMP:
	case SUBTILIS_ARM_INSTR_CMN:
		walker->cmp_fn(walker->user_data, op, instr->type,
			       &instr->operands.data, err);
		break;
	case SUBTILIS_ARM_INSTR_MOV:
	case SUBTILIS_ARM_INSTR_MVN:
		walker->mov_fn(walker->user_data, op, instr->type,
			       &instr->operands.data, err);
		break;
	case SUBTILIS_ARM_INSTR_LDR:
	case SUBTILIS_ARM_INSTR_STR:
		walker->stran_fn(walker->user_data, op, instr->type,
				 &instr->operands.stran, err);
		break;
	case SUBTILIS_ARM_INSTR_LDM:
	case SUBTILIS_ARM_INSTR_STM:
		walker->mtran_fn(walker->user_data, op, instr->type,
				 &instr->operands.mtran, err);
		break;
	case SUBTILIS_ARM_INSTR_B:
	case SUBTILIS_ARM_INSTR_BL:
		walker->br_fn(walker->user_data, op, instr->type,
			      &instr->operands.br, err);
		break;
	case SUBTILIS_ARM_INSTR_SWI:
		walker->swi_fn(walker->user_data, op, instr->type,
			       &instr->operands.swi, err);
		break;
	case SUBTILIS_ARM_INSTR_LDRC:
		walker->ldrc_fn(walker->user_data, op, instr->type,
				&instr->operands.ldrc, err);
		break;
	}
}

static void prv_arm_walk(subtilis_arm_program_t *arm_p, size_t ptr,
			 subtlis_arm_walker_t *walker, subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	while (ptr != SIZE_MAX) {
		op = &arm_p->pool->ops[ptr];

		switch (op->type) {
		case SUBTILIS_OP_LABEL:
			walker->label_fn(walker->user_data, op, op->op.label,
					 err);
			break;
		case SUBTILIS_OP_INSTR:
			prv_walk_instr(walker, op, err);
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			break;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ptr = op->next;
	}
}

void subtilis_arm_walk(subtilis_arm_program_t *arm_p,
		       subtlis_arm_walker_t *walker, subtilis_error_t *err)
{
	prv_arm_walk(arm_p, arm_p->first_op, walker, err);
}

void subtilis_arm_walk_from(subtilis_arm_program_t *arm_p,
			    subtlis_arm_walker_t *walker, subtilis_arm_op_t *op,
			    subtilis_error_t *err)
{
	size_t ptr;

	if (op->next == SIZE_MAX)
		ptr = arm_p->last_op;
	else
		ptr = arm_p->pool->ops[op->next].prev;
	prv_arm_walk(arm_p, ptr, walker, err);
}
