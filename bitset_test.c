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

#include "bitset.h"
#include "bitset_test.h"

static int prv_bitset_basic(void)
{
	subtilis_bitset_t bs1;
	subtilis_bitset_t bs2;
	subtilis_bitset_t bs3;
	subtilis_error_t err;
	size_t i;
	int retval = 1;

	printf("bitset_test");

	subtilis_error_init(&err);

	subtilis_bitset_init(&bs1);
	subtilis_bitset_init(&bs2);
	subtilis_bitset_init(&bs3);

	subtilis_bitset_set(&bs1, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs1, 64, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs2, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs2, 63, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (!subtilis_bitset_isset(&bs1, 1) ||
	    !subtilis_bitset_isset(&bs1, 64) ||
	    !subtilis_bitset_isset(&bs2, 1) || !subtilis_bitset_isset(&bs2, 63))
		goto fail;

	for (i = 2; i < 64; i++)
		if (subtilis_bitset_isset(&bs1, i)) {
			printf("%zu\n", i);
			goto fail;
		}

	for (i = 2; i < 63; i++)
		if (subtilis_bitset_isset(&bs2, i))
			goto fail;

	if (subtilis_bitset_isset(&bs1, 128))
		goto fail;

	subtilis_bitset_or(&bs1, &bs2, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (bs1.max_value != 64)
		goto fail;

	if (!subtilis_bitset_isset(&bs1, 1) ||
	    !subtilis_bitset_isset(&bs1, 64) ||
	    !subtilis_bitset_isset(&bs1, 63))
		goto fail;

	subtilis_bitset_clear(&bs1, 63);
	if (bs1.max_value != 64)
		goto fail;

	subtilis_bitset_and(&bs1, &bs2);

	if (bs1.max_value != 1)
		goto fail;

	if (subtilis_bitset_isset(&bs1, 0) || !subtilis_bitset_isset(&bs1, 1))
		goto fail;

	for (i = 2; i < 65; i++)
		if (subtilis_bitset_isset(&bs1, i))
			goto fail;

	subtilis_bitset_clear(&bs2, 63);
	if (bs2.max_value != 1)
		goto fail;

	subtilis_bitset_and(&bs1, &bs3);
	subtilis_bitset_and(&bs3, &bs1);

	subtilis_bitset_or(&bs3, &bs2, &err);
	if (bs3.max_value != 1)
		goto fail;

	retval = 0;

fail:

	subtilis_bitset_free(&bs3);
	subtilis_bitset_free(&bs2);
	subtilis_bitset_free(&bs1);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
	;
}

static int prv_bitset_clear(void)
{
	subtilis_bitset_t bs;
	size_t i;
	subtilis_error_t err;
	int retval = 1;

	printf("bitset_clear_test");

	subtilis_bitset_init(&bs);
	subtilis_error_init(&err);

	for (i = 0; i <= 130; i++) {
		subtilis_bitset_set(&bs, i, &err);
		if (err.type != SUBTILIS_ERROR_OK)
			goto fail;
	}

	subtilis_bitset_clear(&bs, 130);
	if (bs.max_value != 129)
		goto fail;

	subtilis_bitset_clear(&bs, 129);
	if (bs.max_value != 128)
		goto fail;

	subtilis_bitset_clear(&bs, 128);
	if (bs.max_value != 127)
		goto fail;

	retval = 0;
fail:

	subtilis_bitset_free(&bs);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_reuse(void)
{
	subtilis_bitset_t bs1;
	subtilis_bitset_t bs2;
	subtilis_bitset_t bs3;
	subtilis_error_t err;
	size_t i;
	size_t count;
	int retval = 1;

	printf("bitset_reuse_test");

	subtilis_error_init(&err);
	subtilis_bitset_init(&bs1);
	subtilis_bitset_init(&bs2);
	subtilis_bitset_init(&bs3);

	subtilis_bitset_set(&bs1, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs1, 64, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs1, 128, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_reset(&bs1);
	subtilis_bitset_set(&bs1, 129, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	count = 0;
	for (i = 0; i <= 129; i++)
		if (subtilis_bitset_isset(&bs1, i))
			++count;
	if (count != 1)
		goto fail;

	retval = 0;

fail:

	subtilis_bitset_free(&bs3);
	subtilis_bitset_free(&bs2);
	subtilis_bitset_free(&bs1);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_empty(void)
{
	subtilis_bitset_t bs1;
	subtilis_bitset_t bs2;
	subtilis_error_t err;
	int retval = 1;

	printf("bitset_empty_test");

	subtilis_error_init(&err);
	subtilis_bitset_init(&bs1);
	subtilis_bitset_init(&bs2);

	subtilis_bitset_and(&bs1, &bs2);
	if (bs1.max_value != -1)
		goto fail;

	subtilis_bitset_or(&bs1, &bs2, &err);
	if (err.type != SUBTILIS_ERROR_OK || bs1.max_value != -1)
		goto fail;

	subtilis_bitset_set(&bs2, 129, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_and(&bs1, &bs2);
	if (bs1.max_value != -1)
		goto fail;

	subtilis_bitset_and(&bs2, &bs1);
	if (bs1.max_value != -1)
		goto fail;

	subtilis_bitset_set(&bs2, 129, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_or(&bs1, &bs2, &err);
	if (err.type != SUBTILIS_ERROR_OK || bs1.max_value != 129)
		goto fail;

	subtilis_bitset_reset(&bs2);
	subtilis_bitset_and(&bs1, &bs2);
	if (bs1.max_value != -1)
		goto fail;

	retval = 0;

fail:

	subtilis_bitset_free(&bs2);
	subtilis_bitset_free(&bs1);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_same_size(void)
{
	subtilis_bitset_t bs1;
	subtilis_bitset_t bs2;
	subtilis_error_t err;
	int retval = 1;

	printf("bitset_samesize_test");

	subtilis_error_init(&err);
	subtilis_bitset_init(&bs1);
	subtilis_bitset_init(&bs2);

	subtilis_bitset_set(&bs1, 2, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs2, 1, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_and(&bs1, &bs2);
	if (bs1.max_value != -1)
		goto fail;

	subtilis_bitset_set(&bs1, 2, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_or(&bs1, &bs2, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (bs1.max_value != 2)
		goto fail;

	retval = 0;

fail:

	subtilis_bitset_free(&bs2);
	subtilis_bitset_free(&bs1);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_count(void)
{
	subtilis_bitset_t bs;
	subtilis_error_t err;
	int retval = 1;

	printf("bitset_count_test");

	subtilis_error_init(&err);
	subtilis_bitset_init(&bs);

	if (subtilis_bitset_count(&bs) != 0)
		goto fail;

	subtilis_bitset_set(&bs, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 64, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 128, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (subtilis_bitset_count(&bs) != 3)
		goto fail;

	retval = 0;

fail:

	subtilis_bitset_free(&bs);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_values(void)
{
	subtilis_bitset_t bs;
	subtilis_error_t err;
	size_t *values = NULL;
	size_t count;
	int retval = 1;

	printf("bitset_values_test");

	subtilis_error_init(&err);
	subtilis_bitset_init(&bs);

	values = subtilis_bitset_values(&bs, &count, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (count != 0 || values)
		goto fail;

	subtilis_bitset_set(&bs, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 64, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 128, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	values = subtilis_bitset_values(&bs, &count, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (count != 3)
		goto fail;

	if (values[0] != 0 || values[1] != 64 || values[2] != 128)
		goto fail;

	retval = 0;

fail:

	free(values);
	subtilis_bitset_free(&bs);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_not(void)
{
	subtilis_bitset_t bs;
	size_t i;
	subtilis_error_t err;
	size_t count;
	int retval = 1;
	size_t *values = NULL;

	printf("bitset_not_test");

	subtilis_bitset_init(&bs);
	subtilis_error_init(&err);

	for (i = 0; i <= 130; i++) {
		subtilis_bitset_set(&bs, i, &err);
		if (err.type != SUBTILIS_ERROR_OK)
			goto fail;
	}

	subtilis_bitset_clear(&bs, 1);
	subtilis_bitset_clear(&bs, 128);
	subtilis_bitset_clear(&bs, 129);
	subtilis_bitset_not(&bs);

	values = subtilis_bitset_values(&bs, &count, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (count != 3 || !values)
		goto fail;

	if (values[0] != 1 || values[1] != 128 || values[2] != 129)
		goto fail;

	retval = 0;
fail:

	free(values);
	subtilis_bitset_free(&bs);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_bitset_sub(void)
{
	subtilis_bitset_t bs;
	subtilis_bitset_t bs1;
	subtilis_error_t err;
	size_t count;
	size_t *values = NULL;
	int retval = 1;

	printf("bitset_sub_test");

	subtilis_bitset_init(&bs);
	subtilis_bitset_init(&bs1);
	subtilis_error_init(&err);

	subtilis_bitset_set(&bs, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 67, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 128, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_set(&bs, 140, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_sub(&bs, &bs1);
	values = subtilis_bitset_values(&bs, &count, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (count != 4)
		goto fail;

	if (values[0] != 0 || values[1] != 67 || values[2] != 128 ||
	    values[3] != 140)
		goto fail;

	free(values);
	values = NULL;

	subtilis_bitset_clear(&bs, 128);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_clear(&bs, 0);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_bitset_sub(&bs, &bs1);
	values = subtilis_bitset_values(&bs, &count, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	if (count != 2)
		goto fail;

	if (values[0] != 67 || values[1] != 140)
		goto fail;

	retval = 0;
fail:

	free(values);
	subtilis_bitset_free(&bs);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

int bitset_test(void)
{
	int retval;

	retval = prv_bitset_basic();
	retval |= prv_bitset_clear();
	retval |= prv_bitset_reuse();
	retval |= prv_bitset_empty();
	retval |= prv_bitset_same_size();
	retval |= prv_bitset_count();
	retval |= prv_bitset_values();
	retval |= prv_bitset_not();
	retval |= prv_bitset_sub();

	return retval;
}
