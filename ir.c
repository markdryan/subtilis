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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "ir.h"

subtilis_ir_program_t *subtilis_ir_program_new(subtilis_error_t *err)
{
	subtilis_ir_program_t *p = malloc(sizeof(*p));

	if (!p) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	p->max_len = 0;
	p->reg_counter = SUBTILIS_IR_REG_TEMP_START;
	p->label_counter = 0;
	p->len = 0;
	p->ops = NULL;

	return p;
}

static void prv_ensure_buffer(subtilis_ir_program_t *p, subtilis_error_t *err)
{
	subtilis_ir_op_t **new_ops;
	size_t new_max;

	if (p->len < p->max_len)
		return;

	new_max = p->max_len + SUBTILIS_CONFIG_PROGRAM_GRAN;
	new_ops = realloc(p->ops, new_max * sizeof(subtilis_ir_op_t *));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return;
	}
	p->max_len = new_max;
	p->ops = new_ops;
}

void subtilis_ir_program_delete(subtilis_ir_program_t *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < p->len; i++)
		free(p->ops[i]);
	free(p->ops);
	free(p);
}

size_t subtilis_ir_program_add_instr(subtilis_ir_program_t *p,
				     subtilis_op_instr_type_t type,
				     subtilis_ir_operand_t op1,
				     subtilis_ir_operand_t op2,
				     subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;

	op0.reg = p->reg_counter;
	subtilis_ir_program_add_instr_reg(p, type, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	p->reg_counter++;
	return op0.reg;
}

size_t subtilis_ir_program_add_instr2(subtilis_ir_program_t *p,
				      subtilis_op_instr_type_t type,
				      subtilis_ir_operand_t op1,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;

	memset(&op2, 0, sizeof(op2));
	return subtilis_ir_program_add_instr(p, type, op1, op2, err);
}

void subtilis_ir_program_add_instr_no_reg(subtilis_ir_program_t *p,
					  subtilis_op_instr_type_t type,
					  subtilis_ir_operand_t op0,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	memset(&op1, 0, sizeof(op1));
	memset(&op2, 0, sizeof(op2));
	subtilis_ir_program_add_instr_reg(p, type, op0, op1, op2, err);
}

void subtilis_ir_program_add_instr_reg(subtilis_ir_program_t *p,
				       subtilis_op_instr_type_t type,
				       subtilis_ir_operand_t op0,
				       subtilis_ir_operand_t op1,
				       subtilis_ir_operand_t op2,
				       subtilis_error_t *err)
{
	subtilis_ir_op_t *op;
	subtilis_ir_inst_t *instr;

	prv_ensure_buffer(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}

	op->type = SUBTILIS_OP_INSTR;
	instr = &op->op.instr;
	instr->type = type;
	instr->operands[0] = op0;
	instr->operands[1] = op1;
	instr->operands[2] = op2;

	p->ops[p->len++] = op;
}

/* The ordering of this table is very important.  The functions it
 * contains must correspond to the enumerated types in
 * subtilis_op_instr_type_t
 */

typedef void (*subtilis_ir_dump_fn)(subtilis_ir_op_t *op);

struct subtilis_ir_op_desc_t_ {
	const char *const name;
	subtilis_ir_dump_fn fn;
};

typedef struct subtilis_ir_op_desc_t_ subtilis_ir_op_desc_t;

static void prv_dump_reg_reg_reg(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, r%zu, r%zu", instr->operands[0].reg,
	       instr->operands[1].reg, instr->operands[2].reg);
}

static void prv_dump_reg_reg_i32(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, r%zu, #%d", instr->operands[0].reg,
	       instr->operands[1].reg, instr->operands[2].integer);
}

static void prv_dump_reg_reg_real(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, r%zu, #%lf", instr->operands[0].reg,
	       instr->operands[1].reg, instr->operands[2].real);
}

static void prv_dump_reg_label_label(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, label_%zu, label_%zu", instr->operands[0].reg,
	       instr->operands[1].label, instr->operands[2].label);
}

static void prv_dump_reg_i32(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, #%d", instr->operands[0].reg, instr->operands[1].integer);
}

static void prv_dump_reg_real(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, #%lf", instr->operands[0].reg, instr->operands[1].real);
}

static void prv_dump_reg_reg(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu, r%zu", instr->operands[0].reg, instr->operands[1].reg);
}

static void prv_dump_reg(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("r%zu", instr->operands[0].reg);
}

static void prv_dump_label(subtilis_ir_op_t *op)
{
	subtilis_ir_inst_t *instr = &op->op.instr;

	printf("label_%zu", instr->operands[0].reg);
}

/* clang-format off */
static const subtilis_ir_op_desc_t op_dump_fns[] = {
	{ "addi32", prv_dump_reg_reg_reg },  /* SUBTILIS_OP_INSTR_ADD_I32 */
	{ "addr", prv_dump_reg_reg_reg },    /* SUBTILIS_OP_INSTR_ADD_REAL */
	{ "subi32", prv_dump_reg_reg_reg },  /* SUBTILIS_OP_INSTR_SUB_I32 */
	{ "subr", prv_dump_reg_reg_reg },    /* SUBTILIS_OP_INSTR_SUB_REAL */
	{ "muli32", prv_dump_reg_reg_reg },  /* SUBTILIS_OP_INSTR_MUL_I32 */
	{ "mulr", prv_dump_reg_reg_reg },    /* SUBTILIS_OP_INSTR_MUL_REAL */
	{ "divi32", prv_dump_reg_reg_reg },  /* SUBTILIS_OP_INSTR_DIV_I32 */
	{ "divr", prv_dump_reg_reg_reg },    /* SUBTILIS_OP_INSTR_DIV_REAL */
	{ "addii32", prv_dump_reg_reg_i32 }, /* SUBTILIS_OP_INSTR_ADDI_I32 */
	{ "addir", prv_dump_reg_reg_real },  /* SUBTILIS_OP_INSTR_ADDI_REAL */
	{ "subii32", prv_dump_reg_reg_i32 }, /* SUBTILIS_OP_INSTR_SUBI_I32 */
	{ "subir", prv_dump_reg_reg_real },  /* SUBTILIS_OP_INSTR_SUBI_REAL */
	{ "mulii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_MULI_I32 */
	{ "mulir", prv_dump_reg_reg_real},   /* SUBTILIS_OP_INSTR_MULI_REAL */
	{ "divii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_DIVI_I32 */
	{ "divir", prv_dump_reg_reg_i32},    /* SUBTILIS_OP_INSTR_DIVI_REAL */
	{ "loadoi32", prv_dump_reg_reg_i32}, /* SUBTILIS_OP_INSTR_LOADO_I32 */
	{ "loador", prv_dump_reg_reg_real},  /* SUBTILIS_OP_INSTR_LOADO_REAL */
	{ "loadi32", prv_dump_reg_reg},      /* SUBTILIS_OP_INSTR_LOAD_I32 */
	{ "loadr", prv_dump_reg_reg},        /* SUBTILIS_OP_INSTR_LOAD_REAL */
	{ "storeoi32", prv_dump_reg_reg_i32},/* SUBTILIS_OP_INSTR_STOREO_I32 */
	{ "storeor", prv_dump_reg_real},     /* SUBTILIS_OP_INSTR_STOREO_REAL */
	{ "storei32", prv_dump_reg_reg},     /* SUBTILIS_OP_INSTR_STORE_I32 */
	{ "storer", prv_dump_reg_reg},       /* SUBTILIS_OP_INSTR_STORE_REAL */
	{ "movii32", prv_dump_reg_i32},      /* SUBTILIS_OP_INSTR_MOVI_I32 */
	{ "movir", prv_dump_reg_real},       /* SUBTILIS_OP_INSTR_MOV_REAL */
	{ "mov", prv_dump_reg_reg},          /* SUBTILIS_OP_INSTR_MOV */
	{ "movfp", prv_dump_reg_reg},        /* SUBTILIS_OP_INSTR_MOVFP */
	{ "printi32", prv_dump_reg},         /* SUBTILIS_OP_INSTR_PRINT_I32 */
	{ "rsubii32", prv_dump_reg_reg_i32 },/* SUBTILIS_OP_INSTR_RSUBI_I32 */
	{ "rsubir", prv_dump_reg_reg_real }, /* SUBTILIS_OP_INSTR_RSUBI_REAL */
	{ "rdivii32", prv_dump_reg_reg_i32}, /* SUBTILIS_OP_INSTR_RDIVI_I32 */
	{ "rdivir", prv_dump_reg_reg_i32},   /* SUBTILIS_OP_INSTR_RDIVI_REAL */
	{ "andi32", prv_dump_reg_reg_reg},   /* SUBTILIS_OP_INSTR_AND_I32 */
	{ "andii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_ANDI_I32 */
	{ "ori32", prv_dump_reg_reg_reg},    /* SUBTILIS_OP_INSTR_OR_I32 */
	{ "orii32", prv_dump_reg_reg_i32},   /* SUBTILIS_OP_INSTR_ORI_I32 */
	{ "eori32", prv_dump_reg_reg_reg},   /* SUBTILIS_OP_INSTR_EOR_I32 */
	{ "eorii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_EORI_I32 */
	{ "noti32", prv_dump_reg_reg},       /* SUBTILIS_OP_INSTR_NOT_I32 */
	{ "eqi32", prv_dump_reg_reg_reg},    /* SUBTILIS_OP_INSTR_EQ_I32 */
	{ "eqii32", prv_dump_reg_reg_i32},   /* SUBTILIS_OP_INSTR_EQI_I32 */
	{ "neqi32", prv_dump_reg_reg_reg},   /* SUBTILIS_OP_INSTR_NEQ_I32 */
	{ "neqii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_NEQI_I32 */
	{ "gti32", prv_dump_reg_reg_reg},    /* SUBTILIS_OP_INSTR_GT_I32 */
	{ "gtii32", prv_dump_reg_reg_i32},   /* SUBTILIS_OP_INSTR_GTII_I32 */
	{ "ltei32", prv_dump_reg_reg_reg},   /* SUBTILIS_OP_INSTR_LTE_I32 */
	{ "lteii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_LTEI_I32 */
	{ "lti32", prv_dump_reg_reg_reg},    /* SUBTILIS_OP_INSTR_LT_I32 */
	{ "ltii32", prv_dump_reg_reg_i32},   /* SUBTILIS_OP_INSTR_LTII_I32 */
	{ "gtei32", prv_dump_reg_reg_reg},   /* SUBTILIS_OP_INSTR_GTE_I32 */
	{ "gteii32", prv_dump_reg_reg_i32},  /* SUBTILIS_OP_INSTR_GTEI_I32 */
	{ "jmpc", prv_dump_reg_label_label}, /* SUBTILIS_OP_INSTR_JMPC */
	{ "jmp", prv_dump_label},            /* SUBTILIS_OP_INSTR_JMP */
};

/* clang-format on */

void subtilis_ir_program_dump(subtilis_ir_program_t *p)
{
	size_t i;
	subtilis_op_instr_type_t itype;

	for (i = 0; i < p->len; i++) {
		if (!p->ops[i])
			continue;
		if (p->ops[i]->type == SUBTILIS_OP_INSTR) {
			itype = p->ops[i]->op.instr.type;
			printf("\t%s ", op_dump_fns[itype].name);
			op_dump_fns[itype].fn(p->ops[i]);
		} else if (p->ops[i]->type == SUBTILIS_OP_LABEL) {
			printf("label_%zu", p->ops[i]->op.label);
		} else {
			continue;
		}
		printf("\n");
	}
}

size_t subtilis_ir_program_new_label(subtilis_ir_program_t *p)
{
	return p->label_counter++;
}

void subtilis_ir_program_add_label(subtilis_ir_program_t *p, size_t l,
				   subtilis_error_t *err)
{
	subtilis_ir_op_t *op;

	prv_ensure_buffer(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}
	op->type = SUBTILIS_OP_LABEL;
	op->op.label = l;
	p->ops[p->len++] = op;
}
