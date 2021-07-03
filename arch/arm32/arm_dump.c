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

#include <inttypes.h>

#include "arm_core.h"
#include "arm_walker.h"

/* clang-format off */

static const char *const ccode_desc[] = {
	"EQ", // SUBTILIS_ARM_CCODE_EQ
	"NE", // SUBTILIS_ARM_CCODE_NE
	"CS", // SUBTILIS_ARM_CCODE_CS
	"CC", // SUBTILIS_ARM_CCODE_CC
	"MI", // SUBTILIS_ARM_CCODE_MI
	"PL", // SUBTILIS_ARM_CCODE_PL
	"VS", // SUBTILIS_ARM_CCODE_VS
	"VC", // SUBTILIS_ARM_CCODE_VC
	"HI", // SUBTILIS_ARM_CCODE_HI
	"LS", // SUBTILIS_ARM_CCODE_LS
	"GE", // SUBTILIS_ARM_CCODE_GE
	"LT", // SUBTILIS_ARM_CCODE_LT
	"GT", // SUBTILIS_ARM_CCODE_GT
	"LE", // SUBTILIS_ARM_CCODE_LE
	"AL", // SUBTILIS_ARM_CCODE_AL
	"NV", // SUBTILIS_ARM_CCODE_NV
};

static const char *const instr_desc[] = {
	"AND",      // SUBTILIS_ARM_INSTR_AND
	"EOR",      // SUBTILIS_ARM_INSTR_EOR
	"SUB",      // SUBTILIS_ARM_INSTR_SUB
	"RSB",      // SUBTILIS_ARM_INSTR_RSB
	"ADD",      // SUBTILIS_ARM_INSTR_ADD
	"ADC",      // SUBTILIS_ARM_INSTR_ADC
	"SBC",      // SUBTILIS_ARM_INSTR_SBC
	"RSC",      // SUBTILIS_ARM_INSTR_RSC
	"TST",      // SUBTILIS_ARM_INSTR_TST
	"TEQ",      // SUBTILIS_ARM_INSTR_TEQ
	"CMP",      // SUBTILIS_ARM_INSTR_CMP
	"CMN",      // SUBTILIS_ARM_INSTR_CMN
	"ORR",      // SUBTILIS_ARM_INSTR_ORR
	"MOV",      // SUBTILIS_ARM_INSTR_MOV
	"BIC",      // SUBTILIS_ARM_INSTR_BIC
	"MVN",      // SUBTILIS_ARM_INSTR_MVN
	"MUL",      // SUBTILIS_ARM_INSTR_MUL
	"MLA",      // SUBTILIS_ARM_INSTR_MLA
	"LDR",      // SUBTILIS_ARM_INSTR_LDR
	"STR",      // SUBTILIS_ARM_INSTR_STR
	"LDM",      // SUBTILIS_ARM_INSTR_LDM
	"STM",      // SUBTILIS_ARM_INSTR_STM
	"B",        // SUBTILIS_ARM_INSTR_B
	"SWI",      // SUBTILIS_ARM_INSTR_SWI
	"LDR",      // SUBTILIS_ARM_INSTR_LDRC
	"LDRP",     // SUBTILIS_ARM_INSTR_LDRP
	"CMOV",     // SUBTILIS_ARM_INSTR_CMOV
	"ADR",      // SUBTILIS_ARM_INSTR_ADR
	"MSR",      // SUBTILIS_ARM_INSTR_MSR
	"MRS",      // SUBTILIS_ARM_INSTR_MRS
	"LDF",      // SUBTILIS_FPA_INSTR_LDF
	"STF",      // SUBTILIS_FPA_INSTR_STF
	"LDR",      // SUBTILIS_FPA_INSTR_LDRC
	"MVF",      // SUBTILIS_FPA_INSTR_MVF
	"MNF",      // SUBTILIS_FPA_INSTR_MVF
	"ADF",      // SUBTILIS_FPA_INSTR_ADF
	"MUF",      // SUBTILIS_FPA_INSTR_MUF
	"SUF",      // SUBTILIS_FPA_INSTR_SUF
	"RSF",      // SUBTILIS_FPA_INSTR_RSF
	"DVF",      // SUBTILIS_FPA_INSTR_DVF
	"RDF",      // SUBTILIS_FPA_INSTR_RDF
	"POW",      // SUBTILIS_FPA_INSTR_POW
	"RPW",      // SUBTILIS_FPA_INSTR_RPW
	"RMF",      // SUBTILIS_FPA_INSTR_RMF
	"FML",      // SUBTILIS_FPA_INSTR_FML
	"FDV",      // SUBTILIS_FPA_INSTR_FDV
	"FRD",      // SUBTILIS_FPA_INSTR_FRD
	"POL",      // SUBTILIS_FPA_INSTR_POL
	"ABS",      // SUBTILIS_FPA_INSTR_ABS
	"RND",      // SUBTILIS_FPA_INSTR_RND
	"SQT",      // SUBTILIS_FPA_INSTR_SQT
	"LOG",      // SUBTILIS_FPA_INSTR_LOG
	"LGN",      // SUBTILIS_FPA_INSTR_LGN
	"EXP",      // SUBTILIS_FPA_INSTR_EXP
	"SIN",      // SUBTILIS_FPA_INSTR_SIN
	"COS",      // SUBTILIS_FPA_INSTR_COS
	"TAN",      // SUBTILIS_FPA_INSTR_TAN
	"ASN",      // SUBTILIS_FPA_INSTR_ASN
	"ACS",      // SUBTILIS_FPA_INSTR_ACS
	"ATN",      // SUBTILIS_FPA_INSTR_ATN
	"URD",      // SUBTILIS_FPA_INSTR_URD
	"NRM",      // SUBTILIS_FPA_INSTR_NRM
	"FLT",      // SUBTILIS_FPA_INSTR_FLT
	"FIX",      // SUBTILIS_FPA_INSTR_FIX
	"CMF",      // SUBTILIS_FPA_INSTR_CMF
	"CNF",      // SUBTILIS_FPA_INSTR_CNF
	"CMFE",     // SUBTILIS_FPA_INSTR_CMFE
	"CNFE",     // SUBTILIS_FPA_INSTR_CNFE
	"WFS",      // SUBTILIS_FPA_INSTR_WFS
	"RFS",      // SUBTILIS_FPA_INSTR_RFS
	"FSTS",     // SUBTILIS_VFP_INSTR_FSTS
	"FLDS",     // SUBTILIS_VFP_INSTR_FLDS
	"FSTD",     // SUBTILIS_VFP_INSTR_FSTD
	"FLDD",     // SUBTILIS_VFP_INSTR_FLDD
	"FCPYS",    // SUBTILIS_VFP_INSTR_FCPYS
	"FCPYD",    // SUBTILIS_VFP_INSTR_FCPYD
	"FNGES",    // SUBTILIS_VFP_INSTR_FNEGS
	"FNEGD",    // SUBTILIS_VFP_INSTR_FNEGD
	"FABSS",    // SUBTILIS_VFP_INSTR_FABSS
	"FABSD",    // SUBTILIS_VFP_INSTR_FABSD
	"LDRD",     // SUBTILIS_VFP_INSTR_LDRC
	"FSITOD",   // SUBTILIS_VFP_INSTR_FSITOD
	"FSITOS",   // SUBTILIS_VFP_INSTR_FSITOS
	"FTOSIS",   // SUBTILIS_VFP_INSTR_FTOSIS
	"FTOSID",   // SUBTILIS_VFP_INSTR_FTOSID
	"FTOUIS",   // SUBTILIS_VFP_INSTR_FTOUIS
	"FTOUID",   // SUBTILIS_VFP_INSTR_FTOUID
	"FTOSIZS",  // SUBTILIS_VFP_INSTR_FTOSIZS
	"FTOSIZD",  // SUBTILIS_VFP_INSTR_FTOSIZD
	"FTOUIZS",  // SUBTILIS_VFP_INSTR_FTOUIZS
	"FTOUIZD",  // SUBTILIS_VFP_INSTR_FTOUIZD
	"FUITOD",   // SUBTILIS_VFP_INSTR_FUITOD
	"FUITOS",   // SUBTILIS_VFP_INSTR_FUITOS
	"FMSR",     // SUBTILIS_VFP_INSTR_FMSR
	"FMRS",     // SUBTILIS_VFP_INSTR_FMRS
	"FMACS",    // SUBTILIS_VFP_INSTR_FMACS
	"FMACD",    // SUBTILIS_VFP_INSTR_FMACD
	"FNMACS",   // SUBTILIS_VFP_INSTR_FNMACS
	"FNMACD",   // SUBTILIS_VFP_INSTR_FNMACD
	"FMSCS",    // SUBTILIS_VFP_INSTR_FMSCS
	"FMSCD",    // SUBTILIS_VFP_INSTR_FMSCD
	"FNMSCS",   // SUBTILIS_VFP_INSTR_FNMSCS
	"FNMSCD",   // SUBTILIS_VFP_INSTR_FNMSCD
	"FMULS",    // SUBTILIS_VFP_INSTR_FMULS
	"FMULD",    // SUBTILIS_VFP_INSTR_FMULD
	"FNMULS",   // SUBTILIS_VFP_INSTR_FNMULS
	"FNMULD",   // SUBTILIS_VFP_INSTR_FNMULD
	"FADDS",    // SUBTILIS_VFP_INSTR_FADDS
	"FADDD",    // SUBTILIS_VFP_INSTR_FADDD
	"FSUBS",    // SUBTILIS_VFP_INSTR_FSUBS
	"FSUBD",    // SUBTILIS_VFP_INSTR_FSUBD
	"FDIVS",    // SUBTILIS_VFP_INSTR_FDIVS
	"FDIVD",    // SUBTILIS_VFP_INSTR_FDIVD
	"FCMPS",    // SUBTILIS_VFP_INSTR_FCMPS
	"FCMPD",    // SUBTILIS_VFP_INSTR_FCMPD
	"FCMPES",   // SUBTILIS_VFP_INSTR_FCMPES
	"FCMPED",   // SUBTILIS_VFP_INSTR_FCMPED
	"FCMPZS",   // SUBTILIS_VFP_INSTR_FCMPZS
	"FCMPZD",   // SUBTILIS_VFP_INSTR_FCMPZD
	"FCMPEZS",  // SUBTILIS_VFP_INSTR_FCMPEZS
	"FCMPEZD",  // SUBTILIS_VFP_INSTR_FCMPEZD
	"FSQRTD",   // SUBTILIS_VFP_INSTR_FSQRTD
	"FSQRTS",   // SUBTILIS_VFP_INSTR_FSQRTS
	"FMXR",     // SUBTILIS_VFP_INSTR_FMXR
	"FMRX",     // SUBTILIS_VFP_INSTR_FMRX
	"FMDRR",    // SUBTILIS_VFP_INSTR_FMDRR
	"FMRRD",    // SUBTILIS_VFP_INSTR_FMRRD
	"FMSRR",    // SUBTILIS_VFP_INSTR_FMSRR
	"FMRRS",    // SUBTILIS_VFP_INSTR_FMRRS
	"FCVTDS",   // SUBTILIS_VFP_INSTR_FCVTDS
	"FCVTSD",   // SUBTILIS_VFP_INSTR_FCVTSD
	"LDR",      // SUBTILIS_ARM_STRAN_MISC_LDR
	"STR",      // SUBTILIS_ARM_STRAN_MISC_STR
	"QADD16",   // SUBTILIS_ARM_SIMD_QADD16
	"QADD8",    // SUBTILIS_ARM_SIMD_QADD8
	"QADDSUBX", // SUBTILIS_ARM_SIMD_QADDSUBX
	"QSUB16",   // SUBTILIS_ARM_SIMD_QSUB16
	"QSUB8",    // SUBTILIS_ARM_SIMD_QSUB8
	"QSUBADDX", // SUBTILIS_ARM_SIMD_QSUBADDX
	"SADD16",   // SUBTILIS_ARM_SIMD_SADD16
	"SADD8",    // SUBTILIS_ARM_SIMD_SADD8
	"SADDSUBX", // SUBTILIS_ARM_SIMD_SADDSUBX
	"SSUB16",   // SUBTILIS_ARM_SIMD_SSUB16
	"SSUB8",    // SUBTILIS_ARM_SIMD_SSUB8
	"SSUBADDX", // SUBTILIS_ARM_SIMD_SSUBADDX
	"SHADD16",  // SUBTILIS_ARM_SIMD_SHADD16
	"SHADD8",   // SUBTILIS_ARM_SIMD_SHADD8
	"SHADDSUBX",// SUBTILIS_ARM_SIMD_SHADDSUBX
	"SHSUB16",  // SUBTILIS_ARM_SIMD_SHSUB16
	"SHSUB8",   // SUBTILIS_ARM_SIMD_SHSUB8
	"SHSUBADDX",// SUBTILIS_ARM_SIMD_SHSUBADDX
	"UADD16",   // SUBTILIS_ARM_SIMD_UADD16
	"UADD8",    // SUBTILIS_ARM_SIMD_UADD8
	"UADDSUBX", // SUBTILIS_ARM_SIMD_UADDSUBX
	"USUB16",   // SUBTILIS_ARM_SIMD_USUB16
	"USUB8",    // SUBTILIS_ARM_SIMD_USUB8
	"USUBADDX", // SUBTILIS_ARM_SIMD_USUBADDX
	"UHADD16",  // SUBTILIS_ARM_SIMD_UHADD16
	"UHADD8",   // SUBTILIS_ARM_SIMD_UHADD8
	"UHADDSUBX",// SUBTILIS_ARM_SIMD_UHADDSUBX
	"UHSUB16",  // SUBTILIS_ARM_SIMD_UHSUB16
	"UHSUB8",   // SUBTILIS_ARM_SIMD_UHSUB8
	"UHSUBADDX",// SUBTILIS_ARM_SIMD_UHSUBADDX
	"UQADD16",  // SUBTILIS_ARM_SIMD_UQADD16
	"UQADD8",   // SUBTILIS_ARM_SIMD_UQADD8
	"UQADDSUBX",// SUBTILIS_ARM_SIMD_UQADDSUBX
	"UQSUB16",  // SUBTILIS_ARM_SIMD_UQSUB16
	"UQSUB8",   // SUBTILIS_ARM_SIMD_UQSUB8
	"UQSUBADDX",// SUBTILIS_ARM_SIMD_UQSUBADDX
	"SXTB",     // SUBTILIS_ARM_INSTR_SXTB
	"SXTB16",   // SUBTILIS_ARM_INSTR_SXTB16
	"SXTH",     // SUBTILIS_ARM_INSTR_SXTH
};

static const char *const shift_desc[] = {
	"LSL", // SUBTILIS_ARM_SHIFT_LSL
	"ASL", // SUBTILIS_ARM_SHIFT_ASL
	"LSR", // SUBTILIS_ARM_SHIFT_LSR
	"ASR", // SUBTILIS_ARM_SHIFT_ASR
	"ROR", // SUBTILIS_ARM_SHIFT_ROR
	"RRX", // SUBTILIS_ARM_SHIFT_RRX
};

/* clang-format on */

static void prv_dump_op2(subtilis_arm_op2_t *op2)
{
	switch (op2->type) {
	case SUBTILIS_ARM_OP2_REG:
		printf("R%zu", op2->op.reg);
		break;
	case SUBTILIS_ARM_OP2_I32:
		printf("#%" PRIu32, op2->op.integer);
		break;
	default:
		if (op2->op.shift.shift_reg)
			printf("R%zu, %s R%zu", op2->op.shift.reg,
			       shift_desc[op2->op.shift.type],
			       op2->op.shift.shift.reg);
		else
			printf("R%zu, %s #%" PRIi32, op2->op.shift.reg,
			       shift_desc[op2->op.shift.type],
			       op2->op.shift.shift.integer);

		break;
	}
}

static void prv_dump_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->status)
		printf("S");
	printf(" R%zu, ", instr->dest);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->status)
		printf("S");
	printf(" R%zu, R%zu, ", instr->dest, instr->op1);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_mul_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_mul_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->status)
		printf("S");
	if (type == SUBTILIS_ARM_INSTR_MLA)
		printf(" R%zu, R%zu, R%zu, R%zu\n", instr->dest, instr->rm,
		       instr->rs, instr->rn);
	else
		printf(" R%zu, R%zu, R%zu\n", instr->dest, instr->rm,
		       instr->rs);
}

static void prv_dump_stran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	const char *sub = instr->subtract ? "-" : "";

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->byte)
		printf("B");
	printf(" R%zu", instr->dest);
	if (instr->pre_indexed) {
		printf(", [R%zu, %s", instr->base, sub);
		prv_dump_op2(&instr->offset);
		printf("]");
		if (instr->write_back)
			printf("!");
	} else {
		printf(", [R%zu], %s", instr->base, sub);
		prv_dump_op2(&instr->offset);
	}
	printf("\n");
}

static void prv_dump_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mtran_instr_t *instr,
				 subtilis_error_t *err)
{
	size_t i;
	const char *const direction[] = {
	    "IA", "IB", "DA", "DB", "FA", "FD", "EA", "ED",
	};

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf("%s R%zu", direction[(size_t)instr->type], instr->op0);
	if (instr->write_back)
		printf("!");
	printf(", {");

	for (i = 0; i < 15; i++) {
		if (1 << i & instr->reg_list) {
			printf("R%zu", i);
			i++;
			break;
		}
	}

	for (; i < 15; i++) {
		if ((1 << i) & instr->reg_list)
			printf(", R%zu", i);
	}
	printf("}");
	if (instr->status)
		printf("^");
	printf("\n");
}

static void prv_dump_br_instr(void *user_data, subtilis_arm_op_t *op,
			      subtilis_arm_instr_type_t type,
			      subtilis_arm_br_instr_t *instr,
			      subtilis_error_t *err)
{
	subtilis_arm_prog_t *p = user_data;

	printf("\t%s", (instr->link) ? "BL" : "B");
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (instr->link) {
		if (instr->indirect)
			printf(" [R%zu]\n", instr->target.reg);
		else if (p)
			printf(" %s\n",
			       p->string_pool->strings[instr->target.label]);
		else
			printf(" %" PRIi32 "\n", (int32_t)instr->target.label);
	} else if (p) {
		printf(" label_%zu\n", instr->target.label);
	} else {
		printf(" %" PRIi32 "\n", (int32_t)instr->target.label);
	}
}

static void prv_dump_swi_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" &%zx\n", instr->code);
}

static void prv_dump_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_ldrc_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu, label_%zu\n", instr->dest, instr->label);
}

static void prv_dump_ldrp_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_ldrp_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu, section_%zu via label_%zu\n", instr->dest,
	       instr->section_label, instr->constant_label);
}

static void prv_dump_adr_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_adr_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu, label_%zu\n", instr->dest, instr->label);
}

static void prv_dump_cmov_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_cmov_instr_t *instr,
				subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->fused) {
		printf("%s%s", ccode_desc[instr->false_cond],
		       ccode_desc[instr->true_cond]);
		printf(" R%zu, R%zu, R%zu\n", instr->dest, instr->op2,
		       instr->op3);

	} else {
		printf(" R%zu, R%zu, R%zu, R%zu\n", instr->dest, instr->op1,
		       instr->op2, instr->op3);
	}
}

static void prv_dump_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if ((instr->dest == 15) && (type == SUBTILIS_ARM_INSTR_TEQ))
		printf("P");
	printf(" R%zu, ", instr->op1);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_flags_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_flags_instr_t *instr,
				 subtilis_error_t *err)
{
	const char *flags_reg;
	char fields[5];
	size_t field_count = 0;

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	flags_reg =
	    instr->flag_reg == SUBTILIS_ARM_FLAGS_CPSR ? "CPSR" : "SPSR";

	if (type == SUBTILIS_ARM_INSTR_MSR) {
		if (instr->fields & SUBTILIS_ARM_FLAGS_FIELD_CONTROL)
			fields[field_count++] = 'c';
		if (instr->fields & SUBTILIS_ARM_FLAGS_FIELD_EXTENSION)
			fields[field_count++] = 'x';
		if (instr->fields & SUBTILIS_ARM_FLAGS_FIELD_STATUS)
			fields[field_count++] = 's';
		if (instr->fields & SUBTILIS_ARM_FLAGS_FIELD_FLAGS)
			fields[field_count++] = 'f';
		fields[field_count] = 0;
		if (instr->op2_reg)
			printf(" %s_%s, R%zu\n", flags_reg, fields,
			       instr->op.reg);
		else
			printf(" %s_%s, #%" PRIu32 "\n", flags_reg, fields,
			       instr->op.integer);
	} else {
		printf(" R%zu, %s\n", instr->op.reg, flags_reg);
	}
}

static const char *prv_fpa_size(size_t size)
{
	switch (size) {
	case 4:
		return "s";
	case 10:
		return "e";
	}

	return "d";
}

static double prv_extract_imm(subtilis_fpa_op2_t op2)
{
	subtilis_error_t err;

	subtilis_error_init(&err);
	return subtilis_fpa_extract_imm(op2, &err);
}

static const char *prv_extract_rounding(subtilis_fpa_rounding_t rnd)
{
	const char *rounding = "";

	switch (rnd) {
	case SUBTILIS_FPA_ROUNDING_NEAREST:
		break;
	case SUBTILIS_FPA_ROUNDING_PLUS_INFINITY:
		rounding = "P";
		break;
	case SUBTILIS_FPA_ROUNDING_MINUS_INFINITY:
		rounding = "M";
		break;
	case SUBTILIS_FPA_ROUNDING_ZERO:
		rounding = "Z";
		break;
	}

	return rounding;
}

static void prv_dump_fpa_data(void *user_data, bool dyadic,
			      subtilis_arm_op_t *op,
			      subtilis_arm_instr_type_t type,
			      subtilis_fpa_data_instr_t *instr,
			      subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf("%s", prv_fpa_size(instr->size));
	printf("%s F%zu, ", prv_extract_rounding(instr->rounding), instr->dest);
	if (dyadic)
		printf("F%zu, ", instr->op1);
	if (!instr->immediate)
		printf("F%zu", instr->op2.reg);
	else
		printf("#%f", prv_extract_imm(instr->op2));
	printf("\n");
}

static void prv_dump_fpa_data_dyadic_instr(void *user_data,
					   subtilis_arm_op_t *op,
					   subtilis_arm_instr_type_t type,
					   subtilis_fpa_data_instr_t *instr,
					   subtilis_error_t *err)
{
	prv_dump_fpa_data(user_data, true, op, type, instr, err);
}

static void prv_dump_fpa_data_monadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	prv_dump_fpa_data(user_data, false, op, type, instr, err);
}

static void prv_dump_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	const char *sub = instr->subtract ? "-" : "";

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf("%s", prv_fpa_size(instr->size));
	printf(" F%zu", instr->dest);
	if (instr->pre_indexed)
		printf(", [R%zu, %s#%d]", instr->base, sub, instr->offset * 4);
	else
		printf(", [R%zu], %s#%d", instr->base, sub, instr->offset * 4);
	if (instr->write_back)
		printf("!");
	printf("\n");
}

static void prv_dump_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	if (type == SUBTILIS_FPA_INSTR_FLT)
		printf("%s", prv_fpa_size(instr->size));
	printf("%s", prv_extract_rounding(instr->rounding));
	if (type == SUBTILIS_FPA_INSTR_FLT) {
		printf(" F%zu, ", instr->dest);
		if (instr->immediate)
			printf("#%f", prv_extract_imm(instr->op2));
		else
			printf("R%zu", instr->op2.reg);
	} else {
		printf(" R%zu, F%zu", instr->dest, instr->op2.reg);
	}
	printf("\n");
}

static void prv_dump_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_fpa_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" F%zu,", instr->dest);
	if (instr->immediate)
		printf(" #%f", prv_extract_imm(instr->op2));
	else
		printf(" F%zu", instr->op2.reg);
	printf("\n");
}

static void prv_dump_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_ldrc_instr_t *instr,
				    subtilis_error_t *err)
{
	printf("\t%s%s", instr_desc[type], prv_fpa_size(instr->size));
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" F%zu, label_%zu\n", instr->dest, instr->label);
}

static void prv_dump_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu\n", instr->dest);
}

static void prv_dump_label(void *user_data, subtilis_arm_op_t *op, size_t label,
			   subtilis_error_t *err)
{
	printf(".label_%zu\n", label);
}

static void prv_dump_directive(void *user_data, subtilis_arm_op_t *op,
			       subtilis_error_t *err)
{
	switch (op->type) {
	case SUBTILIS_ARM_OP_ALIGN:
		printf("\tALIGN %" PRIu32 "\n", op->op.alignment);
		break;
	case SUBTILIS_ARM_OP_BYTE:
		printf("\tEQUB %d\n", op->op.byte);
		break;
	case SUBTILIS_ARM_OP_TWO_BYTE:
		printf("\tEQUW %d\n", op->op.two_bytes);
		break;
	case SUBTILIS_ARM_OP_FOUR_BYTE:
		printf("\tEQUD %" PRIu32 "\n", op->op.four_bytes);
		break;
	case SUBTILIS_ARM_OP_DOUBLE:
		printf("\tEQUDBL %f\n", op->op.dbl);
		break;
	case SUBTILIS_ARM_OP_DOUBLER:
		printf("\tEQUDBLR %f\n", op->op.dbl);
		break;
	case SUBTILIS_ARM_OP_FLOAT:
		printf("\tEQUF %f\n", op->op.flt);
		break;
	case SUBTILIS_ARM_OP_STRING:
		printf("\tEQUS \"%s\"\n", op->op.str);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_dump_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_stran_instr_t *instr,
				     subtilis_error_t *err)
{
	const char *sub = instr->subtract ? "-" : "";
	const char *reg_type = "D";

	if (type == SUBTILIS_VFP_INSTR_FSTS || type == SUBTILIS_VFP_INSTR_FLDS)
		reg_type = "S";

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" %s%zu", reg_type, instr->dest);
	if (instr->pre_indexed)
		printf(", [R%zu, %s#%d]", instr->base, sub, instr->offset * 4);
	else
		printf(", [R%zu], %s#%d", instr->base, sub, instr->offset * 4);
	if (instr->write_back)
		printf("!");
	printf("\n");
}

static void prv_dump_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_copy_instr_t *instr,
				    subtilis_error_t *err)
{
	const char *reg_type;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		reg_type = "S";
		return;
	default:
		reg_type = "D";
		break;
	}

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" %s%zu, %s%zu\n", reg_type, instr->dest, reg_type, instr->src);
}

static void prv_dump_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_ldrc_instr_t *instr,
				    subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" D%zu, label_%zu\n", instr->dest, instr->label);
}

static void prv_dump_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_tran_instr_t *instr,
				    subtilis_error_t *err)
{
	const char *dest_reg_type = "S";
	const char *src_reg_type = "S";
	subtilis_arm_reg_t src_reg = instr->src;
	subtilis_arm_reg_t dest_reg = instr->dest;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FUITOS:
		break;
	case SUBTILIS_VFP_INSTR_FSITOD:
	case SUBTILIS_VFP_INSTR_FUITOD:
		dest_reg_type = "D";
		if (instr->use_dregs)
			src_reg *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FTOSID:
	case SUBTILIS_VFP_INSTR_FTOUID:
	case SUBTILIS_VFP_INSTR_FTOSIZD:
	case SUBTILIS_VFP_INSTR_FTOUIZD:
		src_reg_type = "D";
		if (instr->use_dregs)
			dest_reg *= 2;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" %s%zu, %s%zu\n", dest_reg_type, dest_reg, src_reg_type,
	       src_reg);
}

static void prv_dump_vfp_tran_dbl_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_vfp_tran_dbl_instr_t *instr,
					subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMDRR:
		printf(" D%zu, R%zu, R%zu\n", instr->dest1, instr->src1,
		       instr->src2);
		break;
	case SUBTILIS_VFP_INSTR_FMRRD:
		printf(" R%zu, R%zu, D%zu\n", instr->dest1, instr->dest2,
		       instr->src1);
		break;
	case SUBTILIS_VFP_INSTR_FMSRR:
		printf(" S%zu, S%zu, R%zu, R%zu\n", instr->dest1, instr->dest2,
		       instr->src1, instr->src2);
		break;
	case SUBTILIS_VFP_INSTR_FMRRS:
		printf(" R%zu, R%zu, S%zu, S%zu\n", instr->dest1, instr->dest2,
		       instr->src1, instr->src2);
		break;
	default:
		break;
	}
}

static void prv_dump_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_cptran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_arm_reg_t reg;

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	if (type == SUBTILIS_VFP_INSTR_FMSR) {
		reg = instr->use_dregs ? instr->dest * 2 : instr->dest;
		printf(" S%zu, R%zu\n", reg, instr->src);
	} else {
		reg = instr->use_dregs ? instr->src * 2 : instr->src;
		printf(" R%zu, S%zu\n", instr->dest, reg);
	}
}

static void prv_dump_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_data_instr_t *instr,
				    subtilis_error_t *err)
{
	const char *reg;

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
		reg = "S";
		break;
	default:
		reg = "D";
		break;
	}

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	printf(" %s%zu, %s%zu, %s%zu\n", reg, instr->dest, reg, instr->op1, reg,
	       instr->op2);
}

static void prv_dump_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_vfp_cmp_instr_t *instr,
				   subtilis_error_t *err)
{
	const char *reg = "D";
	bool op2 = false;

	printf("\t%s", instr_desc[type]);

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPS:
		reg = "S";
		op2 = true;
		break;
	case SUBTILIS_VFP_INSTR_FCMPD:
	case SUBTILIS_VFP_INSTR_FCMPED:
		op2 = true;
		break;
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
		reg = "S";
		break;
	default:
		break;
	}

	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	printf(" %s%zu", reg, instr->op1);
	if (op2)
		printf(", %s%zu", reg, instr->op2);
	printf("\n");
}

static void prv_dump_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_vfp_sqrt_instr_t *instr,
				    subtilis_error_t *err)
{
	const char *reg = (type == SUBTILIS_VFP_INSTR_FSQRTD) ? "D" : "S";

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	printf(" %s%zu, %s%zu\n", reg, instr->dest, reg, instr->op1);
}

static void prv_dump_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_sysreg_instr_t *instr,
				      subtilis_error_t *err)
{
	const char *sysreg;

	if ((type == SUBTILIS_VFP_INSTR_FMRX) && (instr->arm_reg == 15) &&
	    (instr->sysreg == SUBTILIS_VFP_SYSREG_FPSCR)) {
		printf("\tFMSTAT\n");
		if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
			printf("%s", ccode_desc[instr->ccode]);
		return;
	}

	switch (instr->sysreg) {
	case SUBTILIS_VFP_SYSREG_FPSID:
		sysreg = "FPSID";
		break;
	case SUBTILIS_VFP_SYSREG_FPSCR:
		sysreg = "FPSCR";
		break;
	case SUBTILIS_VFP_SYSREG_FPEXC:
		sysreg = "FPEXC";
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	if (type == SUBTILIS_VFP_INSTR_FMRX)
		printf(" R%zu, %s\n", instr->arm_reg, sysreg);
	else
		printf(" %s, R%zu\n", sysreg, instr->arm_reg);
}

static void prv_dump_vfp_cvt_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_vfp_cvt_instr_t *instr,
				   subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	if (type == SUBTILIS_VFP_INSTR_FCVTDS)
		printf(" D%zu, S%zu\n", instr->dest, instr->op1);
	else
		printf(" S%zu, D%zu\n", instr->dest, instr->op1);
}

static void prv_dump_stran_misc_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_arm_stran_misc_instr_t *instr,
				      subtilis_error_t *err)
{
	const char *itype = "";
	const char *sub = instr->subtract ? "-" : "";

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	switch (instr->type) {
	case SUBTILIS_ARM_STRAN_MISC_SB:
		itype = "SB";
		break;
	case SUBTILIS_ARM_STRAN_MISC_SH:
		itype = "SH";
		break;
	case SUBTILIS_ARM_STRAN_MISC_H:
		itype = "H";
		break;
	case SUBTILIS_ARM_STRAN_MISC_D:
		itype = "D";
		break;
	}
	printf("%s R%zu", itype, instr->dest);
	if (instr->pre_indexed) {
		printf(", [R%zu, %s", instr->base, sub);
		if (instr->reg_offset)
			printf("R%zu", instr->offset.reg);
		else
			printf("%d", instr->offset.imm);
		printf("]");
		if (instr->write_back)
			printf("!");
	} else {
		printf(", [R%zu], %s", instr->base, sub);
		if (instr->reg_offset)
			printf("R%zu", instr->offset.reg);
		else
			printf("%d", instr->offset.imm);
	}
	printf("\n");
}

static void prv_dump_reg_only_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_arm_reg_only_instr_t *instr,
				    subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	printf(" R%zu, R%zu, R%zu\n", instr->dest, instr->op1, instr->op2);
}

static void prv_dump_signx_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_signx_instr_t *instr,
				 subtilis_error_t *err)
{
	int ror = 0;

	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);

	printf(" R%zu, R%zu", instr->dest, instr->op1);
	switch (instr->rotate) {
	case SUBTILIS_ARM_SIGNX_ROR_NONE:
		printf("\n");
		return;
	case SUBTILIS_ARM_SIGNX_ROR_8:
		ror = 8;
		break;
	case SUBTILIS_ARM_SIGNX_ROR_16:
		ror = 16;
		break;
	case SUBTILIS_ARM_SIGNX_ROR_24:
		ror = 24;
		break;
	}

	printf(", ROR %d\n", ror);
}

void subtilis_arm_section_dump(subtilis_arm_prog_t *p,
			       subtilis_arm_section_t *s)
{
	subtlis_arm_walker_t walker;
	subtilis_error_t err;
	size_t i;

	subtilis_error_init(&err);

	walker.user_data = p;
	walker.label_fn = prv_dump_label;
	walker.directive_fn = prv_dump_directive;
	walker.data_fn = prv_dump_data_instr;
	walker.mul_fn = prv_dump_mul_instr;
	walker.cmp_fn = prv_dump_cmp_instr;
	walker.mov_fn = prv_dump_mov_instr;
	walker.stran_fn = prv_dump_stran_instr;
	walker.mtran_fn = prv_dump_mtran_instr;
	walker.br_fn = prv_dump_br_instr;
	walker.swi_fn = prv_dump_swi_instr;
	walker.ldrc_fn = prv_dump_ldrc_instr;
	walker.ldrp_fn = prv_dump_ldrp_instr;
	walker.adr_fn = prv_dump_adr_instr;
	walker.cmov_fn = prv_dump_cmov_instr;
	walker.flags_fn = prv_dump_flags_instr;
	walker.fpa_data_monadic_fn = prv_dump_fpa_data_monadic_instr;
	walker.fpa_data_dyadic_fn = prv_dump_fpa_data_dyadic_instr;
	walker.fpa_stran_fn = prv_dump_fpa_stran_instr;
	walker.fpa_tran_fn = prv_dump_fpa_tran_instr;
	walker.fpa_cmp_fn = prv_dump_fpa_cmp_instr;
	walker.fpa_ldrc_fn = prv_dump_fpa_ldrc_instr;
	walker.fpa_cptran_fn = prv_dump_fpa_cptran_instr;
	walker.vfp_stran_fn = prv_dump_vfp_stran_instr;
	walker.vfp_copy_fn = prv_dump_vfp_copy_instr;
	walker.vfp_ldrc_fn = prv_dump_vfp_ldrc_instr;
	walker.vfp_tran_fn = prv_dump_vfp_tran_instr;
	walker.vfp_tran_dbl_fn = prv_dump_vfp_tran_dbl_instr;
	walker.vfp_cptran_fn = prv_dump_vfp_cptran_instr;
	walker.vfp_data_fn = prv_dump_vfp_data_instr;
	walker.vfp_cmp_fn = prv_dump_vfp_cmp_instr;
	walker.vfp_sqrt_fn = prv_dump_vfp_sqrt_instr;
	walker.vfp_sysreg_fn = prv_dump_vfp_sysreg_instr;
	walker.vfp_cvt_fn = prv_dump_vfp_cvt_instr;
	walker.stran_misc_fn = prv_dump_stran_misc_instr;
	walker.simd_fn = prv_dump_reg_only_instr;
	walker.signx_fn = prv_dump_signx_instr;

	subtilis_arm_walk(s, &walker, &err);

	for (i = 0; i < s->constants.real_count; i++) {
		printf(".label_%zu\n", s->constants.real[i].label);
		printf("\tEQUFD %f\n", s->constants.real[i].real);
	}
	for (i = 0; i < s->constants.ui32_count; i++) {
		printf(".label_%zu\n", s->constants.ui32[i].label);
		printf("\tEQUD &%" PRIx32 "\n", s->constants.ui32[i].integer);
	}
}

void subtilis_arm_prog_dump(subtilis_arm_prog_t *p)
{
	subtilis_arm_section_t *arm_s;
	size_t i;

	if (p->num_sections == 0)
		return;

	arm_s = p->sections[0];
	printf("%s\n", p->string_pool->strings[0]);
	subtilis_arm_section_dump(p, arm_s);

	for (i = 1; i < p->num_sections; i++) {
		printf("\n");
		arm_s = p->sections[i];
		printf("%s\n", p->string_pool->strings[i]);
		subtilis_arm_section_dump(p, arm_s);
	}
}

void subtilis_arm_instr_dump(subtilis_arm_instr_t *instr)
{
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
		prv_dump_data_instr(NULL, NULL, instr->type,
				    &instr->operands.data, NULL);
		break;
	case SUBTILIS_ARM_INSTR_MUL:
	case SUBTILIS_ARM_INSTR_MLA:
		prv_dump_mul_instr(NULL, NULL, instr->type,
				   &instr->operands.mul, NULL);
		break;
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
	case SUBTILIS_ARM_INSTR_CMP:
	case SUBTILIS_ARM_INSTR_CMN:
		prv_dump_cmp_instr(NULL, NULL, instr->type,
				   &instr->operands.data, NULL);
		break;
	case SUBTILIS_ARM_INSTR_MOV:
	case SUBTILIS_ARM_INSTR_MVN:
		prv_dump_mov_instr(NULL, NULL, instr->type,
				   &instr->operands.data, NULL);
		break;
	case SUBTILIS_ARM_INSTR_LDR:
	case SUBTILIS_ARM_INSTR_STR:
		prv_dump_stran_instr(NULL, NULL, instr->type,
				     &instr->operands.stran, NULL);
		break;
	case SUBTILIS_ARM_INSTR_LDRC:
		prv_dump_ldrc_instr(NULL, NULL, instr->type,
				    &instr->operands.ldrc, NULL);
		break;
	case SUBTILIS_ARM_INSTR_LDRP:
		prv_dump_ldrp_instr(NULL, NULL, instr->type,
				    &instr->operands.ldrp, NULL);
		break;
	case SUBTILIS_ARM_INSTR_ADR:
		prv_dump_adr_instr(NULL, NULL, instr->type,
				   &instr->operands.adr, NULL);
		break;
	case SUBTILIS_ARM_INSTR_CMOV:
		prv_dump_cmov_instr(NULL, NULL, instr->type,
				    &instr->operands.cmov, NULL);
		break;
	case SUBTILIS_ARM_INSTR_LDM:
	case SUBTILIS_ARM_INSTR_STM:
		prv_dump_mtran_instr(NULL, NULL, instr->type,
				     &instr->operands.mtran, NULL);
		break;
	case SUBTILIS_ARM_INSTR_B:
		prv_dump_br_instr(NULL, NULL, instr->type, &instr->operands.br,
				  NULL);
		break;
	case SUBTILIS_ARM_INSTR_SWI:
		prv_dump_swi_instr(NULL, NULL, instr->type,
				   &instr->operands.swi, NULL);
		break;
	case SUBTILIS_ARM_INSTR_MSR:
	case SUBTILIS_ARM_INSTR_MRS:
		prv_dump_flags_instr(NULL, NULL, instr->type,
				     &instr->operands.flags, NULL);
		break;
	case SUBTILIS_FPA_INSTR_LDF:
	case SUBTILIS_FPA_INSTR_STF:
		prv_dump_fpa_stran_instr(NULL, NULL, instr->type,
					 &instr->operands.fpa_stran, NULL);
		break;
	case SUBTILIS_FPA_INSTR_LDRC:
		prv_dump_fpa_ldrc_instr(NULL, NULL, instr->type,
					&instr->operands.fpa_ldrc, NULL);
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
		prv_dump_fpa_data_dyadic_instr(NULL, NULL, instr->type,
					       &instr->operands.fpa_data, NULL);
		break;
	case SUBTILIS_FPA_INSTR_MVF:
	case SUBTILIS_FPA_INSTR_MNF:
	case SUBTILIS_FPA_INSTR_RPW:
	case SUBTILIS_FPA_INSTR_POW:
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
		prv_dump_fpa_data_monadic_instr(
		    NULL, NULL, instr->type, &instr->operands.fpa_data, NULL);
		break;
	case SUBTILIS_FPA_INSTR_FLT:
	case SUBTILIS_FPA_INSTR_FIX:
		prv_dump_fpa_tran_instr(NULL, NULL, instr->type,
					&instr->operands.fpa_tran, NULL);
		break;
	case SUBTILIS_FPA_INSTR_CMF:
	case SUBTILIS_FPA_INSTR_CNF:
	case SUBTILIS_FPA_INSTR_CMFE:
	case SUBTILIS_FPA_INSTR_CNFE:
		prv_dump_fpa_cmp_instr(NULL, NULL, instr->type,
				       &instr->operands.fpa_cmp, NULL);
		break;
	case SUBTILIS_FPA_INSTR_WFS:
	case SUBTILIS_FPA_INSTR_RFS:
		prv_dump_fpa_cptran_instr(NULL, NULL, instr->type,
					  &instr->operands.fpa_cptran, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FSTS:
	case SUBTILIS_VFP_INSTR_FLDS:
	case SUBTILIS_VFP_INSTR_FSTD:
	case SUBTILIS_VFP_INSTR_FLDD:
		prv_dump_vfp_stran_instr(NULL, NULL, instr->type,
					 &instr->operands.vfp_stran, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FCPYD:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FNEGD:
	case SUBTILIS_VFP_INSTR_FABSS:
	case SUBTILIS_VFP_INSTR_FABSD:
		prv_dump_vfp_copy_instr(NULL, NULL, instr->type,
					&instr->operands.vfp_copy, NULL);
		break;
	case SUBTILIS_VFP_INSTR_LDRC:
		prv_dump_vfp_ldrc_instr(NULL, NULL, instr->type,
					&instr->operands.vfp_ldrc, NULL);
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
		prv_dump_vfp_tran_instr(NULL, NULL, instr->type,
					&instr->operands.vfp_tran, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FMDRR:
	case SUBTILIS_VFP_INSTR_FMRRD:
	case SUBTILIS_VFP_INSTR_FMSRR:
	case SUBTILIS_VFP_INSTR_FMRRS:
		prv_dump_vfp_tran_dbl_instr(NULL, NULL, instr->type,
					    &instr->operands.vfp_tran_dbl,
					    NULL);
		break;
	case SUBTILIS_VFP_INSTR_FMSR:
	case SUBTILIS_VFP_INSTR_FMRS:
		prv_dump_vfp_cptran_instr(NULL, NULL, instr->type,
					  &instr->operands.vfp_cptran, NULL);
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
		prv_dump_vfp_data_instr(NULL, NULL, instr->type,
					&instr->operands.vfp_data, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPD:
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPED:
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPZD:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
	case SUBTILIS_VFP_INSTR_FCMPEZD:
		prv_dump_vfp_cmp_instr(NULL, NULL, instr->type,
				       &instr->operands.vfp_cmp, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FSQRTD:
	case SUBTILIS_VFP_INSTR_FSQRTS:
		prv_dump_vfp_sqrt_instr(NULL, NULL, instr->type,
					&instr->operands.vfp_sqrt, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FMXR:
	case SUBTILIS_VFP_INSTR_FMRX:
		prv_dump_vfp_sysreg_instr(NULL, NULL, instr->type,
					  &instr->operands.vfp_sysreg, NULL);
		break;
	case SUBTILIS_VFP_INSTR_FCVTDS:
	case SUBTILIS_VFP_INSTR_FCVTSD:
		prv_dump_vfp_cvt_instr(NULL, NULL, instr->type,
				       &instr->operands.vfp_cvt, NULL);
		break;
	case SUBTILIS_ARM_STRAN_MISC_LDR:
	case SUBTILIS_ARM_STRAN_MISC_STR:
		prv_dump_stran_misc_instr(NULL, NULL, instr->type,
					  &instr->operands.stran_misc, NULL);
		break;
	case SUBTILIS_ARM_SIMD_QADD16:
	case SUBTILIS_ARM_SIMD_QADD8:
	case SUBTILIS_ARM_SIMD_QADDSUBX:
	case SUBTILIS_ARM_SIMD_QSUB16:
	case SUBTILIS_ARM_SIMD_QSUB8:
	case SUBTILIS_ARM_SIMD_QSUBADDX:
	case SUBTILIS_ARM_SIMD_SADD16:
	case SUBTILIS_ARM_SIMD_SADD8:
	case SUBTILIS_ARM_SIMD_SADDSUBX:
	case SUBTILIS_ARM_SIMD_SSUB16:
	case SUBTILIS_ARM_SIMD_SSUB8:
	case SUBTILIS_ARM_SIMD_SSUBADDX:
	case SUBTILIS_ARM_SIMD_SHADD16:
	case SUBTILIS_ARM_SIMD_SHADD8:
	case SUBTILIS_ARM_SIMD_SHADDSUBX:
	case SUBTILIS_ARM_SIMD_SHSUB16:
	case SUBTILIS_ARM_SIMD_SHSUB8:
	case SUBTILIS_ARM_SIMD_SHSUBADDX:
	case SUBTILIS_ARM_SIMD_UADD16:
	case SUBTILIS_ARM_SIMD_UADD8:
	case SUBTILIS_ARM_SIMD_UADDSUBX:
	case SUBTILIS_ARM_SIMD_USUB16:
	case SUBTILIS_ARM_SIMD_USUB8:
	case SUBTILIS_ARM_SIMD_USUBADDX:
	case SUBTILIS_ARM_SIMD_UHADD16:
	case SUBTILIS_ARM_SIMD_UHADD8:
	case SUBTILIS_ARM_SIMD_UHADDSUBX:
	case SUBTILIS_ARM_SIMD_UHSUB16:
	case SUBTILIS_ARM_SIMD_UHSUB8:
	case SUBTILIS_ARM_SIMD_UHSUBADDX:
	case SUBTILIS_ARM_SIMD_UQADD16:
	case SUBTILIS_ARM_SIMD_UQADD8:
	case SUBTILIS_ARM_SIMD_UQADDSUBX:
	case SUBTILIS_ARM_SIMD_UQSUB16:
	case SUBTILIS_ARM_SIMD_UQSUB8:
	case SUBTILIS_ARM_SIMD_UQSUBADDX:
		prv_dump_reg_only_instr(NULL, NULL, instr->type,
					&instr->operands.reg_only, NULL);
		break;
	case SUBTILIS_ARM_INSTR_SXTB:
	case SUBTILIS_ARM_INSTR_SXTB16:
	case SUBTILIS_ARM_INSTR_SXTH:
		prv_dump_signx_instr(NULL, NULL, instr->type,
				     &instr->operands.signx, NULL);
		break;
	default:
		printf("\tUNKNOWN INSTRUCTION\n");
		break;
	}
}
