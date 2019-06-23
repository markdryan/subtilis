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

#ifndef __SUBTILIS_BUFFER_H
#define __SUBTILIS_BUFFER_H

#include <stdint.h>

#include "error.h"

struct subtilis_fixed_buffer_t_ {
	size_t max_size;
	size_t start;
	size_t end;
	uint8_t data[1];
};

typedef struct subtilis_fixed_buffer_t_ subtilis_fixed_buffer_t;

struct subtilis_buffer_t_ {
	size_t granularity;
	subtilis_fixed_buffer_t *buffer;
};

typedef struct subtilis_buffer_t_ subtilis_buffer_t;

void subtilis_buffer_init(subtilis_buffer_t *buffer, size_t granularity);
void subtilis_buffer_reserve(subtilis_buffer_t *buffer, size_t length,
			     subtilis_error_t *err);
void subtilis_buffer_expand_if_full(subtilis_buffer_t *buffer,
				    subtilis_error_t *err);
void subtilis_buffer_append_reserve(subtilis_buffer_t *buffer, size_t length,
				    subtilis_error_t *err);
void subtilis_buffer_append(subtilis_buffer_t *buffer, const void *data,
			    size_t length, subtilis_error_t *err);
void subtilis_buffer_append_string(subtilis_buffer_t *buffer, const char *str,
				   subtilis_error_t *err);
void subtilis_buffer_insert(subtilis_buffer_t *buffer, size_t pos,
			    const void *data, size_t length,
			    subtilis_error_t *err);
void subtilis_buffer_delete(subtilis_buffer_t *buffer, size_t pos,
			    size_t length, subtilis_error_t *err);
void subtilis_buffer_zero_terminate(subtilis_buffer_t *buffer,
				    subtilis_error_t *err);
void subtilis_buffer_remove_terminator(subtilis_buffer_t *buffer);
void subtilis_buffer_free(subtilis_buffer_t *buffer);
void subtilis_buffer_reset(subtilis_buffer_t *buffer);
size_t subtilis_buffer_get_size(const subtilis_buffer_t *buffer);
size_t subtilis_buffer_get_max_size(const subtilis_buffer_t *buffer);
size_t subtilis_buffer_get_space(const subtilis_buffer_t *buffer);
const char *subtilis_buffer_get_string(const subtilis_buffer_t *buffer);
void subtilis_fixed_buffer_compress(subtilis_fixed_buffer_t *fixed_buffer);

#endif
