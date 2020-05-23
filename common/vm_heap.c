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

#include <stdint.h>
#include <stdlib.h>

#include "vm_heap.h"

void subtilis_vm_heap_init(subtilis_vm_heap_t *heap)
{
	heap->free_list = NULL;
	heap->used_list = NULL;
}

subtilis_vm_heap_free_block_t *subtilis_vm_heap_new_block(uint32_t start,
							  uint32_t size)
{
	subtilis_vm_heap_free_block_t *block;

	block = (subtilis_vm_heap_free_block_t *)malloc(sizeof(*block));
	if (!block)
		return NULL;

	block->start = start;
	block->size = size;
	block->next = NULL;

	return block;
}

subtilis_vm_heap_free_block_t *
subtilis_vm_heap_find_block(subtilis_vm_heap_t *heap, uint32_t start)
{
	subtilis_vm_heap_free_block_t *block;

	block = heap->used_list;
	while (block && block->start != start)
		block = block->next;

	return block;
}

subtilis_vm_heap_free_block_t *
subtilis_vm_heap_claim_block(subtilis_vm_heap_t *heap, uint32_t size,
			     subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	subtilis_vm_heap_free_block_t *new_block;

	new_block = NULL;
	block = heap->free_list;
	while (block && block->size < size) {
		new_block = block;
		block = block->next;
	}
	if (!block)
		return NULL;
	if (block->size == size) {
		if (new_block)
			new_block->next = block->next;
		else
			heap->free_list = block->next;
		block->next = heap->used_list;
		heap->used_list = block;
	} else {
		new_block = subtilis_vm_heap_new_block(block->start, size);
		if (!new_block) {
			subtilis_error_set_oom(err);
			return NULL;
		}
		new_block->next = heap->used_list;
		heap->used_list = new_block;

		block->start += size;
		block->size -= size;
		block = new_block;
	}

	return block;
}

void subtilis_vm_heap_free_block(subtilis_vm_heap_t *heap, uint32_t ptr,
				 subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	subtilis_vm_heap_free_block_t *new_block;

	new_block = NULL;
	block = heap->used_list;
	while (block && block->start != ptr) {
		new_block = block;
		block = block->next;
	}
	if (!block) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	if (new_block)
		new_block->next = block->next;
	else
		heap->used_list = block->next;
	block->next = heap->free_list;
	heap->free_list = block;
}

subtilis_vm_heap_free_block_t *
subtilis_vm_heap_realloc(subtilis_vm_heap_t *heap, uint32_t ptr, int32_t size,
			 uint32_t *old_size, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	subtilis_vm_heap_free_block_t *new_block;

	block = subtilis_vm_heap_find_block(heap, ptr);
	if (!block) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	new_block = subtilis_vm_heap_claim_block(heap, size, err);
	if (!new_block || err->type != SUBTILIS_ERROR_OK)
		return NULL;

	*old_size = block->size;

	return new_block;
}

void subtilis_vm_heap_free(subtilis_vm_heap_t *heap)
{
	subtilis_vm_heap_free_block_t *ptr;
	subtilis_vm_heap_free_block_t *next;

	ptr = heap->free_list;
	while (ptr) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}

	ptr = heap->used_list;
	while (ptr) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
}

size_t subtilis_vm_heap_free_space(subtilis_vm_heap_t *heap)
{
	subtilis_vm_heap_free_block_t *ptr;
	size_t sum = 0;

	ptr = heap->free_list;
	while (ptr) {
		sum += ptr->size;
		ptr = ptr->next;
	}

	return sum;
}
