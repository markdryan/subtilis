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

#include <string.h>

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
	subtilis_arm_program_t *arm_p = NULL;
	subtilis_arm_vm_t *vm = NULL;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	//	subtilis_ir_program_dump(p->p);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_riscos_generate(pool, p->p, riscos_arm2_rules,
					 riscos_arm2_rules_count,
					 p->st->allocated, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_program_dump(arm_p);

	vm = subtilis_arm_vm_new(arm_p, 16 * 1024, &err);
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
	subtilis_arm_program_delete(arm_p);
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

int arm_test(void)
{
	int res;

	res = prv_test_examples();

	return res;
}
