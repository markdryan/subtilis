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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "arm_core.h"
#include "arm_core_test.h"

static int prv_test_encode_imm(void)
{
	struct test {
		int32_t num;
		uint32_t encoded;
		bool ok;
	};

	size_t i;
	struct test tests[] = {
	    /* clang-format off */
		{ 173, 173, true },
		{ 257, 0, false },
		{ 19968, 0xc4e, true },
		{ 0x3FC00, 0xbff, true },
		{ 0x102, 0, false},
		{ (int32_t)0xFF0000FF, 0, false},
		{ (int32_t)0xC0000034, 0x1d3, true},
	};

	/* clang-format on */
	uint32_t encoded;
	bool ok;

	printf("arm_core_encode_imm");
	for (i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		encoded = 0;
		ok = subtilis_arm_encode_imm(tests[i].num, &encoded);
		if (ok != tests[i].ok || encoded != tests[i].encoded) {
			fprintf(stderr, "Expected %d %u got %d %u\n",
				tests[i].ok, tests[i].encoded, ok, encoded);
			goto fail;
		}
	}

	printf(": [OK]\n");
	return 0;

fail:
	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_encode_nearest(void)
{
	subtilis_error_t err;
	struct test {
		int32_t num;
		uint32_t encoded;
		bool ok;
	};

	size_t i;
	struct test tests[] = {
	    /* clang-format off */
		{ 173, 173, true },
		{ 257, 0xf41, true }, /* 260 */
		{ (int32_t)0xFE010000, 0x4ff, true},
		{ (int32_t)0xfffffff0, 0, false},
	};

	/* clang-format on */
	uint32_t encoded;

	printf("arm_core_encode_nearest");

	subtilis_error_init(&err);
	for (i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		encoded = subtilis_arm_encode_nearest(tests[i].num, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			if (tests[i].ok) {
				fprintf(stderr, "Expected success with 0x%x\n",
					tests[i].num);
				goto fail;
			}
		} else if (!tests[i].ok) {
			fprintf(stderr, "Expected failure with 0x%x\n",
				tests[i].num);
			goto fail;
		} else if (encoded != tests[i].encoded) {
			fprintf(stderr, "Expected %u got %u\n",
				tests[i].encoded, encoded);
			goto fail;
		}
	}

	printf(": [OK]\n");
	return 0;

fail:
	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_encode_lvl2_imm(void)
{
	struct test {
		int32_t num;
		uint32_t encoded;
		uint32_t encoded2;
		bool ok;
	};

	size_t i;
	struct test tests[] = {
	    /* clang-format off */
		{ 257, 1, 0xc01, true },
		{ 65535, 0xff, 0xcff, true},
		{ 0x1ffff, 0, 0, false},
	};

	/* clang-format on */
	uint32_t encoded;
	uint32_t encoded2;
	bool ok;

	printf("arm_core_encode_lvl2_imm");
	for (i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		encoded = 0;
		encoded2 = 0;
		ok = subtilis_arm_encode_lvl2_imm(tests[i].num, &encoded,
						  &encoded2);
		if (ok != tests[i].ok || encoded != tests[i].encoded ||
		    encoded2 != tests[i].encoded2) {
			fprintf(stderr, "Expected %d %u %u got %d %u %u\n",
				tests[i].ok, tests[i].encoded,
				tests[i].encoded2, ok, encoded, encoded2);
			goto fail;
		}
	}

	printf(": [OK]\n");
	return 0;

fail:
	printf(": [FAIL]\n");
	return 1;
}

static subtilis_arm_section_t *prv_add_imm(subtilis_arm_op_pool_t *pool,
					   int32_t num,
					   subtilis_arm_instr_type_t itype,
					   subtilis_arm_instr_type_t alt_type,
					   subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_section_t *s = NULL;
	subtilis_type_section_t *stype = NULL;

	stype = subtilis_type_section_new(&subtilis_type_void, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	dest = 0;

	op1 = 1;

	subtilis_arm_add_addsub_imm(s, itype, alt_type, SUBTILIS_ARM_CCODE_AL,
				    false, dest, op1, num, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_type_section_delete(stype);

	return s;

on_error:
	subtilis_arm_section_delete(s);
	subtilis_type_section_delete(stype);

	return NULL;
}

static int prv_check_imm(subtilis_arm_section_t *s,
			 subtilis_arm_instr_type_t itype, size_t count,
			 uint32_t *nums)
{
	size_t i;
	subtilis_arm_op_t *op;
	subtilis_arm_instr_t *instr;

	if (s->len != count) {
		fprintf(stderr, "Expected %zu instruction, found %zu\n", count,
			s->len);
		return 1;
	}

	if (s->constants.ui32_count != 0) {
		fprintf(stderr, "Expected 0 constants, found %zu\n",
			s->constants.ui32_count);
		return 1;
	}

	for (i = 0; i < count; i++) {
		op = &s->op_pool->ops[i];
		if (op->type != SUBTILIS_OP_INSTR) {
			fprintf(stderr, "Expected instruction %d, found %d\n",
				SUBTILIS_OP_INSTR, op->type);
			return 1;
		}

		instr = &op->op.instr;
		if (instr->type != itype) {
			fprintf(stderr, "Expected instruction %d, found %d\n",
				itype, instr->type);
			return 1;
		}

		if (instr->operands.data.op2.op.integer != nums[i]) {
			fprintf(stderr, "Expected constant %u, found %u\n",
				nums[i], instr->operands.data.op2.op.integer);
			return 1;
		}
	}

	return 0;
}

static int prv_test_arm_add_data_1_imm(void)
{
	subtilis_arm_section_t *s = NULL;
	subtilis_error_t err;
	uint32_t nums[] = {127};
	subtilis_arm_op_pool_t *pool;

	subtilis_error_init(&err);

	printf("arm_add_data_imm");

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	s = prv_add_imm(pool, 127, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_INSTR_SUB, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (prv_check_imm(s, SUBTILIS_ARM_INSTR_ADD, 1, nums))
		goto fail;

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [OK]\n");
	return 0;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_arm_add_data_2_imm(void)
{
	subtilis_arm_section_t *s = NULL;
	subtilis_error_t err;
	uint32_t nums[] = {1, 0xc01};
	subtilis_arm_op_pool_t *pool;

	printf("arm_add_data_2_imm");

	subtilis_error_init(&err);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	s = prv_add_imm(pool, 257, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_INSTR_SUB, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (prv_check_imm(s, SUBTILIS_ARM_INSTR_ADD, 2, nums))
		goto fail;

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [OK]\n");
	return 0;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_arm_add_data_neg_imm(void)
{
	subtilis_arm_section_t *s = NULL;
	subtilis_error_t err;
	uint32_t nums[] = {0xc01};
	subtilis_arm_op_pool_t *pool;

	printf("arm_add_data_neg_imm");

	subtilis_error_init(&err);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	s = prv_add_imm(pool, (int32_t)0xffffff00, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_INSTR_SUB, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (prv_check_imm(s, SUBTILIS_ARM_INSTR_SUB, 1, nums))
		goto fail;

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [OK]\n");
	return 0;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_arm_add_data_neg_2_imm(void)
{
	subtilis_arm_section_t *s = NULL;
	subtilis_error_t err;
	uint32_t nums[] = {0x10, 0x8ff};
	subtilis_arm_op_pool_t *pool;

	printf("arm_add_data_neg_2_imm");

	subtilis_error_init(&err);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	s = prv_add_imm(pool, (int32_t)0xff00fff0, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_INSTR_SUB, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (prv_check_imm(s, SUBTILIS_ARM_INSTR_SUB, 2, nums))
		goto fail;

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [OK]\n");
	return 0;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [FAIL]\n");
	return 1;
}

static int prv_test_arm_add_data_ldr_imm(void)
{
	subtilis_arm_section_t *s = NULL;
	subtilis_error_t err;
	subtilis_arm_op_t *op;
	subtilis_arm_instr_t *instr;
	subtilis_arm_op_pool_t *pool;

	printf("arm_add_data_ldr_imm");

	subtilis_error_init(&err);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	s = prv_add_imm(pool, (int32_t)0xf0f0f0f0, SUBTILIS_ARM_INSTR_ADD,
			SUBTILIS_ARM_INSTR_SUB, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (s->len != 2) {
		fprintf(stderr, "Expected 2 instructions, found %zu\n", s->len);
		return 1;
	}

	if (s->constants.ui32_count != 1) {
		fprintf(stderr, "Expected 1 constant, found %zu\n",
			s->constants.ui32_count);
		return 1;
	}

	op = &s->op_pool->ops[0];
	if (op->type != SUBTILIS_OP_INSTR) {
		fprintf(stderr, "Expected instruction %d, found %d\n",
			SUBTILIS_OP_INSTR, op->type);
		return 1;
	}

	instr = &op->op.instr;
	if (instr->type != SUBTILIS_ARM_INSTR_LDRC) {
		fprintf(stderr, "Expected instruction %d, found %d\n",
			SUBTILIS_ARM_INSTR_LDRC, instr->type);
		return 1;
	}

	op = &s->op_pool->ops[1];
	if (op->type != SUBTILIS_OP_INSTR) {
		fprintf(stderr, "Expected instruction %d, found %d\n",
			SUBTILIS_OP_INSTR, op->type);
		return 1;
	}

	instr = &op->op.instr;
	if (instr->type != SUBTILIS_ARM_INSTR_ADD) {
		fprintf(stderr, "Expected instruction %d, found %d\n",
			SUBTILIS_ARM_INSTR_ADD, instr->type);
		return 1;
	}

	if (s->constants.ui32[0].integer != 0xf0f0f0f0) {
		fprintf(stderr, "Expected constant %d, found %d\n", 0xf0f0f0f0,
			s->constants.ui32[0].integer);
		return 1;
	}

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [OK]\n");
	return 0;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	printf(": [FAIL]\n");
	return 1;
}

int arm_core_test(void)
{
	int retval;

	retval = prv_test_encode_imm();
	retval |= prv_test_encode_nearest();
	retval |= prv_test_encode_lvl2_imm();
	retval |= prv_test_arm_add_data_1_imm();
	retval |= prv_test_arm_add_data_2_imm();
	retval |= prv_test_arm_add_data_neg_imm();
	retval |= prv_test_arm_add_data_neg_2_imm();
	retval |= prv_test_arm_add_data_ldr_imm();

	return retval;
}
