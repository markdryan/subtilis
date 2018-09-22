/*
 * Copyright (c) 2018 Mark Ryan
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

#include "arm_int_dist.h"
#include "arm_reg_alloc.h"

static void prv_dist_handle_op2(subtilis_dist_data_t *ud,
				subtilis_arm_op2_t *op2, subtilis_error_t *err)
{
	if (op2->type == SUBTILIS_ARM_OP2_REG) {
		if (op2->op.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}

		if (op2->op.shift.shift_reg &&
		    op2->op.shift.shift.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}
	ud->last_used++;
}

static void prv_dist_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->op1.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_mul_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_mul_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	if ((instr->rm.num == ud->reg_num) || (instr->rs.num == ud->reg_num)) {
		subtilis_error_set_walker_failed(err);
		return;
	}
	ud->last_used++;
}

static void prv_dist_stran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_ARM_INSTR_LDR) {
		if (instr->dest.num == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	if (instr->base.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->offset, err);
}

static void prv_dist_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mtran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	/*
	 * TODO: Mtran can not currently be used in distance calculations
	 * as we can only represent a subset of the virtual registers
	 * in the instruction.  As we're only currently using it to implement
	 * a stack this is okay.  But long term we'll need to fix this if
	 * we want to use the instructions for general purpose use.
	 */

	ud->last_used++;
}

static void prv_dist_br_instr(void *user_data, subtilis_arm_op_t *op,
			      subtilis_arm_instr_type_t type,
			      subtilis_arm_br_instr_t *instr,
			      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_swi_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((1 << ud->reg_num) & instr->reg_mask) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_ldrc_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->op1.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_label(void *user_data, subtilis_arm_op_t *op, size_t label,
			   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_fpa_data_dyadic_instr(void *user_data,
					   subtilis_arm_op_t *op,
					   subtilis_arm_instr_type_t type,
					   subtilis_fpa_data_instr_t *instr,
					   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_fpa_data_monadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->base.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_FPA_INSTR_FLT) {
		if (!instr->immediate && instr->op2.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
}

static void prv_dist_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_fpa_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_ldrc_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

void subtilis_init_int_dist_walker(subtlis_arm_walker_t *walker,
				   void *user_data)
{
	walker->user_data = user_data;
	walker->label_fn = prv_dist_label;
	walker->data_fn = prv_dist_data_instr;
	walker->mul_fn = prv_dist_mul_instr;
	walker->cmp_fn = prv_dist_cmp_instr;
	walker->mov_fn = prv_dist_mov_instr;
	walker->stran_fn = prv_dist_stran_instr;
	walker->mtran_fn = prv_dist_mtran_instr;
	walker->br_fn = prv_dist_br_instr;
	walker->swi_fn = prv_dist_swi_instr;
	walker->ldrc_fn = prv_dist_ldrc_instr;
	walker->fpa_data_monadic_fn = prv_dist_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_dist_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_dist_fpa_stran_instr;
	walker->fpa_tran_fn = prv_dist_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_dist_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_dist_fpa_ldrc_instr;
}
