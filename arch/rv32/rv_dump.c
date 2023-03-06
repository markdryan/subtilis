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
	"nop",      // SUBTILIS_RV_NOP,
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
	"hint",     // SUBTILIS_RV_HINT,
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
	if ((itype == SUBTILIS_RV_ADDI) && (i->rs1 == 0)) {
		if (i->rd == 0) {
			printf("\tnop\n");
		} else {
			printf("\tli x%zu, %d\n", i->rd, i->imm);
		}
		return;
	}

	if ((itype == SUBTILIS_RV_ECALL) || (itype == SUBTILIS_RV_EBREAK)) {
		printf("\t%s\n", instr_desc[itype]);
		return;
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
		       sb->rs2, sb->imm, sb->rs1);
		return;
	}
	printf("\t%s x%zu, x%zu, %d\n", instr_desc[itype],
	       sb->rs1, sb->rs2, sb->imm);
}

static void prv_dump_uj(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_ujtype_t *uj, subtilis_error_t *err)
{
	if ((itype == SUBTILIS_RV_JAL) && (uj->rd == 0)) {
		printf("\tj  label_%d\n", uj->imm);
		return;
	}
	if (itype == SUBTILIS_RV_LUI) {
		printf("\t%s x%zu, %d\n", instr_desc[itype], uj->rd,
		       uj->imm << 12);
		return;
	}
	printf("\t%s x%zu, %d\n", instr_desc[itype], uj->rd, uj->imm);
}

void subtilis_rv_section_dump(subtilis_rv_prog_t *p,
			      subtilis_rv_section_t *s)
{
	subtilis_rv_walker_t walker;
	subtilis_error_t err;

	subtilis_error_init(&err);

	walker.user_data = p;
	walker.label_fn = prv_dump_label;
	walker.directive_fn = prv_dump_directive;
	walker.r_fn = prv_dump_r;
	walker.i_fn = prv_dump_i;
	walker.sb_fn = prv_dump_sb;
	walker.uj_fn = prv_dump_uj;

	subtilis_rv_walk(s, &walker, &err);
/*
	for (i = 0; i < s->constants.real_count; i++) {
		printf(".label_%zu\n", s->constants.real[i].label);
		printf("\tEQUFD %f\n", s->constants.real[i].real);
	}
	for (i = 0; i < s->constants.ui32_count; i++) {
		printf(".label_%zu\n", s->constants.ui32[i].label);
		printf("\tEQUD &%" PRIx32 "\n", s->constants.ui32[i].integer);
	}
*/
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
	default:
		break;

	}
}
