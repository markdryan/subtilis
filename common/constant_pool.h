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

#ifndef __SUBTILIS_CONSTANT_POOL_H
#define __SUBTILIS_CONSTANT_POOL_H

#include "error.h"
#include "type.h"

struct subtilis_constant_data_t_ {
	size_t data_size;
	uint8_t *data;
	bool dbl;
};

typedef struct subtilis_constant_data_t_ subtilis_constant_data_t;

struct subtilis_constant_pool_t_ {
	size_t size;
	size_t max_size;
	subtilis_constant_data_t *data;
	size_t ref;
};

typedef struct subtilis_constant_pool_t_ subtilis_constant_pool_t;

subtilis_constant_pool_t *subtilis_constant_pool_new(subtilis_error_t *err);
void subtilis_constant_pool_delete(subtilis_constant_pool_t *pool);
subtilis_constant_pool_t *
subtilis_constant_pool_clone(subtilis_constant_pool_t *src);

void subtilis_constant_pool_dump(subtilis_constant_pool_t *pool);
size_t subtilis_constant_pool_mem_size(subtilis_constant_pool_t *pool,
				       size_t **ptrs, subtilis_error_t *err);

/* Ownership of data is transferred to this function on success. */

size_t subtilis_constant_pool_add(subtilis_constant_pool_t *pool, uint8_t *data,
				  size_t data_size, bool dbl,
				  subtilis_error_t *err);

#endif
