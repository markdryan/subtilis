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

#include "fpa_alloc.h"
#include "arm_reg_alloc.h"

#include <limits.h>

static void prv_alloc_fpa_data_dyadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	int dist_op1;
	int dist_op2 = -1;
	size_t vreg_op1;
	size_t vreg_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

	vreg_op1 = instr->op1;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->real_regs->dist_walker,
		    ud->real_regs);

		if (dist_op2 == -1)
			ud->real_regs->phys_to_virt[instr->op2.reg] = INT_MAX;
	}

	dist_op1 = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_op1, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_op1 == -1)
		ud->real_regs->phys_to_virt[instr->op1] = INT_MAX;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->real_regs->next[instr->op1] = dist_op1;
	if (!instr->immediate)
		ud->real_regs->next[instr->op2.reg] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_fpa_data_monadic_instr(void *user_data,
					     subtilis_arm_op_t *op,
					     subtilis_arm_instr_type_t type,
					     subtilis_fpa_data_instr_t *instr,
					     subtilis_error_t *err)
{
	size_t vreg_op2;
	int dist_op2 = -1;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
		if (dist_op2 == -1)
			ud->real_regs->phys_to_virt[instr->op2.reg] = INT_MAX;
	}

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Need to check for useless moves */

	if (!instr->immediate &&
	    ((type != SUBTILIS_FPA_INSTR_MVF) || instr->op2.reg != instr->dest))
		ud->real_regs->next[instr->op2.reg] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_stran_instr_t *instr,
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

	if (type == SUBTILIS_FPA_INSTR_STF) {
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

	if (type == SUBTILIS_FPA_INSTR_STF) {
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

static void prv_alloc_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	size_t vreg_op2;
	int dist_op2;
	subtilis_arm_reg_ud_t *ud = user_data;
	bool fixed_op2 = true;

	if (type != SUBTILIS_FPA_INSTR_FLT) {
		if (!instr->immediate) {
			vreg_op2 = instr->op2.reg;
			subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs,
						      ud->real_regs,
						      &instr->op2.reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
			    ud, vreg_op2, op, &ud->real_regs->dist_walker,
			    ud->real_regs);
			if (dist_op2 == -1)
				ud->real_regs->phys_to_virt[instr->op2.reg] =
				    INT_MAX;
			ud->real_regs->next[instr->op2.reg] = dist_op2;
		}

		subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		if (!instr->immediate) {
			vreg_op2 = instr->op2.reg;
			fixed_op2 = subtilis_arm_reg_alloc_ensure(
			    ud, op, ud->int_regs, ud->int_regs, &instr->op2.reg,
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
			    ud, vreg_op2, op, &ud->int_regs->dist_walker,
			    ud->int_regs);
			if (dist_op2 == -1)
				ud->int_regs->phys_to_virt[instr->op2.reg] =
				    INT_MAX;
		}

		subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_op2)
			ud->int_regs->next[instr->op2.reg] = dist_op2;
	}

	ud->instr_count++;
}

static void prv_alloc_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	int dist_dest;
	int dist_op2;
	size_t vreg_dest;
	size_t vreg_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

	vreg_dest = instr->dest;
	(void)subtilis_arm_reg_alloc_ensure(ud, op, ud->int_regs, ud->real_regs,
					    &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg;
		(void)subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->real_regs, &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 = subtilis_arm_reg_alloc_calculate_dist(
		    ud, vreg_op2, op, &ud->real_regs->dist_walker,
		    ud->real_regs);
		if (dist_op2 == -1)
			ud->real_regs->phys_to_virt[instr->op2.reg] = INT_MAX;
		ud->real_regs->next[instr->op2.reg] = dist_op2;
	}

	dist_dest = subtilis_arm_reg_alloc_calculate_dist(
	    ud, vreg_dest, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_dest == -1)
		ud->real_regs->phys_to_virt[instr->dest] = INT_MAX;
	ud->real_regs->next[instr->dest] = dist_dest;

	ud->instr_count++;
}

static void prv_alloc_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	subtilis_arm_reg_alloc_alloc_fp_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

static void prv_alloc_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_fpa_cptran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;
	size_t vreg_dest;
	bool fixed_reg_dest;
	int dist_dest;

	if (type == SUBTILIS_FPA_INSTR_RFS) {
		subtilis_arm_reg_alloc_alloc_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		vreg_dest = instr->dest;
		fixed_reg_dest = subtilis_arm_reg_alloc_ensure(
		    ud, op, ud->int_regs, ud->int_regs, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (!fixed_reg_dest) {
			dist_dest = subtilis_arm_reg_alloc_calculate_dist(
			    ud, vreg_dest, op, &ud->int_regs->dist_walker,
			    ud->int_regs);
			if (dist_dest == -1)
				ud->int_regs->phys_to_virt[instr->dest] =
				    INT_MAX;

			ud->int_regs->next[instr->dest] = dist_dest;
		}
	}

	ud->instr_count++;
}

static void prv_alloc_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_stran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_copy_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_tran_dbl_instr(void *user_data, subtilis_arm_op_t *op,
					 subtilis_arm_instr_type_t type,
					 subtilis_vfp_tran_dbl_instr_t *instr,
					 subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_vfp_cptran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_data_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_sqrt_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_vfp_sysreg_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_fpa_alloc_init_walker(subtlis_arm_walker_t *walker,
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
}
