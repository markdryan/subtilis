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

#ifndef __SUBTILIS_BITSET_H
#define __SUBTILIS_BITSET_H

#include "error.h"

struct subtilis_bitset_t_ {
	unsigned int *bits;
	int max_value;
	size_t allocated;
};

typedef struct subtilis_bitset_t_ subtilis_bitset_t;

void subtilis_bitset_init(subtilis_bitset_t *bs);
void subtilis_bitset_reset(subtilis_bitset_t *bs);
void subtilis_bitset_set(subtilis_bitset_t *bs, size_t bit,
			 subtilis_error_t *err);
bool subtilis_bitset_isset(subtilis_bitset_t *bs, size_t bit);
void subtilis_bitset_clear(subtilis_bitset_t *bs, size_t bit);
void subtilis_bitset_and(subtilis_bitset_t *bs, subtilis_bitset_t *bs1);
void subtilis_bitset_sub(subtilis_bitset_t *bs, subtilis_bitset_t *bs1);
void subtilis_bitset_or(subtilis_bitset_t *bs, subtilis_bitset_t *bs1,
			subtilis_error_t *err);
void subtilis_bitset_not(subtilis_bitset_t *bs);
void subtilis_bitset_dump(subtilis_bitset_t *bs);
void subtilis_bitset_claim(subtilis_bitset_t *dst, subtilis_bitset_t *src);
void subtilis_bitset_free(subtilis_bitset_t *bs);
size_t subtilis_bitset_count(subtilis_bitset_t *bs);
size_t *subtilis_bitset_values(subtilis_bitset_t *bs, size_t *count,
			       subtilis_error_t *err);

#endif
