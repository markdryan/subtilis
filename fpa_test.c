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
#include <stdint.h>
#include <stdio.h>

#include "arm_core.h"
#include "fpa_test.h"

static int prv_test_encode_imm(void)
{
	struct test {
		double num;
		uint8_t encoded;
		bool ok;
	};

	size_t i;
	struct test tests[] = {
	    /* clang-format off */
		{0.0, 0x8, true },
		{1.0, 0x9, true },
		{3.0, 0xB, true },
		{4.0, 0xC, true },
		{5.0, 0xD, true },
		{0.5, 0xE, true },
		{10.0, 0xF, true},
		{-1.0, 0, false},
		{11.0, 0, false},
		{1.1, 0, false},
	};

	/* clang-format on */
	uint8_t encoded;
	bool ok;

	printf("fpa_encode_imm");
	for (i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		encoded = 0;
		ok = subtilis_fpa_encode_real(tests[i].num, &encoded);
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

static subtilis_arm_section_t *prv_mov_imm(subtilis_arm_op_pool_t *pool,
					   double num,
					   subtilis_arm_instr_type_t itype,
					   subtilis_arm_instr_type_t alt_type,
					   subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_section_t *s = NULL;
	subtilis_type_section_t *stype = NULL;

	stype = subtilis_type_section_new(SUBTILIS_TYPE_VOID, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	dest.type = SUBTILIS_ARM_REG_FIXED;
	dest.num = 0;

	subtilis_fpa_add_mvfmnf_imm(s, SUBTILIS_ARM_CCODE_AL, itype, alt_type,
				    SUBTILIS_FPA_ROUNDING_NEAREST, dest, num,
				    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	subtilis_type_section_delete(stype);

	return s;

on_error:
	subtilis_arm_section_delete(s);
	subtilis_type_section_delete(stype);

	return NULL;
}

static bool prv_check_mov_imm(subtilis_arm_section_t *s,
			      subtilis_arm_instr_type_t expected_type,
			      bool encodable, double num, double expected_num)
{
	subtilis_arm_op_t *op;
	subtilis_arm_instr_t *instr;
	uint8_t encoded;

	if (s->len != 1) {
		fprintf(stderr, "Expected 1 instruction, found %zu\n", s->len);
		return false;
	}

	op = &s->op_pool->ops[0];
	instr = &op->op.instr;
	if (instr->type != expected_type) {
		fprintf(stderr, "Expected instruction %d, found %d\n",
			expected_type, instr->type);
		return false;
	}

	if (encodable) {
		if (s->constants.real_count != 0) {
			fprintf(stderr, "Expected 0 constants, found %zu\n",
				s->constants.real_count);
			return false;
		}

		if (op->type != SUBTILIS_OP_INSTR) {
			fprintf(stderr, "Expected instruction %d, found %d\n",
				SUBTILIS_OP_INSTR, op->type);
			return false;
		}

		if (!subtilis_fpa_encode_real(expected_num, &encoded)) {
			fprintf(stderr, "Failed to encode: %f\n", expected_num);
			return false;
		}

		if (instr->operands.fpa_data.op2.imm != encoded) {
			fprintf(stderr, "Expected 0x%x, found 0x%x\n", encoded,
				instr->operands.fpa_data.op2.imm);
			return false;
		}
	} else {
		if (s->constants.real_count != 1) {
			fprintf(stderr, "Expected 1 constants, found %zu\n",
				s->constants.real_count);
			return false;
		}
		if (s->constants.real[0].real != expected_num) {
			fprintf(stderr, "Expected %f, found %f\n", num,
				s->constants.real[0].real);
			return false;
		}
	}

	return true;
}

static bool prv_section_mov_imm(double num, subtilis_arm_instr_type_t itype,
				subtilis_arm_instr_type_t expected_type,
				double expected_num, bool encodable,
				subtilis_error_t *err)

{
	bool retval = false;
	subtilis_arm_op_pool_t *pool;
	subtilis_arm_instr_type_t alt_type;
	subtilis_arm_section_t *s = NULL;

	pool = subtilis_arm_op_pool_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

	alt_type = itype == SUBTILIS_FPA_INSTR_MVF ? SUBTILIS_FPA_INSTR_MNF
						   : SUBTILIS_FPA_INSTR_MVF;

	s = prv_mov_imm(pool, num, itype, alt_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

	if (!prv_check_mov_imm(s, expected_type, encodable, num, expected_num))
		goto fail;

	retval = true;

fail:
	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	return retval;
}

static int prv_test_mov_imm(void)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	printf("fpa_test_mov_imm");

	if (!prv_section_mov_imm(1.0, SUBTILIS_FPA_INSTR_MVF,
				 SUBTILIS_FPA_INSTR_MVF, 1.0, true, &err))
		goto fail;

	if (!prv_section_mov_imm(-1.0, SUBTILIS_FPA_INSTR_MVF,
				 SUBTILIS_FPA_INSTR_MNF, 1.0, true, &err))
		goto fail;

	if (!prv_section_mov_imm(-1.0, SUBTILIS_FPA_INSTR_MNF,
				 SUBTILIS_FPA_INSTR_MVF, 1.0, true, &err))
		goto fail;

	if (!prv_section_mov_imm(3.14, SUBTILIS_FPA_INSTR_MVF,
				 SUBTILIS_FPA_INSTR_LDRC, 3.14, false, &err))
		goto fail;

	printf(": [OK]\n");
	return 0;

fail:

	printf(": [FAIL]\n");
	return 1;
}

int fpa_test(void)
{
	int retval;

	retval = prv_test_encode_imm();
	retval |= prv_test_mov_imm();

	return retval;
}
