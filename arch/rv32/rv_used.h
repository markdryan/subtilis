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

#ifndef __SUBTILIS_RV_USED_H
#define __SUBTILIS_RV_USED_H

#include "rv32_core.h"
#include "rv_walker.h"

#include "../../common/regs_used_virt.h"

void subtilis_rv_int_regs_used_before_from_tov(subtilis_rv_section_t *rv_s,
					       subtilis_rv_op_t *from,
					       subtilis_rv_op_t *op,
					       size_t int_args,
					       subtilis_regs_used_virt_t *used,
					       subtilis_error_t *err);

void subtilis_rv_real_regs_used_before_from_tov(subtilis_rv_section_t *rv_s,
						subtilis_rv_op_t *from,
						subtilis_rv_op_t *op,
						size_t real_args,
						subtilis_regs_used_virt_t *used,
						subtilis_error_t *err);

void subtilis_rv_int_regs_used_afterv(subtilis_rv_section_t *rv_s,
				      subtilis_rv_op_t *from,
				      subtilis_rv_op_t *to, size_t int_args,
				      size_t count,
				      subtilis_regs_used_virt_t *used,
				      subtilis_error_t *err);

void subtilis_rv_real_regs_used_afterv(subtilis_rv_section_t *rv_s,
				       subtilis_rv_op_t *from,
				       subtilis_rv_op_t *op,
				       size_t real_args,
				       size_t count,
				       subtilis_regs_used_virt_t *used,
				       subtilis_error_t *err);

#endif
