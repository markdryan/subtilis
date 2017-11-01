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

#ifndef __SUBTILIS_RISCOS_ARM_H
#define __SUBTILIS_RISCOS_ARM_H

#include "arm_core.h"

/* clang-format off */
subtilis_arm_program_t *
subtilis_riscos_generate(
	subtilis_ir_program_t *p, const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals, subtilis_error_t *err);
/* clang-format on */

#endif
