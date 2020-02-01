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

void subtilis_arm_link_constant_add(subtilis_arm_link_t *link,
				    size_t code_index, size_t constant_offset,
				    size_t constant_index,
				    subtilis_error_t *err)
{
	size_t new_max;
	subtilis_arm_link_constant_t *new_constants;

	if (link->num_constants == link->max_constants) {
		new_max = link->num_constants + SUBTILIS_CONFIG_PROC_GRAN;
		new_constants = realloc(link->constants,
					new_max * sizeof(*link->constants));
		if (!new_constants) {
			subtilis_error_set_oom(err);
			return;
		}
		link->constants = new_constants;
		link->max_constants = new_max;
	}

	link->constants[link->num_constants].index = constant_index;
	link->constants[link->num_constants].constant_offset = constant_offset;
	link->constants[link->num_constants++].code_index = code_index;
}

void subtilis_arm_link_section(subtilis_arm_link_t *link, size_t num,
			       size_t offset)
{
	link->sections[num] = offset;
}

void subtilis_arm_link_link(subtilis_arm_link_t *link, uint32_t *buf,
			    size_t buf_size, const size_t *raw_constants,
			    size_t num_raw_constants, subtilis_error_t *err)
{
	size_t i;
	size_t si;
	size_t index;
	subtilis_arm_link_constant_t *cnst;

	for (i = 0; i < link->num_externals; i++) {
		index = link->externals[i];
		if (index >= buf_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		si = buf[index] & 0xffffff;
		if (si >= link->num_sections) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (link->sections[si] == SIZE_MAX) {
			/* TODO: Need proper error here */
			subtilis_error_set_assertion_failed(err);
			return;
		}
		buf[index] &= 0xff000000;
		buf[index] |= (link->sections[si] - (index + 2)) & 0xffffff;
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
		buf[cnst->constant_offset] =
		    (raw_constants[cnst->index] - cnst->code_index) * 4;
		/* Adjust for PC relative addressing. */
		buf[cnst->constant_offset] -= 12;
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
