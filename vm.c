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
	subitlis_vm_t *vm = calloc(sizeof(*vm), 1);

	//	printf("\n");
	//	subtilis_ir_section_dump(s);

	if (!vm) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	vm->labels = NULL;
	vm->max_labels = 0;
	vm->p = p;
	vm->s = p->sections[0];
	vm->max_regs = vm->s->reg_counter;
	vm->st = st;

	vm->regs = calloc(sizeof(int32_t), vm->max_regs);
	if (!vm->regs) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	vm->memory_size = st->allocated + vm->s->locals;

	vm->memory = calloc(sizeof(uint8_t), vm->memory_size);
	if (!vm->memory) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;
	vm->regs[SUBTILIS_IR_REG_GLOBAL] = 0;
	vm->regs[SUBTILIS_IR_REG_LOCAL] = st->allocated;
	vm->top = vm->memory_size;

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

static void prv_mov(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	vm->regs[ops[0].reg] = vm->regs[ops[1].reg];
}

static void prv_storeoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			  subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *dst = &vm->memory[base + ops[2].integer];

	memcpy(dst, &vm->regs[ops[0].reg], sizeof(int32_t));
}

static void prv_loadoi32(subitlis_vm_t *vm, subtilis_buffer_t *b,
			 subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	int32_t base = vm->regs[ops[1].reg];
	uint8_t *src = &vm->memory[base + ops[2].integer];

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
	int32_t *int_values = NULL;
	size_t int_args = 0;
	size_t real_args = 0;
	size_t int_count = 0;

	for (i = 0; i < call->arg_count; i++) {
		if (call->args[i].type == SUBTILIS_IR_REG_TYPE_INTEGER)
			int_args++;
		else
			real_args++;
	}

	if (int_args > 0) {
		int_values = malloc(sizeof(*int_values) * int_args);
		if (!int_values) {
			subtilis_error_set_oom(err);
			return;
		}
	}

	for (i = 0; i < call->arg_count; i++) {
		if (call->args[i].type == SUBTILIS_IR_REG_TYPE_INTEGER)
			int_values[int_count++] = vm->regs[call->args[i].reg];
	}

	for (i = 0; i < int_args; i++)
		vm->regs[SUBTILIS_IR_REG_TEMP_START + i] = int_values[i];

	free(int_values);
}

static void prv_call(subitlis_vm_t *vm, subtilis_buffer_t *b,
		     subtilis_ir_call_t *call, subtilis_error_t *err)
{
	subtilis_ir_section_t *s;
	int32_t *new_regs;
	size_t section_index = call->proc_id;

	if (section_index >= vm->p->num_sections) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	s = vm->p->sections[section_index];
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

	prv_reserve_stack(vm, ((vm->s->reg_counter + 2) * sizeof(*vm->regs)) +
				  s->locals,
			  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	memcpy(&vm->memory[vm->top], &vm->regs[0],
	       vm->s->reg_counter * sizeof(*vm->regs));
	vm->top += vm->s->reg_counter * sizeof(*vm->regs);
	*((int32_t *)&vm->memory[vm->top]) = vm->pc;
	vm->top += 4;
	*((int32_t *)&vm->memory[vm->top]) = vm->current_index;
	vm->top += 4;
	vm->regs[SUBTILIS_IR_REG_LOCAL] = vm->top;
	prv_set_args(vm, call, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->locals > 0) {
		memset(&vm->memory[vm->top], 0, s->locals);
		vm->top += s->locals;
	}

	vm->s = s;
	vm->current_index = section_index;
	vm->pc = -1;
}

static void prv_ret(subitlis_vm_t *vm, subtilis_buffer_t *b,
		    subtilis_ir_operand_t *ops, subtilis_error_t *err)
{
	size_t caller_index;
	size_t to_pop;
	subtilis_ir_section_t *cs;

	if (vm->top == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	vm->top -= vm->s->locals + sizeof(*vm->regs);
	caller_index = *((int32_t *)&vm->memory[vm->top]);
	cs = vm->p->sections[caller_index];
	to_pop = cs->reg_counter + 1;
	if (to_pop > vm->top) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	vm->s = cs;
	vm->top -= sizeof(*vm->regs);
	vm->pc = *((int32_t *)&vm->memory[vm->top]);
	vm->current_index = caller_index;
	vm->top -= vm->s->reg_counter * sizeof(*vm->regs);
	memcpy(&vm->regs[0], &vm->memory[vm->top],
	       vm->s->reg_counter * sizeof(*vm->regs));
	vm->regs[SUBTILIS_IR_REG_LOCAL] = vm->top - cs->locals;
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
	prv_mov,                                /* SUBTILIS_OP_INSTR_MOV */
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
	prv_ret,                             /* SUBTILIS_OP_INSTR_RET */
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
			prv_call(vm, b, &vm->s->ops[vm->pc]->op.call, err);
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
		}
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
	free(vm->memory);
	free(vm);
}
