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

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"

void subtilis_bitset_init(subtilis_bitset_t *bs)
{
	bs->bits = NULL;
	bs->max_value = -1;
	bs->allocated = 0;
}

void subtilis_bitset_reset(subtilis_bitset_t *bs)
{
	size_t i;

	bs->max_value = -1;

	for (i = 0; i < bs->allocated; i++)
		bs->bits[i] = 0;
}

static void prv_ensure(subtilis_bitset_t *bs, size_t bit, subtilis_error_t *err)
{
	unsigned int *new_bits;
	size_t needed = (bit / (sizeof(unsigned int) * 8)) + 1;

	if (needed <= bs->allocated)
		return;

	new_bits = realloc(bs->bits, needed * sizeof(unsigned int));
	if (!new_bits) {
		subtilis_error_set_oom(err);
		return;
	}
	bs->bits = new_bits;
	memset(&bs->bits[bs->allocated], 0,
	       sizeof(unsigned int) * (needed - bs->allocated));
	bs->allocated = needed;
}

void subtilis_bitset_set(subtilis_bitset_t *bs, size_t bit,
			 subtilis_error_t *err)
{
	size_t cell;
	size_t offset;

	if (bit > INT_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prv_ensure(bs, bit, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cell = bit / (sizeof(unsigned int) * 8);
	offset = bit - (cell * (sizeof(unsigned int) * 8));
	bs->bits[cell] |= 1 << offset;
	if ((int)bit > bs->max_value)
		bs->max_value = (int)bit;
}

bool subtilis_bitset_isset(subtilis_bitset_t *bs, size_t bit)
{
	size_t cell;
	size_t offset;

	if (!bs->bits || bit > INT_MAX || ((int)bit) > bs->max_value)
		return false;

	cell = bit / (sizeof(unsigned int) * 8);
	offset = bit - (cell * (sizeof(unsigned int) * 8));
	return (bs->bits[cell] & (1 << offset)) != 0;
}

static void prv_set_max(subtilis_bitset_t *bs, size_t block)
{
	size_t i;
	size_t byte;
	unsigned int mask;

	do {
		if (bs->bits[block] != 0)
			break;
		if (block == 0) {
			bs->max_value = -1;
			return;
		}
		block--;
	} while (true);

	byte = sizeof(unsigned int);
	while (byte > 0) {
		byte--;
		mask = 0xff << (byte * 8);
		if (mask & bs->bits[block])
			break;
	}

	i = 8;
	while (i > 0) {
		i--;
		mask = 1 << (i + (byte * 8));
		if (mask & bs->bits[block])
			break;
	}
	bs->max_value = (block * sizeof(unsigned int)) + (byte * 8) + i;
}

void subtilis_bitset_clear(subtilis_bitset_t *bs, size_t bit)
{
	size_t cell;
	size_t offset;

	if (bit > bs->max_value)
		return;

	cell = bit / (sizeof(unsigned int) * 8);
	offset = bit - (cell * (sizeof(unsigned int) * 8));
	bs->bits[cell] &= ~((unsigned int)(1 << offset));
	if (bit == (size_t)bs->max_value)
		prv_set_max(bs, cell);
}

void subtilis_bitset_and(subtilis_bitset_t *bs, subtilis_bitset_t *bs1)
{
	size_t i;
	size_t limit;

	if (bs->max_value < 0)
		return;

	if (bs1->max_value < bs->max_value)
		bs->max_value = bs1->max_value;

	if (bs->max_value >= 0 && bs1->max_value >= 0)
		limit = (bs->max_value / (sizeof(unsigned int) * 8)) + 1;
	else
		limit = 0;

	for (i = 0; i < limit; i++)
		bs->bits[i] &= bs1->bits[i];

	for (i = limit; i < bs->allocated; i++)
		bs->bits[i] = 0;

	if (limit > 0)
		prv_set_max(bs, limit - 1);
}

void subtilis_bitset_or(subtilis_bitset_t *bs, subtilis_bitset_t *bs1,
			subtilis_error_t *err)
{
	size_t i;
	size_t limit;

	if (bs1->max_value < 0)
		return;

	prv_ensure(bs, bs1->max_value, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	limit = (bs1->max_value / (sizeof(unsigned int) * 8)) + 1;
	for (i = 0; i < limit; i++)
		bs->bits[i] |= bs1->bits[i];

	if (bs->max_value < bs1->max_value)
		bs->max_value = bs1->max_value;
}

void subtilis_bitset_dump(subtilis_bitset_t *bs)
{
	size_t i;

	if (bs->max_value > 0) {
		for (i = 0; i <= bs->max_value; i++) {
			if (subtilis_bitset_isset(bs, i))
				printf("%zu ", i);
		}
	}
	printf("\n");
}

void subtilis_bitset_claim(subtilis_bitset_t *dst, subtilis_bitset_t *src)
{
	subtilis_bitset_free(dst);
	dst->bits = src->bits;
	dst->max_value = src->max_value;
	dst->allocated = src->allocated;
	subtilis_bitset_init(src);
}

void subtilis_bitset_free(subtilis_bitset_t *bs) { free(bs->bits); }
