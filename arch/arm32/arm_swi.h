/*
 * Copyright (c) 2020 Mark Ryan
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

#ifndef SUBTILIS_ARM_SWI_H__
#define SUBTILIS_ARM_SWI_H__

#include <stddef.h>
#include <stdint.h>

struct subtilis_arm_swi_t_ {
	uint32_t num;
	const char *name;
	uint32_t in_regs;
	uint32_t out_regs;
};

typedef struct subtilis_arm_swi_t_ subtilis_arm_swi_t;

struct subtilis_arm_swi_info_t_ {
	size_t count;
	const subtilis_arm_swi_t *swi_list;
	const size_t *swi_index;
};

typedef struct subtilis_arm_swi_info_t_ subtilis_arm_swi_info_t;

#endif
