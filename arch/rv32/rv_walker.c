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

#include "rv_walker.h"

static void prv_call_rtype_fn(subtilis_rv_walker_t *walker, void *user_data,
			      subtilis_rv_op_t *op, subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr = &op->op.instr;

	walker->r_fn(walker->user_data, op, instr->itype, instr->etype,
		     &instr->operands.r, err);
}

static void prv_call_itype_fn(subtilis_rv_walker_t *walker, void *user_data,
			      subtilis_rv_op_t *op, subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr = &op->op.instr;

	walker->i_fn(walker->user_data, op, instr->itype, instr->etype,
		     &instr->operands.i, err);
}

static void prv_call_ujtype_fn(subtilis_rv_walker_t *walker, void *user_data,
			       subtilis_rv_op_t *op, subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr = &op->op.instr;

	walker->uj_fn(walker->user_data, op, instr->itype, instr->etype,
		      &instr->operands.uj, err);
}

static void prv_call_sbtype_fn(subtilis_rv_walker_t *walker, void *user_data,
			       subtilis_rv_op_t *op, subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr = &op->op.instr;

	walker->sb_fn(walker->user_data, op, instr->itype,
		      instr->etype, &instr->operands.sb, err);
}

static void prv_call_fencetype_fn(subtilis_rv_walker_t *walker, void *user_data,
			      subtilis_rv_op_t *op, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

typedef void (*subtilis_walker_fn_t)(subtilis_rv_walker_t *walker,
				     void *user_data, subtilis_rv_op_t *op,
				     subtilis_error_t *err);

/* clang-format off */

static const subtilis_walker_fn_t walker_table[] = {
	prv_call_rtype_fn,
	prv_call_itype_fn,
	prv_call_sbtype_fn,
	prv_call_sbtype_fn,
	prv_call_ujtype_fn,
	prv_call_ujtype_fn,
	prv_call_fencetype_fn
};

/* clang-format on */

static void prv_walk_instr(subtilis_rv_walker_t *walker, subtilis_rv_op_t *op,
			   subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr = &op->op.instr;

	if (((size_t)instr->etype) >=
	    sizeof(walker_table) / sizeof(walker_table[0])) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	walker_table[instr->etype](walker, walker->user_data, op, err);
}

static void prv_rv_walk(subtilis_rv_section_t *rv_s, size_t ptr,
			subtilis_rv_op_t *to, subtilis_rv_walker_t *walker,
			subtilis_error_t *err)
{
	subtilis_rv_op_t *op;

	while (ptr != SIZE_MAX) {
		op = &rv_s->op_pool->ops[ptr];

		switch (op->type) {
		case SUBTILIS_RV_OP_LABEL:
			walker->label_fn(walker->user_data, op, op->op.label,
					 err);
			break;
		case SUBTILIS_RV_OP_INSTR:
			prv_walk_instr(walker, op, err);
			break;
		case SUBTILIS_RV_OP_ALIGN:
		case SUBTILIS_RV_OP_BYTE:
		case SUBTILIS_RV_OP_TWO_BYTE:
		case SUBTILIS_RV_OP_FOUR_BYTE:
		case SUBTILIS_RV_OP_DOUBLE:
		case SUBTILIS_RV_OP_FLOAT:
		case SUBTILIS_RV_OP_STRING:
			if (!walker->directive_fn) {
				subtilis_error_set_assertion_failed(err);
				break;
			}
			walker->directive_fn(walker->user_data, op, err);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (op == to)
			break;

		ptr = op->next;
	}
}

void subtilis_rv_walk(subtilis_rv_section_t *rv_s,
		      subtilis_rv_walker_t *walker, subtilis_error_t *err)
{
	prv_rv_walk(rv_s, rv_s->first_op, NULL, walker, err);
}

void subtilis_rv_walk_from(subtilis_rv_section_t *rv_s,
			   subtilis_rv_walker_t *walker, subtilis_rv_op_t *op,
			   subtilis_error_t *err)
{
	subtilis_rv_walk_from_to(rv_s, walker, op, NULL, err);
}

void subtilis_rv_walk_from_to(subtilis_rv_section_t *rv_s,
			      subtilis_rv_walker_t *walker,
			      subtilis_rv_op_t *from, subtilis_rv_op_t *to,
			      subtilis_error_t *err)
{
	size_t ptr;

	if (from->next == SIZE_MAX)
		ptr = rv_s->last_op;
	else
		ptr = rv_s->op_pool->ops[from->next].prev;
	prv_rv_walk(rv_s, ptr, to, walker, err);
}
