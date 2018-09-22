/*
 * Copyright (c) 2017 Mark Ryan
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

#ifndef __SUBTILIS_ARM_REG_ALLOC_H
#define __SUBTILIS_ARM_REG_ALLOC_H

#include "arm_core.h"

struct subtilis_dist_data_t_ {
	size_t reg_num;
	int last_used;
};

typedef struct subtilis_dist_data_t_ subtilis_dist_data_t;

size_t subtilis_arm_reg_alloc(subtilis_arm_section_t *arm_s,
			      subtilis_error_t *err);

size_t subtilis_arm_regs_used_before(subtilis_arm_section_t *arm_s,
				     subtilis_arm_op_t *op,
				     subtilis_error_t *err);
size_t subtilis_arm_regs_used_after(subtilis_arm_section_t *arm_s,
				    subtilis_arm_op_t *op,
				    subtilis_error_t *err);

void subtilis_arm_save_regs(subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err);

#endif
