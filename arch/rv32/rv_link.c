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

#include "rv32_core.h"
#include "rv_opcodes.h"
#include "rv_link.h"

subtilis_rv_link_t *subtilis_rv_link_new(size_t sections,
					 subtilis_error_t *err)
{
	size_t i;
	subtilis_rv_link_t *link = calloc(1, sizeof(*link));

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

void subtilis_rv_link_add(subtilis_rv_link_t *link, size_t offset,
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

static void prv_ensure_const_arr(subtilis_rv_link_constant_t **arr,
				 size_t num_constants, size_t *max_constants,
				 subtilis_error_t *err)
{
	size_t new_max;
	subtilis_rv_link_constant_t *new_constants;

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

void subtilis_rv_link_constant_add(subtilis_rv_link_t *link,
				   size_t code_index,
				   size_t constant_index,
				   subtilis_error_t *err)
{
	prv_ensure_const_arr(&link->constants, link->num_constants,
			     &link->max_constants, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	link->constants[link->num_constants].index = constant_index;
	link->constants[link->num_constants++].code_index = code_index;
}

void subtilis_rv_link_extref_add(subtilis_rv_link_t *link, size_t code_index,
				 size_t section_index,
				 subtilis_error_t *err)
{
	prv_ensure_const_arr(&link->extrefs, link->num_extrefs,
			     &link->max_extrefs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	link->extrefs[link->num_extrefs].index = section_index;
	link->extrefs[link->num_extrefs++].code_index = code_index;
}

void subtilis_rv_link_section(subtilis_rv_link_t *link, size_t num,
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

void subtilis_rv_link_encode_jal(uint32_t *ptr, int32_t offset)
{
	offset >>= 1;

	*ptr |= (offset & 0x3ff) << 21;
	if (offset & (1 << 10))
		*ptr |= 1 << 20;
	*ptr |= (offset & 0x7f800) << 1;
	if (offset & (1 << 19))
		*ptr |= 0x80000000;
}

static void prv_fixup_relative(uint8_t *buf, size_t buf_size,
			       subtilis_rv_link_constant_t *cnst,
			       int32_t dist, subtilis_error_t *err)
{
	uint32_t *auipc;
	uint32_t *addi;
	const rv_opcode_t *op_code;

	/*
	 * We need at least two instructions
	 */

	if (cnst->code_index + 8 > buf_size) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	auipc = prv_get_word_ptr(buf, cnst->code_index, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	addi = prv_get_word_ptr(buf, cnst->code_index + 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * check this is an aupic
	 */
	op_code = &rv_opcodes[SUBTILIS_RV_AUIPC];
	if ((*auipc & 0x7f) != op_code->opcode) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/*
	 * check this is an addi
	 */

	op_code = &rv_opcodes[SUBTILIS_RV_ADDI];
	if (((*addi & 0x7f) != op_code->opcode) ||
	    ((*addi >> 12) & 0x7) != op_code->funct3) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	*auipc |= dist & 0xfffff000;
	*addi |= (dist & 0xfff) << 20;
}

void subtilis_rv_link_link(subtilis_rv_link_t *link, uint8_t *buf,
			   size_t buf_size, const size_t *raw_constants,
			   size_t num_raw_constants, subtilis_error_t *err)
{
	size_t i;
	size_t si;
	size_t index;
	uint32_t *ptr;
	int32_t dist;
	subtilis_rv_link_constant_t *cnst;
	int32_t offset;

	/*
	 * For direct function calls.
	 */

	for (i = 0; i < link->num_externals; i++) {
		index = link->externals[i];
		if (index >= buf_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		ptr = prv_get_word_ptr(buf, index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		/*
		 * The encoder sticks the section number we're jumping to
		 * in the instruction.
		 */

		si = *ptr >> 12;

		if (si >= link->num_sections) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		if (link->sections[si] == SIZE_MAX) {
			/* TODO: Need proper error here */
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*ptr &= 0xfff;
		offset = (link->sections[si] - index);
		if (offset & 1) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		subtilis_rv_link_encode_jal(ptr, offset);
	}

	/*
	 * For real constants and buffers.  Here we're just adjusting the
	 * offset in the la (auipc and addi) instructions.
	 */

	for (i = 0; i < link->num_constants; i++) {
		cnst = &link->constants[i];

		if (cnst->index >= num_raw_constants) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		dist = raw_constants[cnst->index] - cnst->code_index;
		prv_fixup_relative(buf, buf_size, cnst, dist, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/*
	 * For function pointers.
	 */

	for (i = 0; i < link->num_extrefs; i++) {
		cnst = &link->extrefs[i];
		if (cnst->index >= link->num_sections) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		dist = link->sections[cnst->index] - cnst->code_index;
		prv_fixup_relative(buf, buf_size, cnst, dist, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_rv_link_delete(subtilis_rv_link_t *link)
{
	if (!link)
		return;

	free(link->extrefs);
	free(link->constants);
	free(link->externals);
	free(link->sections);
	free(link);
}
