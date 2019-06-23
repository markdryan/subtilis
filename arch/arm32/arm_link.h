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

struct subtilis_arm_link_t_ {
	size_t *externals;
	size_t num_externals;
	size_t max_externals;
	size_t *sections;
	size_t num_sections;
};

typedef struct subtilis_arm_link_t_ subtilis_arm_link_t;

subtilis_arm_link_t *subtilis_arm_link_new(size_t sections,
					   subtilis_error_t *err);
void subtilis_arm_link_add(subtilis_arm_link_t *link, size_t offset,
			   subtilis_error_t *err);
void subtilis_arm_link_section(subtilis_arm_link_t *link, size_t num,
			       size_t offset);
void subtilis_arm_link_link(subtilis_arm_link_t *link, uint32_t *buf,
			    size_t buf_size, subtilis_error_t *err);
void subtilis_arm_link_delete(subtilis_arm_link_t *resolve);

#endif
