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

#ifndef __SUBTILIS_SIZET_VECTOR_H
#define __SUBTILIS_SIZET_VECTOR_H

#include "error.h"

struct subtilis_sizet_vector_t_ {
	size_t len;
	size_t max_len;
	size_t *vals;
};

typedef struct subtilis_sizet_vector_t_ subtilis_sizet_vector_t;

void subtilis_sizet_vector_init(subtilis_sizet_vector_t *v);
void subtilis_sizet_vector_append(subtilis_sizet_vector_t *v, size_t val,
				  subtilis_error_t *err);
void subtilis_sizet_vector_free(subtilis_sizet_vector_t *v);

#endif
