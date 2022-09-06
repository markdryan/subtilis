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

#ifndef __SUBTILIS_VM_H
#define __SUBTILIS_VM_H

#include <stdint.h>
#include <stdio.h>

#include "../common/buffer.h"
#include "../common/ir.h"
#include "../common/vm_heap.h"
#include "symbol_table.h"

#define SUBTILIS_VM_HEAP_SIZE (32 * 1024)
#define SUBTILIS_VM_MAX_FILES 16
/*
 * TODO: We need this as we only have 32 bit registers.
 */

struct subitlis_vm_t_ {
	/* TODO: This type should be configurable to allow for 64 bit regs */
	int32_t *regs;
	size_t max_regs;
	double *fregs;
	size_t max_fregs;
	uint8_t *memory;
	size_t memory_size;
	subtilis_ir_prog_t *p;
	subtilis_ir_section_t *s; /*I'd like to get rid of this */
	size_t current_index;
	subtilis_symbol_table_t *st;
	size_t pc;
	size_t *labels;
	size_t label_len;
	size_t max_labels;
	size_t top;
	bool quit_flag;
	subtilis_vm_heap_t heap;
	size_t *constants;
	size_t max_constants;
	// clang-format off
	FILE * files[SUBTILIS_VM_MAX_FILES];

	// clang-format on
	size_t cmd_line_ptr;
};

typedef struct subitlis_vm_t_ subitlis_vm_t;

subitlis_vm_t *subitlis_vm_new(subtilis_ir_prog_t *p,
			       subtilis_symbol_table_t *st, int argc,
			       char *argv[], subtilis_error_t *err);
void subitlis_vm_run(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_error_t *err);
void subitlis_vm_delete(subitlis_vm_t *vm);

#endif
