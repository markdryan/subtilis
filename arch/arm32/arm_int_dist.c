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
		if (op2->op.reg == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}

		if (op2->op.shift.shift_reg &&
		    op2->op.shift.shift.reg == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}
}

static void prv_dist_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	prv_dist_handle_op2(ud, &instr->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->op1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_mul_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_mul_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((instr->rm == ud->reg_num) || (instr->rs == ud->reg_num)) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
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

	/*
	 * TODO: register allocator does not correctly cope with
	 * pre-index addressing or writeback.
	 */

	if (instr->base == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_ARM_INSTR_LDR) {
		if (instr->dest == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
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

	if ((instr->link_type == SUBTILIS_ARM_BR_LINK_INT) &&
	    (ud->reg_num == 0)) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_swi_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	/* SWIs can only use the first 10 regs */

	if ((ud->reg_num < 10) && (1 << ud->reg_num) & instr->reg_read_mask) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if ((ud->reg_num < 10) && (1 << ud->reg_num) & instr->reg_write_mask) {
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

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_adr_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_adr_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_cmov_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_cmov_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->op3 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->op2 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (!instr->fused && instr->op1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->dest == ud->reg_num) {
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

	if (instr->op1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);

	ud->last_used++;
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

	if (instr->base == ud->reg_num) {
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
		if (!instr->immediate && instr->op2.reg == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest == ud->reg_num) {
			ud->last_used = -1;
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

static void prv_dist_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		if (type == SUBTILIS_FPA_INSTR_RFS)
			ud->last_used = -1;

		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_directive(void *user_data, subtilis_arm_op_t *op,
			       subtilis_error_t *err)
{
}

static void prv_dist_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->base == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_copy_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_ldrc_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FMSR) {
		if (instr->src == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
}

static void prv_dist_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_data_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_vfp_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_sqrt_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_sysreg_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FMRX) {
		if (instr->arm_reg == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else if (instr->arm_reg == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

void subtilis_init_int_dist_walker(subtlis_arm_walker_t *walker,
				   void *user_data)
{
	walker->user_data = user_data;
	walker->label_fn = prv_dist_label;
	walker->directive_fn = prv_dist_directive;
	walker->data_fn = prv_dist_data_instr;
	walker->mul_fn = prv_dist_mul_instr;
	walker->cmp_fn = prv_dist_cmp_instr;
	walker->mov_fn = prv_dist_mov_instr;
	walker->stran_fn = prv_dist_stran_instr;
	walker->mtran_fn = prv_dist_mtran_instr;
	walker->br_fn = prv_dist_br_instr;
	walker->swi_fn = prv_dist_swi_instr;
	walker->ldrc_fn = prv_dist_ldrc_instr;
	walker->cmov_fn = prv_dist_cmov_instr;
	walker->fpa_data_monadic_fn = prv_dist_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_dist_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_dist_fpa_stran_instr;
	walker->fpa_tran_fn = prv_dist_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_dist_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_dist_fpa_ldrc_instr;
	walker->fpa_cptran_fn = prv_dist_fpa_cptran_instr;
	walker->vfp_stran_fn = prv_dist_vfp_stran_instr;
	walker->vfp_copy_fn = prv_dist_vfp_copy_instr;
	walker->vfp_ldrc_fn = prv_dist_vfp_ldrc_instr;
	walker->vfp_tran_fn = prv_dist_vfp_tran_instr;
	walker->vfp_cptran_fn = prv_dist_vfp_cptran_instr;
	walker->vfp_data_fn = prv_dist_vfp_data_instr;
	walker->vfp_cmp_fn = prv_dist_vfp_cmp_instr;
	walker->vfp_sqrt_fn = prv_dist_vfp_sqrt_instr;
	walker->vfp_sysreg_fn = prv_dist_vfp_sysreg_instr;
}

static void prv_used_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_mul_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_mul_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_stran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	/*
	 * TODO: register allocator does not correctly cope with
	 * pre-index addressing or writeback.
	 */

	if (type == SUBTILIS_ARM_INSTR_LDR) {
		if (instr->dest == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
}

static void prv_used_swi_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((ud->reg_num < 10) && (1 << ud->reg_num) & instr->reg_write_mask) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_cmov_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_cmov_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_used_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_used_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type != SUBTILIS_FPA_INSTR_FLT) {
		if (instr->dest == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
}

static void prv_used_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_used_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((type != SUBTILIS_VFP_INSTR_FMSR) && (instr->dest == ud->reg_num)) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_sysreg_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FMRX) {
		if (instr->arm_reg == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	ud->last_used++;
}

void subtilis_init_int_used_walker(subtlis_arm_walker_t *walker,
				   void *user_data)
{
	walker->user_data = user_data;
	walker->label_fn = prv_dist_label;
	walker->directive_fn = prv_dist_directive;
	walker->data_fn = prv_used_data_instr;
	walker->mul_fn = prv_used_mul_instr;
	walker->cmp_fn = prv_used_cmp_instr;
	walker->mov_fn = prv_used_mov_instr;
	walker->stran_fn = prv_used_stran_instr;
	walker->mtran_fn = prv_dist_mtran_instr;
	walker->br_fn = prv_dist_br_instr;
	walker->swi_fn = prv_used_swi_instr;
	walker->ldrc_fn = prv_dist_ldrc_instr;
	walker->adr_fn = prv_dist_adr_instr;
	walker->cmov_fn = prv_used_cmov_instr;
	walker->fpa_data_monadic_fn = prv_dist_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_dist_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_used_fpa_stran_instr;
	walker->fpa_tran_fn = prv_used_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_dist_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_dist_fpa_ldrc_instr;
	walker->fpa_cptran_fn = prv_dist_fpa_cptran_instr;
	walker->vfp_stran_fn = prv_used_vfp_stran_instr;
	walker->vfp_copy_fn = prv_dist_vfp_copy_instr;
	walker->vfp_ldrc_fn = prv_dist_vfp_ldrc_instr;
	walker->vfp_tran_fn = prv_dist_vfp_tran_instr;
	walker->vfp_cptran_fn = prv_used_vfp_cptran_instr;
	walker->vfp_data_fn = prv_dist_vfp_data_instr;
	walker->vfp_cmp_fn = prv_dist_vfp_cmp_instr;
	walker->vfp_sqrt_fn = prv_dist_vfp_sqrt_instr;
	walker->vfp_sysreg_fn = prv_used_vfp_sysreg_instr;
}
