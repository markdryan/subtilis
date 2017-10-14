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

struct subtilis_ir_op_desc_t_ {
	const char *const name;
	subtilis_op_instr_class_t cls;
};

typedef struct subtilis_ir_op_desc_t_ subtilis_ir_op_desc_t;

/* The ordering of this table is very important.  The entries in
   it must correspond to the enumerated types in
 * subtilis_op_instr_type_t
 */

/* clang-format off */
static const subtilis_ir_op_desc_t op_desc[] = {
	{ "addi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "addr", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "subi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "subr", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "muli32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "mulr", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "divi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "divr", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "addii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "addir", SUBTILIS_OP_CLASS_REG_REG_REAL },
	{ "subii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "subir", SUBTILIS_OP_CLASS_REG_REG_REAL },
	{ "mulii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "mulir", SUBTILIS_OP_CLASS_REG_REG_REAL},
	{ "divii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "divir", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "loadoi32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "loador", SUBTILIS_OP_CLASS_REG_REG_REAL},
	{ "loadi32", SUBTILIS_OP_CLASS_REG_REG},
	{ "loadr", SUBTILIS_OP_CLASS_REG_REG},
	{ "storeoi32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "storeor", SUBTILIS_OP_CLASS_REG_REAL},
	{ "storei32", SUBTILIS_OP_CLASS_REG_REG},
	{ "storer", SUBTILIS_OP_CLASS_REG_REG},
	{ "movii32", SUBTILIS_OP_CLASS_REG_I32},
	{ "movir", SUBTILIS_OP_CLASS_REG_REAL},
	{ "mov", SUBTILIS_OP_CLASS_REG_REG},
	{ "movfp", SUBTILIS_OP_CLASS_REG_REG},
	{ "printi32", SUBTILIS_OP_CLASS_REG},
	{ "rsubii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "rsubir", SUBTILIS_OP_CLASS_REG_REG_REAL },
	{ "rdivii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "rdivir", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "andi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "andii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "ori32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "orii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "eori32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "eorii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "noti32", SUBTILIS_OP_CLASS_REG_REG},
	{ "eqi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "eqii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "neqi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "neqii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "gti32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "gtii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "ltei32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "lteii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "lti32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "ltii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "gtei32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "gteii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "jmpc", SUBTILIS_OP_CLASS_REG_LABEL_LABEL},
	{ "jmp", SUBTILIS_OP_CLASS_LABEL},
};

/* clang-format on */

static void prv_dump_instr(subtilis_ir_inst_t *instr)
{
	printf("\t%s ", op_desc[instr->type].name);
	switch (op_desc[instr->type].cls) {
	case SUBTILIS_OP_CLASS_REG_REG_REG:
		printf("r%zu, r%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_REG_I32:
		printf("r%zu, r%zu, #%d", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].integer);
		break;
	case SUBTILIS_OP_CLASS_REG_REG_REAL:
		printf("r%zu, r%zu, #%lf", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].real);
		break;
	case SUBTILIS_OP_CLASS_REG_LABEL_LABEL:
		printf("r%zu, label_%zu, label_%zu", instr->operands[0].reg,
		       instr->operands[1].label, instr->operands[2].label);
		break;
	case SUBTILIS_OP_CLASS_REG_I32:
		printf("r%zu, #%d", instr->operands[0].reg,
		       instr->operands[1].integer);
		break;
	case SUBTILIS_OP_CLASS_REG_REAL:
		printf("r%zu, #%lf", instr->operands[0].reg,
		       instr->operands[1].real);
		break;
	case SUBTILIS_OP_CLASS_REG_REG:
		printf("r%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg);
		break;
	case SUBTILIS_OP_CLASS_REG:
		printf("r%zu", instr->operands[0].reg);
		break;
	case SUBTILIS_OP_CLASS_LABEL:
		printf("r%zu", instr->operands[0].reg);
		break;
	}
}

void subtilis_ir_program_dump(subtilis_ir_program_t *p)
{
	size_t i;

	for (i = 0; i < p->len; i++) {
		if (!p->ops[i])
			continue;
		if (p->ops[i]->type == SUBTILIS_OP_INSTR)
			prv_dump_instr(&p->ops[i]->op.instr);
		else if (p->ops[i]->type == SUBTILIS_OP_LABEL)
			printf("label_%zu", p->ops[i]->op.label);
		 else
			continue;
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

