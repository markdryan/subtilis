/*
 * Copyright (c) 2019 Mark Ryan
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

#include "config.h"
#include "sizet_vector.h"

void subtilis_sizet_vector_init(subtilis_sizet_vector_t *v)
{
	v->len = 0;
	v->max_len = 0;
	v->vals = NULL;
}

void subtilis_sizet_vector_append(subtilis_sizet_vector_t *v, size_t val,
				  subtilis_error_t *err)
{
	size_t new_max;
	size_t *new_vals;

	if (v->len == v->max_len) {
		new_max = v->len + SUBTILIS_CONFIG_SIZET_SIZE;
		new_vals = realloc(v->vals, new_max * sizeof(size_t));
		if (!new_vals) {
			subtilis_error_set_oom(err);
			return;
		}
		v->max_len = new_max;
		v->vals = new_vals;
	}
	v->vals[v->len++] = val;
}

void subtilis_sizet_vector_free(subtilis_sizet_vector_t *v)
{
	if (v->vals)
		free(v->vals);
}
