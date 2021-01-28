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
	case SUBTILIS_ARM_INSTR_ORR:
	case SUBTILIS_ARM_INSTR_BIC:
		walker->data_fn(walker->user_data, op, instr->type,
				&instr->operands.data, err);
		break;
	case SUBTILIS_ARM_INSTR_MUL:
	case SUBTILIS_ARM_INSTR_MLA:
		walker->mul_fn(walker->user_data, op, instr->type,
			       &instr->operands.mul, err);
		break;
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
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
	case SUBTILIS_ARM_INSTR_ADR:
		walker->adr_fn(walker->user_data, op, instr->type,
			       &instr->operands.adr, err);
		break;
	case SUBTILIS_ARM_INSTR_MSR:
	case SUBTILIS_ARM_INSTR_MRS:
		walker->flags_fn(walker->user_data, op, instr->type,
				 &instr->operands.flags, err);
		break;
	case SUBTILIS_ARM_INSTR_CMOV:
		walker->cmov_fn(walker->user_data, op, instr->type,
				&instr->operands.cmov, err);
		break;
	case SUBTILIS_FPA_INSTR_MVF:
	case SUBTILIS_FPA_INSTR_MNF:
		walker->fpa_data_monadic_fn(walker->user_data, op, instr->type,
					    &instr->operands.fpa_data, err);
		break;
	case SUBTILIS_FPA_INSTR_LDF:
	case SUBTILIS_FPA_INSTR_STF:
		walker->fpa_stran_fn(walker->user_data, op, instr->type,
				     &instr->operands.fpa_stran, err);
		break;
	case SUBTILIS_FPA_INSTR_LDRC:
		walker->fpa_ldrc_fn(walker->user_data, op, instr->type,
				    &instr->operands.fpa_ldrc, err);
		break;
	case SUBTILIS_FPA_INSTR_ADF:
	case SUBTILIS_FPA_INSTR_MUF:
	case SUBTILIS_FPA_INSTR_SUF:
	case SUBTILIS_FPA_INSTR_RSF:
	case SUBTILIS_FPA_INSTR_DVF:
	case SUBTILIS_FPA_INSTR_RDF:
	case SUBTILIS_FPA_INSTR_RMF:
	case SUBTILIS_FPA_INSTR_FML:
	case SUBTILIS_FPA_INSTR_FDV:
	case SUBTILIS_FPA_INSTR_FRD:
	case SUBTILIS_FPA_INSTR_RPW:
	case SUBTILIS_FPA_INSTR_POW:
		walker->fpa_data_dyadic_fn(walker->user_data, op, instr->type,
					   &instr->operands.fpa_data, err);
		break;
	case SUBTILIS_FPA_INSTR_POL:
	case SUBTILIS_FPA_INSTR_ABS:
	case SUBTILIS_FPA_INSTR_RND:
	case SUBTILIS_FPA_INSTR_LOG:
	case SUBTILIS_FPA_INSTR_LGN:
	case SUBTILIS_FPA_INSTR_EXP:
	case SUBTILIS_FPA_INSTR_SIN:
	case SUBTILIS_FPA_INSTR_COS:
	case SUBTILIS_FPA_INSTR_TAN:
	case SUBTILIS_FPA_INSTR_ASN:
	case SUBTILIS_FPA_INSTR_ACS:
	case SUBTILIS_FPA_INSTR_ATN:
	case SUBTILIS_FPA_INSTR_SQT:
	case SUBTILIS_FPA_INSTR_URD:
	case SUBTILIS_FPA_INSTR_NRM:
		walker->fpa_data_monadic_fn(walker->user_data, op, instr->type,
					    &instr->operands.fpa_data, err);
		break;
	case SUBTILIS_FPA_INSTR_FLT:
	case SUBTILIS_FPA_INSTR_FIX:
		walker->fpa_tran_fn(walker->user_data, op, instr->type,
				    &instr->operands.fpa_tran, err);
		break;
	case SUBTILIS_FPA_INSTR_CMF:
	case SUBTILIS_FPA_INSTR_CNF:
	case SUBTILIS_FPA_INSTR_CMFE:
	case SUBTILIS_FPA_INSTR_CNFE:
		walker->fpa_cmp_fn(walker->user_data, op, instr->type,
				   &instr->operands.fpa_cmp, err);
		break;
	case SUBTILIS_FPA_INSTR_WFS:
	case SUBTILIS_FPA_INSTR_RFS:
		walker->fpa_cptran_fn(walker->user_data, op, instr->type,
				      &instr->operands.fpa_cptran, err);
		break;
	case SUBTILIS_VFP_INSTR_FSTS:
	case SUBTILIS_VFP_INSTR_FLDS:
	case SUBTILIS_VFP_INSTR_FSTD:
	case SUBTILIS_VFP_INSTR_FLDD:
		walker->vfp_stran_fn(walker->user_data, op, instr->type,
				     &instr->operands.vfp_stran, err);
		break;
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FCPYD:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FNEGD:
	case SUBTILIS_VFP_INSTR_FABSS:
	case SUBTILIS_VFP_INSTR_FABSD:
		walker->vfp_copy_fn(walker->user_data, op, instr->type,
				    &instr->operands.vfp_copy, err);
		break;
	case SUBTILIS_VFP_INSTR_LDRC:
		walker->vfp_ldrc_fn(walker->user_data, op, instr->type,
				    &instr->operands.vfp_ldrc, err);
		break;
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FSITOD:
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOSID:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUID:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZD:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOUIZD:
	case SUBTILIS_VFP_INSTR_FUITOD:
	case SUBTILIS_VFP_INSTR_FUITOS:
		walker->vfp_tran_fn(walker->user_data, op, instr->type,
				    &instr->operands.vfp_tran, err);
		break;
	case SUBTILIS_VFP_INSTR_FMSR:
	case SUBTILIS_VFP_INSTR_FMRS:
		walker->vfp_cptran_fn(walker->user_data, op, instr->type,
				      &instr->operands.vfp_cptran, err);
		break;
	case SUBTILIS_VFP_INSTR_FMACS:
	case SUBTILIS_VFP_INSTR_FMACD:
	case SUBTILIS_VFP_INSTR_FNMACS:
	case SUBTILIS_VFP_INSTR_FNMACD:
	case SUBTILIS_VFP_INSTR_FMSCS:
	case SUBTILIS_VFP_INSTR_FMSCD:
	case SUBTILIS_VFP_INSTR_FNMSCS:
	case SUBTILIS_VFP_INSTR_FNMSCD:
	case SUBTILIS_VFP_INSTR_FMULS:
	case SUBTILIS_VFP_INSTR_FMULD:
	case SUBTILIS_VFP_INSTR_FNMULS:
	case SUBTILIS_VFP_INSTR_FNMULD:
	case SUBTILIS_VFP_INSTR_FADDS:
	case SUBTILIS_VFP_INSTR_FADDD:
	case SUBTILIS_VFP_INSTR_FSUBS:
	case SUBTILIS_VFP_INSTR_FSUBD:
	case SUBTILIS_VFP_INSTR_FDIVS:
	case SUBTILIS_VFP_INSTR_FDIVD:
		walker->vfp_data_fn(walker->user_data, op, instr->type,
				    &instr->operands.vfp_data, err);
		break;
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPD:
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPED:
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPZD:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
	case SUBTILIS_VFP_INSTR_FCMPEZD:
		walker->vfp_cmp_fn(walker->user_data, op, instr->type,
				   &instr->operands.vfp_cmp, err);
		break;
	case SUBTILIS_VFP_INSTR_FSQRTD:
	case SUBTILIS_VFP_INSTR_FSQRTS:
		walker->vfp_sqrt_fn(walker->user_data, op, instr->type,
				    &instr->operands.vfp_sqrt, err);
		break;
	case SUBTILIS_VFP_INSTR_FMXR:
	case SUBTILIS_VFP_INSTR_FMRX:
		walker->vfp_sysreg_fn(walker->user_data, op, instr->type,
				      &instr->operands.vfp_sysreg, err);
		break;
	case SUBTILIS_VFP_INSTR_FMDRR:
	case SUBTILIS_VFP_INSTR_FMRRD:
	case SUBTILIS_VFP_INSTR_FMSRR:
	case SUBTILIS_VFP_INSTR_FMRRS:
		walker->vfp_tran_dbl_fn(walker->user_data, op, instr->type,
					&instr->operands.vfp_tran_dbl, err);
		break;
	case SUBTILIS_VFP_INSTR_FCVTDS:
	case SUBTILIS_VFP_INSTR_FCVTSD:
		walker->vfp_cvt_fn(walker->user_data, op, instr->type,
				   &instr->operands.vfp_cvt, err);
		break;
	case SUBTILIS_ARM_STRAN_MISC_LDR:
	case SUBTILIS_ARM_STRAN_MISC_STR:
		if (!walker->stran_misc_fn) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		walker->stran_misc_fn(walker->user_data, op, instr->type,
				      &instr->operands.stran_misc, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}
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
