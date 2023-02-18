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

#include "regs_used_virt.h"

void subtilis_regs_used_virt_init(subtilis_regs_used_virt_t *regs_usedv)
{
	subtilis_bitset_init(&regs_usedv->int_regs);
	subtilis_bitset_init(&regs_usedv->real_regs);
}

void subtilis_regs_used_virt_free(subtilis_regs_used_virt_t *regs_usedv)
{
	subtilis_bitset_free(&regs_usedv->int_regs);
	subtilis_bitset_free(&regs_usedv->real_regs);
}
