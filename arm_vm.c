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

#include "arm_vm.h"

subtilis_arm_vm_t *subtilis_arm_vm_new(subtilis_arm_program_t *arm_p,
				       size_t mem_size, subtilis_error_t *err)
{
	subtilis_arm_op_t *op;
	size_t ptr;
	subtilis_arm_vm_t *arm_vm = calloc(1, sizeof(*arm_vm));

	if (!arm_vm) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	arm_vm->regs = malloc(arm_p->reg_counter * sizeof(*arm_vm->regs));
	if (!arm_vm->regs) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	arm_vm->labels = malloc(arm_p->label_counter * sizeof(*arm_vm->labels));
	if (!arm_vm->labels) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	arm_vm->memory = malloc(mem_size);
	if (!arm_vm->memory) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	arm_vm->ops = malloc(arm_p->len * sizeof(*arm_vm->ops));
	if (!arm_vm->ops) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	arm_vm->mem_size = mem_size;
	arm_vm->p = arm_p;
	arm_vm->label_len = arm_p->label_counter;
	arm_vm->op_len = 0;

	ptr = arm_vm->p->first_op;
	while (ptr != SIZE_MAX) {
		op = &arm_vm->p->pool->ops[ptr];
		if (op->type == SUBTILIS_OP_LABEL)
			arm_vm->labels[op->op.label] = arm_vm->op_len;
		else
			arm_vm->ops[arm_vm->op_len++] = op;
		ptr = op->next;
	}

	return arm_vm;

fail:

	subtilis_arm_vm_delete(arm_vm);
	return NULL;
}

void subtilis_arm_vm_delete(subtilis_arm_vm_t *vm)
{
	if (!vm)
		return;
	free(vm->ops);
	free(vm->memory);
	free(vm->labels);
	free(vm->regs);
	free(vm);
}

static size_t prv_calc_pc(subtilis_arm_vm_t *vm)
{
	return (size_t)(((vm->regs[15] - 0x8000) + 8) / 4);
}

static bool prv_match_ccode(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_ccode_type_t ccode)
{
	switch (ccode) {
	case SUBTILIS_ARM_CCODE_EQ:
		return arm_vm->zero_flag;
	case SUBTILIS_ARM_CCODE_NE:
		return !arm_vm->zero_flag;
	case SUBTILIS_ARM_CCODE_CS:
		return arm_vm->carry_flag;
	case SUBTILIS_ARM_CCODE_CC:
		return !arm_vm->carry_flag;
	case SUBTILIS_ARM_CCODE_MI:
		return !arm_vm->negative_flag;
	case SUBTILIS_ARM_CCODE_PL:
		return arm_vm->negative_flag;
	case SUBTILIS_ARM_CCODE_VS:
		return arm_vm->overflow_flag;
	case SUBTILIS_ARM_CCODE_VC:
		return !arm_vm->overflow_flag;
	case SUBTILIS_ARM_CCODE_HI:
		return arm_vm->carry_flag && !arm_vm->zero_flag;
	case SUBTILIS_ARM_CCODE_LS:
		return !arm_vm->carry_flag || arm_vm->zero_flag;
	case SUBTILIS_ARM_CCODE_GE:
		return arm_vm->negative_flag == arm_vm->overflow_flag;
	case SUBTILIS_ARM_CCODE_LT:
		return arm_vm->negative_flag != arm_vm->overflow_flag;
	case SUBTILIS_ARM_CCODE_GT:
		return !arm_vm->zero_flag &&
		       (arm_vm->negative_flag == arm_vm->overflow_flag);
	case SUBTILIS_ARM_CCODE_LE:
		return arm_vm->zero_flag ||
		       (arm_vm->negative_flag != arm_vm->overflow_flag);
	case SUBTILIS_ARM_CCODE_AL:
		return true;
	case SUBTILIS_ARM_CCODE_NV:
		return false;
	}

	return false;
}

static uint8_t *prv_get_vm_address(subtilis_arm_vm_t *arm_vm, int32_t addr,
				   size_t buf_size, subtilis_error_t *err)
{
	addr -= 0x8000;
	if (addr < 0 || addr + buf_size > arm_vm->mem_size) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return &arm_vm->memory[addr];
}

static int32_t prv_eval_op2(subtilis_arm_vm_t *arm_vm, bool status,
			    subtilis_arm_op2_t *op2, subtilis_error_t *err)
{
	int32_t reg;
	bool neg;
	bool bit_one;
	int32_t shift;
	int32_t val;

	switch (op2->type) {
	case SUBTILIS_ARM_OP2_REG:
		return arm_vm->regs[op2->op.reg.num];
	case SUBTILIS_ARM_OP2_I32:
		val = op2->op.integer;
		if ((val & 0xf00) == 0)
			return val;
		shift = (val & 0xf00) >> 7;
		val &= 0xff;
		return val >> shift | val << (32 - shift);
	case SUBTILIS_ARM_OP2_SHIFTED:
		shift = op2->op.shift.shift;
		reg = arm_vm->regs[op2->op.shift.reg.num];
		switch (op2->op.shift.type) {
		case SUBTILIS_ARM_SHIFT_LSL:
		case SUBTILIS_ARM_SHIFT_ASL:
			if (shift < 0 || shift > 31) {
				subtilis_error_set_assertion_failed(err);
				return 0;
			}
			reg = reg << shift;
			break;
		case SUBTILIS_ARM_SHIFT_LSR:
			if (shift < 1 || shift > 32) {
				subtilis_error_set_assertion_failed(err);
				return 0;
			}
			neg = reg < 0;
			reg = reg >> shift;
			if (neg && reg < 0)
				reg &= ((uint32_t)0xffffffff) >> shift;
			break;
		case SUBTILIS_ARM_SHIFT_ASR:
			if (shift < 1 || shift > 32) {
				subtilis_error_set_assertion_failed(err);
				return 0;
			}
			neg = reg < 0;
			reg = reg >> shift;
			if (neg && reg > 0)
				reg |= ~(((uint32_t)0xffffffff) >> shift);
			break;
		case SUBTILIS_ARM_SHIFT_ROR:
			if (shift < 1 || shift > 31) {
				subtilis_error_set_assertion_failed(err);
				return 0;
			}
			reg = reg >> shift | reg << (32 - shift);
			break;
		case SUBTILIS_ARM_SHIFT_RRX:
			bit_one = reg & 1 ? true : false;
			neg = reg < 0;
			reg = reg >> 1;
			if (arm_vm->carry_flag)
				reg |= 0x80000000;
			else
				reg &= 0x7fffffff;
			if (status)
				arm_vm->carry_flag = bit_one;
			break;
		}
		return reg;
	}

	return 0;
}

static void prv_process_and(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] & op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_eor(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] ^ op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_sub(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2 = 0;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] - op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_rsb(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = op2 - arm_vm->regs[op->op1.num];
	arm_vm->regs[15] += 4;
}

static void prv_process_add(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] + op2;
	arm_vm->regs[15] += 4;
}

static void prv_set_status_flags(subtilis_arm_vm_t *arm_vm, int32_t op1,
				 int32_t op2)
{
	int32_t res;

	arm_vm->negative_flag = false;
	arm_vm->zero_flag = false;
	arm_vm->carry_flag = false;
	arm_vm->overflow_flag = false;

	res = op1 - op2;
	if (res == 0)
		arm_vm->zero_flag = true;
	else if (res < 0)
		arm_vm->negative_flag = true;
}

static void prv_process_cmp(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t op1;

	if (!op->status) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op1 = arm_vm->regs[op->op1.num];
	prv_set_status_flags(arm_vm, op1, op2);

	arm_vm->regs[15] += 4;
}

static void prv_process_cmn(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t op1;

	if (!op->status) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = -prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op1 = arm_vm->regs[op->op1.num];
	prv_set_status_flags(arm_vm, op1, op2);

	arm_vm->regs[15] += 4;
}

static void prv_process_orr(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] | op2;
	arm_vm->regs[15] += 4;
}

static size_t prv_compute_stran_addr(subtilis_arm_vm_t *arm_vm,
				     subtilis_arm_stran_instr_t *op,
				     subtilis_error_t *err)
{
	int32_t offset;
	size_t addr;

	offset = prv_eval_op2(arm_vm, false, &op->offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	if (op->pre_indexed) {
		addr = arm_vm->regs[op->base.num] + offset;
		if (op->write_back)
			arm_vm->regs[op->base.num] = (int32_t)addr;
	} else {
		addr = arm_vm->regs[op->base.num];
		arm_vm->regs[op->base.num] += offset;
	}
	addr -= 0x8000;
	if (addr + 4 > arm_vm->mem_size) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	return addr;
}

static void prv_process_mov(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_mvn(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = ~op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_mul(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	arm_vm->regs[op->dest.num] = arm_vm->regs[op->op1.num] * op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_ldr(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_stran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_stran_addr(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	arm_vm->regs[op->dest.num] = *((int32_t *)&arm_vm->memory[addr]);
	arm_vm->regs[15] += 4;
}

static void prv_process_str(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_stran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_stran_addr(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	*((int32_t *)&arm_vm->memory[addr]) = arm_vm->regs[op->dest.num];
	arm_vm->regs[15] += 4;
}

static void prv_process_br(subtilis_arm_vm_t *arm_vm,
			   subtilis_arm_br_instr_t *op, subtilis_error_t *err)
{
	if (op->link) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->label >= arm_vm->label_len) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] = 0x8000 + (arm_vm->labels[op->label] * 4) - 8;
}

static void prv_process_swi(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			    subtilis_arm_swi_instr_t *op, subtilis_error_t *err)
{
	char buf[64];
	uint8_t *addr;
	size_t buf_len;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	switch (op->code) {
	case 0x10:
		/* OS_GetEnv */
		arm_vm->regs[0] = 0;
		arm_vm->regs[1] = 0x8000 + arm_vm->mem_size;
		arm_vm->regs[2] = 0;
		break;
	case 0xdc:
		/* OS_ConvertInteger4  */
		buf_len = sprintf(buf, "%d", arm_vm->regs[0]) + 1;
		if (buf_len > arm_vm->regs[2]) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		addr =
		    prv_get_vm_address(arm_vm, arm_vm->regs[1], buf_len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(addr, buf, buf_len);
		arm_vm->regs[0] = arm_vm->regs[1];
		arm_vm->regs[1] += buf_len - 1;
		arm_vm->regs[2] -= buf_len;
		break;
	case 0x2:
		/* OS_Write0  */
		addr = prv_get_vm_address(arm_vm, arm_vm->regs[0], 1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_buffer_append_string(b, (const char *)addr, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		arm_vm->regs[0] += strlen((const char *)addr) + 1;
		break;
	case 0x3:
		/* OS_Newline  */
		subtilis_buffer_append_string(b, "\n", err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		break;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_ldrc(subtilis_arm_vm_t *arm_vm,
			     subtilis_arm_ldrc_instr_t *op,
			     subtilis_error_t *err)
{
	size_t i;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	for (i = 0; i < arm_vm->p->constant_count; i++) {
		if (arm_vm->p->constants[i].label == op->label) {
			arm_vm->regs[op->dest.num] =
			    arm_vm->p->constants[i].integer;
			break;
		}
	}

	if (i == arm_vm->p->constant_count) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

void subtilis_arm_vm_run(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			 subtilis_error_t *err)
{
	size_t pc;
	subtilis_arm_op_t *op;

	arm_vm->negative_flag = false;
	arm_vm->zero_flag = false;
	arm_vm->carry_flag = false;
	arm_vm->overflow_flag = false;
	arm_vm->regs[15] = 0x8000 - 8;

	pc = prv_calc_pc(arm_vm);
	while (pc < arm_vm->op_len) {
		op = arm_vm->ops[pc];
		switch (op->op.instr.type) {
		case SUBTILIS_ARM_INSTR_AND:
			prv_process_and(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_EOR:
			prv_process_eor(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_SUB:
			prv_process_sub(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_RSB:
			prv_process_rsb(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_ADD:
			prv_process_add(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_CMP:
			prv_process_cmp(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_CMN:
			prv_process_cmn(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_ORR:
			prv_process_orr(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_MOV:
			prv_process_mov(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_MVN:
			prv_process_mvn(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_MUL:
			prv_process_mul(arm_vm, &op->op.instr.operands.data,
					err);
			break;
		case SUBTILIS_ARM_INSTR_LDR:
			prv_process_ldr(arm_vm, &op->op.instr.operands.stran,
					err);
			break;
		case SUBTILIS_ARM_INSTR_STR:
			prv_process_str(arm_vm, &op->op.instr.operands.stran,
					err);
			break;
		case SUBTILIS_ARM_INSTR_B:
			prv_process_br(arm_vm, &op->op.instr.operands.br, err);
			break;
		case SUBTILIS_ARM_INSTR_SWI:
			prv_process_swi(arm_vm, b, &op->op.instr.operands.swi,
					err);
			break;
		case SUBTILIS_ARM_INSTR_LDRC:
			prv_process_ldrc(arm_vm, &op->op.instr.operands.ldrc,
					 err);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		pc = prv_calc_pc(arm_vm);
	}
}
