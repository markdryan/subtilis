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

#include <stdlib.h>

#include "arm_link.h"

subtilis_arm_link_t *subtilis_arm_link_new(size_t sections,
					   subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_link_t *link = calloc(1, sizeof(*link));

	if (!link) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	link->sections = malloc(sections * sizeof(*link->sections));
	if (!link->sections) {
		free(link);
		subtilis_error_set_oom(err);
		return NULL;
	}
	link->num_sections = sections;

	for (i = 0; i < sections; i++)
		link->sections[i] = SIZE_MAX;

	return link;
}

void subtilis_arm_link_add(subtilis_arm_link_t *link, size_t offset,
			   subtilis_error_t *err)
{
	size_t new_max;
	size_t *new_ext;

	if (link->num_externals == link->max_externals) {
		new_max = link->num_externals + SUBTILIS_CONFIG_PROC_GRAN;
		new_ext = realloc(link->externals, new_max * sizeof(*new_ext));
		if (!new_ext) {
			subtilis_error_set_oom(err);
			return;
		}
		link->externals = new_ext;
		link->max_externals = new_max;
	}

	link->externals[link->num_externals++] = offset;
}

static void prv_ensure_const_arr(subtilis_arm_link_constant_t **arr,
				 size_t num_constants, size_t *max_constants,
				 subtilis_error_t *err)
{
	size_t new_max;
	subtilis_arm_link_constant_t *new_constants;

	if (num_constants == *max_constants) {
		new_max = num_constants + SUBTILIS_CONFIG_PROC_GRAN;
		new_constants = realloc(*arr, new_max * sizeof(**arr));
		if (!new_constants) {
			subtilis_error_set_oom(err);
			return;
		}
		*arr = new_constants;
		*max_constants = new_max;
	}
}

void subtilis_arm_link_constant_add(subtilis_arm_link_t *link,
				    size_t code_index, size_t constant_offset,
				    size_t constant_index,
				    subtilis_error_t *err)
{
	prv_ensure_const_arr(&link->constants, link->num_constants,
			     &link->max_constants, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	link->constants[link->num_constants].index = constant_index;
	link->constants[link->num_constants].constant_offset = constant_offset;
	link->constants[link->num_constants++].code_index = code_index;
}

void subtilis_arm_link_extref_add(subtilis_arm_link_t *link, size_t code_index,
				  size_t constant_offset, size_t section_index,
				  subtilis_error_t *err)
{
	prv_ensure_const_arr(&link->extrefs, link->num_extrefs,
			     &link->max_extrefs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	link->extrefs[link->num_extrefs].index = section_index;
	link->extrefs[link->num_extrefs].constant_offset = constant_offset;
	link->extrefs[link->num_extrefs++].code_index = code_index;
}

void subtilis_arm_link_section(subtilis_arm_link_t *link, size_t num,
			       size_t offset)
{
	link->sections[num] = offset;
}

static uint32_t *prv_get_word_ptr(uint8_t *buf, size_t index,
				  subtilis_error_t *err)
{
	if (index & 3) {
		subtilis_error_set_ass_bad_alignment(err);
		return NULL;
	}

	return (uint32_t *)&buf[index];
}

void subtilis_arm_link_link(subtilis_arm_link_t *link, uint8_t *buf,
			    size_t buf_size, const size_t *raw_constants,
			    size_t num_raw_constants, subtilis_error_t *err)
{
	size_t i;
	size_t si;
	size_t index;
	uint32_t *ptr;
	subtilis_arm_link_constant_t *cnst;

	for (i = 0; i < link->num_externals; i++) {
		index = link->externals[i];
		if (index >= buf_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		ptr = prv_get_word_ptr(buf, index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		si = *ptr & 0xffffff;
		if (si >= link->num_sections) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (link->sections[si] == SIZE_MAX) {
			/* TODO: Need proper error here */
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*ptr &= 0xff000000;
		*ptr |= ((link->sections[si] - (index + 8)) / 4) & 0xffffff;
	}

	for (i = 0; i < link->num_constants; i++) {
		cnst = &link->constants[i];
		if (cnst->constant_offset >= buf_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (cnst->index >= num_raw_constants) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		ptr = prv_get_word_ptr(buf, cnst->constant_offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*ptr = (raw_constants[cnst->index] - cnst->code_index);
		/* Adjust for PC relative addressing. */
		*ptr -= 12;
	}

	for (i = 0; i < link->num_extrefs; i++) {
		cnst = &link->extrefs[i];
		if (cnst->constant_offset >= buf_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (cnst->index >= link->num_sections) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		ptr = prv_get_word_ptr(buf, cnst->constant_offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*ptr = ((link->sections[cnst->index] - cnst->code_index) - 12);
	}
}

void subtilis_arm_link_delete(subtilis_arm_link_t *link)
{
	if (!link)
		return;

	free(link->constants);
	free(link->externals);
	free(link->sections);
	free(link);
}
