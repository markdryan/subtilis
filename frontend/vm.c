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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../common/error_codes.h"
#include "../common/utils.h"
#include "vm.h"

static void prv_ensure_label_buffer(subitlis_vm_t *vm, size_t label,
				    subtilis_error_t *err)
{
	size_t new_max;
	size_t *new_labels;

	if (label < vm->max_labels)
		return;

	new_max = label + SUBTILIS_CONFIG_LABEL_GRAN;
	new_labels = realloc(vm->labels, new_max * sizeof(size_t));
	if (!new_labels) {
		subtilis_error_set_oom(err);
		return;
	}
	vm->max_labels = new_max;
	vm->labels = new_labels;
}

static void prv_compute_labels(subitlis_vm_t *vm, subtilis_error_t *err)
{
	size_t i;
	size_t label;

	for (i = 0; i < vm->s->len; i++) {
		if (vm->s->ops[i]->type != SUBTILIS_OP_LABEL)
			continue;
		label = vm->s->ops[i]->op.label;
		prv_ensure_label_buffer(vm, label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		vm->labels[label] = i;
	}
}

static void prv_reserve_stack(subitlis_vm_t *vm, size_t bytes,
			      subtilis_error_t *err)
{
	size_t needed;
	size_t new_max;
	uint8_t *new_mem;

	needed = vm->top + bytes;
	if (needed <= vm->memory_size)
		return;

	new_max = vm->memory_size + 4096;
	if (needed > new_max)
		new_max = needed;
	new_mem = realloc(vm->memory, sizeof(*vm->memory) * new_max);
	if (!new_mem) {
		subtilis_error_set_oom(err);
		return;
	}
	vm->memory = new_mem;
	vm->memory_size = new_max;
}

subitlis_vm_t *subitlis_vm_new(subtilis_ir_prog_t *p,
			       subtilis_symbol_table_t *st,
			       subtilis_error_t *err)
{
	size_t const_size;
	size_t i;
	size_t ptr;
	subitlis_vm_t *vm = calloc(sizeof(*vm), 1);

	if (!vm) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	vm->labels = NULL;
	vm->max_labels = 0;
	vm->p = p;
	vm->s = p->sections[0];
	vm->max_regs = vm->s->reg_counter;
	vm->max_fregs = vm->s->freg_counter;
	vm->st = st;

	vm->regs = calloc(sizeof(int32_t), vm->max_regs);
	if (!vm->regs) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	vm->fregs = calloc(sizeof(double), vm->max_fregs);
	if (!vm->fregs) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	const_size = subtilis_constant_pool_mem_size(p->constant_pool,
						     &vm->constants, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;
	vm->max_constants = p->constant_pool->size;

	vm->memory_size = const_size + SUBTILIS_VM_HEAP_SIZE +
			  st->max_allocated + vm->s->locals;

	vm->memory = calloc(sizeof(uint8_t), vm->memory_size);
	if (!vm->memory) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	ptr = 0;
	for (i = 0; i < p->constant_pool->size; i++) {
		memcpy(&vm->memory[ptr], p->constant_pool->data[i].data,
		       p->constant_pool->data[i].data_size);
		ptr += p->constant_pool->data[i].data_size;
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;
	vm->regs[SUBTILIS_IR_REG_GLOBAL] = SUBTILIS_VM_HEAP_SIZE;
	vm->regs[SUBTILIS_IR_REG_LOCAL] =
	    vm->regs[SUBTILIS_IR_REG_GLOBAL] + st->max_allocated;
	vm->top = vm->memory_size;

	prv_compute_labels(vm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

	subtilis_vm_heap_init(&vm->heap);
	vm->heap.free_list =
	    subtilis_vm_heap_new_block(const_size, SUBTILIS_VM_HEAP_SIZE);
	if (!vm->heap.free_list) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	return vm;

fail:

	subitlis_vm_delete(vm);
	return NULL;
}

typedef void (*subtilis_vm_op_fn)(subitlis_vm_t *, subtilis_buffer_t *,
				  subtilis_ir_operand_t *, subtilis_error_t *);

static void prv_addi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] + vm->regs[ops[2].integer];
}

static void prv_addr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] + vm->fregs[ops[2].reg];
}

static void prv_subi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] - vm->regs[ops[2].integer];
}

static void prv_subr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] - vm->fregs[ops[2].reg];
}

static void prv_muli32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] * vm->regs[ops[2].integer];
}

static void prv_mulr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] * vm->fregs[ops[2].reg];
}

static void prv_movii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = ops[1].integer;
}

static void prv_movir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = ops[1].real;
}

static void prv_mov(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg];
}

static void prv_movfp(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg];
}

static void prv_storeoi8(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *dst = &vm->memory[base + ops[2].integer];

	*dst = (uint8_t)vm->regs[ops[0].reg];
}

static void prv_storeoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			  subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *dst = &vm->memory[base + ops[2].integer];

	memcpy(dst, &vm->regs[ops[0].reg], sizeof(int32_t));
}

static void prv_storeor(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *dst = &vm->memory[base + ops[2].integer];

	memcpy(dst, &vm->fregs[ops[0].reg], sizeof(double));
}

static void prv_loadoi8(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *src = &vm->memory[base + ops[2].integer];

	vm->regs[ops[0].reg] = *src;
}

static void prv_loadoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *src = &vm->memory[base + ops[2].integer];

	memcpy(&vm->regs[ops[0].reg], src, sizeof(int32_t));
}

static void prv_loador(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *src = &vm->memory[base + ops[2].integer];

	memcpy(&vm->fregs[ops[0].reg], src, sizeof(double));
}

static void prv_divide_by_zero(subitlis_vm_t *vm, int32_t code)
{
	int32_t base = vm->regs[SUBTILIS_IR_REG_GLOBAL];

	vm->memory[base + vm->s->eflag_offset] = -1;
	vm->memory[base + vm->s->error_offset] = code;
}

static void prv_divi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t divisor = vm->regs[ops[2].reg];

	if (divisor == 0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_DIV_BY_ZERO);
		return;
	}
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] / divisor;
}

static void prv_divr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	double divisor = vm->fregs[ops[2].reg];

	if (divisor == 0.0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_DIV_BY_ZERO);
		return;
	}
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] / divisor;
}

static void prv_modi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t divisor = vm->regs[ops[2].reg];

	if (divisor == 0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_DIV_BY_ZERO);
		return;
	}
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] % divisor;
}

static void prv_addii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] + ops[2].integer;
}

static void prv_addir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] + ops[2].real;
}

static void prv_subii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] - ops[2].integer;
}

static void prv_subir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] - ops[2].real;
}

static void prv_mulii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] * ops[2].integer;
}

static void prv_mulir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] * ops[2].real;
}

static void prv_divii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] / ops[2].integer;
}

static void prv_divir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = vm->fregs[ops[1].reg] / ops[2].real;
}

static void prv_printi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	char buf[64];

	sprintf(buf, "%d", vm->regs[ops[0].reg]);
	subtilis_buffer_append_string(b, buf, err);
}

static void prv_printfp(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	char buf[64];

	sprintf(buf, "%f", vm->fregs[ops[0].reg]);
	subtilis_buffer_append_string(b, buf, err);
}

static void prv_printstr(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	uint8_t *buf = &vm->memory[vm->regs[ops[0].reg]];

	subtilis_buffer_append(b, buf, vm->regs[ops[1].reg], err);
}

static void prv_printnl(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_buffer_append_string(b, "\n", err);
}

static void prv_rsubii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = ops[2].integer - vm->regs[ops[1].reg];
}

static void prv_rsubir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = ops[2].real - vm->fregs[ops[1].reg];
}

static void prv_rdivir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	double divisor = vm->fregs[ops[1].reg];

	if (divisor == 0.0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_DIV_BY_ZERO);
		return;
	}
	vm->fregs[ops[0].reg] = ops[2].real / divisor;
}

static void prv_andi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] & vm->regs[ops[2].integer];
}

static void prv_andii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] & ops[2].integer;
}

static void prv_ori32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] | vm->regs[ops[2].integer];
}

static void prv_orii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] | ops[2].integer;
}

static void prv_eori32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] ^ vm->regs[ops[2].integer];
}

static void prv_eorii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] ^ ops[2].integer;
}

static void prv_noti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = ~vm->regs[ops[1].reg];
}

static void prv_eqi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] == vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_eqr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] == vm->fregs[ops[2].reg] ? -1 : 0;
}

static void prv_eqii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] == ops[2].integer ? -1 : 0;
}

static void prv_eqir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] == ops[2].real ? -1 : 0;
}

static void prv_neqi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] == vm->regs[ops[2].integer] ? 0 : -1;
}

static void prv_neqr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] == vm->fregs[ops[2].reg] ? 0 : -1;
}

static void prv_neqii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] == ops[2].integer ? 0 : -1;
}

static void prv_neqir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] == ops[2].real ? 0 : -1;
}

static void prv_gti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] > vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_gtr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] > vm->fregs[ops[2].reg] ? -1 : 0;
}

static void prv_gtii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] > ops[2].integer ? -1 : 0;
}

static void prv_gtir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] > ops[2].real ? -1 : 0;
}

static void prv_ltei32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] <= vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_lter(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] <= vm->fregs[ops[2].reg] ? -1 : 0;
}

static void prv_lteii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] <= ops[2].integer ? -1 : 0;
}

static void prv_lteir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] <= ops[2].real ? -1 : 0;
}

static void prv_lti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] < vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_ltr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] < vm->fregs[ops[2].reg] ? -1 : 0;
}

static void prv_ltii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] < ops[2].integer ? -1 : 0;
}

static void prv_ltir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] < ops[2].real ? -1 : 0;
}

static void prv_gtei32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] >= vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_gter(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->fregs[ops[1].reg] >= vm->fregs[ops[2].reg] ? -1 : 0;
}

static void prv_gteii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] >= ops[2].integer ? -1 : 0;
}

static void prv_gteir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->fregs[ops[1].reg] >= ops[2].real ? -1 : 0;
}

static void prv_jmpc(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t label;

	label = (vm->regs[ops[0].reg]) ? ops[1].label : ops[2].label;
	if (label > vm->max_labels) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	vm->pc = vm->labels[label];
	if (vm->pc >= vm->s->len) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
}

static void prv_jmp(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t label;

	label = ops[0].label;
	if (label > vm->max_labels) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	vm->pc = vm->labels[label];
	if (vm->pc >= vm->s->len) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
}

static void prv_set_args(subitlis_vm_t *vm, subtilis_ir_call_t *call,
			 subtilis_error_t *err)
{
	size_t i;
	size_t int_args = 0;
	size_t real_args = 0;
	double *real_args_copy = NULL;
	int32_t *int_args_copy = NULL;

	real_args_copy = malloc(sizeof(double) * call->arg_count);
	if (!real_args_copy) {
		subtilis_error_set_oom(err);
		return;
	}

	int_args_copy = malloc(sizeof(int32_t) * call->arg_count);
	if (!real_args_copy) {
		free(real_args_copy);
		subtilis_error_set_oom(err);
		return;
	}

	for (i = 0; i < call->arg_count; i++) {
		if (call->args[i].type == SUBTILIS_IR_REG_TYPE_REAL)
			real_args_copy[real_args++] =
			    vm->fregs[call->args[i].reg];
		else
			int_args_copy[int_args++] = vm->regs[call->args[i].reg];
	}

	for (i = 0; i < int_args; i++)
		vm->regs[SUBTILIS_IR_REG_TEMP_START + i] = int_args_copy[i];

	for (i = 0; i < real_args; i++)
		vm->fregs[i] = real_args_copy[i];

	free(int_args_copy);
	free(real_args_copy);
}

static void prv_memset(subitlis_vm_t *vm, subtilis_ir_call_t *call,
		       subtilis_error_t *err)
{
	memset(&vm->memory[vm->regs[call->args[0].reg]],
	       vm->regs[call->args[2].reg], vm->regs[call->args[1].reg]);
}

static void prv_memcpy(subitlis_vm_t *vm, subtilis_ir_call_t *call,
		       subtilis_error_t *err)
{
	memcpy(&vm->memory[vm->regs[call->args[0].reg]],
	       &vm->memory[vm->regs[call->args[1].reg]],
	       vm->regs[call->args[2].reg]);
}

static void prv_memcmp(subitlis_vm_t *vm, subtilis_ir_call_t *call,
		       subtilis_error_t *err)
{
	const char *a = (const char *)&vm->memory[vm->regs[call->args[0].reg]];
	const char *b = (const char *)&vm->memory[vm->regs[call->args[1].reg]];
	size_t len = vm->regs[call->args[2].reg];

	vm->regs[call->reg] = memcmp(a, b, len) == 0 ? -1 : 0;
}

static void prv_compare(subitlis_vm_t *vm, subtilis_ir_call_t *call,
			subtilis_error_t *err)
{
	const char *a = (const char *)&vm->memory[vm->regs[call->args[0].reg]];
	size_t a_len = vm->regs[call->args[1].reg];
	const char *b = (const char *)&vm->memory[vm->regs[call->args[2].reg]];
	size_t b_len = vm->regs[call->args[3].reg];

	if (a_len == 0 && b_len > 0) {
		vm->regs[call->reg] = -1;
	} else if (a_len > 0 && b_len == 0) {
		vm->regs[call->reg] = 1;
	} else {
		if (b_len < a_len)
			a_len = b_len;

		vm->regs[call->reg] = strncmp(a, b, a_len);
	}
}

static void prv_handle_builtin(subitlis_vm_t *vm, subtilis_builtin_type_t ftype,
			       subtilis_ir_call_t *call, subtilis_error_t *err)
{
	switch (ftype) {
	case SUBTILIS_BUILTINS_MEMSETI32:
		return prv_memset(vm, call, err);
	case SUBTILIS_BUILTINS_MEMCPY:
		return prv_memcpy(vm, call, err);
	case SUBTILIS_BUILTINS_MEMCMP:
		return prv_memcmp(vm, call, err);
	case SUBTILIS_BUILTINS_COMPARE:
		return prv_compare(vm, call, err);
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_call(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     const subtilis_type_t *call_type, subtilis_ir_call_t *call,
		     subtilis_error_t *err)
{
	subtilis_ir_section_t *s;
	int32_t *new_regs;
	double *new_fregs;
	size_t space_needed;
	size_t section_index = call->proc_id;

	if (section_index >= vm->p->num_sections) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	s = vm->p->sections[section_index];
	if (s->ftype != SUBTILIS_BUILTINS_MAX) {
		prv_handle_builtin(vm, s->ftype, call, err);
		return;
	}

	if (s->reg_counter > vm->max_regs) {
		new_regs =
		    realloc(vm->regs, sizeof(*vm->regs) * s->reg_counter);
		if (!new_regs) {
			subtilis_error_set_oom(err);
			return;
		}
		vm->regs = new_regs;
		vm->max_regs = s->reg_counter;
	}

	if (s->freg_counter > vm->max_fregs) {
		new_fregs =
		    realloc(vm->fregs, sizeof(*vm->fregs) * s->freg_counter);
		if (!new_fregs) {
			subtilis_error_set_oom(err);
			return;
		}
		vm->fregs = new_fregs;
		vm->max_fregs = s->freg_counter;
	}

	space_needed = ((vm->s->label_counter) * sizeof(*vm->labels));
	space_needed += ((vm->s->reg_counter + 2) * sizeof(*vm->regs));
	space_needed += ((vm->s->freg_counter) * sizeof(*vm->fregs));
	space_needed += s->locals;
	if (call_type->type != SUBTILIS_TYPE_VOID)
		space_needed += sizeof(*vm->regs);
	prv_reserve_stack(vm, space_needed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	memcpy(&vm->memory[vm->top], &vm->labels[0],
	       vm->s->label_counter * sizeof(*vm->labels));
	vm->top += vm->s->label_counter * sizeof(*vm->labels);
	memcpy(&vm->memory[vm->top], &vm->regs[0],
	       vm->s->reg_counter * sizeof(*vm->regs));
	vm->top += vm->s->reg_counter * sizeof(*vm->regs);

	memcpy(&vm->memory[vm->top], &vm->fregs[0],
	       vm->s->freg_counter * sizeof(*vm->fregs));
	vm->top += vm->s->freg_counter * sizeof(*vm->fregs);

	*((int32_t *)&vm->memory[vm->top]) = vm->pc;
	vm->top += 4;
	*((int32_t *)&vm->memory[vm->top]) = vm->current_index;
	vm->top += 4;
	if (call_type->type != SUBTILIS_TYPE_VOID) {
		*((int32_t *)&vm->memory[vm->top]) = call->reg;
		vm->top += 4;
	}
	prv_set_args(vm, call, err);
	vm->regs[SUBTILIS_IR_REG_LOCAL] = vm->top;

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->locals > 0) {
		memset(&vm->memory[vm->top], 0, s->locals);
		vm->top += s->locals;
	}

	vm->s = s;
	vm->current_index = section_index;
	vm->pc = -1;

	prv_compute_labels(vm, err);
}

static size_t prv_ret_gen(subitlis_vm_t *vm, const subtilis_type_t *call_type,
			  subtilis_buffer_t *b, subtilis_ir_operand_t *ops,
			  subtilis_error_t *err)
{
	size_t caller_index;
	subtilis_ir_section_t *cs;
	size_t to_pop;
	size_t reg = SUBTILIS_IR_REG_UNDEFINED;

	to_pop = vm->s->locals + sizeof(*vm->regs);
	if (call_type->type != SUBTILIS_TYPE_VOID)
		to_pop += sizeof(*vm->regs);

	if (to_pop > vm->top) {
		subtilis_error_set_assertion_failed(err);
		return reg;
	}

	vm->top -= vm->s->locals + sizeof(*vm->regs);
	if (call_type->type != SUBTILIS_TYPE_VOID) {
		reg = *((int32_t *)&vm->memory[vm->top]);
		vm->top -= sizeof(*vm->regs);
	}

	caller_index = *((int32_t *)&vm->memory[vm->top]);
	if (caller_index >= vm->p->num_sections) {
		subtilis_error_set_assertion_failed(err);
		return reg;
	}

	cs = vm->p->sections[caller_index];

	to_pop = (cs->reg_counter + 1) * sizeof(*vm->regs);
	to_pop += cs->freg_counter * sizeof(*vm->fregs);
	if (to_pop > vm->top) {
		subtilis_error_set_assertion_failed(err);
		return reg;
	}
	vm->s = cs;
	vm->top -= sizeof(*vm->regs);
	vm->pc = *((int32_t *)&vm->memory[vm->top]);
	vm->current_index = caller_index;

	vm->top -= vm->s->freg_counter * sizeof(*vm->fregs);
	memcpy(&vm->fregs[0], &vm->memory[vm->top],
	       vm->s->freg_counter * sizeof(*vm->fregs));

	vm->top -= vm->s->reg_counter * sizeof(*vm->regs);
	memcpy(&vm->regs[0], &vm->memory[vm->top],
	       vm->s->reg_counter * sizeof(*vm->regs));

	vm->top -= vm->s->label_counter * sizeof(*vm->labels);
	memcpy(&vm->labels[0], &vm->memory[vm->top],
	       vm->s->label_counter * sizeof(*vm->labels));

	return reg;
}

static void prv_ret(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	(void)prv_ret_gen(vm, &subtilis_type_void, b, ops, err);
}

static void prv_reti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t reg;
	int32_t val;

	val = vm->regs[ops[0].reg];

	reg = prv_ret_gen(vm, &subtilis_type_integer, b, ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vm->regs[reg] = val;
}

static void prv_retii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t reg;

	reg = prv_ret_gen(vm, &subtilis_type_integer, b, ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vm->regs[reg] = ops[0].integer;
}

static void prv_retr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t reg;
	double val;

	val = vm->fregs[ops[0].reg];

	reg = prv_ret_gen(vm, &subtilis_type_real, b, ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vm->fregs[reg] = val;
}

static void prv_retir(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t reg;

	reg = prv_ret_gen(vm, &subtilis_type_real, b, ops, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vm->fregs[reg] = ops[0].real;
}

static void prv_lsli32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg]
			       << (vm->regs[ops[2].reg] & 63);
}

static void prv_lslii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] << (ops[2].integer & 63);
}

static void prv_lsri32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	uint32_t r1 = (uint32_t)vm->regs[ops[1].reg];

	vm->regs[ops[0].reg] = (int32_t)(r1 >> (vm->regs[ops[2].reg] & 63));
}

static void prv_lsrii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	uint32_t r1 = (uint32_t)vm->regs[ops[1].reg];

	vm->regs[ops[0].reg] = r1 >> (ops[2].integer & 63);
}

static void prv_asri32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].reg] >> (vm->regs[ops[2].reg] & 63);
}

static void prv_asrii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] >> (ops[2].integer & 63);
}

static void prv_movi32fp(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = (double)vm->regs[ops[1].reg];
}

static void prv_movfpi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = (int32_t)vm->fregs[ops[1].reg];
}

static void prv_movfprdi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			   subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = (int32_t)floor(vm->fregs[ops[1].reg]);
}

static void prv_nop(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
}

static void prv_gettime(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = subtilis_get_i32_time();
}

static void prv_sin(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = sin(vm->fregs[ops[1].reg]);
}

static void prv_cos(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = cos(vm->fregs[ops[1].reg]);
}

static void prv_tan(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = tan(vm->fregs[ops[1].reg]);
}

static void prv_asn(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = asin(vm->fregs[ops[1].reg]);
}

static void prv_acs(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = acos(vm->fregs[ops[1].reg]);
}

static void prv_atn(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = atan(vm->fregs[ops[1].reg]);
}

static void prv_sqr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = sqrt(vm->fregs[ops[1].reg]);
}

static void prv_log(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	double arg = vm->fregs[ops[1].reg];

	if (arg <= 0.0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_LOG_RANGE);
		return;
	}
	vm->fregs[ops[0].reg] = log10(arg);
}

static void prv_ln(subitlis_vm_t *vm, subtilis_buffer_t *b,
		   subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	double arg = vm->fregs[ops[1].reg];

	if (arg <= 0.0) {
		prv_divide_by_zero(vm, SUBTILIS_ERROR_CODE_LOG_RANGE);
		return;
	}

	vm->fregs[ops[0].reg] = log(arg);
}

static void prv_absr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] = fabs(vm->fregs[ops[1].reg]);
}

static void prv_powr(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->fregs[ops[0].reg] =
	    pow(vm->fregs[ops[1].reg], vm->fregs[ops[2].reg]);
}

static void prv_get(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = getchar();
}

static void prv_osbyteid(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = 1;
}

static void prv_vdui(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	char buf[2];

	buf[0] = ops[0].integer;
	buf[1] = 0;
	subtilis_buffer_append_string(b, buf, err);
}

static void prv_vdu(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	char buf[2];

	buf[0] = vm->regs[ops[0].reg];
	buf[1] = 0;
	subtilis_buffer_append_string(b, buf, err);
}

static void prv_point(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = 0;
}

static void prv_tint(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = 0;
}

static void prv_end(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->quit_flag = true;
}

static void prv_testesc(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
}

static void prv_check_heap(subitlis_vm_t *vm, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;

	block = vm->heap.used_list;
	while (block) {
		printf("block %d size %d still allocated\n", block->start,
		       block->size);
		subtilis_error_set_assertion_failed(err);
		block = block->next;
	}
}

static void prv_alloc(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	size_t to_alloc = vm->regs[ops[1].reg] + sizeof(size_t);
	int32_t base = vm->regs[SUBTILIS_IR_REG_GLOBAL];

	block = subtilis_vm_heap_claim_block(&vm->heap, to_alloc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (!block) {
		vm->memory[base + vm->s->eflag_offset] = -1;
		vm->memory[base + vm->s->error_offset] =
		    SUBTILIS_ERROR_CODE_OOM;
		vm->regs[ops[2].reg] = 0;
		return;
	}

	*((size_t *)&vm->memory[block->start]) = 1;
	vm->regs[ops[0].reg] = block->start + sizeof(size_t);
}

static void prv_realloc(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	uint32_t old_size;
	size_t start = vm->regs[ops[0].reg] - sizeof(size_t);
	size_t size = vm->regs[ops[1].reg] + sizeof(size_t);
	int32_t base = vm->regs[SUBTILIS_IR_REG_GLOBAL];

	block =
	    subtilis_vm_heap_realloc(&vm->heap, start, size, &old_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (!block) {
		vm->memory[base + vm->s->eflag_offset] = -1;
		vm->memory[base + vm->s->error_offset] =
		    SUBTILIS_ERROR_CODE_OOM;
		vm->regs[ops[2].reg] = 0;
		return;
	}

	memcpy(&vm->memory[block->start], &vm->memory[start], old_size);
	vm->regs[ops[2].reg] = block->start + sizeof(size_t);
}

static void prv_ref(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	size_t *ptr;
	size_t start = vm->regs[ops[0].reg] - sizeof(size_t);

	block = subtilis_vm_heap_find_block(&vm->heap, start);
	if (!block) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	ptr = (size_t *)&vm->memory[block->start];
	*ptr = *ptr + 1;
}

static void prv_deref(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	size_t *count;
	size_t start = vm->regs[ops[0].reg] - sizeof(size_t);

	block = subtilis_vm_heap_find_block(&vm->heap, start);
	if (!block) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	count = (size_t *)&vm->memory[block->start];
	if (*count == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (*count > 1) {
		*count = *count - 1;
		return;
	}

	subtilis_vm_heap_free_block(&vm->heap, start, err);
}

static void prv_getref(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	subtilis_vm_heap_free_block_t *block;
	size_t *count;
	size_t start = vm->regs[ops[1].reg] - sizeof(size_t);

	block = subtilis_vm_heap_find_block(&vm->heap, start);
	if (!block) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	count = (size_t *)&vm->memory[block->start];
	if (*count == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	vm->regs[ops[0].reg] = (int32_t)*count;
}

static void prv_pushi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t *ptr;

	prv_reserve_stack(vm, 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ptr = (int32_t *)&vm->memory[vm->top];
	*ptr = vm->regs[ops[0].reg];
	vm->top += 4;
}

static void prv_popi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t *ptr;

	vm->top -= 4;
	ptr = (int32_t *)&vm->memory[vm->top];
	vm->regs[ops[0].reg] = *ptr;
}

static void prv_lca(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->constants[ops[1].integer];
}

static void prv_pos(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = 0;
}

/* clang-format off */
static subtilis_vm_op_fn op_execute_fns[] = {
	prv_addi32,                          /* SUBTILIS_OP_INSTR_ADD_I32 */
	prv_addr,                            /* SUBTILIS_OP_INSTR_ADD_REAL */
	prv_subi32,                          /* SUBTILIS_OP_INSTR_SUB_I32 */
	prv_subr,                            /* SUBTILIS_OP_INSTR_SUB_REAL */
	prv_muli32,                          /* SUBTILIS_OP_INSTR_MUL_I32 */
	prv_mulr,                            /* SUBTILIS_OP_INSTR_MUL_REAL */
	prv_divi32,                          /* SUBTILIS_OP_INSTR_DIV_I32 */
	prv_modi32,                          /* SUBTILIS_OP_INSTR_MOD_I32 */
	prv_divr,                            /* SUBTILIS_OP_INSTR_DIV_REAL */
	prv_addii32,                         /* SUBTILIS_OP_INSTR_ADDI_I32 */
	prv_addir,                           /* SUBTILIS_OP_INSTR_ADDI_REAL */
	prv_subii32,                         /* SUBTILIS_OP_INSTR_SUBI_I32 */
	prv_subir,                           /* SUBTILIS_OP_INSTR_SUBI_REAL */
	prv_mulii32,                         /* SUBTILIS_OP_INSTR_MULI_I32 */
	prv_mulir,                           /* SUBTILIS_OP_INSTR_MULI_REAL */
	prv_divii32,                         /* SUBTILIS_OP_INSTR_DIVI_I32 */
	prv_divir,                           /* SUBTILIS_OP_INSTR_DIVI_REAL */
	prv_loadoi8,                         /* SUBTILIS_OP_INSTR_LOADO_I8 */
	prv_loadoi32,                        /* SUBTILIS_OP_INSTR_LOADO_I32 */
	prv_loador,                          /* SUBTILIS_OP_INSTR_LOADO_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_LOAD_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_LOAD_REAL */
	prv_storeoi8,                        /* SUBTILIS_OP_INSTR_STOREO_I8 */
	prv_storeoi32,                       /* SUBTILIS_OP_INSTR_STOREO_I32 */
	prv_storeor,                         /* SUBTILIS_OP_INSTR_STOREO_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_STORE_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_STORE_REAL */
	prv_movii32,                         /* SUBTILIS_OP_INSTR_MOVI_I32 */
	prv_movir,                           /* SUBTILIS_OP_INSTR_MOVI_REAL */
	prv_mov,                             /* SUBTILIS_OP_INSTR_MOV */
	prv_movfp,                           /* SUBTILIS_OP_INSTR_MOVFP */
	prv_printi32,                        /* SUBTILIS_OP_INSTR_PRINT_I32 */
	prv_printfp,                         /* SUBTILIS_OP_INSTR_PRINT_FP */
	prv_printstr,                        /* SUBTILIS_OP_INSTR_PRINT_STR */
	prv_printnl,                         /* SUBTILIS_OP_INSTR_PRINT_NL */
	prv_rsubii32,                        /* SUBTILIS_OP_INSTR_RSUBI_I32 */
	prv_rsubir,                          /* SUBTILIS_OP_INSTR_RSUBI_REAL */
	prv_rdivir,                          /* SUBTILIS_OP_INSTR_RDIVI_REAL */
	prv_andi32,                          /* SUBTILIS_OP_INSTR_AND_I32 */
	prv_andii32,                         /* SUBTILIS_OP_INSTR_ANDI_I32 */
	prv_ori32,                           /* SUBTILIS_OP_INSTR_OR_I32 */
	prv_orii32,                          /* SUBTILIS_OP_INSTR_ORI_I32 */
	prv_eori32,                          /* SUBTILIS_OP_INSTR_EOR_I32 */
	prv_eorii32,                         /* SUBTILIS_OP_INSTR_EORI_I32 */
	prv_noti32,                          /* SUBTILIS_OP_INSTR_NOT_I32 */
	prv_eqi32,                           /* SUBTILIS_OP_INSTR_EQ_I32 */
	prv_eqr,                             /* SUBTILIS_OP_INSTR_EQ_REAL */
	prv_eqii32,                          /* SUBTILIS_OP_INSTR_EQI_I32 */
	prv_eqir,                            /* SUBTILIS_OP_INSTR_EQI_REAL */
	prv_neqi32,                          /* SUBTILIS_OP_INSTR_NEQ_I32 */
	prv_neqr,                            /* SUBTILIS_OP_INSTR_NEQ_REAL */
	prv_neqii32,                         /* SUBTILIS_OP_INSTR_NEQI_I32 */
	prv_neqir,                           /* SUBTILIS_OP_INSTR_NEQI_REAL */
	prv_gti32,                           /* SUBTILIS_OP_INSTR_GT_I32 */
	prv_gtr,                             /* SUBTILIS_OP_INSTR_GT_REAL */
	prv_gtii32,                          /* SUBTILIS_OP_INSTR_GTI_I32 */
	prv_gtir,                            /* SUBTILIS_OP_INSTR_GTI_REAL */
	prv_ltei32,                          /* SUBTILIS_OP_INSTR_LTE_I32 */
	prv_lter,                            /* SUBTILIS_OP_INSTR_LTE_REAL */
	prv_lteii32,                         /* SUBTILIS_OP_INSTR_LTEI_I32 */
	prv_lteir,                           /* SUBTILIS_OP_INSTR_LTEI_REAL */
	prv_lti32,                           /* SUBTILIS_OP_INSTR_LT_I32 */
	prv_ltr,                             /* SUBTILIS_OP_INSTR_LT_REAL */
	prv_ltii32,                          /* SUBTILIS_OP_INSTR_LTI_I32 */
	prv_ltir,                            /* SUBTILIS_OP_INSTR_LTI_REAL */
	prv_gtei32,                          /* SUBTILIS_OP_INSTR_GTE_I32 */
	prv_gter,                            /* SUBTILIS_OP_INSTR_GTE_REAL */
	prv_gteii32,                         /* SUBTILIS_OP_INSTR_GTEI_I32 */
	prv_gteir,                           /* SUBTILIS_OP_INSTR_GTEI_REAL */
	prv_jmpc,                            /* SUBTILIS_OP_INSTR_JMPC */
	prv_jmpc,                            /* SUBTILIS_OP_INSTR_JMPC_NF */
	prv_jmp,                             /* SUBTILIS_OP_INSTR_JMP */
	prv_ret,                             /* SUBTILIS_OP_INSTR_RET */
	prv_reti32,                          /* SUBTILIS_OP_INSTR_RET_I32 */
	prv_retii32,                         /* SUBTILIS_OP_INSTR_RETI_I32 */
	prv_retr,                            /* SUBTILIS_OP_INSTR_RET_REAL */
	prv_retir,                           /* SUBTILIS_OP_INSTR_RETI_REAL */
	prv_lsli32,                          /* SUBTILIS_OP_INSTR_LSL_I32 */
	prv_lslii32,                         /* SUBTILIS_OP_INSTR_LSLI_I32 */
	prv_lsri32,                          /* SUBTILIS_OP_INSTR_LSR_I32 */
	prv_lsrii32,                         /* SUBTILIS_OP_INSTR_LSRI_I32 */
	prv_asri32,                          /* SUBTILIS_OP_INSTR_ASR_I32 */
	prv_asrii32,                         /* SUBTILIS_OP_INSTR_ASRI_I32 */
	prv_movi32fp,                        /* SUBTILIS_OP_INSTR_MOV_I32_FP */
	prv_movfpi32,                        /* SUBTILIS_OP_INSTR_MOV_FP_I32 */
	prv_movfprdi32,                      /* SUBTILIS_OP_INSTR_MOV_FPRD_I32*/
	prv_nop,                             /* SUBTILIS_OP_INSTR_NOP */
	prv_nop,                             /* SUBTILIS_OP_INSTR_MODE_I32 */
	prv_nop,                             /* SUBTILIS_OP_INSTR_PLOT */
	prv_nop,                             /* SUBTILIS_OP_INSTR_GCOL */
	prv_nop,                             /* SUBTILIS_OP_INSTR_ORIGIN */
	prv_gettime,                         /* SUBTILIS_OP_INSTR_GETTIME */
	prv_nop,                             /* SUBTILIS_OP_INSTR_CLS */
	prv_nop,                             /* SUBTILIS_OP_INSTR_CLG */
	prv_nop,                             /* SUBTILIS_OP_INSTR_ON */
	prv_nop,                             /* SUBTILIS_OP_INSTR_OFF */
	prv_nop,                             /* SUBTILIS_OP_INSTR_WAIT */
	prv_sin,                             /* SUBTILIS_OP_INSTR_SIN */
	prv_cos,                             /* SUBTILIS_OP_INSTR_COS */
	prv_tan,                             /* SUBTILIS_OP_INSTR_TAN */
	prv_asn,                             /* SUBTILIS_OP_INSTR_ASN */
	prv_acs,                             /* SUBTILIS_OP_INSTR_ACS */
	prv_atn,                             /* SUBTILIS_OP_INSTR_ATN */
	prv_sqr,                             /* SUBTILIS_OP_INSTR_SQR */
	prv_log,                             /* SUBTILIS_OP_INSTR_LOG */
	prv_ln,                              /* SUBTILIS_OP_INSTR_LN */
	prv_absr,                            /* SUBTILIS_OP_INSTR_ABSR */
	prv_powr,                            /* SUBTILIS_OP_INSTR_POWR */
	prv_get,                             /* SUBTILIS_OP_INSTR_GET */
	prv_get,                             /* SUBTILIS_OP_INSTR_GETTIMEOUT */
	prv_nop,                             /* SUBTILIS_OP_INSTR_INKEY */
	prv_osbyteid,                        /* SUBTILIS_OP_INSTR_OS_BYTE_ID */
	prv_vdui,                            /* SUBTILIS_OP_INSTR_VDUI */
	prv_vdu,                             /* SUBTILIS_OP_INSTR_VDU */
	prv_point,                           /* SUBTILIS_OP_INSTR_POINT */
	prv_tint,                            /* SUBTILIS_OP_INSTR_TINT */
	prv_end,                             /* SUBTILIS_OP_INSTR_END */
	prv_testesc,                         /* SUBTILIS_OP_INSTR_TESTESC */
	prv_alloc,                           /* SUBTILIS_OP_INSTR_ALLOC */
	prv_realloc,                         /* SUBTILIS_OP_INSTR_REALLOC */
	prv_ref,                             /* SUBTILIS_OP_INSTR_REF */
	prv_deref,                           /* SUBTILIS_OP_INSTR_DEREF */
	prv_getref,                          /* SUBTILIS_OP_INSTR_GETREF */
	prv_pushi32,                         /* SUBTILIS_OP_INSTR_PUSH_I32 */
	prv_popi32,                          /* SUBTILIS_OP_INSTR_POP_I32 */
	prv_lca,                             /* SUBTILIS_OP_INSTR_LCA_I32 */
	prv_nop,                             /* SUBTILIS_OP_INSTR_AT */
	prv_pos,                             /* SUBTILIS_OP_INSTR_POS */
	prv_pos,                             /* SUBTILIS_OP_INSTR_VPOS */
};

/* clang-format on */

void subitlis_vm_run(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_error_t *err)
{
	subtilis_ir_operand_t *ops;
	subtilis_vm_op_fn fn;
	subtilis_op_instr_type_t itype;

	for (vm->pc = 0; vm->pc < vm->s->len; vm->pc++) {
		if (!vm->s->ops[vm->pc])
			continue;
		if (vm->s->ops[vm->pc]->type == SUBTILIS_OP_CALL) {
			prv_call(vm, b, &subtilis_type_void,
				 &vm->s->ops[vm->pc]->op.call, err);
		} else if (vm->s->ops[vm->pc]->type == SUBTILIS_OP_CALLI32) {
			prv_call(vm, b, &subtilis_type_integer,
				 &vm->s->ops[vm->pc]->op.call, err);
		} else if (vm->s->ops[vm->pc]->type == SUBTILIS_OP_CALLREAL) {
			prv_call(vm, b, &subtilis_type_real,
				 &vm->s->ops[vm->pc]->op.call, err);
		} else if (vm->s->ops[vm->pc]->type != SUBTILIS_OP_INSTR) {
			continue;
		} else {
			itype = vm->s->ops[vm->pc]->op.instr.type;
			ops = vm->s->ops[vm->pc]->op.instr.operands;
			fn = op_execute_fns[itype];
			if (!fn) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			fn(vm, b, ops, err);
			if (vm->quit_flag)
				return;
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
	prv_check_heap(vm, err);
}

void subitlis_vm_delete(subitlis_vm_t *vm)
{
	if (!vm)
		return;
	free(vm->constants);
	free(vm->labels);
	free(vm->fregs);
	free(vm->regs);
	subtilis_vm_heap_free(&vm->heap);
	free(vm->memory);
	free(vm);
}
