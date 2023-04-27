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

#ifndef __SUBTILIS_RV_SUB_SECTION_H
#define __SUBTILIS_RV_SUB_SECTION_H

#include "../../common/bitset.h"
#include "rv32_core.h"

struct subtilis_rv_ss_link_t_ {
	subtilis_bitset_t int_outputs;
	subtilis_bitset_t real_outputs;
	subtilis_bitset_t int_save;
	subtilis_bitset_t real_save;
	size_t link;
	size_t op;
};

typedef struct subtilis_rv_ss_link_t_ subtilis_rv_ss_link_t;

struct subtilis_rv_ss_t_ {
	size_t size;
	size_t start;
	size_t load_spill;
	size_t end;
	subtilis_bitset_t int_inputs;
	subtilis_bitset_t real_inputs;
	subtilis_rv_ss_link_t links[2];
	size_t num_links;
};

typedef struct subtilis_rv_ss_t_ subtilis_rv_ss_t;

struct subtilis_rv_subsections_t_ {
	subtilis_rv_ss_t *sub_sections;
	size_t count;
	size_t max_count;
	size_t link_count;
	size_t max_links;
	size_t *ss_link_map;
	subtilis_bitset_t int_save;
	subtilis_bitset_t real_save;
};

typedef struct subtilis_rv_subsections_t_ subtilis_rv_subsections_t;

void subtilis_rv_subsections_init(subtilis_rv_subsections_t *sss);
void subtilis_rv_subsections_calculate(subtilis_rv_subsections_t *sss,
					subtilis_rv_section_t *arm_s,
					subtilis_error_t *err);
void subtilis_rv_subsections_must_save(subtilis_rv_subsections_t *sss,
					subtilis_rv_ss_t *ss, size_t link,
					subtilis_bitset_t *int_save,
					subtilis_bitset_t *real_save,
					subtilis_error_t *err);
void subtilis_rv_subsections_dump(subtilis_rv_subsections_t *sss,
				   subtilis_rv_section_t *rv_s);
void subtilis_rv_subsections_free(subtilis_rv_subsections_t *sss);

#endif
