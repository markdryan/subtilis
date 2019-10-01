/*
 * Copyright (c) 2019 Mark Ryan
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

#ifndef __SUBTILIS_VM_HEAP_H
#define __SUBTILIS_VM_HEAP_H

#include "error.h"

typedef struct subtilis_vm_heap_free_block_t_ subtilis_vm_heap_free_block_t;
struct subtilis_vm_heap_free_block_t_ {
	uint32_t start;
	uint32_t size;
	subtilis_vm_heap_free_block_t *next;
};

struct subtilis_vm_heap_t_ {
	subtilis_vm_heap_free_block_t *free_list;
	subtilis_vm_heap_free_block_t *used_list;
};

typedef struct subtilis_vm_heap_t_ subtilis_vm_heap_t;

void subtilis_vm_heap_init(subtilis_vm_heap_t *heap);
subtilis_vm_heap_free_block_t *
subtilis_vm_heap_claim_block(subtilis_vm_heap_t *heap, uint32_t size,
			     subtilis_error_t *err);
subtilis_vm_heap_free_block_t *
subtilis_vm_heap_find_block(subtilis_vm_heap_t *heap, uint32_t start);
subtilis_vm_heap_free_block_t *subtilis_vm_heap_new_block(uint32_t start,
							  uint32_t size);
void subtilis_vm_heap_free_block(subtilis_vm_heap_t *heap, uint32_t ptr,
				 subtilis_error_t *err);
subtilis_vm_heap_free_block_t *
subtilis_vm_heap_realloc(subtilis_vm_heap_t *heap, uint32_t ptr, int32_t size,
			 uint32_t *old_size, subtilis_error_t *err);

void subtilis_vm_heap_free(subtilis_vm_heap_t *heap);

#endif
