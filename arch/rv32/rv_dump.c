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

#include <inttypes.h>
#include <string.h>

#include "rv32_core.h"
#include "rv_walker.h"

/* clang-format off */

static const char *const instr_desc[] = {
	"addi",     // SUBTILIS_RV_ADDI,
	"slti",     // SUBTILIS_RV_SLTI,
	"sltiu",    // SUBTILIS_RV_SLTIU,
	"andi",     // SUBTILIS_RV_ANDI,
	"ori",      // SUBTILIS_RV_ORI,
	"xori",     // SUBTILIS_RV_XORI,
	"slli",     // SUBTILIS_RV_SLLI,
	"srli",     // SUBTILIS_RV_SRLI,
	"srai",     // SUBTILIS_RV_SRAI,
	"lui",      // SUBTILIS_RV_LUI,
	"auipc",    // SUBTILIS_RV_AUIPC,
	"add",      // SUBTILIS_RV_ADD,
	"slt",      // SUBTILIS_RV_SLT,
	"sltu",     // SUBTILIS_RV_SLTU,
	"and",      // SUBTILIS_RV_AND,
	"or",       // SUBTILIS_RV_OR,
	"xor",      // SUBTILIS_RV_XOR,
	"sll",      // SUBTILIS_RV_SLL,
	"srl",      // SUBTILIS_RV_SRL,
	"sub",      // SUBTILIS_RV_SUB,
	"sra",      // SUBTILIS_RV_SRA,
	"jal",      // SUBTILIS_RV_JAL,
	"jalr",     // SUBTILIS_RV_JALR,
	"beq",      // SUBTILIS_RV_BEQ,
	"bne",      // SUBTILIS_RV_BNE,
	"blt",      // SUBTILIS_RV_BLT,
	"bltu",     // SUBTILIS_RV_BLTU,
	"bge",      // SUBTILIS_RV_BGE,
	"bgeu",     // SUBTILIS_RV_BGEU,
	"lw",       // SUBTILIS_RV_LW,
	"lh",       // SUBTILIS_RV_LH,
	"lhu",      // SUBTILIS_RV_LHU,
	"lb",       // SUBTILIS_RV_LB,
	"lbu",      // SUBTILIS_RV_LBU,
	"sw",       // SUBTILIS_RV_SW,
	"sh",       // SUBTILIS_RV_SH,
	"sb",       // SUBTILIS_RV_SB,
	"fence",    // SUBTILIS_RV_FENCE,
	"ecall",    // SUBTILIS_RV_ECALL,
	"ebreak",   // SUBTILIS_RV_EBREAK,
	"mul",      // SUBTILIS_RV_MUL,
	"mulh",     // SUBTILIS_RV_MULH,
	"mulhsu",   // SUBTILIS_RV_MULHSU,
	"mulhu",    // SUBTILIS_RV_MULHU,
	"div",      // SUBTILIS_RV_DIV,
	"divu",     // SUBTILIS_RV_DIVU,
	"rem",      // SUBTILIS_RV_REM,
	"remu",     // SUBTILIS_RV_REMU,

	"flw",      // SUBTILIS_RV_FLW,
	"fsw",      // SUBTILIS_RV_FSW,
	"fmadd.s",  // SUBTILIS_RV_FMADD_S,
	"fmsub.s",  // SUBTILIS_RV_FMSUB_S,
	"fnmsub.s", // SUBTILIS_RV_FNMSUB_S,
	"fnmadd.s", // SUBTILIS_RV_FNMADD_S,
	"fadd.s",   // SUBTILIS_RV_FADD_S,
	"fsub.s",   // SUBTILIS_RV_FSUB_S,
	"fmul.s",   // SUBTILIS_RV_FMUL_S,
	"fdiv.s",   // SUBTILIS_RV_FDIV_S,
	"fsqrt.s",  // SUBTILIS_RV_FSQRT_S,
	"fsgnj.s",  // SUBTILIS_RV_FSGNJ_S,
	"fsgnjn.s", // SUBTILIS_RV_FSGNJN_S,
	"fsgnjx.s", // SUBTILIS_RV_FSGNJX_S,
	"fmin.s",   // SUBTILIS_RV_FMIN_S,
	"fmax.s",   // SUBTILIS_RV_FMAX_S,
	"fcvt.w.s", // SUBTILIS_RV_FCVT_W_S,
	"fcvt.wu.s",// SUBTILIS_RV_FCVT_WU_S,
	"fmw.x.s",  // SUBTILIS_RV_FMV_X_S,
	"feq.s",    // SUBTILIS_RV_FEQ_S,
	"flt.s",    // SUBTILIS_RV_FLT_S,
	"fle.s",    // SUBTILIS_RV_FLE_S,
	"fclass.s", // SUBTILIS_RV_FCLASS_S,
	"fcvt.s.w", // SUBTILIS_RV_FCVT_S_W,
	"fcvt.s.wu",// SUBTILIS_RV_FCVT_S_WU,
	"fmw.w.x",  // SUBTILIS_RV_FMV_W_X,

	"fld",      // SUBTILIS_RV_FLD,
	"fsd",      // SUBTILIS_RV_FSD,
	"fmadd.d",  // SUBTILIS_RV_FMADD_D,
	"fmsub.d",  // SUBTILIS_RV_FMSUB_D,
	"fnmsub.d", // SUBTILIS_RV_FNMSUB_D,
	"fnmadd.d", // SUBTILIS_RV_FNMADD_D,
	"fadd.d",   // SUBTILIS_RV_FADD_D,
	"fsub.d",   // SUBTILIS_RV_FSUB_D,
	"fmul.d",   // SUBTILIS_RV_FMUL_D,
	"fdiv.d",   // SUBTILIS_RV_FDIV_D,
	"fsqrt.d",  // SUBTILIS_RV_FSQRT_D,
	"fsgnj.d",  // SUBTILIS_RV_FSGNJ_D,
	"fsgnjn.d", // SUBTILIS_RV_FSGNJN_D,
	"fsgnjx.d", // SUBTILIS_RV_FSGNJX_D,
	"fmin.d",   // SUBTILIS_RV_FMIN_D,
	"fax.d",    // SUBTILIS_RV_FMAX_D,
	"fcvt.s.d", // SUBTILIS_RV_FCVT_S_D,
	"fcvt.d.s", // SUBTILIS_RV_FCVT_D_S,
	"feq.d",    // SUBTILIS_RV_FEQ_D,
	"flt.d",    // SUBTILIS_RV_FLT_D,
	"fle.d",    // SUBTILIS_RV_FLE_D,
	"fclass.d", // SUBTILIS_RV_FCLASS_D,
	"fcvt.w.d", // SUBTILIS_RV_FCVT_W_D,
	"fcvt.wu.d",// SUBTILIS_RV_FCVT_WU_D,
	"fcvt.d.w", // SUBTILIS_RV_FCVT_D_W,
	"fcvt.d.wu",// SUBTILIS_RV_FCVT_D_WU,

	"lc",       // SUBTILIS_RV_LC,
	"lp",       // SUBTILIS_RV_LP,
	"ldrcf",    // SUBTILIS_RV_LDRCF,
};

static const char *const rounding_modes[] = {
	"rne", // SUBTILIS_RV_FRM_RTE,
	"rtz", // SUBTILIS_RV_FRM_RTZ,
	"rdn", // SUBTILIS_RV_FRM_RDN,
	"rup", // SUBTILIS_RV_FRM_RUP,
	"rmm", // SUBTILIS_RV_FRM_RMM,
	"",    // SUBTILIS_RV_FRM_RES1,
	"",    // SUBTILIS_RV_FRM_RES2,
	"",    // SUBTILIS_RV_FRM_DYN,
};

/* clang-format on */


static void prv_dump_label(void *user_data, subtilis_rv_op_t *op, size_t label,
			   subtilis_error_t *err)
{
	printf(".label_%zu\n", label);
}

static void prv_dump_directive(void *user_data, subtilis_rv_op_t *op,
			       subtilis_error_t *err)
{
	switch (op->type) {
	case SUBTILIS_RV_OP_ALIGN:
		printf("\tALIGN %" PRIu32 "\n", op->op.alignment);
		break;
	case SUBTILIS_RV_OP_BYTE:
		printf("\tEQUB %d\n", op->op.byte);
		break;
	case SUBTILIS_RV_OP_TWO_BYTE:
		printf("\tEQUW %d\n", op->op.two_bytes);
		break;
	case SUBTILIS_RV_OP_FOUR_BYTE:
		printf("\tEQUD %" PRIu32 "\n", op->op.four_bytes);
		break;
	case SUBTILIS_RV_OP_DOUBLE:
		printf("\tEQUDBL %f\n", op->op.dbl);
		break;
	case SUBTILIS_RV_OP_FLOAT:
		printf("\tEQUF %f\n", op->op.flt);
		break;
	case SUBTILIS_RV_OP_STRING:
		printf("\tEQUS \"%s\"\n", op->op.str);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_dump_r(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_rtype_t *r, subtilis_error_t *err)
{
	printf("\t%s x%zu, x%zu, x%zu\n", instr_desc[itype],
	       r->rd, r->rs1, r->rs2);
}

static void prv_dump_i(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_itype_t *i, subtilis_error_t *err)
{
	if (itype == SUBTILIS_RV_ADDI) {
		if (i->rs1 == 0) {
			if (i->rd == 0)
				printf("\tnop\n");
			else
				printf("\tli x%zu, %d\n", i->rd, i->imm);
			return;
		} else if (i->imm == 0) {
			printf("\tmv x%zu, x%zu\n", i->rd, i->rs1);
			return;
		}
	}

	if ((itype == SUBTILIS_RV_ECALL) || (itype == SUBTILIS_RV_EBREAK)) {
		printf("\t%s\n", instr_desc[itype]);
		return;
	}

	switch (itype) {
	case SUBTILIS_RV_LW:
	case SUBTILIS_RV_LH:
	case SUBTILIS_RV_LHU:
	case SUBTILIS_RV_LB:
	case SUBTILIS_RV_LBU:
		printf("\t%s x%zu, %d(x%zu)\n", instr_desc[itype],
		       i->rd, i->imm, i->rs1);
		return;
	default:
		break;
	}

	printf("\t%s x%zu, x%zu, %d\n", instr_desc[itype],
	       i->rd, i->rs1, i->imm);
}

static void prv_dump_sb(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_sbtype_t *sb, subtilis_error_t *err)
{
	if (etype == SUBTILIS_RV_S_TYPE) {
		printf("\t%s x%zu, (%d)x%zu\n", instr_desc[itype],
		       sb->rs2, sb->op.imm, sb->rs1);
		return;
	}

	if (sb->is_label)
		printf("\t%s x%zu, x%zu, label_%zu\n", instr_desc[itype],
		       sb->rs1, sb->rs2, sb->op.label);
	else
		printf("\t%s x%zu, x%zu, %d\n", instr_desc[itype],
		       sb->rs1, sb->rs2, sb->op.imm);
}

static void prv_dump_uj(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_ujtype_t *uj, subtilis_error_t *err)
{
	if ((itype == SUBTILIS_RV_JAL) && (uj->rd == 0)) {
		if (!uj->is_label)
			printf("\tj  %d\n", (int32_t) uj->op.imm);
		else
			printf("\tj  label_%d\n", (int32_t) uj->op.label);
		return;
	}
	if (itype == SUBTILIS_RV_LUI) {
		printf("\t%s x%zu, %d\n", instr_desc[itype], uj->rd,
		       uj->op.imm << 12);
		return;
	}
	printf("\t%s x%zu, %d\n", instr_desc[itype], uj->rd, uj->op.imm);
}

static void prv_dump_real_r(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_rrtype_t *rr, subtilis_error_t *err)
{
	const char *frm = rounding_modes[rr->frm];

	switch (itype) {
	case SUBTILIS_RV_FCLASS_S:
	case SUBTILIS_RV_FCLASS_D:
	case SUBTILIS_RV_FMV_X_W:
		frm = "";
	case SUBTILIS_RV_FCVT_W_S:
	case SUBTILIS_RV_FCVT_WU_S:
	case SUBTILIS_RV_FCVT_W_D:
	case SUBTILIS_RV_FCVT_WU_D:
		printf("\t%s x%zu, f%zu", instr_desc[itype],
		       rr->rd, rr->rs1);
		break;
	case SUBTILIS_RV_FMV_W_X:
		frm = "";
	case SUBTILIS_RV_FCVT_S_W:
	case SUBTILIS_RV_FCVT_S_WU:
	case SUBTILIS_RV_FCVT_S_D:
	case SUBTILIS_RV_FCVT_D_S:
	case SUBTILIS_RV_FCVT_D_W:
	case SUBTILIS_RV_FCVT_D_WU:
		printf("\t%s f%zu, x%zu", instr_desc[itype],
		       rr->rd, rr->rs1);
		break;
	case SUBTILIS_RV_FSQRT_S:
	case SUBTILIS_RV_FSQRT_D:
		printf("\t%s f%zu, f%zu", instr_desc[itype],
		       rr->rd, rr->rs1);
		break;
	case SUBTILIS_RV_FEQ_S:
	case SUBTILIS_RV_FLT_S:
	case SUBTILIS_RV_FLE_S:
	case SUBTILIS_RV_FEQ_D:
	case SUBTILIS_RV_FLT_D:
	case SUBTILIS_RV_FLE_D:
		frm = "";
		printf("\t%s x%zu, f%zu, f%zu", instr_desc[itype],
		       rr->rd, rr->rs1, rr->rs2);
		break;
	case SUBTILIS_RV_FSGNJ_S:
	case SUBTILIS_RV_FSGNJN_S:
	case SUBTILIS_RV_FSGNJX_S:
	case SUBTILIS_RV_FMIN_S:
	case SUBTILIS_RV_FMAX_S:
	case SUBTILIS_RV_FSGNJ_D:
	case SUBTILIS_RV_FSGNJN_D:
	case SUBTILIS_RV_FSGNJX_D:
	case SUBTILIS_RV_FMIN_D:
	case SUBTILIS_RV_FMAX_D:
		frm = "";
	default:
		printf("\t%s f%zu, f%zu, f%zu", instr_desc[itype],
		       rr->rd, rr->rs1, rr->rs2);
		break;
	}

	if (strlen(frm) > 0)
		printf(", %s\n", frm);
	else
		printf("\n");
}

static void prv_dump_real_r4(void *user_data, subtilis_rv_op_t *op,
			     subtilis_rv_instr_type_t itype,
			     subtilis_rv_instr_encoding_t etype,
			     rv_r4type_t *r4, subtilis_error_t *err)
{
	const char *frm = rounding_modes[r4->frm];

	printf("\t%s f%zu, f%zu, f%zu, f%zu", instr_desc[itype],
	       r4->rd, r4->rs1, r4->rs2, r4->rs2);
	if (strlen(frm) > 0)
		printf(", %s\n", frm);
	else
		printf("\n");
}

static void prv_dump_real_i(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_itype_t *i, subtilis_error_t *err)
{
	if ((itype == SUBTILIS_RV_FLW) || (itype == SUBTILIS_RV_FLD)) {
		printf("\t%s f%zu, %d(x%zu)\n", instr_desc[itype],
		       i->rd, i->imm, i->rs1);
		return;
	}
	printf("\t%s f%zu, f%zu, %d\n", instr_desc[itype],
	       i->rd, i->rs1, i->imm);
}

static void prv_dump_real_s(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_sbtype_t *sb, subtilis_error_t *err)
{
	printf("\t%s f%zu, (%d)x%zu\n", instr_desc[itype],
	       sb->rs2, sb->op.imm, sb->rs1);
}

static void prv_dump_ldrc_f(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_ldrctype_t *ldrc, subtilis_error_t *err)
{
	printf("\t%s f%zu via x%zu, label_%zu\n", instr_desc[itype], ldrc->rd,
	       ldrc->rd2, ldrc->label);
}

void subtilis_rv_section_dump(subtilis_rv_prog_t *p,
			      subtilis_rv_section_t *s)
{
	subtilis_rv_walker_t walker;
	subtilis_error_t err;
	size_t i;

	subtilis_error_init(&err);

	walker.user_data = p;
	walker.label_fn = prv_dump_label;
	walker.directive_fn = prv_dump_directive;
	walker.r_fn = prv_dump_r;
	walker.i_fn = prv_dump_i;
	walker.sb_fn = prv_dump_sb;
	walker.uj_fn = prv_dump_uj;
	walker.real_r_fn = prv_dump_real_r;
	walker.real_i_fn = prv_dump_real_i;
	walker.real_s_fn = prv_dump_real_s;
	walker.real_ldrc_f_fn = prv_dump_ldrc_f;

	subtilis_rv_walk(s, &walker, &err);

	for (i = 0; i < s->constants.real_count; i++) {
		printf(".label_%zu\n", s->constants.real[i].label);
		printf("\tEQUFD %f\n", s->constants.real[i].real);
	}
}

void subtilis_rv_prog_dump(subtilis_rv_prog_t *p)
{
	subtilis_rv_section_t *rv_s;
	size_t i;

	if (p->num_sections == 0)
		return;

	rv_s = p->sections[0];
	printf("%s\n", p->string_pool->strings[0]);
	subtilis_rv_section_dump(p, rv_s);

	for (i = 1; i < p->num_sections; i++) {
		printf("\n");
		rv_s = p->sections[i];
		printf("%s\n", p->string_pool->strings[i]);
		subtilis_rv_section_dump(p, rv_s);
	}
}

void subtilis_rv_instr_dump(subtilis_rv_instr_t *instr)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	switch (instr->etype) {
	case SUBTILIS_RV_R_TYPE:
		prv_dump_r(NULL, NULL, instr->itype, instr->etype,
			   &instr->operands.r, &err);
		break;
	case SUBTILIS_RV_I_TYPE:
		prv_dump_i(NULL, NULL, instr->itype, instr->etype,
			   &instr->operands.i, &err);
		break;
//	SUBTILIS_RV_FENCE_TYPE
	case SUBTILIS_RV_S_TYPE:
	case SUBTILIS_RV_B_TYPE:
		prv_dump_sb(NULL, NULL, instr->itype, instr->etype,
			   &instr->operands.sb, &err);
		break;
	case SUBTILIS_RV_U_TYPE:
	case SUBTILIS_RV_J_TYPE:
		prv_dump_uj(NULL, NULL, instr->itype, instr->etype,
			    &instr->operands.uj, &err);
		break;
	case SUBTILIS_RV_REAL_R_TYPE:
		prv_dump_real_r(NULL, NULL, instr->itype, instr->etype,
				&instr->operands.rr, &err);
		break;
	case SUBTILIS_RV_REAL_R4_TYPE:
		prv_dump_real_r4(NULL, NULL, instr->itype, instr->etype,
				&instr->operands.r4, &err);
		break;
	case SUBTILIS_RV_REAL_I_TYPE:
		prv_dump_real_i(NULL, NULL, instr->itype, instr->etype,
			   &instr->operands.i, &err);
		break;
	case SUBTILIS_RV_REAL_S_TYPE:
		prv_dump_real_s(NULL, NULL, instr->itype, instr->etype,
				&instr->operands.sb, &err);
		break;
	case SUBTILIS_RV_LDRC_F_TYPE:
		prv_dump_ldrc_f(NULL, NULL, instr->itype, instr->etype,
				&instr->operands.ldrc, &err);
		break;
	default:
		break;

	}
}
