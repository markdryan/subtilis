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
#include <string.h>

#include "buffer.h"

void subtilis_buffer_init(subtilis_buffer_t *buffer, size_t granularity)
{
	memset(buffer, 0, sizeof(subtilis_buffer_t));
	buffer->granularity = granularity;
}

void subtilis_buffer_reserve(subtilis_buffer_t *buffer, size_t length,
			     subtilis_error_t *err)
{
	size_t rem = 0;
	size_t new_max_size = 0;
	size_t extra_space = sizeof(subtilis_fixed_buffer_t);
	subtilis_fixed_buffer_t *b = buffer->buffer;

	if (b) {
		length += b->start;
		if (length <= b->max_size)
			return;
	}
	rem = length % buffer->granularity;
	new_max_size =
	    (rem == 0) ? length : (length - rem) + buffer->granularity;

	b = (subtilis_fixed_buffer_t *)realloc(buffer->buffer,
					       new_max_size + extra_space);
	if (!b) {
		subtilis_error_set_oom(err);
		return;
	}

	if (!buffer->buffer) {
		b->start = 0;
		b->end = 0;
	}
	b->max_size = new_max_size;
	buffer->buffer = b;
}

void subtilis_buffer_expand_if_full(subtilis_buffer_t *buffer,
				    subtilis_error_t *err)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	if (!b)
		subtilis_buffer_reserve(buffer, buffer->granularity, err);
	else if ((b->max_size - b->end) == 0)
		subtilis_buffer_reserve(buffer,
					buffer->granularity + b->max_size, err);
}

void subtilis_buffer_append_reserve(subtilis_buffer_t *buffer, size_t length,
				    subtilis_error_t *err)
{
	size_t new_end = length;
	size_t new_length;
	subtilis_fixed_buffer_t *b = buffer->buffer;

	if (b) {
		new_end += b->end;
		new_length = new_end - b->start;
	} else {
		new_length = length;
	}

	subtilis_buffer_reserve(buffer, new_length, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	b = buffer->buffer;
	b->end = new_end;
}

void subtilis_buffer_append(subtilis_buffer_t *buffer, const void *data,
			    size_t length, subtilis_error_t *err)
{
	size_t old_size = subtilis_buffer_get_size(buffer);

	subtilis_buffer_append_reserve(buffer, length, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	(void)memcpy(buffer->buffer->data + old_size, data, length);
}

void subtilis_buffer_append_string(subtilis_buffer_t *buffer, const char *str,
				   subtilis_error_t *err)
{
	subtilis_buffer_append(buffer, str, strlen(str), err);
}

void subtilis_buffer_insert(subtilis_buffer_t *buffer, size_t pos,
			    const void *data, size_t length,
			    subtilis_error_t *err)
{
	size_t new_length;
	void *move_from;
	subtilis_fixed_buffer_t *b;
	size_t size = subtilis_buffer_get_size(buffer);

	if (pos > size) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}

	new_length = size + length;

	subtilis_buffer_reserve(buffer, new_length, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	b = buffer->buffer;
	move_from = b->data + pos + b->start;
	if (pos < size)
		memmove(move_from + length, move_from, size - pos);

	memcpy(move_from, data, length);
	b->end += length;
}

void subtilis_buffer_delete(subtilis_buffer_t *buffer, size_t pos,
			    size_t length, subtilis_error_t *err)
{
	size_t last_pos = pos + length;
	void *move_to;
	void *move_from;
	size_t bytes_to_move;
	subtilis_fixed_buffer_t *b = buffer->buffer;
	size_t size;

	if (!b) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}

	size = b->end - b->start;

	if (last_pos > size) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}

	if (last_pos == size) {
		b->end = pos + b->start;
	} else {
		move_to = b->data + pos + b->start;
		move_from = move_to + length;
		bytes_to_move = size - last_pos;
		memmove(move_to, move_from, bytes_to_move);
		b->end -= length;
	}
}

void subtilis_buffer_zero_terminate(subtilis_buffer_t *buffer,
				    subtilis_error_t *err)
{
	uint8_t zero = 0;
	subtilis_fixed_buffer_t *b = buffer->buffer;

	if (!b || (b->start == b->end) || (((char *)b->data)[b->end - 1] != 0))
		subtilis_buffer_append(buffer, &zero, 1, err);
}

void subtilis_buffer_free(subtilis_buffer_t *buffer)
{
	if (!buffer->buffer)
		return;

	free(buffer->buffer);
	buffer->buffer = 0;
}

void subtilis_buffer_reset(subtilis_buffer_t *buffer)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	if (b) {
		b->start = 0;
		b->end = 0;
	}
}

size_t subtilis_buffer_get_size(const subtilis_buffer_t *buffer)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	return (b) ? b->end - b->start : 0;
}

size_t subtilis_buffer_get_max_size(const subtilis_buffer_t *buffer)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	return (b) ? b->max_size : 0;
}

size_t subtilis_buffer_get_space(const subtilis_buffer_t *buffer)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	return (b) ? (b->max_size - (b->end - b->start)) : 0;
}

const char *subtilis_buffer_get_string(const subtilis_buffer_t *buffer)
{
	subtilis_fixed_buffer_t *b = buffer->buffer;

	return (b) ? ((const char *)b->data + b->start) : NULL;
}

void subtilis_fixed_buffer_compress(subtilis_fixed_buffer_t *fixed_buffer)
{
	if (fixed_buffer->start > 0) {
		(void)memmove(fixed_buffer->data,
			      fixed_buffer->data + fixed_buffer->start,
			      fixed_buffer->end - fixed_buffer->start);
		fixed_buffer->end -= fixed_buffer->start;
		fixed_buffer->start = 0;
	}
}
