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

#include "arm_vfp_dist.h"
#include "arm_reg_alloc.h"

static void prv_dist_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_mul_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_mul_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_stran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mtran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_br_instr(void *user_data, subtilis_arm_op_t *op,
			      subtilis_arm_instr_type_t type,
			      subtilis_arm_br_instr_t *instr,
			      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((instr->link_type == SUBTILIS_ARM_BR_LINK_REAL) &&
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

	ud->last_used++;
}

static void prv_dist_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_ldrc_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_adr_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_adr_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_cmov_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_cmov_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

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
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_data_monadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_fpa_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_ldrc_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
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

	if (type == SUBTILIS_VFP_INSTR_FSTS ||
	    type == SUBTILIS_VFP_INSTR_FLDS) {
		subtilis_error_set_assertion_failed(err);
	}

	/*
	 * The function calling code can generate LDFNV and STFNV when
	 * preserving fpa registers.  These instructions should be
	 * ignored for the purposes of register allocation.
	 */

	if (instr->ccode != SUBTILIS_ARM_CCODE_NV) {
		if (type == SUBTILIS_VFP_INSTR_FSTD) {
			if (instr->dest == ud->reg_num) {
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
	}

	ud->last_used++;
}

static void prv_dist_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_copy_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (instr->src == ud->reg_num) {
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

static void prv_dist_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_ldrc_instr_t *instr,
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

static void prv_dist_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FUITOS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (instr->src == ud->reg_num) {
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

static void prv_dist_vfp_tran_dbl_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_vfp_tran_dbl_instr_t *instr,
					subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type != SUBTILIS_VFP_INSTR_FMRRD) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (instr->src1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (type != SUBTILIS_VFP_INSTR_FMSR) {
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

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMACS:
	case SUBTILIS_VFP_INSTR_FNMACS:
	case SUBTILIS_VFP_INSTR_FMSCS:
	case SUBTILIS_VFP_INSTR_FNMSCS:
	case SUBTILIS_VFP_INSTR_FMULS:
	case SUBTILIS_VFP_INSTR_FNMULS:
	case SUBTILIS_VFP_INSTR_FADDS:
	case SUBTILIS_VFP_INSTR_FSUBS:
	case SUBTILIS_VFP_INSTR_FDIVS:
		subtilis_error_set_assertion_failed(err);
		return;
	case SUBTILIS_VFP_INSTR_FMACD:
	case SUBTILIS_VFP_INSTR_FNMACD:
	case SUBTILIS_VFP_INSTR_FMSCD:
	case SUBTILIS_VFP_INSTR_FNMSCD:
		/*
		 * Special case, in which the destination register is also the
		 * source register.
		 */

		if (instr->dest == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	default:
		break;
	}

	if (instr->op1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->op2 == ud->reg_num) {
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

static void prv_dist_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_vfp_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (type == SUBTILIS_VFP_INSTR_FCMPED ||
	    type == SUBTILIS_VFP_INSTR_FCMPD) {
		if (instr->op2 == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	if (instr->op1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_sqrt_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->op1 == ud->reg_num) {
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

static void prv_dist_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_sysreg_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

void subtilis_init_vfp_dist_walker(subtlis_arm_walker_t *walker,
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
	walker->vfp_tran_dbl_fn = prv_dist_vfp_tran_dbl_instr;
	walker->vfp_cptran_fn = prv_dist_vfp_cptran_instr;
	walker->vfp_data_fn = prv_dist_vfp_data_instr;
	walker->vfp_cmp_fn = prv_dist_vfp_cmp_instr;
	walker->vfp_sqrt_fn = prv_dist_vfp_sqrt_instr;
	walker->vfp_sysreg_fn = prv_dist_vfp_sysreg_instr;
}

static void prv_used_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FSTS ||
	    type == SUBTILIS_VFP_INSTR_FLDS) {
		subtilis_error_set_assertion_failed(err);
	}

	/*
	 * The function calling code can generate LDFNV and STFNV when
	 * preserving fpa registers.  These instructions should be
	 * ignored for the purposes of register allocation.
	 */

	if (instr->ccode != SUBTILIS_ARM_CCODE_NV) {
		if (type != SUBTILIS_VFP_INSTR_FSTD) {
			if (instr->dest == ud->reg_num) {
				ud->last_used = -1;
				subtilis_error_set_walker_failed(err);
				return;
			}
		}
	}

	ud->last_used++;
}

static void prv_used_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_copy_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FUITOS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_tran_dbl_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_vfp_tran_dbl_instr_t *instr,
					subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type != SUBTILIS_VFP_INSTR_FMRRD) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if ((type == SUBTILIS_VFP_INSTR_FMSR) && (instr->dest == ud->reg_num)) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_data_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMACS:
	case SUBTILIS_VFP_INSTR_FNMACS:
	case SUBTILIS_VFP_INSTR_FMSCS:
	case SUBTILIS_VFP_INSTR_FNMSCS:
	case SUBTILIS_VFP_INSTR_FMULS:
	case SUBTILIS_VFP_INSTR_FNMULS:
	case SUBTILIS_VFP_INSTR_FADDS:
	case SUBTILIS_VFP_INSTR_FSUBS:
	case SUBTILIS_VFP_INSTR_FDIVS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	if (instr->dest == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_vfp_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	ud->last_used++;
}

static void prv_used_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_sqrt_instr_t *instr,
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

void subtilis_init_vfp_used_walker(subtlis_arm_walker_t *walker,
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
	walker->adr_fn = prv_dist_adr_instr;
	walker->cmov_fn = prv_dist_cmov_instr;
	walker->fpa_data_monadic_fn = prv_dist_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_dist_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_dist_fpa_stran_instr;
	walker->fpa_tran_fn = prv_dist_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_dist_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_dist_fpa_ldrc_instr;
	walker->fpa_cptran_fn = prv_dist_fpa_cptran_instr;
	walker->vfp_stran_fn = prv_used_vfp_stran_instr;
	walker->vfp_copy_fn = prv_used_vfp_copy_instr;
	walker->vfp_ldrc_fn = prv_dist_vfp_ldrc_instr;
	walker->vfp_tran_fn = prv_used_vfp_tran_instr;
	walker->vfp_tran_dbl_fn = prv_used_vfp_tran_dbl_instr;
	walker->vfp_cptran_fn = prv_used_vfp_cptran_instr;
	walker->vfp_data_fn = prv_used_vfp_data_instr;
	walker->vfp_cmp_fn = prv_used_vfp_cmp_instr;
	walker->vfp_sqrt_fn = prv_used_vfp_sqrt_instr;
	walker->vfp_sysreg_fn = prv_dist_vfp_sysreg_instr;
}
