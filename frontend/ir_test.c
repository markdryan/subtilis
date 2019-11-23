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
#include <string.h>

#include "../common/ir.h"
#include "ir_test.h"
#include "parser_test.h"

static int prv_validate_instruction(subtilis_ir_op_match_t *matches,
				    subtilis_op_instr_type_t typ)
{
	subtilis_ir_inst_match_t *instr;

	if (matches->type != SUBTILIS_OP_INSTR) {
		fprintf(stderr, "Expected match type %d got %d\n",
			SUBTILIS_OP_INSTR, matches->type);
		return 1;
	}
	instr = &matches->op.instr;
	if (instr->type != typ) {
		fprintf(stderr, "Expected instr type %d got %d\n", typ,
			instr->type);
		return 1;
	}

	return 0;
}

static int prv_test_wildcards_rule(void)
{
	subtilis_ir_rule_t parsed;
	subtilis_error_t err;
	subtilis_ir_op_match_t *matches;
	subtilis_ir_inst_match_t *instr;
	const subtilis_ir_rule_raw_t raw = {"movii32 *, *", NULL};

	printf("ir_test_wildcards_rule");

	subtilis_error_init(&err);
	subtilis_ir_parse_rules(&raw, &parsed, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto fail;
	}
	matches = &parsed.matches[0];
	if (prv_validate_instruction(matches, SUBTILIS_OP_INSTR_MOVI_I32) != 0)
		goto fail;
	instr = &matches->op.instr;
	if ((instr->op_match[0] != SUBTILIS_OP_MATCH_ANY) ||
	    (instr->op_match[1] != SUBTILIS_OP_MATCH_ANY)) {
		fprintf(stderr, "op_match values incorrect %d %d\n",
			instr->op_match[0], instr->op_match[1]);
		goto fail;
	}

	printf(": [OK]\n");

	return 0;

fail:
	printf(": [FAIL]\n");

	return 1;
}

static int prv_test_i32_rule(void)
{
	subtilis_ir_rule_t parsed;
	subtilis_error_t err;
	subtilis_ir_op_match_t *matches;
	subtilis_ir_inst_match_t *instr;
	const subtilis_ir_rule_raw_t raw = {"movii32 *, #3", NULL};

	printf("ir_test_i32_rule");

	subtilis_error_init(&err);
	subtilis_ir_parse_rules(&raw, &parsed, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto fail;
	}
	matches = &parsed.matches[0];
	if (prv_validate_instruction(matches, SUBTILIS_OP_INSTR_MOVI_I32) != 0)
		goto fail;
	instr = &matches->op.instr;
	if ((instr->op_match[0] != SUBTILIS_OP_MATCH_ANY) ||
	    (instr->op_match[1] != SUBTILIS_OP_MATCH_FIXED)) {
		fprintf(stderr, "op_match values incorrect %d %d\n",
			instr->op_match[0], instr->op_match[1]);
		goto fail;
	}

	if (instr->operands[1].integer != 3) {
		fprintf(stderr, "operand values incorrect %d\n",
			instr->operands[1].integer);
		goto fail;
	}

	printf(": [OK]\n");

	return 0;

fail:
	printf(": [FAIL]\n");

	return 1;
}

static int prv_test_regs_rule(void)
{
	subtilis_ir_rule_t parsed;
	subtilis_error_t err;
	subtilis_ir_op_match_t *matches;
	subtilis_ir_inst_match_t *instr;
	size_t i;
	const subtilis_ir_rule_raw_t raw = {"addi32 r0, r1, r2", NULL};

	printf("ir_test_regs_rule");

	subtilis_error_init(&err);
	subtilis_ir_parse_rules(&raw, &parsed, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto fail;
	}
	matches = &parsed.matches[0];
	if (prv_validate_instruction(matches, SUBTILIS_OP_INSTR_ADD_I32) != 0)
		goto fail;
	instr = &matches->op.instr;
	if ((instr->op_match[0] != SUBTILIS_OP_MATCH_FIXED) ||
	    (instr->op_match[1] != SUBTILIS_OP_MATCH_FIXED) ||
	    (instr->op_match[2] != SUBTILIS_OP_MATCH_FIXED)) {
		fprintf(stderr, "op_match values incorrect %d %d %d\n",
			instr->op_match[0], instr->op_match[1],
			instr->op_match[2]);
		goto fail;
	}

	for (i = 0; i < 3; i++) {
		if (instr->operands[i].reg != i) {
			fprintf(stderr, "operand value %zu incorrect %zu\n", i,
				instr->operands[i].reg);
			goto fail;
		}
	}

	printf(": [OK]\n");

	return 0;

fail:
	printf(": [FAIL]\n");

	return 1;
}

static int prv_test_floating_rule(void)
{
	subtilis_ir_rule_t parsed;
	subtilis_error_t err;
	subtilis_ir_op_match_t *matches;
	subtilis_ir_inst_match_t *instr;
	subtilis_ir_op_match_t *matches2;
	subtilis_ir_inst_match_t *instr2;
	subtilis_ir_op_match_t *matches3;
	subtilis_ir_label_match_t *label;
	const subtilis_ir_rule_raw_t raw = {"ltii32 r_1, *, *\n"
					    "jmpc r_1, label_2, *\n"
					    "label_2",
					    NULL};

	printf("ir_test_floating_rule");

	subtilis_error_init(&err);
	subtilis_ir_parse_rules(&raw, &parsed, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto fail;
	}
	matches = &parsed.matches[0];
	if (prv_validate_instruction(matches, SUBTILIS_OP_INSTR_LTI_I32) != 0)
		goto fail;
	instr = &matches->op.instr;
	if ((instr->op_match[0] != SUBTILIS_OP_MATCH_FLOATING) ||
	    (instr->op_match[1] != SUBTILIS_OP_MATCH_ANY) ||
	    (instr->op_match[2] != SUBTILIS_OP_MATCH_ANY)) {
		fprintf(stderr, "op_match values incorrect %d %d %d\n",
			instr->op_match[0], instr->op_match[1],
			instr->op_match[2]);
		goto fail;
	}

	matches2 = &parsed.matches[1];
	if (prv_validate_instruction(matches2, SUBTILIS_OP_INSTR_JMPC) != 0)
		goto fail;
	instr2 = &matches2->op.instr;
	if ((instr2->op_match[0] != SUBTILIS_OP_MATCH_FLOATING) ||
	    (instr2->op_match[1] != SUBTILIS_OP_MATCH_FLOATING) ||
	    (instr2->op_match[2] != SUBTILIS_OP_MATCH_ANY)) {
		fprintf(stderr, "op_match values incorrect %d %d %d\n",
			instr2->op_match[0], instr2->op_match[1],
			instr2->op_match[2]);
		goto fail;
	}

	matches3 = &parsed.matches[2];
	if (matches3->type != SUBTILIS_OP_LABEL) {
		fprintf(stderr, "Expected match type %d got %d\n",
			SUBTILIS_OP_LABEL, matches3->type);
		goto fail;
	}
	label = &matches3->op.label;
	if (label->op_match != SUBTILIS_OP_MATCH_FLOATING) {
		fprintf(stderr, "op_match value incorrect %d\n",
			label->op_match);
		goto fail;
	}

	if (label->label != instr2->operands[1].label) {
		fprintf(stderr, "label value incorrect. Expected %zu got %zu\n",
			instr2->operands[1].label, label->label);
		goto fail;
	}

	if (instr->operands[0].reg != instr2->operands[0].reg) {
		fprintf(stderr, "register incorrect. Expected %zu got %zu\n",
			instr2->operands[0].reg, instr->operands[0].reg);
		goto fail;
	}

	printf(": [OK]\n");

	return 0;

fail:
	printf(": [FAIL]\n");

	return 1;
}

static int prv_test_label_rule(void)
{
	subtilis_ir_rule_t parsed;
	subtilis_error_t err;
	subtilis_ir_op_match_t *matches;
	subtilis_ir_label_match_t *label;
	const subtilis_ir_rule_raw_t raw = {" label_1", NULL};

	printf("ir_test_label_rule");

	subtilis_error_init(&err);
	subtilis_ir_parse_rules(&raw, &parsed, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto fail;
	}
	matches = &parsed.matches[0];
	if (matches->type != SUBTILIS_OP_LABEL) {
		fprintf(stderr, "Expected match type %d got %d\n",
			SUBTILIS_OP_LABEL, matches->type);
		goto fail;
	}
	label = &matches->op.label;
	if (label->op_match != SUBTILIS_OP_MATCH_FLOATING) {
		fprintf(stderr, "op_match value incorrect %d\n",
			label->op_match);
		goto fail;
	}

	if (label->label != 1) {
		fprintf(stderr, "label value incorrect.  Expected 1 got %zu\n",
			label->label);
		goto fail;
	}

	printf(": [OK]\n");

	return 0;

fail:
	printf(": [FAIL]\n");

	return 1;
}

struct ir_test_matcher_data_t_ {
	int sequence[32];
	int count;
};

typedef struct ir_test_matcher_data_t_ ir_test_matcher_data_t;

static void prv_matched_1(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 1;
}

static void prv_matched_2(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 2;
}

static void prv_matched_3(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 3;
}

static void prv_matched_4(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 4;
}

static void prv_matched_5(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 5;
}

static void prv_matched_6(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 6;
}

static void prv_matched_7(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 7;
}

static void prv_matched_8(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	ir_test_matcher_data_t *d = (ir_test_matcher_data_t *)user_data;

	d->sequence[d->count++] = 8;
}

static int prv_check_matcher(subtilis_lexer_t *l, subtilis_parser_t *p,
			     subtilis_error_type_t expected_err,
			     const char *expected)
{
	ir_test_matcher_data_t data;
	subtilis_error_t err;
	int i;
	const subtilis_ir_rule_raw_t raw_rules[] = {
	    {"ltii32 r_1, *, *\n"
	     "jmpc r_1, label_1, *\n"
	     "label_1",
	     prv_matched_1},
	    {"ltii32 *, *, *", prv_matched_2},
	    {"movii32 *, #1", prv_matched_3},
	    {"movii32 *, *", prv_matched_4},
	    {"storeoi32 *, *, *", prv_matched_5},
	    {"loadoi32 *, *, *", prv_matched_6},
	    {"label_1", prv_matched_7},
	    {"end", prv_matched_8},
	};
	const size_t rule_count =
	    sizeof(raw_rules) / sizeof(subtilis_ir_rule_raw_t);
	subtilis_ir_rule_t parsed[rule_count];
	const size_t rule_order[] = {4, 5, 4, 5, 4, 5, 3, 5, 4, 5,
				     6, 1, 6, 2, 5, 7, 7, 7, 8};

	subtilis_error_init(&err);
	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	data.count = 0;

	subtilis_ir_parse_rules(raw_rules, parsed, rule_count, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	subtilis_ir_match(p->main, parsed, rule_count, &data, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	if (data.count != sizeof(rule_order) / sizeof(size_t)) {
		fprintf(stderr, "Bad number of rules, expected %zu got %d",
			sizeof(rule_order) / sizeof(size_t), data.count);
		return 1;
	}

	for (i = 0; i < data.count; i++)
		if (rule_order[i] != data.sequence[i]) {
			fprintf(stderr, "Bad Match, expected %zu got %d",
				rule_order[i], data.sequence[i]);
			return 1;
		}

	return 0;
}

static int prv_test_matcher(void)
{
	const char *source = "LET x% = 1\n"
			     "LET y% = 0\n"
			     "IF x% < 1 THEN\n"
			     "  LET y% = x% < 1\n"
			     "ENDIF\n";

	printf("ir_test_matcher");
	return parser_test_wrapper(source, SUBTILIS_BACKEND_INTER_CAPS,
				   prv_check_matcher, SUBTILIS_ERROR_OK, NULL);
}

int ir_test(void)
{
	int res;

	res = prv_test_wildcards_rule();
	res |= prv_test_i32_rule();
	res |= prv_test_regs_rule();
	res |= prv_test_floating_rule();
	res |= prv_test_label_rule();
	res |= prv_test_matcher();

	return res;
}
