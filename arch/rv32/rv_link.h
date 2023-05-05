/*
 * Copyright (c) 2023 Mark Ryan
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

#ifndef SUBTILIS_RV_LINK_H
#define SUBTILIS_RV_LINK_H

#include <stdint.h>

#include "../../common/error.h"

struct subtilis_rv_link_constant_t_ {
	size_t index;
	size_t code_index;
};

typedef struct subtilis_rv_link_constant_t_ subtilis_rv_link_constant_t;

struct subtilis_rv_link_t_ {
	size_t *externals;
	size_t num_externals;
	size_t max_externals;
	size_t *sections;
	size_t num_sections;
	subtilis_rv_link_constant_t *constants;
	size_t num_constants;
	size_t max_constants;
	subtilis_rv_link_constant_t *extrefs;
	size_t num_extrefs;
	size_t max_extrefs;
};

typedef struct subtilis_rv_link_t_ subtilis_rv_link_t;

subtilis_rv_link_t *subtilis_rv_link_new(size_t sections,
					 subtilis_error_t *err);
void subtilis_rv_link_add(subtilis_rv_link_t *link, size_t offset,
			  subtilis_error_t *err);
void subtilis_rv_link_constant_add(subtilis_rv_link_t *link, size_t offset,
				   size_t constant_index,
				   subtilis_error_t *err);
void subtilis_rv_link_extref_add(subtilis_rv_link_t *link, size_t offset,
				 size_t section_index,
				 subtilis_error_t *err);
void subtilis_rv_link_section(subtilis_rv_link_t *link, size_t num,
			      size_t offset);
void subtilis_rv_link_encode_jal(uint32_t *ptr, int32_t offset);
void subtilis_rv_link_fixup_relative(uint8_t *buf, size_t buf_size,
				     size_t code_index, int32_t dist,
				     subtilis_rv_instr_type_t itype2,
				     subtilis_error_t *err);
void subtilis_rv_link_link(subtilis_rv_link_t *link, uint8_t *buf,
			   size_t buf_size, const size_t *raw_constants,
			   size_t num_raw_constants, subtilis_error_t *err);
void subtilis_rv_link_delete(subtilis_rv_link_t *resolve);

#endif