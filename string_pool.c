/*
 * Copyright (c) 2018 Mark Ryan
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

#include "string_pool.h"

subtilis_string_pool_t *subtilis_string_pool_new(subtilis_error_t *err)
{
	subtilis_string_pool_t *pool;

	pool = calloc(1, sizeof(*pool));
	if (!pool)
		subtilis_error_set_oom(err);
	pool->ref = 1;
	return pool;
}

subtilis_string_pool_t *subtilis_string_pool_clone(subtilis_string_pool_t *src)
{
	src->ref++;
	return src;
}

size_t subtilis_string_pool_register(subtilis_string_pool_t *pool,
				     const char *str, subtilis_error_t *err)
{
	size_t i;
	char **new_pool;
	size_t new_max;
	char *str_dup = NULL;

	for (i = 0; i < pool->length; i++)
		if (!strcmp(pool->strings[i], str))
			break;

	if (i < pool->length)
		return i;

	str_dup = malloc(strlen(str) + 1);
	if (!str_dup) {
		subtilis_error_set_oom(err);
		return 0;
	}
	(void)strcpy(str_dup, str);

	if (pool->length == pool->max_length) {
		new_max = pool->max_length + SUBTILIS_CONFIG_PROC_GRAN;
		new_pool =
		    realloc(pool->strings, new_max * sizeof(*pool->strings));
		if (!new_pool) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		pool->max_length = new_max;
		pool->strings = new_pool;
	}

	pool->strings[i] = str_dup;
	pool->length++;

	return i;

cleanup:

	free(str_dup);

	return 0;
}

void subtilis_string_pool_delete(subtilis_string_pool_t *pool)
{
	size_t i;

	if (!pool)
		return;

	if (pool->ref > 1) {
		pool->ref--;
		return;
	}

	for (i = 0; i < pool->length; i++)
		free(pool->strings[i]);
	free(pool->strings);
	free(pool);
}
