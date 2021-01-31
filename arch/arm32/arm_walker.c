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

static void prv_call_data_fn(subtlis_arm_walker_t *walker, void *user_data,
			     subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->data_fn(walker->user_data, op, instr->type,
			&instr->operands.data, err);
}

static void prv_call_mul_fn(subtlis_arm_walker_t *walker, void *user_data,
			    subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->mul_fn(walker->user_data, op, instr->type, &instr->operands.mul,
		       err);
}

static void prv_call_cmp_fn(subtlis_arm_walker_t *walker, void *user_data,
			    subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->cmp_fn(walker->user_data, op, instr->type,
		       &instr->operands.data, err);
}

static void prv_call_mov_fn(subtlis_arm_walker_t *walker, void *user_data,
			    subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->mov_fn(walker->user_data, op, instr->type,
		       &instr->operands.data, err);
}

static void prv_call_stran_fn(subtlis_arm_walker_t *walker, void *user_data,
			      subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->stran_fn(walker->user_data, op, instr->type,
			 &instr->operands.stran, err);
}

static void prv_call_mtran_fn(subtlis_arm_walker_t *walker, void *user_data,
			      subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->mtran_fn(walker->user_data, op, instr->type,
			 &instr->operands.mtran, err);
}

static void prv_call_br_fn(subtlis_arm_walker_t *walker, void *user_data,
			   subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->br_fn(walker->user_data, op, instr->type, &instr->operands.br,
		      err);
}

static void prv_call_swi_fn(subtlis_arm_walker_t *walker, void *user_data,
			    subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->swi_fn(walker->user_data, op, instr->type, &instr->operands.swi,
		       err);
}

static void prv_call_ldrc_fn(subtlis_arm_walker_t *walker, void *user_data,
			     subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->ldrc_fn(walker->user_data, op, instr->type,
			&instr->operands.ldrc, err);
}

static void prv_call_adr_fn(subtlis_arm_walker_t *walker, void *user_data,
			    subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->adr_fn(walker->user_data, op, instr->type, &instr->operands.adr,
		       err);
}

static void prv_call_flags_fn(subtlis_arm_walker_t *walker, void *user_data,
			      subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->flags_fn(walker->user_data, op, instr->type,
			 &instr->operands.flags, err);
}

static void prv_call_cmov_fn(subtlis_arm_walker_t *walker, void *user_data,
			     subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->cmov_fn(walker->user_data, op, instr->type,
			&instr->operands.cmov, err);
}

static void prv_call_fpa_data_monadic_fn(subtlis_arm_walker_t *walker,
					 void *user_data, subtilis_arm_op_t *op,
					 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_data_monadic_fn(walker->user_data, op, instr->type,
				    &instr->operands.fpa_data, err);
}

static void prv_call_fpa_data_dyadic_fn(subtlis_arm_walker_t *walker,
					void *user_data, subtilis_arm_op_t *op,
					subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_data_dyadic_fn(walker->user_data, op, instr->type,
				   &instr->operands.fpa_data, err);
}

static void prv_call_fpa_stran_fn(subtlis_arm_walker_t *walker, void *user_data,
				  subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_stran_fn(walker->user_data, op, instr->type,
			     &instr->operands.fpa_stran, err);
}

static void prv_call_fpa_tran_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_tran_fn(walker->user_data, op, instr->type,
			    &instr->operands.fpa_tran, err);
}

static void prv_call_fpa_cmp_fn(subtlis_arm_walker_t *walker, void *user_data,
				subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_cmp_fn(walker->user_data, op, instr->type,
			   &instr->operands.fpa_cmp, err);
}

static void prv_call_fpa_ldrc_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_ldrc_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_ldrc, err);
}

static void prv_call_fpa_cptran_fn(subtlis_arm_walker_t *walker,
				   void *user_data, subtilis_arm_op_t *op,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->fpa_cptran_fn(walker->user_data, op, instr->type,
			      &instr->operands.fpa_cptran, err);
}

static void prv_call_vfp_stran_fn(subtlis_arm_walker_t *walker, void *user_data,
				  subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_stran_fn(walker->user_data, op, instr->type,
			     &instr->operands.vfp_stran, err);
}

static void prv_call_vfp_copy_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_copy_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_copy, err);
}

static void prv_call_vfp_ldrc_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_ldrc_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_ldrc, err);
}

static void prv_call_vfp_tran_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_tran_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_tran, err);
}

static void prv_call_vfp_tran_dbl_fn(subtlis_arm_walker_t *walker,
				     void *user_data, subtilis_arm_op_t *op,
				     subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_tran_dbl_fn(walker->user_data, op, instr->type,
				&instr->operands.vfp_tran_dbl, err);
}

static void prv_call_vfp_cptran_fn(subtlis_arm_walker_t *walker,
				   void *user_data, subtilis_arm_op_t *op,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_cptran_fn(walker->user_data, op, instr->type,
			      &instr->operands.vfp_cptran, err);
}

static void prv_call_vfp_data_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_data_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_data, err);
}

static void prv_call_vfp_cmp_fn(subtlis_arm_walker_t *walker, void *user_data,
				subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_cmp_fn(walker->user_data, op, instr->type,
			   &instr->operands.vfp_cmp, err);
}

static void prv_call_vfp_sqrt_fn(subtlis_arm_walker_t *walker, void *user_data,
				 subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_sqrt_fn(walker->user_data, op, instr->type,
			    &instr->operands.vfp_sqrt, err);
}

static void prv_call_vfp_sysreg_fn(subtlis_arm_walker_t *walker,
				   void *user_data, subtilis_arm_op_t *op,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_sysreg_fn(walker->user_data, op, instr->type,
			      &instr->operands.vfp_sysreg, err);
}

static void prv_call_vfp_cvt_fn(subtlis_arm_walker_t *walker, void *user_data,
				subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	walker->vfp_cvt_fn(walker->user_data, op, instr->type,
			   &instr->operands.vfp_cvt, err);
}

static void prv_call_stran_misc_fn(subtlis_arm_walker_t *walker,
				   void *user_data, subtilis_arm_op_t *op,
				   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	if (!walker->stran_misc_fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	walker->stran_misc_fn(walker->user_data, op, instr->type,
			      &instr->operands.stran_misc, err);
}

static void prv_call_simd_fn(subtlis_arm_walker_t *walker, void *user_data,
			     subtilis_arm_op_t *op, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	if (!walker->simd_fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	walker->simd_fn(walker->user_data, op, instr->type,
			&instr->operands.reg_only, err);
}

typedef void (*subtilis_walker_fn_t)(subtlis_arm_walker_t *walker,
				     void *user_data, subtilis_arm_op_t *op,
				     subtilis_error_t *err);

/* clang-format off */

static const subtilis_walker_fn_t walker_table[] = {
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_AND
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_EOR
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_SUB
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_RSB
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_ADD
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_ADC
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_SBC
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_RSC
	prv_call_cmp_fn,		// SUBTILIS_ARM_INSTR_TST
	prv_call_cmp_fn,		// SUBTILIS_ARM_INSTR_TEQ
	prv_call_cmp_fn,		// SUBTILIS_ARM_INSTR_CMP
	prv_call_cmp_fn,		// SUBTILIS_ARM_INSTR_CMN
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_ORR
	prv_call_mov_fn,		// SUBTILIS_ARM_INSTR_MOV
	prv_call_data_fn,		// SUBTILIS_ARM_INSTR_BIC
	prv_call_mov_fn,		// SUBTILIS_ARM_INSTR_MVN
	prv_call_mul_fn,		// SUBTILIS_ARM_INSTR_MUL
	prv_call_mul_fn,		// SUBTILIS_ARM_INSTR_MLA
	prv_call_stran_fn,		// SUBTILIS_ARM_INSTR_LDR
	prv_call_stran_fn,		// SUBTILIS_ARM_INSTR_STR
	prv_call_mtran_fn,		// SUBTILIS_ARM_INSTR_LDM
	prv_call_mtran_fn,		// SUBTILIS_ARM_INSTR_STM
	prv_call_br_fn,		// SUBTILIS_ARM_INSTR_B
	prv_call_swi_fn,		// SUBTILIS_ARM_INSTR_SWI
	prv_call_ldrc_fn,		// SUBTILIS_ARM_INSTR_LDRC
	prv_call_cmov_fn,		// SUBTILIS_ARM_INSTR_CMOV
	prv_call_adr_fn,		// SUBTILIS_ARM_INSTR_ADR
	prv_call_flags_fn,		// SUBTILIS_ARM_INSTR_MSR
	prv_call_flags_fn,		// SUBTILIS_ARM_INSTR_MRS
	prv_call_fpa_stran_fn,		// SUBTILIS_FPA_INSTR_LDF
	prv_call_fpa_stran_fn,		// SUBTILIS_FPA_INSTR_STF
	prv_call_fpa_ldrc_fn,		// SUBTILIS_FPA_INSTR_LDRC
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_MVF
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_MNF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_ADF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_MUF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_SUF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_RSF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_DVF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_RDF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_POW
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_RPW
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_RMF
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_FML
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_FDV
	prv_call_fpa_data_dyadic_fn,	// SUBTILIS_FPA_INSTR_FRD
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_POL
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_ABS
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_RND
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_SQT
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_LOG
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_LGN
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_EXP
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_SIN
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_COS
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_TAN
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_ASN
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_ACS
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_ATN
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_URD
	prv_call_fpa_data_monadic_fn,	// SUBTILIS_FPA_INSTR_NRM
	prv_call_fpa_tran_fn,		// SUBTILIS_FPA_INSTR_FLT
	prv_call_fpa_tran_fn,		// SUBTILIS_FPA_INSTR_FIX
	prv_call_fpa_cmp_fn,		// SUBTILIS_FPA_INSTR_CMF
	prv_call_fpa_cmp_fn,		// SUBTILIS_FPA_INSTR_CNF
	prv_call_fpa_cmp_fn,		// SUBTILIS_FPA_INSTR_CMFE
	prv_call_fpa_cmp_fn,		// SUBTILIS_FPA_INSTR_CNFE
	prv_call_fpa_cptran_fn,	// SUBTILIS_FPA_INSTR_WFS
	prv_call_fpa_cptran_fn,	// SUBTILIS_FPA_INSTR_RFS
	prv_call_vfp_stran_fn,		// SUBTILIS_VFP_INSTR_FSTS
	prv_call_vfp_stran_fn,		// SUBTILIS_VFP_INSTR_FLDS
	prv_call_vfp_stran_fn,		// SUBTILIS_VFP_INSTR_FSTD
	prv_call_vfp_stran_fn,		// SUBTILIS_VFP_INSTR_FLDD
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FCPYS
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FCPYD
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FNEGS
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FNEGD
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FABSS
	prv_call_vfp_copy_fn,		// SUBTILIS_VFP_INSTR_FABSD
	prv_call_vfp_ldrc_fn,		// SUBTILIS_VFP_INSTR_LDRC
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FSITOD
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FSITOS
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOSIS
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOSID
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOUIS
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOUID
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOSIZS
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOSIZD
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOUIZS
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FTOUIZD
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FUITOD
	prv_call_vfp_tran_fn,		// SUBTILIS_VFP_INSTR_FUITOS
	prv_call_vfp_cptran_fn,	// SUBTILIS_VFP_INSTR_FMSR
	prv_call_vfp_cptran_fn,	// SUBTILIS_VFP_INSTR_FMRS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMACS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMACD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMACS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMACD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMSCS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMSCD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMSCS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMSCD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMULS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FMULD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMULS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FNMULD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FADDS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FADDD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FSUBS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FSUBD
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FDIVS
	prv_call_vfp_data_fn,		// SUBTILIS_VFP_INSTR_FDIVD
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPS
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPD
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPES
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPED
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPZS
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPZD
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPEZS
	prv_call_vfp_cmp_fn,		// SUBTILIS_VFP_INSTR_FCMPEZD
	prv_call_vfp_sqrt_fn,		// SUBTILIS_VFP_INSTR_FSQRTD
	prv_call_vfp_sqrt_fn,		// SUBTILIS_VFP_INSTR_FSQRTS
	prv_call_vfp_sysreg_fn,	// SUBTILIS_VFP_INSTR_FMXR
	prv_call_vfp_sysreg_fn,	// SUBTILIS_VFP_INSTR_FMRX
	prv_call_vfp_tran_dbl_fn,	// SUBTILIS_VFP_INSTR_FMDRR
	prv_call_vfp_tran_dbl_fn,	// SUBTILIS_VFP_INSTR_FMRRD
	prv_call_vfp_tran_dbl_fn,	// SUBTILIS_VFP_INSTR_FMSRR
	prv_call_vfp_tran_dbl_fn,	// SUBTILIS_VFP_INSTR_FMRRS
	prv_call_vfp_cvt_fn,		// SUBTILIS_VFP_INSTR_FCVTDS
	prv_call_vfp_cvt_fn,		// SUBTILIS_VFP_INSTR_FCVTSD
	prv_call_stran_misc_fn,	// SUBTILIS_ARM_STRAN_MISC_LDR
	prv_call_stran_misc_fn,	// SUBTILIS_ARM_STRAN_MISC_STR
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QSUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QSUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_QSUBADDX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SSUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SSUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SSUBADDX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHSUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHSUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_SHSUBADDX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_USUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_USUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_USUBADDX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHSUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHSUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UHSUBADDX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQADD16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQADD8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQADDSUBX,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQSUB16,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQSUB8,
	prv_call_simd_fn,		// SUBTILIS_ARM_SIMD_UQSUBADDX,
};

/* clang-format on */

static void prv_walk_instr(subtlis_arm_walker_t *walker, subtilis_arm_op_t *op,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr = &op->op.instr;

	if (((size_t)instr->type) >=
	    sizeof(walker_table) / sizeof(walker_table[0])) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	walker_table[instr->type](walker, walker->user_data, op, err);
}

static void prv_arm_walk(subtilis_arm_section_t *arm_s, size_t ptr,
			 subtilis_arm_op_t *to, subtlis_arm_walker_t *walker,
			 subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	while (ptr != SIZE_MAX) {
		op = &arm_s->op_pool->ops[ptr];

		switch (op->type) {
		case SUBTILIS_ARM_OP_LABEL:
			walker->label_fn(walker->user_data, op, op->op.label,
					 err);
			break;
		case SUBTILIS_ARM_OP_INSTR:
			prv_walk_instr(walker, op, err);
			break;
		case SUBTILIS_ARM_OP_ALIGN:
		case SUBTILIS_ARM_OP_BYTE:
		case SUBTILIS_ARM_OP_TWO_BYTE:
		case SUBTILIS_ARM_OP_FOUR_BYTE:
		case SUBTILIS_ARM_OP_DOUBLE:
		case SUBTILIS_ARM_OP_DOUBLER:
		case SUBTILIS_ARM_OP_FLOAT:
		case SUBTILIS_ARM_OP_STRING:
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

void subtilis_arm_walk(subtilis_arm_section_t *arm_s,
		       subtlis_arm_walker_t *walker, subtilis_error_t *err)
{
	prv_arm_walk(arm_s, arm_s->first_op, NULL, walker, err);
}

void subtilis_arm_walk_from(subtilis_arm_section_t *arm_s,
			    subtlis_arm_walker_t *walker, subtilis_arm_op_t *op,
			    subtilis_error_t *err)
{
	subtilis_arm_walk_from_to(arm_s, walker, op, NULL, err);
}

void subtilis_arm_walk_from_to(subtilis_arm_section_t *arm_s,
			       subtlis_arm_walker_t *walker,
			       subtilis_arm_op_t *from, subtilis_arm_op_t *to,
			       subtilis_error_t *err)
{
	size_t ptr;

	if (from->next == SIZE_MAX)
		ptr = arm_s->last_op;
	else
		ptr = arm_s->op_pool->ops[from->next].prev;
	prv_arm_walk(arm_s, ptr, to, walker, err);
}
