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

#ifndef SUBTILIS_RV_REAL_USED_H
#define SUBTILIS_RV_REAL_USED_H

#include "rv32_core.h"
#include "rv_walker.h"

#include "../../common/regs_used_virt.h"


void subtilis_rv_init_real_used_walker(subtilis_rv_walker_t *walker,
				       void *user_data);


#endif
