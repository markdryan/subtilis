/*
 * Copyright (c) 2017-2023 Mark Ryan
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

#ifndef PRESPILT_OFFSETS_H
#define PRESPILT_OFFSETS_H

#include <stddef.h>
#include <stdlib.h>

#include "bitset.h"
#include "error.h"

struct subtilis_prespilt_offsets_t_ {
	size_t *int_offsets;
	size_t *real_offsets;
	size_t int_count;
	size_t real_count;
};

typedef struct subtilis_prespilt_offsets_t_ subtilis_prespilt_offsets_t;

void subtilis_prespilt_offsets_init(subtilis_prespilt_offsets_t *off);
void subtilis_prespilt_free(subtilis_prespilt_offsets_t *off);
size_t subtilis_prespilt_calculate(subtilis_prespilt_offsets_t *off,
				   subtilis_bitset_t *int_save,
				   subtilis_bitset_t *real_save,
				   subtilis_error_t *err);
size_t subtilis_prespilt_int_offset(subtilis_prespilt_offsets_t *off,
				    size_t reg, subtilis_error_t *err);
size_t subtilis_prespilt_real_offset(subtilis_prespilt_offsets_t *off,
				     size_t reg, subtilis_error_t *err);
void subtilis_compute_save_sets(subtilis_bitset_t *old_link1_save,
				subtilis_bitset_t *old_link2_save,
				subtilis_bitset_t *common_save,
				subtilis_bitset_t *link1_save,
				subtilis_bitset_t *link2_save,
				subtilis_error_t *err);
#endif
