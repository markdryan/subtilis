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

#ifndef __SUBTILIS_ARM2_DIV_H__
#define __SUBTILIS_ARM2_DIV_H__

#include "arm_core.h"
#include "common/ir.h"

void subtilis_arm2_idiv_add(subtilis_ir_section_t *s,
			    subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err);

#endif
