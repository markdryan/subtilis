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

#include <stdio.h>

#include "../../common/buffer.h"
#include "arm_core.h"

#define SUBTILIS_ARM_VM_MAX_FILES 16

struct subtilis_arm_vm_freg_t_ {
	union {
		double real64;
		float real32;
	} val;
	size_t size;
};

typedef struct subtilis_arm_vm_freg_t_ subtilis_arm_vm_freg_t;

union subtilis_arm_vm_vfpregs_t_ {
	double d[16];
	float f[32];
};

typedef union subtilis_arm_vm_vfpregs_t_ subtilis_arm_vm_vfpregs_t;

struct subtilis_arm_vm_t_ {
	int32_t regs[16];
	subtilis_arm_vm_freg_t fregs[8];
	subtilis_arm_vm_vfpregs_t vpfregs;
	uint32_t fpa_status;
	uint8_t *memory;
	size_t mem_size;
	size_t stack_size;
	size_t heap_size;
	size_t code_size;
	size_t op_len;
	bool negative_flag;
	bool zero_flag;
	bool carry_flag;
	bool overflow_flag;
	bool quit;
	bool reverse_fpa_consts;
	int32_t start_address;
	uint32_t fpscr;
	bool vfp;
	// clang-format off
	FILE * files[SUBTILIS_ARM_VM_MAX_FILES];

	// clang-format on
};

typedef struct subtilis_arm_vm_t_ subtilis_arm_vm_t;

subtilis_arm_vm_t *subtilis_arm_vm_new(uint8_t *code, size_t code_size,
				       size_t mem_size, int32_t start_address,
				       bool vfp, subtilis_error_t *err);
void subtilis_arm_vm_delete(subtilis_arm_vm_t *vm);
void subtilis_arm_vm_run(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			 subtilis_error_t *err);

#endif
