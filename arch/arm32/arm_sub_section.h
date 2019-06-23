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

#ifndef __SUBTILIS_ARM_SUB_SECTION_H
#define __SUBTILIS_ARM_SUB_SECTION_H

#include "../../common/bitset.h"
#include "arm_core.h"

struct subtilis_arm_ss_link_t_ {
	subtilis_bitset_t int_outputs;
	subtilis_bitset_t real_outputs;
	subtilis_bitset_t int_save;
	subtilis_bitset_t real_save;
	size_t link;
	size_t op;
};

typedef struct subtilis_arm_ss_link_t_ subtilis_arm_ss_link_t;

struct subtilis_arm_ss_t_ {
	size_t size;
	size_t start;
	size_t end;
	subtilis_bitset_t int_inputs;
	subtilis_bitset_t real_inputs;
	subtilis_arm_ss_link_t links[2];
	size_t num_links;
};

typedef struct subtilis_arm_ss_t_ subtilis_arm_ss_t;

struct subtilis_arm_subsections_t_ {
	subtilis_arm_ss_t *sub_sections;
	size_t count;
	size_t max_count;
	size_t link_count;
	size_t max_links;
	size_t *ss_link_map;
	subtilis_bitset_t int_save;
	subtilis_bitset_t real_save;
};

typedef struct subtilis_arm_subsections_t_ subtilis_arm_subsections_t;

void subtilis_arm_subsections_init(subtilis_arm_subsections_t *sss);
void subtilis_arm_subsections_calculate(subtilis_arm_subsections_t *sss,
					subtilis_arm_section_t *arm_s,
					subtilis_error_t *err);
void subtilis_arm_subsections_must_save(subtilis_arm_subsections_t *sss,
					subtilis_arm_ss_t *ss, size_t link,
					subtilis_bitset_t *int_save,
					subtilis_bitset_t *real_save,
					subtilis_error_t *err);
void subtilis_arm_subsections_dump(subtilis_arm_subsections_t *sss,
				   subtilis_arm_section_t *arm_s);
void subtilis_arm_subsections_free(subtilis_arm_subsections_t *sss);

#endif
