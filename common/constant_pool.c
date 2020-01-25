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

#include "constant_pool.h"

subtilis_constant_pool_t *subtilis_constant_pool_new(subtilis_error_t *err)
{
	subtilis_constant_pool_t *pool;

	pool = calloc(1, sizeof(*pool));
	if (!pool) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	pool->ref = 1;
	return pool;
}

subtilis_constant_pool_t *
subtilis_constant_pool_clone(subtilis_constant_pool_t *src)
{
	src->ref++;
	return src;
}

void subtilis_constant_pool_delete(subtilis_constant_pool_t *pool)
{
	size_t i;

	if (!pool)
		return;

	if (pool->ref > 1) {
		pool->ref--;
		return;
	}

	for (i = 0; i < pool->size; i++)
		free(pool->data[i].data);

	free(pool);
}

void subtilis_constant_pool_dump(subtilis_constant_pool_t *pool)
{
	size_t i;
	size_t j;
	size_t count;

	for (i = 0; i < pool->size; i++) {
		printf("[%zu]:\n", i);
		count = 0;
		for (;;) {
			for (j = 0; j < 16; j++) {
				if (count == pool->data[i].data_size)
					goto next;
				printf("%x ", pool->data[i].data[count++]);
			}
			printf("\n");
		}
	/* clang-format off */
next:
	/* clang-format off */

		printf("\n");
	}
}

size_t subtilis_constant_pool_add(subtilis_constant_pool_t *pool, uint8_t *data,
				  size_t data_size, bool dbl,
				  subtilis_error_t *err)
{
	size_t i;
	size_t new_max_size;
	subtilis_constant_data_t *new_data;

	for (i = 0; i < pool->size; i++) {
		if (dbl != pool->data[i].dbl)
			continue;
		if (data_size != pool->data[i].data_size)
			continue;
		if (!memcmp(data, pool->data[i].data, data_size)) {
			free(data);
			return i;
		}
	}

	if (pool->size == pool->max_size) {
		new_max_size = pool->size + SUBTILIS_CONFIG_CONSTANT_POOL_GRAN;
		new_data =
		    realloc(pool->data, sizeof(*new_data) * new_max_size);
		if (!new_data) {
			subtilis_error_set_oom(err);
			return SIZE_MAX;
		}
		pool->max_size = new_max_size;
		pool->data = new_data;
	}

	pool->data[pool->size].data = data;
	pool->data[pool->size].dbl = dbl;
	pool->data[pool->size++].data_size = data_size;

	return i;
}

size_t subtilis_constant_pool_mem_size(subtilis_constant_pool_t *pool,
				       size_t **ptrs, subtilis_error_t *err)
{
	size_t i;
	size_t mem_size = 0;
	size_t *constants;

	constants = malloc(sizeof(*pool) * pool->size);
	if (!constants)
		subtilis_error_set_oom(err);

	for (i = 0; i < pool->size; i++) {
		constants[i] = mem_size;
		mem_size += pool->data[i].data_size;
	}

	*ptrs = constants;

	return mem_size;
}
