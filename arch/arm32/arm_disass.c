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

#include "arm_disass.h"

static void prv_decode_mul(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_mul_instr_t *mul = &instr->operands.mul;

	if (encoded & (1 << 21)) {
		instr->type = SUBTILIS_ARM_INSTR_MLA;
		mul->rn = (encoded >> 12) & 0x0f;
	} else {
		instr->type = SUBTILIS_ARM_INSTR_MUL;
	}

	mul->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	mul->status = ((encoded >> 20) & 1);

	mul->dest = (encoded >> 16) & 0x0f;

	mul->rs = (encoded >> 8) & 0x0f;
	mul->rm = encoded & 0x0f;
}

static void prv_decode_swi(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_swi_instr_t *swi = &instr->operands.swi;

	instr->type = SUBTILIS_ARM_INSTR_SWI;
	swi->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	swi->code = encoded & 0xffffff;
}

static void prv_decode_branch(subtilis_arm_instr_t *instr, uint32_t encoded,
			      subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br = &instr->operands.br;

	instr->type = SUBTILIS_ARM_INSTR_B;
	br->link = (encoded & (1 << 24));
	br->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	br->target.offset = encoded & 0xffffff;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	if (encoded & 0x800000)
		br->target.offset = encoded | 0xff000000;
}

static void prv_decode_mtran(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_mtran_instr_t *mtran = &instr->operands.mtran;

	/* Status bit not yet supported. */
	if (encoded & (1 << 22)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_ARM_INSTR_LDM;
	else
		instr->type = SUBTILIS_ARM_INSTR_STM;

	mtran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);

	mtran->op0 = (encoded >> 16) & 0x0f;
	mtran->reg_list = encoded & 0xffff;
	mtran->write_back = (encoded & (1 << 21)) != 0;

	if (encoded & (1 << 23)) {
		if (encoded & (1 << 24))
			mtran->type = SUBTILIS_ARM_MTRAN_IB;
		else
			mtran->type = SUBTILIS_ARM_MTRAN_IA;
	} else {
		if (encoded & (1 << 24))
			mtran->type = SUBTILIS_ARM_MTRAN_DB;
		else
			mtran->type = SUBTILIS_ARM_MTRAN_DA;
	}
}

static void prv_decode_op2(uint32_t encoded, subtilis_arm_op2_t *op2)
{
	const subtilis_arm_shift_type_t shtype[] = {
	    SUBTILIS_ARM_SHIFT_LSL, SUBTILIS_ARM_SHIFT_LSR,
	    SUBTILIS_ARM_SHIFT_ASR, SUBTILIS_ARM_SHIFT_ROR,
	};

	if (encoded & (1 << 25)) {
		op2->type = SUBTILIS_ARM_OP2_I32;
		op2->op.integer = encoded & 0xfff;
	} else if (encoded & 0xff0) {
		op2->type = SUBTILIS_ARM_OP2_SHIFTED;
		op2->op.shift.reg = encoded & 0xf;
		op2->op.shift.shift_reg = (encoded & (1 << 4)) != 0;
		if (op2->op.shift.shift_reg) {
			op2->op.shift.shift.reg = (encoded >> 8) & 0xf;
		} else {
			op2->op.shift.shift.integer = (encoded >> 7) & 0x1f;
			if (op2->op.shift.shift.integer == 0)
				op2->op.shift.shift.integer = 32;
		}
		op2->op.shift.type = shtype[((encoded >> 5) & 3)];
	} else {
		op2->type = SUBTILIS_ARM_OP2_REG;
		op2->op.reg = encoded & 0xf;
	}
}

static void prv_decode_stran_op2(uint32_t encoded, subtilis_arm_op2_t *op2)
{
	const subtilis_arm_shift_type_t shtype[] = {
	    SUBTILIS_ARM_SHIFT_LSL, SUBTILIS_ARM_SHIFT_LSR,
	    SUBTILIS_ARM_SHIFT_ASR, SUBTILIS_ARM_SHIFT_ROR,
	};

	if (encoded & (1 << 25)) {
		if (encoded & 0xff0) {
			op2->type = SUBTILIS_ARM_OP2_SHIFTED;
			op2->op.shift.reg = encoded & 0xf;
			op2->op.shift.shift_reg = false;
			op2->op.shift.shift.integer = (encoded >> 7) & 0x1f;
			if (op2->op.shift.shift.integer == 0)
				op2->op.shift.shift.integer = 32;
			op2->op.shift.type = shtype[((encoded >> 5) & 3)];
		} else {
			op2->type = SUBTILIS_ARM_OP2_REG;
			op2->op.reg = encoded & 0xf;
		}
	} else {
		op2->type = SUBTILIS_ARM_OP2_I32;
		op2->op.integer = encoded & 0xfff;
	}
}

static void prv_decode_stran(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_stran_instr_t *stran = &instr->operands.stran;

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_ARM_INSTR_LDR;
	else
		instr->type = SUBTILIS_ARM_INSTR_STR;

	stran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	stran->pre_indexed = (encoded & (1 << 24)) != 0;
	stran->byte = (encoded & (1 << 22)) != 0;
	stran->write_back = (encoded & (1 << 21)) != 0;
	stran->subtract = (encoded & (1 << 23)) == 0;
	stran->dest = (encoded >> 12) & 0x0f;
	stran->base = (encoded >> 16) & 0x0f;
	prv_decode_stran_op2(encoded, &stran->offset);
}

static void prv_decode_datai(subtilis_arm_instr_t *instr, uint32_t encoded,
			     subtilis_error_t *err)
{
	subtilis_arm_data_instr_t *datai = &instr->operands.data;

	instr->type = (subtilis_arm_instr_type_t)((encoded >> 21) & 0xf);
	datai->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	datai->status = ((encoded >> 20) & 1);

	datai->dest = (encoded >> 12) & 0x0f;

	datai->op1 = (encoded >> 16) & 0x0f;
	prv_decode_op2(encoded, &datai->op2);
}

static void prv_decode_mrs(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_flags_instr_t *flags = &instr->operands.flags;

	instr->type = SUBTILIS_ARM_INSTR_MRS;
	flags->fields = 0;
	flags->ccode = encoded >> 28;
	flags->flag_reg = (encoded & (1 << 22)) ? SUBTILIS_ARM_FLAGS_SPSR
						: SUBTILIS_ARM_FLAGS_CPSR;
	flags->op2_reg = true;
	flags->op.reg = (encoded >> 12) & 0xf;
}

static void prv_decode_msr(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	subtilis_arm_flags_instr_t *flags = &instr->operands.flags;

	instr->type = SUBTILIS_ARM_INSTR_MSR;
	flags->fields = (encoded >> 16) & 0xf;
	flags->ccode = encoded >> 28;
	flags->flag_reg = (encoded & (1 << 22)) ? SUBTILIS_ARM_FLAGS_SPSR
						: SUBTILIS_ARM_FLAGS_CPSR;
	if ((encoded & 1 << 25)) {
		flags->op.integer = encoded & 0xfff;
		flags->op2_reg = false;
	} else {
		flags->op2_reg = true;
		flags->op.reg = (encoded >> 12) & 0xf;
	}
}

static void prv_decode_stran_misc(subtilis_arm_instr_t *instr, uint32_t encoded,
				  subtilis_error_t *err)
{
	subtilis_arm_stran_misc_instr_t *stran_misc =
	    &instr->operands.stran_misc;
	uint32_t type = ((encoded & (1 << 20)) >> 18) | ((encoded >> 5) & 3);

	switch (type) {
	case 1:
		instr->type = SUBTILIS_ARM_STRAN_MISC_STR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_H;
		break;
	case 2:
		instr->type = SUBTILIS_ARM_STRAN_MISC_LDR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_D;
		break;
	case 3:
		instr->type = SUBTILIS_ARM_STRAN_MISC_STR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_D;
		break;
	case 5:
		instr->type = SUBTILIS_ARM_STRAN_MISC_LDR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_H;
		break;
	case 6:
		instr->type = SUBTILIS_ARM_STRAN_MISC_LDR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_SB;
		break;
	case 7:
		instr->type = SUBTILIS_ARM_STRAN_MISC_LDR;
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_SH;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (encoded & 22) {
		stran_misc->offset.imm =
		    (encoded & 0xf) | ((encoded >> 8) & 0xf);
		stran_misc->reg_offset = false;
	} else {
		stran_misc->offset.reg = encoded & 0xf;
		stran_misc->reg_offset = true;
	}

	stran_misc->ccode = encoded >> 28;
	stran_misc->write_back = (encoded & (1 << 21)) != 0;
	stran_misc->subtract = (encoded & (1 << 23)) == 0;
	stran_misc->dest = (encoded >> 12) & 0x0f;
	stran_misc->base = (encoded >> 16) & 0x0f;
}

static void prv_decode_fpa_stran(subtilis_arm_instr_t *instr, uint32_t encoded,
				 subtilis_error_t *err)
{
	uint32_t scratch;
	subtilis_fpa_stran_instr_t *stran = &instr->operands.fpa_stran;

	if (encoded & (1 << 20))
		instr->type = SUBTILIS_FPA_INSTR_LDF;
	else
		instr->type = SUBTILIS_FPA_INSTR_STF;

	scratch = ((encoded >> 21) & 2) | ((encoded >> 15) & 1);
	if (scratch == 3) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	stran->size = 4 << scratch;

	stran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	stran->pre_indexed = (encoded & (1 << 24)) != 0;
	stran->write_back = (encoded & (1 << 21)) != 0;
	stran->subtract = (encoded & (1 << 23)) == 0;
	stran->dest = (encoded >> 12) & 0x7;
	stran->base = (encoded >> 16) & 0x0f;
	stran->offset = encoded & 0xff;
}

/* clang-format off */
static subtilis_arm_instr_type_t prv_fpa_data_opcodes[] = {
	SUBTILIS_FPA_INSTR_ADF,
	SUBTILIS_FPA_INSTR_MVF,
	SUBTILIS_FPA_INSTR_MUF,
	SUBTILIS_FPA_INSTR_MNF,
	SUBTILIS_FPA_INSTR_SUF,
	SUBTILIS_FPA_INSTR_ABS,
	SUBTILIS_FPA_INSTR_RSF,
	SUBTILIS_FPA_INSTR_RND,
	SUBTILIS_FPA_INSTR_DVF,
	SUBTILIS_FPA_INSTR_SQT,
	SUBTILIS_FPA_INSTR_RDF,
	SUBTILIS_FPA_INSTR_LOG,
	SUBTILIS_FPA_INSTR_POW,
	SUBTILIS_FPA_INSTR_LGN,
	SUBTILIS_FPA_INSTR_RPW,
	SUBTILIS_FPA_INSTR_EXP,
	SUBTILIS_FPA_INSTR_RMF,
	SUBTILIS_FPA_INSTR_SIN,
	SUBTILIS_FPA_INSTR_FML,
	SUBTILIS_FPA_INSTR_COS,
	SUBTILIS_FPA_INSTR_FDV,
	SUBTILIS_FPA_INSTR_TAN,
	SUBTILIS_FPA_INSTR_FRD,
	SUBTILIS_FPA_INSTR_ASN,
	SUBTILIS_FPA_INSTR_POL,
	SUBTILIS_FPA_INSTR_ACS,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_ATN,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_URD,
	SUBTILIS_ARM_INSTR_MAX,
	SUBTILIS_FPA_INSTR_NRM,
};

/* clang-format on */

static subtilis_fpa_op2_t prv_fpa_encode_op2(uint32_t encoded, bool *immediate)
{
	subtilis_fpa_op2_t op2;

	if (encoded & (1 << 3)) {
		*immediate = true;
		op2.imm = encoded & 0xf;
	} else {
		*immediate = false;
		op2.reg = encoded & 7;
	}

	return op2;
}

static void prv_decode_fpa_data(subtilis_arm_instr_t *instr, uint32_t encoded,
				subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	size_t scratch;
	subtilis_fpa_data_instr_t *data = &instr->operands.fpa_data;

	scratch = ((encoded >> 19) & 0x1E) | ((encoded >> 15) & 1);
	if (scratch >= sizeof(prv_fpa_data_opcodes) / sizeof(type)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	type = prv_fpa_data_opcodes[scratch];
	if (type == SUBTILIS_ARM_INSTR_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	data->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);

	scratch = ((encoded >> 18) & 2) | ((encoded >> 7) & 1);
	if (scratch == 3) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	data->size = 4 << scratch;

	scratch = ((encoded >> 5) & 3);
	data->rounding = (subtilis_fpa_rounding_t)scratch;

	data->dest = (encoded >> 12) & 0x7;

	if (encoded & (1 << 15))
		data->op1 = 6;
	else
		data->op1 = (encoded >> 16) & 0x7;

	data->op2 = prv_fpa_encode_op2(encoded, &data->immediate);
}

static void prv_decode_fpa_cmp(subtilis_arm_instr_t *instr, uint32_t encoded,
			       subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	subtilis_fpa_cmp_instr_t *cmp = &instr->operands.fpa_cmp;

	switch ((encoded >> 21) & 0x7) {
	case 4:
		type = SUBTILIS_FPA_INSTR_CMF;
		break;
	case 5:
		type = SUBTILIS_FPA_INSTR_CNF;
		break;
	case 6:
		type = SUBTILIS_FPA_INSTR_CMFE;
		break;
	case 7:
		type = SUBTILIS_FPA_INSTR_CNFE;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	cmp->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);

	cmp->dest = (encoded >> 16) & 0x7;
	cmp->op2 = prv_fpa_encode_op2(encoded, &cmp->immediate);
}

static void prv_decode_fpa_tran(subtilis_arm_instr_t *instr, uint32_t encoded,
				subtilis_error_t *err)
{
	subtilis_arm_instr_type_t type;
	size_t scratch;
	subtilis_fpa_tran_instr_t *tran = &instr->operands.fpa_tran;

	switch ((encoded >> 20) & 0xf) {
	case 0:
		type = SUBTILIS_FPA_INSTR_FLT;
		break;
	case 1:
		type = SUBTILIS_FPA_INSTR_FIX;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	instr->type = type;

	tran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	scratch = ((encoded >> 5) & 3);
	tran->rounding = (subtilis_fpa_rounding_t)scratch;

	if (type == SUBTILIS_FPA_INSTR_FLT) {
		scratch = ((encoded >> 18) & 2) | ((encoded >> 7) & 1);
		if (scratch == 3) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		tran->size = 4 << scratch;
		tran->dest = (encoded >> 16) & 0x7;
		tran->immediate = false;
		tran->op2.reg = (encoded >> 12) & 0xf;
	} else {
		tran->dest = (encoded >> 12) & 0xf;
		tran->size = 0;
		tran->op2 = prv_fpa_encode_op2(encoded, &tran->immediate);
	}
}

static void prv_decode_fpa_cptran(subtilis_arm_instr_t *instr, uint32_t mask,
				  uint32_t encoded, subtilis_error_t *err)
{
	subtilis_fpa_cptran_instr_t *cptran = &instr->operands.fpa_cptran;

	instr->type =
	    (mask == 2) ? SUBTILIS_FPA_INSTR_WFS : SUBTILIS_FPA_INSTR_RFS;
	cptran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	cptran->dest = (encoded >> 12) & 0xf;
}

static bool prv_fpa_disass(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	uint32_t mask;

	mask = (encoded >> 25) & 7;
	if (mask == 6) {
		prv_decode_fpa_stran(instr, encoded, err);
		return true;
	}

	if (mask == 7) {
		if ((encoded & (1 << 4)) == 0) {
			prv_decode_fpa_data(instr, encoded, err);
			return true;
		}

		if (((encoded >> 12) & 0xf) == 0xf) {
			prv_decode_fpa_cmp(instr, encoded, err);
			return true;
		}

		mask = (encoded >> 20) & 7;
		if (mask <= 1) {
			prv_decode_fpa_tran(instr, encoded, err);
			return true;
		}

		if (mask == 2 || mask == 3) {
			prv_decode_fpa_cptran(instr, mask, encoded, err);
			return true;
		}
	}

	return false;
}

static void prv_vfp_disass_sysreg(subtilis_arm_instr_t *instr, uint32_t encoded,
				  subtilis_error_t *err)
{
	subtilis_vfp_sysreg_instr_t *sysreg = &instr->operands.vfp_sysreg;

	if ((encoded >> 20) & 1)
		instr->type = SUBTILIS_VFP_INSTR_FMRX;
	else
		instr->type = SUBTILIS_VFP_INSTR_FMXR;

	switch ((encoded >> 16) & 0xf) {
	case 0:
		sysreg->sysreg = SUBTILIS_VFP_SYSREG_FPSID;
		break;
	case 1:
		sysreg->sysreg = SUBTILIS_VFP_SYSREG_FPSCR;
		break;
	case 8:
		sysreg->sysreg = SUBTILIS_VFP_SYSREG_FPEXC;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	if ((encoded >> 7) & 1) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	sysreg->arm_reg = (encoded >> 12) & 0xf;
	sysreg->ccode = encoded >> 28;
}

static void prv_vfp_disass_cptrans(subtilis_arm_instr_t *instr,
				   uint32_t encoded, subtilis_error_t *err)
{
	subtilis_vfp_cptran_instr_t *cptran;
	uint32_t mask = (encoded >> 20) & 0xf;

	cptran = &instr->operands.vfp_cptran;

	if (mask == 0) {
		instr->type = SUBTILIS_VFP_INSTR_FMSR;
		cptran->src = (encoded >> 12) & 0xf;
		cptran->dest = (encoded >> 15) & 0x1e;
		cptran->dest |= (encoded >> 7) & 1;
	} else if (mask == 1) {
		instr->type = SUBTILIS_VFP_INSTR_FMRS;
		cptran->dest = (encoded >> 12) & 0xf;
		cptran->src = (encoded >> 15) & 0x1e;
		cptran->src |= (encoded >> 7) & 1;
	}

	cptran->ccode = encoded >> 28;
	cptran->use_dregs = false;
}

static void prv_vfp_disass_copys(subtilis_arm_instr_t *instr, uint32_t encoded,
				 uint32_t opcode, uint32_t n,
				 subtilis_error_t *err)
{
	subtilis_vfp_copy_instr_t *copy = &instr->operands.vfp_copy;

	if (opcode == 0)
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FABSS : SUBTILIS_VFP_INSTR_FCPYS;
	else
		instr->type = SUBTILIS_VFP_INSTR_FNEGS;

	copy->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
	copy->src = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
	copy->ccode = encoded >> 28;
}

static void prv_vfp_disass_cmps(subtilis_arm_instr_t *instr, uint32_t encoded,
				uint32_t opcode, uint32_t n,
				subtilis_error_t *err)
{
	subtilis_vfp_cmp_instr_t *cmp = &instr->operands.vfp_cmp;

	if (opcode == 4)
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FCMPES : SUBTILIS_VFP_INSTR_FCMPS;
	else
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FCMPEZS : SUBTILIS_VFP_INSTR_FCMPZS;

	cmp->op1 = ((encoded >> 11) & 0x1e) | ((encoded >> 5) & 1);
	cmp->op2 = ((encoded & 0xf) << 1) | ((encoded >> 22) & 1);
	cmp->ccode = encoded >> 28;
}

static void prv_vfp_disass_trans(subtilis_arm_instr_t *instr, uint32_t encoded,
				 uint32_t opcode, uint32_t n,
				 subtilis_error_t *err)
{
	subtilis_vfp_tran_instr_t *tran = &instr->operands.vfp_tran;

	if (opcode == 8) {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FSITOS : SUBTILIS_VFP_INSTR_FUITOS;
	} else if (opcode == 12) {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FTOUIZS : SUBTILIS_VFP_INSTR_FTOUIS;
	} else {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FTOSIZS : SUBTILIS_VFP_INSTR_FTOSIS;
	}

	tran->use_dregs = false;
	tran->src = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
	tran->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);

	tran->ccode = encoded >> 28;
}

static void prv_vfp_disass_extensions(subtilis_arm_instr_t *instr,
				      uint32_t encoded, subtilis_error_t *err)
{
	subtilis_vfp_sqrt_instr_t *sqrt;
	uint32_t opcode = (encoded >> 16) & 0xf;
	uint32_t n = (encoded >> 7) & 1;

	if (opcode == 0) {
		prv_vfp_disass_copys(instr, encoded, opcode, n, err);
	} else if (opcode == 1) {
		if (n == 0) {
			prv_vfp_disass_copys(instr, encoded, opcode, n, err);
		} else {
			instr->type = SUBTILIS_VFP_INSTR_FSQRTS;
			sqrt = &instr->operands.vfp_sqrt;
			sqrt->dest =
			    ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
			sqrt->op1 =
			    ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
			sqrt->ccode = encoded >> 28;
		}
	} else if (opcode == 4 || opcode == 5) {
		prv_vfp_disass_cmps(instr, encoded, opcode, n, err);
	} else if (opcode == 8 || opcode == 12 || opcode == 13) {
		prv_vfp_disass_trans(instr, encoded, opcode, n, err);
	}
}

static void prv_vfp_disass_copyd(subtilis_arm_instr_t *instr, uint32_t encoded,
				 uint32_t opcode, uint32_t n,
				 subtilis_error_t *err)
{
	subtilis_vfp_copy_instr_t *copy = &instr->operands.vfp_copy;

	if (opcode == 0)
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FABSD : SUBTILIS_VFP_INSTR_FCPYD;
	else
		instr->type = SUBTILIS_VFP_INSTR_FNEGD;

	copy->dest = (encoded >> 12) & 0xf;
	copy->src = encoded & 0xf;
	copy->ccode = encoded >> 28;
}

static void prv_vfp_disass_cmpd(subtilis_arm_instr_t *instr, uint32_t encoded,
				uint32_t opcode, uint32_t n,
				subtilis_error_t *err)
{
	subtilis_vfp_cmp_instr_t *cmp = &instr->operands.vfp_cmp;

	if (opcode == 4)
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FCMPED : SUBTILIS_VFP_INSTR_FCMPD;
	else
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FCMPEZD : SUBTILIS_VFP_INSTR_FCMPZD;

	cmp->op1 = (encoded >> 12) & 0xf;
	cmp->op2 = encoded & 0xf;
	cmp->ccode = encoded >> 28;
}

static void prv_vfp_disass_trand(subtilis_arm_instr_t *instr, uint32_t encoded,
				 uint32_t opcode, uint32_t n,
				 subtilis_error_t *err)
{
	bool float32_src;
	subtilis_vfp_tran_instr_t *tran = &instr->operands.vfp_tran;

	if (opcode == 8) {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FSITOD : SUBTILIS_VFP_INSTR_FUITOD;
		float32_src = true;
	} else if (opcode == 12) {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FTOUIZD : SUBTILIS_VFP_INSTR_FTOUID;
		float32_src = false;
	} else {
		instr->type =
		    n ? SUBTILIS_VFP_INSTR_FTOSIZD : SUBTILIS_VFP_INSTR_FTOSID;
		float32_src = false;
	}

	tran->use_dregs = false;
	if (float32_src) {
		tran->dest = (encoded >> 12) & 0xf;
		tran->src = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
	} else {
		tran->src = encoded & 0xf;
		tran->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
	}

	tran->ccode = encoded >> 28;
}

static void prv_vfp_disass_extensiond(subtilis_arm_instr_t *instr,
				      uint32_t encoded, subtilis_error_t *err)
{
	subtilis_vfp_sqrt_instr_t *sqrt;
	uint32_t opcode = (encoded >> 16) & 0xf;
	uint32_t n = (encoded >> 7) & 1;

	if (opcode == 0) {
		prv_vfp_disass_copyd(instr, encoded, opcode, n, err);
	} else if (opcode == 1) {
		if (n == 0) {
			prv_vfp_disass_copyd(instr, encoded, opcode, n, err);
		} else {
			instr->type = SUBTILIS_VFP_INSTR_FSQRTD;
			sqrt = &instr->operands.vfp_sqrt;
			sqrt->dest = (encoded >> 12) & 0xf;
			sqrt->op1 = encoded & 0xf;
			sqrt->ccode = encoded >> 28;
		}
	} else if (opcode == 4 || opcode == 5) {
		prv_vfp_disass_cmpd(instr, encoded, opcode, n, err);
	} else if (opcode == 8 || opcode == 12 || opcode == 13) {
		prv_vfp_disass_trand(instr, encoded, opcode, n, err);
	}
}

static void prv_vfp_disass_tran_dbl(subtilis_arm_instr_t *instr,
				    uint32_t encoded, subtilis_error_t *err)
{
	subtilis_vfp_tran_dbl_instr_t *tran_dbl = &instr->operands.vfp_tran_dbl;

	switch (encoded & 0xf400af0) {
	case 0xc400b10:
		instr->type = SUBTILIS_VFP_INSTR_FMDRR;
		tran_dbl->dest1 = encoded & 0xf;
		tran_dbl->dest2 = 0;
		tran_dbl->src1 = (encoded >> 12) & 0xf;
		tran_dbl->src2 = (encoded >> 16) & 0xf;
		break;
	case 0xc500b10:
		instr->type = SUBTILIS_VFP_INSTR_FMRRD;
		tran_dbl->dest1 = (encoded >> 12) & 0xf;
		tran_dbl->dest2 = (encoded >> 16) & 0xf;
		tran_dbl->src1 = encoded & 0xf;
		tran_dbl->src2 = 0;
		break;
	case 0xc400a10:
		instr->type = SUBTILIS_VFP_INSTR_FMRRS;
		tran_dbl->dest1 = (encoded >> 12) & 0xf;
		tran_dbl->dest2 = (encoded >> 16) & 0xf;
		tran_dbl->src1 = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
		tran_dbl->src2 = tran_dbl->src1 + 1;
		break;
	case 0xc500a10:
		instr->type = SUBTILIS_VFP_INSTR_FMSRR;
		tran_dbl->dest1 = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
		tran_dbl->dest2 = tran_dbl->dest1 + 1;
		tran_dbl->src1 = (encoded >> 12) & 0xf;
		tran_dbl->src2 = (encoded >> 16) & 0xf;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	tran_dbl->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
}

static void prv_vfp_disass_stran(subtilis_arm_instr_t *instr, uint32_t encoded,
				 subtilis_error_t *err)
{
	subtilis_vfp_stran_instr_t *stran = &instr->operands.vfp_stran;
	bool dbl = ((encoded >> 8) & 0xf) == 0xb;

	if (dbl) {
		if ((encoded >> 22) & 1) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (encoded & (1 << 20))
			instr->type = SUBTILIS_VFP_INSTR_FLDD;
		else
			instr->type = SUBTILIS_VFP_INSTR_FSTD;
		stran->dest = (encoded >> 12) & 0xf;
	} else {
		if (encoded & (1 << 20))
			instr->type = SUBTILIS_VFP_INSTR_FLDS;
		else
			instr->type = SUBTILIS_VFP_INSTR_FSTS;
		stran->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
	}
	stran->ccode = (subtilis_arm_ccode_type_t)(encoded >> 28);
	stran->pre_indexed = (encoded & (1 << 24)) != 0;
	stran->write_back = (encoded & (1 << 21)) != 0;
	stran->subtract = (encoded & (1 << 23)) == 0;

	stran->base = (encoded >> 16) & 0x0f;
	stran->offset = encoded & 0xff;
}

static void prv_vfp_disass_data(subtilis_arm_instr_t *instr, uint32_t encoded,
				subtilis_error_t *err)
{
	subtilis_vfp_data_instr_t *data;
	uint32_t opcode = ((encoded >> 6) & 1) | ((encoded >> 19) & 6) |
			  ((encoded >> 20) & 8);
	bool dbl = ((encoded >> 8) & 0xf) == 0xb;

	data = &instr->operands.vfp_data;
	if (dbl) {
		switch (opcode) {
		case 0:
			instr->type = SUBTILIS_VFP_INSTR_FMACD;
			break;
		case 1:
			instr->type = SUBTILIS_VFP_INSTR_FNMACD;
			break;
		case 2:
			instr->type = SUBTILIS_VFP_INSTR_FMSCD;
			break;
		case 3:
			instr->type = SUBTILIS_VFP_INSTR_FNMSCD;
			break;
		case 4:
			instr->type = SUBTILIS_VFP_INSTR_FMULD;
			break;
		case 5:
			instr->type = SUBTILIS_VFP_INSTR_FNMULD;
			break;
		case 6:
			instr->type = SUBTILIS_VFP_INSTR_FADDD;
			break;
		case 7:
			instr->type = SUBTILIS_VFP_INSTR_FSUBD;
			break;
		case 8:
			instr->type = SUBTILIS_VFP_INSTR_FDIVD;
			break;
		case 15:
			prv_vfp_disass_extensiond(instr, encoded, err);
			return;
		default:
			subtilis_error_set_assertion_failed(err);
			return;
		}
		data->dest = (encoded >> 12) & 0xf;
		data->op1 = (encoded >> 16) & 0xf;
		data->op2 = encoded & 0xf;
	} else {
		switch (opcode) {
		case 0:
			instr->type = SUBTILIS_VFP_INSTR_FMACS;
			break;
		case 1:
			instr->type = SUBTILIS_VFP_INSTR_FNMACS;
			break;
		case 2:
			instr->type = SUBTILIS_VFP_INSTR_FMSCS;
			break;
		case 3:
			instr->type = SUBTILIS_VFP_INSTR_FNMSCS;
			break;
		case 4:
			instr->type = SUBTILIS_VFP_INSTR_FMULS;
			break;
		case 5:
			instr->type = SUBTILIS_VFP_INSTR_FNMULS;
			break;
		case 6:
			instr->type = SUBTILIS_VFP_INSTR_FADDS;
			break;
		case 7:
			instr->type = SUBTILIS_VFP_INSTR_FSUBS;
			break;
		case 8:
			instr->type = SUBTILIS_VFP_INSTR_FDIVS;
			break;
		case 15:
			prv_vfp_disass_extensions(instr, encoded, err);
			return;
		default:
			subtilis_error_set_assertion_failed(err);
			return;
		}
		data->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
		data->op1 = ((encoded >> 15) & 0x1e) | ((encoded >> 7) & 1);
		data->op2 = ((encoded & 0xf) << 1) | ((encoded >> 5) & 1);
	}

	data->ccode = encoded >> 28;
}

static void prv_vfp_disass_cvt(subtilis_arm_instr_t *instr, uint32_t encoded,
			       subtilis_error_t *err)
{
	subtilis_vfp_cvt_instr_t *cvt = &instr->operands.vfp_cvt;

	if (encoded & 0x100) {
		instr->type = SUBTILIS_VFP_INSTR_FCVTSD;
		cvt->op1 = encoded & 0xf;
		cvt->dest = ((encoded >> 11) & 0x1e) | ((encoded >> 22) & 1);
	} else {
		instr->type = SUBTILIS_VFP_INSTR_FCVTDS;
		cvt->op1 = (encoded & 0xf) << 1 | ((encoded >> 5) & 1);
		cvt->dest = (encoded >> 12) & 0xf;
	}

	cvt->ccode = encoded >> 28;
}

static bool prv_vfp_disass(subtilis_arm_instr_t *instr, uint32_t encoded,
			   subtilis_error_t *err)
{
	if ((encoded & 0xeb70ac0) == 0xeb70ac0) {
		prv_vfp_disass_cvt(instr, encoded, err);
		return true;
	}

	if ((encoded & 0xf000a10) == 0xe000a10) {
		if (((encoded >> 21) & 0x7) == 0x7)
			prv_vfp_disass_sysreg(instr, encoded, err);
		else
			prv_vfp_disass_cptrans(instr, encoded, err);
		return true;
	}

	if ((encoded & 0xf400af0) == 0xc400a10) {
		prv_vfp_disass_tran_dbl(instr, encoded, err);
		return true;
	}

	if (((encoded & 0x0e000000) == 0x0c000000) &&
	    (((encoded & 0xf00) == 0xa00) || ((encoded & 0xf00) == 0xb00))) {
		prv_vfp_disass_stran(instr, encoded, err);
		return true;
	}

	if (((encoded & 0xe000a00) == 0xe000a00) ||
	    (encoded & 0xe000b00) == 0xe000b00) {
		prv_vfp_disass_data(instr, encoded, err);
		return true;
	}

	return false;
}

void subtilis_arm_disass(subtilis_arm_instr_t *instr, uint32_t encoded,
			 bool vfp, subtilis_error_t *err)
{
	uint32_t mask;

	mask = encoded & (0x3f << 22);
	if ((mask == 0) && (((encoded >> 4) & 0xf) == 9)) {
		prv_decode_mul(instr, encoded, err);
		return;
	}

	mask = encoded & (0xf << 24);
	if (mask == 0x0f000000) {
		prv_decode_swi(instr, encoded, err);
		return;
	}

	mask = (encoded & (0x7 << 25)) >> 25;
	if (mask == 5) {
		prv_decode_branch(instr, encoded, err);
		return;
	}

	if (mask == 4) {
		prv_decode_mtran(instr, encoded, err);
		return;
	}

	mask = (encoded & (0x3 << 26)) >> 26;
	if (mask == 1) {
		prv_decode_stran(instr, encoded, err);
		return;
	}

	if ((encoded & 0x0fbf0fff) == (1 << 24)) {
		prv_decode_mrs(instr, encoded, err);
		return;
	}

	if ((encoded & 0x0db0f000) == 0x01200000) {
		prv_decode_msr(instr, encoded, err);
		return;
	}

	if ((((encoded & 0x0e400090) == 0x400090) ||
	     (encoded & 0x0e400f90) == 0x90) &&
	    ((encoded & 0x60) != 0)) {
		prv_decode_stran_misc(instr, encoded, err);
		return;
	}

	if (mask == 0) {
		prv_decode_datai(instr, encoded, err);
		return;
	}

	if (!vfp) {
		if (prv_fpa_disass(instr, encoded, err))
			return;
	} else {
		if (prv_vfp_disass(instr, encoded, err))
			return;
	}

	subtilis_error_set_bad_instruction(err, encoded);
}

void subtilis_arm_disass_dump(uint8_t *code, size_t len, bool vfp)
{
	size_t i;
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	size_t words = len / 4;

	for (i = 0; i < words; i++) {
		subtilis_error_init(&err);
		printf("%lx\t", 0x8000 + (i * 4));
		subtilis_arm_disass(&instr, ((uint32_t *)code)[i], vfp, &err);
		if (err.type == SUBTILIS_ERROR_OK)
			subtilis_arm_instr_dump(&instr);
		else
			printf("DCW &%x\n", ((uint32_t *)code)[i]);
	}

	for (i = words * 4; i < len; i++)
		printf("DCB &%x\n", code[i]);
}
