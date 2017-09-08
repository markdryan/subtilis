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

#include <stdlib.h>
#include <string.h>

#include "vm.h"

subitlis_vm_t *subitlis_vm_new(subtilis_ir_program_t *p,
			       subtilis_symbol_table_t *st,
			       subtilis_error_t *err)
{
	subitlis_vm_t *vm = calloc(sizeof(*vm), 1);

	//	printf("\n");
	//	subtilis_ir_program_dump(p);

	if (!vm) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	vm->regs = calloc(sizeof(int32_t), p->reg_counter);
	if (!vm->regs) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	vm->globals = calloc(sizeof(uint8_t), st->allocated);
	if (!vm->globals) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	vm->p = p;
	vm->st = st;

	return vm;

fail:

	subitlis_vm_delete(vm);
	return NULL;
}

typedef void (*subtilis_vm_op_fn)(subitlis_vm_t *, subtilis_buffer_t *,
				  subtilis_ir_operand_t *, subtilis_error_t *);

static void prv_subi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] - vm->regs[ops[2].integer];
}

static void prv_movii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = ops[1].integer;
}

static void prv_storeoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			  subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *dst = &vm->globals[base + ops[2].integer];

	memcpy(dst, &vm->regs[ops[0].reg], sizeof(int32_t));
}

static void prv_loadoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *src = &vm->globals[base + ops[2].integer];

	memcpy(&vm->regs[ops[0].reg], src, sizeof(int32_t));
}

static void prv_divi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t divisor = vm->regs[ops[2].reg];

	if (divisor == 0) {
		subtilis_error_set_divide_by_zero(err, "", 0);
		return;
	}
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] / divisor;
}

static void prv_addii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] + ops[2].integer;
}

static void prv_subii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] - ops[2].integer;
}

static void prv_divii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] / ops[2].integer;
}

static void prv_printi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	char buf[64];

	sprintf(buf, "%d\n", vm->regs[ops[0].reg]);
	subtilis_buffer_append_string(b, buf, err);
}

static void prv_rsubii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = ops[2].integer - vm->regs[ops[1].reg];
}

static void prv_rdivii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t divisor = vm->regs[ops[1].reg];

	if (divisor == 0) {
		subtilis_error_set_divide_by_zero(err, "", 0);
		return;
	}
	vm->regs[ops[0].reg] = ops[2].integer / divisor;
}

/* clang-format off */
static subtilis_vm_op_fn op_execute_fns[] = {
	NULL,                                /* SUBTILIS_OP_INSTR_ADD_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_ADD_REAL */
	prv_subi32,                          /* SUBTILIS_OP_INSTR_SUB_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_SUB_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_MUL_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_MUL_REAL */
	prv_divi32,                          /* SUBTILIS_OP_INSTR_DIV_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_DIV_REAL */
	prv_addii32,                         /* SUBTILIS_OP_INSTR_ADDI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_ADDI_REAL */
	prv_subii32,                         /* SUBTILIS_OP_INSTR_SUBI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_SUBI_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_MULI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_MULI_REAL */
	prv_divii32,                         /* SUBTILIS_OP_INSTR_DIVI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_DIVI_REAL */
	prv_loadoi32,                        /* SUBTILIS_OP_INSTR_LOADO_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_LOADO_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_LOAD_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_LOAD_REAL */
	prv_storeoi32,                       /* SUBTILIS_OP_INSTR_STOREO_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_STOREO_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_STORE_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_STORE_REAL */
	prv_movii32,                         /* SUBTILIS_OP_INSTR_MOVI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_MOV_REAL */
	NULL,                                /* SUBTILIS_OP_INSTR_MOV */
	NULL,                                /* SUBTILIS_OP_INSTR_MOVFP */
	prv_printi32,                        /* SUBTILIS_OP_INSTR_PRINT_I32 */
	prv_rsubii32,                        /* SUBTILIS_OP_INSTR_RSUBI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_RSUBI_REAL */
	prv_rdivii32,                        /* SUBTILIS_OP_INSTR_RDIVI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_RDIVI_REAL */
};

/* clang-format on */

void subitlis_vm_run(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t *ops;
	subtilis_vm_op_fn fn;
	subtilis_op_instr_type_t itype;

	for (i = 0; i < vm->p->len; i++) {
		if (!vm->p->ops[i])
			continue;
		if (vm->p->ops[i]->type != SUBTILIS_OP_INSTR)
			continue;
		itype = vm->p->ops[i]->op.instr.type;
		ops = vm->p->ops[i]->op.instr.operands;
		fn = op_execute_fns[itype];
		if (!fn) {
			subtilis_error_set_asssertion_failed(err);
			return;
		}
		fn(vm, b, ops, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subitlis_vm_delete(subitlis_vm_t *vm)
{
	if (!vm)
		return;
	free(vm->regs);
	free(vm->globals);
	free(vm);
}
