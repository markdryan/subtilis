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

#include <errno.h>
#include <limits.h>
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

typedef enum {
	SUBTILIS_IR_OPERAND_REGISTER,
	SUBTILIS_IR_OPERAND_I32,
	SUBTILIS_IR_OPERAND_REAL,
	SUBTILIS_IR_OPERAND_LABEL,
} subtilis_ir_operand_class_t;

struct subtilis_ir_class_info_t_ {
	size_t op_count;
	subtilis_ir_operand_class_t classes[3];
};

typedef struct subtilis_ir_class_info_t_ subtilis_ir_class_info_t;

/* The ordering of this table is very important.  The entries in
 * it must correspond to the enumerated types in
 * subtilis_op_instr_type_t
 */

/* TODO We should probably order this by alpabetical ordering
 * of instructions so we can do a bsearch
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

/*
 * Must match the order of the enumerated types declared in
 * subtilis_op_instr_class_t.
 */

static const subtilis_ir_class_info_t class_details[] = {
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_REGISTER} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_I32} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_REAL} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_LABEL,
	      SUBTILIS_IR_OPERAND_LABEL} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_I32} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REAL} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER} },
	{1, { SUBTILIS_IR_OPERAND_REGISTER} },
	{1, { SUBTILIS_IR_OPERAND_LABEL} }
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

/*
 * Not using strsep as not sure it will be available on all C compilers
 * subtilis will be compiled with.
 */

static bool prv_is_white_space(const char *s)
{
	return *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r';
}

#define LABEL_STR_SIZE (sizeof("label_") - 1)

static void prv_parse_label(const char *text, subtilis_ir_label_match_t *label,
			    subtilis_error_t *err)
{
	unsigned long num;
	char *end_ptr = 0;

	if (!strcmp(text, "*")) {
		label->label = 0;
		label->op_match = SUBTILIS_OP_MATCH_ANY;
		return;
	}

	errno = 0;
	num = strtoul(text, &end_ptr, 10);
	if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}

	label->label = (size_t)num;
	label->op_match = SUBTILIS_OP_MATCH_FLOATING;
}

static const char *prv_parse_operands(const char *rule,
				      subtilis_ir_op_match_t *match,
				      subtilis_error_t *err)
{
	const int max_op = 32;
	char operand[max_op];
	size_t i;
	size_t j;
	const subtilis_ir_class_info_t *details;
	unsigned long num;
	const char *num_ptr;
	char *end_ptr = 0;
	subtilis_ir_label_match_t label;

	details = &class_details[op_desc[match->op.instr.type].cls];

	for (j = 0; j < details->op_count; j++) {
		while (prv_is_white_space(rule))
			rule++;

		for (i = 0; *rule && i < max_op && !prv_is_white_space(rule);
		     i++) {
			if (*rule == ',') {
				rule++;
				break;
			}
			operand[i] = *rule++;
		}
		operand[i] = 0;
		if (!strcmp(operand, "*")) {
			match->op.instr.op_match[j] = SUBTILIS_OP_MATCH_ANY;
		} else if (!strncmp(operand, "label_", LABEL_STR_SIZE)) {
			if (details->classes[j] != SUBTILIS_IR_OPERAND_LABEL) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			}
			prv_parse_label(operand + LABEL_STR_SIZE, &label, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return NULL;
			match->op.instr.operands[j].label = label.label;
			match->op.instr.op_match[j] = label.op_match;
		} else if (operand[0] == '#') {
			if (details->classes[j] != SUBTILIS_IR_OPERAND_I32) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			}
			errno = 0;
			num = strtoul(operand + 1, &end_ptr, 10);
			if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			}
			match->op.instr.operands[j].integer = num;
			match->op.instr.op_match[j] = SUBTILIS_OP_MATCH_FIXED;
		} else if (operand[0] == 'r') {
			if (details->classes[j] !=
			    SUBTILIS_IR_OPERAND_REGISTER) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			}
			num_ptr = operand + 1;
			if (operand[1] == 0) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			} else if (operand[1] == '_') {
				match->op.instr.op_match[j] =
				    SUBTILIS_OP_MATCH_FLOATING;
				num_ptr++;
			} else {
				match->op.instr.op_match[j] =
				    SUBTILIS_OP_MATCH_FIXED;
			}
			errno = 0;
			num = strtoul(num_ptr, &end_ptr, 10);
			if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
				subtilis_error_set_asssertion_failed(err);
				return NULL;
			}
			match->op.instr.operands[j].reg = num;
		}
	}

	return rule;
}

static const char *prv_parse_match(const char *rule,
				   subtilis_ir_op_match_t *match,
				   subtilis_error_t *err)
{
	const int max_instr = 32;
	char instr[max_instr];
	size_t i;

	while (prv_is_white_space(rule))
		rule++;
	if (!rule[0])
		return NULL;

	for (i = 0; *rule && i < max_instr && !prv_is_white_space(rule); i++)
		instr[i] = *rule++;
	instr[i] = 0;

	if (!strncmp(instr, "label_", LABEL_STR_SIZE)) {
		match->type = SUBTILIS_OP_LABEL;
		prv_parse_label(instr + LABEL_STR_SIZE, &match->op.label, err);
		return rule;
	}

	for (i = 0; i < sizeof(op_desc) / sizeof(subtilis_ir_op_desc_t); i++)
		if (!strcmp(instr, op_desc[i].name))
			break;

	if (i == sizeof(op_desc) / sizeof(subtilis_ir_op_desc_t)) {
		subtilis_error_set_asssertion_failed(err);
		return NULL;
	}

	match->type = SUBTILIS_OP_INSTR;
	match->op.instr.type = i;
	return prv_parse_operands(rule, match, err);
}

void subtilis_ir_parse_rules(const subtilis_ir_rule_raw_t *raw,
			     subtilis_ir_rule_t *parsed, size_t count,
			     subtilis_error_t *err)
{
	size_t i;
	size_t j;
	const char *rule;

	for (i = 0; i < count; i++) {
		rule = raw[i].text;
		j = 0;
		while (
		    j < SUBTILIS_IR_MAX_MATCHES &&
		    (rule = prv_parse_match(rule, &parsed[i].matches[j], err)))
			j++;
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (j == 0) {
			subtilis_error_set_asssertion_failed(err);
			return;
		}
		parsed[i].action = raw[i].action;
		parsed[i].matches_count = j;
	}
}

#define MAX_MATCH_WILDCARDS 16

struct subtilis_ir_match_pair_t_ {
	size_t key;
	size_t value;
};

typedef struct subtilis_ir_match_pair_t_ subtilis_ir_match_pair_t;

struct subtilis_ir_match_state_t_ {
	subtilis_ir_match_pair_t regs[MAX_MATCH_WILDCARDS];
	subtilis_ir_match_pair_t labels[MAX_MATCH_WILDCARDS];
	size_t regs_count;
	size_t labels_count;
};

typedef struct subtilis_ir_match_state_t_ subtilis_ir_match_state_t;

static bool prv_process_floating_label(subtilis_ir_match_state_t *state,
				       size_t instr_label, size_t match_label,
				       subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_match_pair_t *pair;

	for (i = 0;
	     i < state->labels_count && state->labels[i].key != match_label;
	     i++)
		;
	if (i == state->labels_count) {
		if (i == MAX_MATCH_WILDCARDS) {
			subtilis_error_set_asssertion_failed(err);
			return false;
		}
		pair = &state->labels[i];
		pair->key = match_label;
		pair->value = instr_label;
		state->labels_count++;
		return true;
	}

	return state->labels[i].value == instr_label;
}

static bool prv_process_floating_reg(subtilis_ir_match_state_t *state,
				     size_t instr_reg, size_t match_reg,
				     subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_match_pair_t *pair;

	for (i = 0; i < state->regs_count && state->regs[i].key != match_reg;
	     i++)
		;
	if (i == state->regs_count) {
		if (i == MAX_MATCH_WILDCARDS) {
			subtilis_error_set_asssertion_failed(err);
			return false;
		}
		pair = &state->regs[i];
		pair->key = match_reg;
		pair->value = instr_reg;
		state->regs_count++;
		return true;
	}

	return state->regs[i].value == instr_reg;
}

static bool prv_match_op(subtilis_ir_op_t *op, subtilis_ir_op_match_t *match,
			 subtilis_ir_match_state_t *state,
			 subtilis_error_t *err)
{
	size_t j;
	subtilis_ir_inst_t *instr;
	subtilis_ir_inst_match_t *match_instr;
	const subtilis_ir_class_info_t *details;

	if (op->type != match->type)
		return false;
	if (op->type == SUBTILIS_OP_LABEL)
		return prv_process_floating_label(state, op->op.label,
						  match->op.label.label, err);

	instr = &op->op.instr;
	match_instr = &match->op.instr;
	if (instr->type != match_instr->type)
		return false;

	details = &class_details[op_desc[match->op.instr.type].cls];

	for (j = 0; j < details->op_count; j++) {
		if (match_instr->op_match[j] == SUBTILIS_OP_MATCH_ANY)
			continue;
		if (match_instr->op_match[j] == SUBTILIS_OP_MATCH_FIXED) {
			switch (details->classes[j]) {
			case SUBTILIS_IR_OPERAND_REGISTER:
				if (match_instr->operands[j].reg ==
				    instr->operands[j].reg)
					continue;
				break;
			case SUBTILIS_IR_OPERAND_I32:
				if (match_instr->operands[j].integer ==
				    instr->operands[j].integer)
					continue;
				break;
			case SUBTILIS_IR_OPERAND_REAL:
				if (match_instr->operands[j].real ==
				    instr->operands[j].real)
					continue;
				break;
			default:
				subtilis_error_set_asssertion_failed(err);
				return false;
			}
			return false;
		}
		switch (details->classes[j]) {
		case SUBTILIS_IR_OPERAND_REGISTER:
			if (prv_process_floating_reg(
				state, instr->operands[j].reg,
				match_instr->operands[j].reg, err))
				continue;
			break;
		case SUBTILIS_IR_OPERAND_LABEL:
			if (prv_process_floating_label(
				state, instr->operands[j].label,
				match_instr->operands[j].label, err))
				continue;
			break;
		default:
			subtilis_error_set_asssertion_failed(err);
			return false;
		}
		return false;
	}

	return true;
}

void subtilis_ir_match(subtilis_ir_program_t *p, subtilis_ir_rule_t *rules,
		       size_t rule_count, void *user_data,
		       subtilis_error_t *err)
{
	size_t pc;
	size_t i;
	size_t j;
	subtilis_ir_match_state_t state;
	subtilis_ir_op_t *op;

	for (pc = 0; pc < p->len;) {
		for (i = 0; i < rule_count; i++) {
			state.regs_count = 0;
			state.labels_count = 0;
			for (j = 0; j < rules[i].matches_count; j++) {
				op = p->ops[pc + j];
				if (!prv_match_op(op, &rules[i].matches[j],
						  &state, err))
					break;
			}
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (j == rules[i].matches_count) {
				rules[i].action(p, pc, user_data, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				pc += j;
				break;
			}
		}
		if (i == rule_count) {
			subtilis_error_set_asssertion_failed(err);
			return;
		}
	}
}
