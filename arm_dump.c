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
	"AND", // SUBTILIS_ARM_INSTR_AND
	"EOR", // SUBTILIS_ARM_INSTR_EOR
	"SUB", // SUBTILIS_ARM_INSTR_SUB
	"RSB", // SUBTILIS_ARM_INSTR_RSB
	"ADD", // SUBTILIS_ARM_INSTR_ADD
	"ADC", // SUBTILIS_ARM_INSTR_ADC
	"SBC", // SUBTILIS_ARM_INSTR_SBC
	"RSC", // SUBTILIS_ARM_INSTR_RSC
	"TST", // SUBTILIS_ARM_INSTR_TST
	"TEQ", // SUBTILIS_ARM_INSTR_TEQ
	"CMP", // SUBTILIS_ARM_INSTR_CMP
	"CMN", // SUBTILIS_ARM_INSTR_CMN
	"ORR", // SUBTILIS_ARM_INSTR_ORR
	"MOV", // SUBTILIS_ARM_INSTR_MOV
	"BIC", // SUBTILIS_ARM_INSTR_BIC
	"MVN", // SUBTILIS_ARM_INSTR_MVN
	"MUL", // SUBTILIS_ARM_INSTR_MUL
	"MLA", // SUBTILIS_ARM_INSTR_MLA
	"LDR", // SUBTILIS_ARM_INSTR_LDR
	"STR", // SUBTILIS_ARM_INSTR_STR
	"LDM", // SUBTILIS_ARM_INSTR_LDM
	"STM", // SUBTILIS_ARM_INSTR_STM
	"B",   // SUBTILIS_ARM_INSTR_B
	"SWI", // SUBTILIS_ARM_INSTR_SWI
	"LDR", //SUBTILIS_ARM_INSTR_LDRC
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
		printf("R%zu", op2->op.reg.num);
		break;
	case SUBTILIS_ARM_OP2_I32:
		printf("#%d", op2->op.integer);
		break;
	default:
		if (op2->op.shift.shift_reg)
			printf("R%zu, %s R%zu", op2->op.shift.reg.num,
			       shift_desc[op2->op.shift.type],
			       op2->op.shift.shift.reg.num);
		else
			printf("R%zu, %s #%d", op2->op.shift.reg.num,
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
	printf(" R%zu, ", instr->dest.num);
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
	printf(" R%zu, R%zu, ", instr->dest.num, instr->op1.num);
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
	printf(" R%zu, R%zu, R%zu\n", instr->dest.num, instr->rm.num,
	       instr->rs.num);
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
	printf(" R%zu", instr->dest.num);
	if (instr->pre_indexed) {
		printf(", [R%zu, %s", instr->base.num, sub);
		prv_dump_op2(&instr->offset);
		printf("]");
		if (instr->write_back)
			printf("!");
	} else {
		printf(", [R%zu], %s", instr->base.num, sub);
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

	printf("\t%s%s %zu", instr_desc[type], direction[(size_t)instr->type],
	       instr->op0.num);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
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
	printf("}\n");
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
		if (p)
			printf(" %s\n",
			       p->string_pool->strings[instr->target.label]);
		else
			printf(" %d\n", (int32_t)instr->target.label + 2);
	} else if (p) {
		printf(" label_%zu\n", instr->target.label);
	} else {
		printf(" %d\n", (int32_t)instr->target.label + 2);
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
	printf(" R%zu, label_%zu\n", instr->dest.num, instr->label);
}

static void prv_dump_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	printf("\t%s", instr_desc[type]);
	if (instr->ccode != SUBTILIS_ARM_CCODE_AL)
		printf("%s", ccode_desc[instr->ccode]);
	printf(" R%zu, ", instr->op1.num);
	prv_dump_op2(&instr->op2);
	printf("\n");
}

static void prv_dump_label(void *user_data, subtilis_arm_op_t *op, size_t label,
			   subtilis_error_t *err)
{
	printf(".label_%zu\n", label);
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
	walker.data_fn = prv_dump_data_instr;
	walker.mul_fn = prv_dump_mul_instr;
	walker.cmp_fn = prv_dump_cmp_instr;
	walker.mov_fn = prv_dump_mov_instr;
	walker.stran_fn = prv_dump_stran_instr;
	walker.mtran_fn = prv_dump_mtran_instr;
	walker.br_fn = prv_dump_br_instr;
	walker.swi_fn = prv_dump_swi_instr;
	walker.ldrc_fn = prv_dump_ldrc_instr;

	subtilis_arm_walk(s, &walker, &err);

	for (i = 0; i < s->constant_count; i++) {
		printf(".label_%zu\n", s->constants[i].label);
		printf("\tEQUD &%x\n", s->constants[i].integer);
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
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
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
	default:
		printf("\tUNKNOWN INSTRUCTION\n");
		break;
	}
}
