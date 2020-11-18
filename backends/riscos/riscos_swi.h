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

#ifndef SUBTILIS_RISCOS_SWI_H__
#define SUBTILIS_RISCOS_SWI_H__

#include <stddef.h>
#include <stdint.h>

#include "../../arch/arm32/arm_swi.h"

extern const size_t subtilis_riscos_known_swis;

extern const subtilis_arm_swi_t subtilis_riscos_swi_list[];
extern const size_t subtilis_riscos_swi_index[];

#endif
