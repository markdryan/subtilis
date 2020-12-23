/*
 * Copyright (c) 2020 Mark Ryan
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

#include "../../arch/arm32/arm_disass.h"
#include "../../arch/arm32/arm_encode.h"
#include "../../arch/arm32/arm_keywords.h"
#include "../../arch/arm32/arm_vm.h"
#include "../../arch/arm32/vfp_gen.h"
#include "../../frontend/parser_test.h"
#include "../../test_cases/bad_test_cases.h"
#include "../../test_cases/test_cases.h"
#include "../ptd/ptd.h"
#include "../riscos_common/riscos_arm.h"
#include "ptd_test.h"

static int prv_test_example(subtilis_lexer_t *l, subtilis_parser_t *p,
			    subtilis_error_type_t expected_err,
			    const char *expected, bool mem_leaks_ok)
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	size_t code_size;
	subtilis_arm_fp_if_t fp_if;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_vm_t *vm = NULL;
	uint8_t *code = NULL;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->backend.backend_data = pool;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	//	subtilis_ir_prog_dump(p->prog);
	subtilis_arm_vfp_if_init(&fp_if);

	arm_p = subtilis_riscos_generate(
	    pool, p->prog, ptd_rules, ptd_rules_count, p->st->max_allocated,
	    &fp_if, SUBTILIS_PTD_PROGRAM_START, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_prog_dump(arm_p);

	code = subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	/* Insert heap start */

	if (code_size < 8) {
		subtilis_error_set_assertion_failed(&err);
		goto cleanup;
	}

	((uint32_t *)code)[1] = SUBTILIS_PTD_PROGRAM_START + code_size;

	//	for (size_t i = 0; i < code_size; i++) {
	//		printf("0x%x\n",code[i]);
	///	}
	vm = subtilis_arm_vm_new(code, code_size, 512 * 1024,
				 SUBTILIS_PTD_PROGRAM_START, true, &err);
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

	if (retval != 0) {
		subtilis_error_fprintf(stdout, &err, true);
	} else if (err.type != expected_err) {
		fprintf(stderr, "expected error %u got %u\n", expected_err,
			err.type);
		retval = 1;
	}

	subtilis_arm_vm_delete(vm);
	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_ptd_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_PTD_CAPS;
	backend.sys_trans = subtilis_ptd_sys_trans;
	backend.sys_check = subtilis_ptd_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_ptd_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0; i < SUBTILIS_TEST_CASE_ID_MAX; i++) {
		test = &test_cases[i];

		switch (i) {
		case SUBTILIS_TEST_CASE_ID_SIN_COS:
		case SUBTILIS_TEST_CASE_ID_POINT_TINT:
		case SUBTILIS_TEST_CASE_ID_TRIG:
		case SUBTILIS_TEST_CASE_ID_LOG:
		case SUBTILIS_TEST_CASE_ID_ERROR_LOGRANGE:
		case SUBTILIS_TEST_CASE_ID_POW:
		case SUBTILIS_TEST_CASE_ID_STR_EXP:
			continue;
		default:
			break;
		}

		printf("ptd_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

int ptd_test(void) { return prv_test_ptd_examples(); }
