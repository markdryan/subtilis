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

#include "prespilt_offsets.h"

void subtilis_prespilt_offsets_init(subtilis_prespilt_offsets_t *off)
{
	off->int_offsets = NULL;
	off->real_offsets = NULL;
	off->int_count = 0;
	off->real_count = 0;
}

void subtilis_prespilt_free(subtilis_prespilt_offsets_t *off)
{
	free(off->int_offsets);
	free(off->real_offsets);
}

size_t subtilis_prespilt_calculate(subtilis_prespilt_offsets_t *off,
				   subtilis_bitset_t *int_save,
				   subtilis_bitset_t *real_save,
				   subtilis_error_t *err)
{
	size_t space;

	off->int_offsets =
	    subtilis_bitset_values(int_save, &off->int_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	off->real_offsets =
	    subtilis_bitset_values(real_save, &off->real_count, err);

	space = (off->int_count * sizeof(int32_t)) +
		(off->real_count * sizeof(double));

	/*
	 * Make sure that the double section is 8 byte aligned.
	 */

	if (off->int_count & 1)
		space += sizeof(int32_t);

	return space;
}

size_t subtilis_prespilt_int_offset(subtilis_prespilt_offsets_t *off,
				    size_t reg, subtilis_error_t *err)
{
	size_t i;

	for (i = 0; i < off->int_count; i++)
		if (off->int_offsets[i] == reg)
			return i * sizeof(int32_t);

	subtilis_error_set_assertion_failed(err);

	return 0;
}

size_t subtilis_prespilt_real_offset(subtilis_prespilt_offsets_t *off,
				     size_t reg, subtilis_error_t *err)
{
	size_t i;
	size_t offset = off->int_count * sizeof(int32_t);

	if (off->int_count & 1)
		offset += sizeof(int32_t);

	for (i = 0; i < off->real_count; i++)
		if (off->real_offsets[i] == reg)
			return offset + (i * sizeof(double));

	subtilis_error_set_assertion_failed(err);

	return 0;
}

void subtilis_compute_save_sets(subtilis_bitset_t *old_link1_save,
				subtilis_bitset_t *old_link2_save,
				subtilis_bitset_t *common_save,
				subtilis_bitset_t *link1_save,
				subtilis_bitset_t *link2_save,
				subtilis_error_t *err)
{
	subtilis_bitset_or(common_save, old_link1_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_bitset_and(common_save, old_link2_save);

	subtilis_bitset_or(link1_save, old_link1_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_bitset_or(link2_save, old_link2_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_bitset_sub(link1_save, common_save);
	subtilis_bitset_sub(link2_save, common_save);
}
