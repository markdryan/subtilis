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

#ifndef REGS_USED_VIRT_H
#define REGS_USED_VIRT_H

#include "bitset.h"

struct subtilis_regs_used_virt_t_ {
	subtilis_bitset_t int_regs;
	subtilis_bitset_t real_regs;
};

typedef struct subtilis_regs_used_virt_t_ subtilis_regs_used_virt_t;


void subtilis_regs_used_virt_init(subtilis_regs_used_virt_t *regs_usedv);
void subtilis_regs_used_virt_free(subtilis_regs_used_virt_t *regs_usedv);

struct subtilis_dist_data_t_ {
	size_t reg_num;
	int last_used;
};

typedef struct subtilis_dist_data_t_ subtilis_dist_data_t;

struct subtilis_regs_used_t_ {
	size_t int_regs;
	size_t real_regs;
};

typedef struct subtilis_regs_used_t_ subtilis_regs_used_t;


#endif
