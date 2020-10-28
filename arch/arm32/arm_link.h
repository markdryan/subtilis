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

#ifndef __SUBTILIS_ARM_LINK_H
#define __SUBTILIS_ARM_LINK_H

#include <stdint.h>

#include "../../common/error.h"

struct subtilis_arm_link_constant_t_ {
	size_t index;
	size_t code_index;
	size_t constant_offset;
};

typedef struct subtilis_arm_link_constant_t_ subtilis_arm_link_constant_t;

struct subtilis_arm_link_t_ {
	size_t *externals;
	size_t num_externals;
	size_t max_externals;
	size_t *sections;
	size_t num_sections;
	subtilis_arm_link_constant_t *constants;
	size_t num_constants;
	size_t max_constants;
};

typedef struct subtilis_arm_link_t_ subtilis_arm_link_t;

subtilis_arm_link_t *subtilis_arm_link_new(size_t sections,
					   subtilis_error_t *err);
void subtilis_arm_link_add(subtilis_arm_link_t *link, size_t offset,
			   subtilis_error_t *err);
void subtilis_arm_link_constant_add(subtilis_arm_link_t *link, size_t offset,
				    size_t constant_offset,
				    size_t constant_index,
				    subtilis_error_t *err);
void subtilis_arm_link_section(subtilis_arm_link_t *link, size_t num,
			       size_t offset);
void subtilis_arm_link_link(subtilis_arm_link_t *link, uint8_t *buf,
			    size_t buf_size, const size_t *raw_constants,
			    size_t num_raw_constants, subtilis_error_t *err);
void subtilis_arm_link_delete(subtilis_arm_link_t *resolve);

#endif
