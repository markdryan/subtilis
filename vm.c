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

	for (i = 0; i < vm->p->len; i++) {
		if (vm->p->ops[i]->type != SUBTILIS_OP_LABEL)
			continue;
		label = vm->p->ops[i]->op.label;
		prv_ensure_label_buffer(vm, label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		vm->labels[label] = i;
	}
}

subitlis_vm_t *subitlis_vm_new(subtilis_ir_program_t *p,
			       subtilis_symbol_table_t *st,
			       subtilis_error_t *err)
{
	subitlis_vm_t *vm = calloc(sizeof(*vm), 1);

	//	printf("\n");
	//	subtilis_ir_program_dump(p);

	vm->labels = NULL;
	vm->max_labels = 0;

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

	prv_compute_labels(vm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

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

static void prv_subi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] - vm->regs[ops[2].integer];
}

static void prv_muli32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] * vm->regs[ops[2].integer];
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

static void prv_mulii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] * ops[2].integer;
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

static void prv_eqii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] == ops[2].integer ? -1 : 0;
}

static void prv_neqi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] == vm->regs[ops[2].integer] ? 0 : -1;
}

static void prv_neqii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] == ops[2].integer ? 0 : -1;
}

static void prv_gti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] > vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_gtii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] > ops[2].integer ? -1 : 0;
}

static void prv_ltei32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] <= vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_lteii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] <= ops[2].integer ? -1 : 0;
}

static void prv_lti32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		      subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] < vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_ltii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] < ops[2].integer ? -1 : 0;
}

static void prv_gtei32(subitlis_vm_t *vm, subtilis_buffer_t *b,
		       subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] =
	    vm->regs[ops[1].integer] >= vm->regs[ops[2].integer] ? -1 : 0;
}

static void prv_gteii32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg] >= ops[2].integer ? -1 : 0;
}

static void prv_jmpc(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t label;

	label = (vm->regs[ops[0].reg]) ? ops[1].label : ops[2].label;
	if (label > vm->max_labels) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}
	vm->pc = vm->labels[label];
	if (vm->pc >= vm->p->len) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}
}

static void prv_jmp(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t label;

	label = ops[0].label;
	if (label > vm->max_labels) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}
	vm->pc = vm->labels[label];
	if (vm->pc >= vm->p->len) {
		subtilis_error_set_asssertion_failed(err);
		return;
	}
}

/* clang-format off */
static subtilis_vm_op_fn op_execute_fns[] = {
	prv_addi32,                          /* SUBTILIS_OP_INSTR_ADD_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_ADD_REAL */
	prv_subi32,                          /* SUBTILIS_OP_INSTR_SUB_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_SUB_REAL */
	prv_muli32,                          /* SUBTILIS_OP_INSTR_MUL_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_MUL_REAL */
	prv_divi32,                          /* SUBTILIS_OP_INSTR_DIV_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_DIV_REAL */
	prv_addii32,                         /* SUBTILIS_OP_INSTR_ADDI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_ADDI_REAL */
	prv_subii32,                         /* SUBTILIS_OP_INSTR_SUBI_I32 */
	NULL,                                /* SUBTILIS_OP_INSTR_SUBI_REAL */
	prv_mulii32,                         /* SUBTILIS_OP_INSTR_MULI_I32 */
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
	prv_andi32,                          /* SUBTILIS_OP_INSTR_AND_I32 */
	prv_andii32,                         /* SUBTILIS_OP_INSTR_ANDI_I32 */
	prv_ori32,                           /* SUBTILIS_OP_INSTR_OR_I32 */
	prv_orii32,                          /* SUBTILIS_OP_INSTR_ORI_I32 */
	prv_eori32,                          /* SUBTILIS_OP_INSTR_EOR_I32 */
	prv_eorii32,                         /* SUBTILIS_OP_INSTR_EORI_I32 */
	prv_noti32,                          /* SUBTILIS_OP_INSTR_NOT_I32 */
	prv_eqi32,                           /* SUBTILIS_OP_INSTR_EQ_I32 */
	prv_eqii32,                          /* SUBTILIS_OP_INSTR_EQI_I32 */
	prv_neqi32,                          /* SUBTILIS_OP_INSTR_NEQ_I32 */
	prv_neqii32,                         /* SUBTILIS_OP_INSTR_NEQI_I32 */
	prv_gti32,                           /* SUBTILIS_OP_INSTR_GT_I32 */
	prv_gtii32,                          /* SUBTILIS_OP_INSTR_GTI_I32 */
	prv_ltei32,                          /* SUBTILIS_OP_INSTR_LTE_I32 */
	prv_lteii32,                         /* SUBTILIS_OP_INSTR_LTEI_I32 */
	prv_lti32,                           /* SUBTILIS_OP_INSTR_LT_I32 */
	prv_ltii32,                          /* SUBTILIS_OP_INSTR_LTI_I32 */
	prv_gtei32,                          /* SUBTILIS_OP_INSTR_GTE_I32 */
	prv_gteii32,                         /* SUBTILIS_OP_INSTR_GTEI_I32 */
	prv_jmpc,                            /* SUBTILIS_OP_INSTR_JMPC */
	prv_jmp,                             /* SUBTILIS_OP_INSTR_JMP */
};

/* clang-format on */

void subitlis_vm_run(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_error_t *err)
{
	subtilis_ir_operand_t *ops;
	subtilis_vm_op_fn fn;
	subtilis_op_instr_type_t itype;

	for (vm->pc = 0; vm->pc < vm->p->len; vm->pc++) {
		if (!vm->p->ops[vm->pc])
			continue;
		if (vm->p->ops[vm->pc]->type != SUBTILIS_OP_INSTR)
			continue;
		itype = vm->p->ops[vm->pc]->op.instr.type;
		ops = vm->p->ops[vm->pc]->op.instr.operands;
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
	free(vm->labels);
	free(vm->regs);
	free(vm->globals);
	free(vm);
}
