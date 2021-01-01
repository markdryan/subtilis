/*
 * Copyright (c) 2020 Mark Ryan
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

#include "arm_reg_alloc.h"
#include "fpa_alloc.h"

#include <limits.h>

static void prv_alloc_fpa_data_dyadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_data_monadic_instr(void *user_data,
					     subtilis_arm_op_t *op,
					     subtilis_arm_instr_type_t type,
					     subtilis_fpa_data_instr_t *instr,
					     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_stran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_fpa_cptran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_stran_instr_t *instr,
				      subtilis_error_t *err)
{
	int dist_dest;
	int dist_base;
	size_t vreg_base;
	bool fixed_reg_base;
	size_t vreg_dest = 0;
	subtilis_arm_reg_ud_t *ud = user_data;

	/*
	 * The function calling code can generate LDFNV and STFNV when
	 * preserving fpa registers.  These instructions should be
	 * ignored for the purposes of register allocation.
	 */

	if (instr->ccode == SUBTILIS_ARM_CCODE_NV) {
		ud->instr_count++;
		return;
	}

	if (type == SUBTILIS_VFP_INSTR_FSTS ||
	    type == SUBTILIS_VFP_INSTR_FLDS) {
		subtilis_error_set_assertion_failed(err);
	}

	if (type == SUBTILIS_VFP_INSTR_FSTD) {
		vreg_dest = instr->dest;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	vreg_base = instr->base;
	fixed_reg_base = subtilis_arm_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &instr->base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_base) {
		dist_base = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_base, op, &ud->int_regs->dist_walker,
		    ud->int_regs);
		if (dist_base == -1)
			ud->int_regs->phys_to_virt[instr->base] = INT_MAX;
	}

	if (type == SUBTILIS_VFP_INSTR_FSTD) {
		dist_dest = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_dest, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
		if (dist_dest == -1)
			ud->real_regs->phys_to_virt[instr->dest] = INT_MAX;
		ud->real_regs->next[instr->dest] = dist_dest;
	} else {
		subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (!fixed_reg_base)
		ud->int_regs->next[instr->base] = dist_base;

	ud->instr_count++;
}

static void prv_alloc_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_copy_instr_t *instr,
				     subtilis_error_t *err)
{
	int dist_src;
	size_t vreg_src;

	subtilis_arm_reg_ud_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		subtilis_error_set_assertion_failed(err);
		return;
	default:
		break;
	}

	vreg_src = instr->src;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_src = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_src, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_src == -1)
		ud->real_regs->phys_to_virt[instr->src] = INT_MAX;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Need to check for useless moves */

	if ((type != SUBTILIS_VFP_INSTR_FCPYD) || (instr->src != instr->dest))
		ud->real_regs->next[instr->src] = dist_src;

	ud->instr_count++;
}

static void prv_alloc_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

static void prv_alloc_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	size_t vreg_src;
	int dist_src;
	subtilis_arm_reg_ud_t *ud = user_data;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FUITOS:
		subtilis_error_set_assertion_failed(err);
		break;
	default:
		break;
	}

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	vreg_src = instr->src;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->src, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_src = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_src, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_src == -1)
		ud->real_regs->phys_to_virt[instr->src] = INT_MAX;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->real_regs->next[instr->src] = dist_src;

	ud->instr_count++;
}

static void prv_alloc_vfp_tran_dbl_instr(void *user_data, subtilis_arm_op_t *op,
					 subtilis_arm_instr_type_t type,
					 subtilis_vfp_tran_dbl_instr_t *instr,
					 subtilis_error_t *err)
{
	size_t vreg_src;
	int dist_src;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (type != SUBTILIS_VFP_INSTR_FMRRD) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	vreg_src = instr->src1;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->src1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_src = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_src, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_src == -1)
		ud->real_regs->phys_to_virt[instr->src1] = INT_MAX;

	subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->dest1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->dest2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->real_regs->next[instr->src1] = dist_src;

	ud->instr_count++;
}

static void prv_alloc_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_vfp_cptran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_arm_reg_t vreg_op2;
	int dist_op2;
	subtilis_arm_reg_ud_t *ud = user_data;
	bool fixed_op2 = true;

	if (!instr->use_dregs) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (type != SUBTILIS_VFP_INSTR_FMSR) {
		vreg_op2 = instr->src;
		subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs,
					      ud->real_regs, &instr->src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
		if (dist_op2 == -1)
			ud->real_regs->phys_to_virt[instr->src] = INT_MAX;

		subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ud->real_regs->next[instr->src] = dist_op2;
	} else {
		vreg_op2 = instr->src;
		fixed_op2 = subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->int_regs, &instr->src, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->int_regs->dist_walker, ud->int_regs);
		if (dist_op2 == -1)
			ud->int_regs->phys_to_virt[instr->src] = INT_MAX;

		subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_op2)
			ud->int_regs->next[instr->src] = dist_op2;
	}

	ud->instr_count++;
}

static void prv_alloc_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_data_instr_t *instr,
				     subtilis_error_t *err)
{
	size_t vreg_op1;
	int dist_op1;
	size_t vreg_op2;
	int dist_op2;
	size_t vreg_dest;
	int dist_dest;
	subtilis_arm_reg_ud_t *ud = user_data;

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

	vreg_op1 = instr->op1;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_op2 = instr->op2;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_op2, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_op2 == -1)
		ud->real_regs->phys_to_virt[instr->op2] = INT_MAX;

	dist_op1 = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_op1, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_op1 == -1)
		ud->real_regs->phys_to_virt[instr->op1] = INT_MAX;

	/*
	 * Special case here for FMA.  We need to ensure the destination
	 * register not allocate it as it is read from as well as written to.
	 */

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMACD:
	case SUBTILIS_VFP_INSTR_FNMACD:
	case SUBTILIS_VFP_INSTR_FMSCD:
	case SUBTILIS_VFP_INSTR_FNMSCD:
		vreg_dest = instr->dest;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_dest = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_dest, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
		if (dist_dest == -1)
			ud->real_regs->phys_to_virt[instr->dest] = INT_MAX;
		ud->real_regs->next[instr->dest] = dist_dest;
		break;
	default:
		subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		break;
	}
	ud->real_regs->next[instr->op1] = dist_op1;
	ud->real_regs->next[instr->op2] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	size_t vreg_op1;
	int dist_op1;
	size_t vreg_op2;
	int dist_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

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
		vreg_op2 = instr->op2;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
	}

	vreg_op1 = instr->op1;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_op1 = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_op1, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_op1 == -1)
		ud->real_regs->phys_to_virt[instr->op1] = INT_MAX;

	if ((type == SUBTILIS_VFP_INSTR_FCMPED ||
	     type == SUBTILIS_VFP_INSTR_FCMPD) &&
	    dist_op2 == -1)
		ud->real_regs->phys_to_virt[instr->op2] = INT_MAX;

	ud->real_regs->next[instr->op1] = dist_op1;
	if (type == SUBTILIS_VFP_INSTR_FCMPED ||
	    type == SUBTILIS_VFP_INSTR_FCMPD)
		ud->real_regs->next[instr->op2] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_sqrt_instr_t *instr,
				     subtilis_error_t *err)
{
	size_t vreg_op1;
	int dist_op1;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FSQRTS) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	vreg_op1 = instr->op1;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_op1 = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_op1, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_op1 == -1)
		ud->real_regs->phys_to_virt[instr->op1] = INT_MAX;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->real_regs->next[instr->op1] = dist_op1;

	ud->instr_count++;
}

static void prv_alloc_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_vfp_sysreg_instr_t *instr,
				       subtilis_error_t *err)
{
	size_t vreg_op1;
	int dist_op1;
	bool fixed_op1 = true;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (type == SUBTILIS_VFP_INSTR_FMRX) {
		subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->arm_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		vreg_op1 = instr->arm_reg;
		fixed_op1 = subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->int_regs, &instr->arm_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (!fixed_op1) {
			dist_op1 = subtilis_arm_reg_alloc_calculate_dist(
			    ud, vreg_op1, op, &ud->int_regs->dist_walker,
			    ud->int_regs);
			if (dist_op1 == -1)
				ud->int_regs->phys_to_virt[instr->arm_reg] =
				    INT_MAX;
		}
	}

	ud->instr_count++;
}

static void prv_alloc_vfp_cvt_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_cvt_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_alloc_init_walker(subtlis_arm_walker_t *walker,
				    void *user_data)
{
	walker->fpa_data_monadic_fn = prv_alloc_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_alloc_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_alloc_fpa_stran_instr;
	walker->fpa_tran_fn = prv_alloc_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_alloc_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_alloc_fpa_ldrc_instr;
	walker->fpa_cptran_fn = prv_alloc_fpa_cptran_instr;
	walker->vfp_stran_fn = prv_alloc_vfp_stran_instr;
	walker->vfp_copy_fn = prv_alloc_vfp_copy_instr;
	walker->vfp_ldrc_fn = prv_alloc_vfp_ldrc_instr;
	walker->vfp_tran_fn = prv_alloc_vfp_tran_instr;
	walker->vfp_tran_dbl_fn = prv_alloc_vfp_tran_dbl_instr;
	walker->vfp_cptran_fn = prv_alloc_vfp_cptran_instr;
	walker->vfp_data_fn = prv_alloc_vfp_data_instr;
	walker->vfp_cmp_fn = prv_alloc_vfp_cmp_instr;
	walker->vfp_sqrt_fn = prv_alloc_vfp_sqrt_instr;
	walker->vfp_sysreg_fn = prv_alloc_vfp_sysreg_instr;
	walker->vfp_cvt_fn = prv_alloc_vfp_cvt_instr;
}
