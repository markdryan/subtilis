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

#ifndef __SUBTILIS_ARM_VM_H
#define __SUBTILIS_ARM_VM_H

#include "arm_core.h"
#include "buffer.h"

struct subtilis_arm_vm_t_ {
	int32_t *regs;
	uint8_t *memory;
	size_t mem_size;
	subtilis_arm_section_t *s;
	size_t *labels;
	size_t label_len;
	subtilis_arm_op_t **ops;
	size_t op_len;
	bool negative_flag;
	bool zero_flag;
	bool carry_flag;
	bool overflow_flag;
};

typedef struct subtilis_arm_vm_t_ subtilis_arm_vm_t;

subtilis_arm_vm_t *subtilis_arm_vm_new(subtilis_arm_prog_t *arm_p,
				       size_t mem_size, subtilis_error_t *err);
void subtilis_arm_vm_delete(subtilis_arm_vm_t *vm);
void subtilis_arm_vm_run(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			 subtilis_error_t *err);

#endif
