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

#include <stdlib.h>
#include <string.h>

#include "arm_encode.h"
#include "arm_test.h"
#include "arm_vm.h"
#include "parser_test.h"
#include "riscos_arm.h"
#include "riscos_arm2.h"
#include "test_cases.h"

static int prv_test_example(subtilis_lexer_t *l, subtilis_parser_t *p,
			    const char *expected)
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_section_t *arm_s = NULL;
	subtilis_arm_vm_t *vm = NULL;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	//	subtilis_ir_section_dump(p->p);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_s = subtilis_riscos_generate(pool, p->main, riscos_arm2_rules,
					 riscos_arm2_rules_count,
					 p->st->allocated, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_section_dump(arm_s);

	vm = subtilis_arm_vm_new(arm_s, 16 * 1024, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_vm_run(vm, &b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (strcmp(subtilis_buffer_get_string(&b), expected)) {
		printf("%s expected got %s\n", expected,
		       subtilis_buffer_get_string(&b));
		retval = 1;
		goto cleanup;
	}

	retval = 0;

cleanup:

	if (err.type != SUBTILIS_ERROR_OK)
		subtilis_error_fprintf(stdout, &err, true);
	subtilis_arm_vm_delete(vm);
	subtilis_arm_section_delete(arm_s);
	subtilis_arm_op_pool_delete(pool);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	int ret = 0;

	for (i = 0; i < SUBTILIS_TEST_CASE_ID_MAX; i++) {
		/* Division is not implemented yet */
		if (i == SUBTILIS_TEST_CASE_ID_DIVISION)
			continue;
		test = &test_cases[i];
		printf("arm_%s", test->name);
		pass = parser_test_wrapper(test->source, prv_test_example,
					   test->result);
		ret |= pass;
	}

	return ret;
}

/* clang-format off */
static const uint32_t prv_expected_code[] = {
	0x01A00001, /* MOVEQ R0, r1 */
	0x11B00001, /* MVNSNE R0, r1 */
	0xC1500001, /* CMPGT R0, R1 */
	0xB0000192, /* MULLT R0, R2, R1 */
	0x25920010, /* LDRCS R0, [R2, #16] */
	0x3F0000DC, /* SWICS &DC */
	0x4AFFFFFE, /* BMI #-8 */
};

/* clang-format on */

static void prv_add_ops(subtilis_arm_section_t *arm_s, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_instr_t *instr;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	op1.type = SUBTILIS_ARM_REG_FIXED;
	op2.type = SUBTILIS_ARM_REG_FIXED;

	/* MOVEQ R0, r1 */
	dest.num = 0;
	op1.num = 2;
	op2.num = 1;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_EQ, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MVNSNE R0, r1 */
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_NE, true, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* CMPGT R0, R1 */
	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_GT, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MULLT R0, R2, R1 */
	subtilis_arm_add_mul(arm_s, SUBTILIS_ARM_CCODE_LT, false, dest, op1,
			     op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDRCS R0, [R2, #16] */
	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_CS, dest, op1, 16, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* SWINV &DC */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_CC, 0xdc, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* BMI label_0 */
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.br.ccode = SUBTILIS_ARM_CCODE_MI;
	instr->operands.br.label = 0;
}

static int prv_test_encode(void)
{
	subtilis_error_t err;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_section_t *arm_s = NULL;
	uint32_t *code = NULL;
	size_t i;

	printf("arm_test_encode");

	subtilis_error_init(&err);
	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_s = subtilis_arm_section_new(pool, 16, 0, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_add_ops(arm_s, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	code = subtilis_arm_encode_buf(arm_s, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 0; i < sizeof(prv_expected_code) / sizeof(uint32_t); i++)
		if (code[i] != prv_expected_code[i]) {
			fprintf(stderr, "%zu: expected 0x%x got 0x%x\n", i,
				prv_expected_code[i], code[i]);
			goto cleanup;
		}

	retval = 0;

cleanup:
	if (err.type != SUBTILIS_ERROR_OK) {
		printf(": [FAIL]\n");
		subtilis_error_fprintf(stdout, &err, true);
	} else {
		printf(": [OK]\n");
	}

	free(code);
	subtilis_arm_section_delete(arm_s);
	subtilis_arm_op_pool_delete(pool);

	return retval;
}

int arm_test(void)
{
	int res;

	res = prv_test_examples();
	res |= prv_test_encode();

	return res;
}
