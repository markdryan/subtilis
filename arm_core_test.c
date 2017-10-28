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
		{ 0xFF0000FF, 0, false},
		{ 0xC0000034, 0x1d3, true},
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

int arm_core_test(void) { return prv_test_encode_imm(); }
