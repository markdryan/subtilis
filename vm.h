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

#include "buffer.h"
#include "ir.h"
#include "symbol_table.h"

struct subitlis_vm_t_ {
	/* TODO: This type should be configurable to allow for 64 bit regs */
	int32_t *regs;
	uint8_t *globals;
	subtilis_ir_program_t *p;
	subtilis_symbol_table_t *st;
	size_t pc;
};

typedef struct subitlis_vm_t_ subitlis_vm_t;

subitlis_vm_t *subitlis_vm_new(subtilis_ir_program_t *p,
			       subtilis_symbol_table_t *st,
			       subtilis_error_t *err);
void subitlis_vm_run(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_error_t *err);
void subitlis_vm_delete(subitlis_vm_t *vm);

#endif
