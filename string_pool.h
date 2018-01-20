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

#ifndef __SUBTILIS_STRING_POOL_H
#define __SUBTILIS_STRING_POOL_H

#include "error.h"

struct subtilis_string_pool_t_ {
	char **strings;
	size_t length;
	size_t max_length;
	size_t ref;
};

typedef struct subtilis_string_pool_t_ subtilis_string_pool_t;

subtilis_string_pool_t *subtilis_string_pool_new(subtilis_error_t *err);
subtilis_string_pool_t *subtilis_string_pool_clone(subtilis_string_pool_t *src);
size_t subtilis_string_pool_register(subtilis_string_pool_t *pool,
				     const char *str, subtilis_error_t *err);
void subtilis_string_pool_delete(subtilis_string_pool_t *pool);

#endif
