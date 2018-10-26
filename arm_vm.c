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

#include "arm_disass.h"
#include "arm_vm.h"

subtilis_arm_vm_t *subtilis_arm_vm_new(uint32_t *code, size_t code_size,
				       size_t mem_size, subtilis_error_t *err)
{
	double dummy_float = 1.0;
	uint32_t *lower_word = (uint32_t *)((void *)&dummy_float);
	subtilis_arm_vm_t *arm_vm = calloc(1, sizeof(*arm_vm));

	if (!arm_vm) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	arm_vm->memory = malloc(mem_size);
	if (!arm_vm->memory) {
		subtilis_error_set_oom(err);
		goto fail;
	}

	memcpy(arm_vm->memory, code, sizeof(*code) * code_size);
	arm_vm->code_size = code_size;
	arm_vm->mem_size = mem_size;

	arm_vm->reverse_fpa_consts = (*lower_word) == 0;

	return arm_vm;

fail:

	subtilis_arm_vm_delete(arm_vm);
	return NULL;
}

void subtilis_arm_vm_delete(subtilis_arm_vm_t *vm)
{
	if (!vm)
		return;
	free(vm->memory);
	free(vm);
}

static size_t prv_calc_pc(subtilis_arm_vm_t *vm)
{
	return (size_t)(((vm->regs[15] - 0x8000) - 8) / 4);
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
		return arm_vm->negative_flag;
	case SUBTILIS_ARM_CCODE_PL:
		return !arm_vm->negative_flag;
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
		if (op2->op.shift.shift_reg) {
			shift = arm_vm->regs[op2->op.shift.shift.reg.num];
			if (shift > 32)
				shift = 32;
		} else {
			shift = op2->op.shift.shift.integer;
		}
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
			if (shift == 0) {
				shift = 32;
			} else if (shift < 1 || shift > 32) {
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

static void prv_set_and_flags(subtilis_arm_vm_t *arm_vm, int32_t res)
{
	arm_vm->negative_flag = res < 0;
	arm_vm->zero_flag = res == 0;
}

static void prv_set_shift_flags(subtilis_arm_vm_t *arm_vm, int32_t shifted,
				int32_t res, subtilis_arm_shift_t shift)
{
	int32_t shift_val;

	if (shift.shift_reg) {
		shift_val = arm_vm->regs[shift.shift.reg.num];
		if (shift_val > 32)
			shift_val = 32;
	} else {
		shift_val = shift.shift.integer;
	}

	switch (shift.type) {
	case SUBTILIS_ARM_SHIFT_LSL:
	case SUBTILIS_ARM_SHIFT_ASL:
		if (shift_val > 0 && shift_val <= 31)
			arm_vm->carry_flag =
			    (shifted & (1 << (32 - shift_val))) != 0;
		else if (shift_val == 32)
			arm_vm->carry_flag = (shifted & 1) != 0;
		else if (shift_val > 32)
			arm_vm->carry_flag = false;
		break;
	case SUBTILIS_ARM_SHIFT_LSR:
		if (shift_val > 0 && shift_val <= 31)
			arm_vm->carry_flag =
			    (shifted & (1 << (shift_val - 1))) != 0;
		else if (shift_val == 32)
			arm_vm->carry_flag = (shifted & (1 << 31)) != 0;
		else if (shift_val > 32)
			arm_vm->carry_flag = false;
		break;
	case SUBTILIS_ARM_SHIFT_ASR:
		if (shift_val > 0 && shift_val <= 31)
			arm_vm->carry_flag =
			    (shifted & (1 << (shift_val - 1))) != 0;
		else if (shift_val >= 32)
			arm_vm->carry_flag = (shifted & (1 << 31)) != 0;
		break;
	case SUBTILIS_ARM_SHIFT_ROR:
		if (shift_val > 0 && shift_val <= 31)
			arm_vm->carry_flag =
			    (shifted & (1 << (shift_val - 1))) != 0;
		else if (shift_val >= 32)
			arm_vm->carry_flag =
			    (shifted & (1 << ((shift_val - 1) & 31))) != 0;
		break;
	case SUBTILIS_ARM_SHIFT_RRX:
		arm_vm->carry_flag = (shifted & 1) != 0;
	}
}

static void prv_process_and(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;
	int32_t op2_old;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1.num] & op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg.num];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest.num] = res;
	arm_vm->regs[15] += 4;
}

static void prv_process_eor(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;
	int32_t op2_old;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1.num] ^ op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg.num];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest.num] = res;
	arm_vm->regs[15] += 4;
}

static void prv_reset_flags(subtilis_arm_vm_t *arm_vm)
{
	arm_vm->negative_flag = false;
	arm_vm->zero_flag = false;
	arm_vm->carry_flag = false;
	arm_vm->overflow_flag = false;
}

static void prv_set_sub_flags(subtilis_arm_vm_t *arm_vm, int32_t op1,
			      int32_t op2, int32_t res)
{
	prv_reset_flags(arm_vm);
	arm_vm->zero_flag = res == 0;
	arm_vm->negative_flag = res < 0;
	arm_vm->carry_flag = (op1 < 0 && op2 >= 0) || (op1 < 0 && res >= 0) ||
			     (op2 >= 0 && res >= 0);
	if (((op1 < 0 && op2 >= 0) || (op1 >= 0 && op2 < 0)) &&
	    ((op1 >= 0 && res < 0) || (op1 < 0 && res >= 0)))
		arm_vm->overflow_flag = true;
}

static void prv_process_sub(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t res;
	int32_t op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1.num] - op2;
	if (op->status)
		prv_set_sub_flags(arm_vm, arm_vm->regs[op->op1.num], op2, res);
	arm_vm->regs[op->dest.num] = res;
	arm_vm->regs[15] += 4;
}

static void prv_process_rsb(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = op2 - arm_vm->regs[op->op1.num];
	if (op->status)
		prv_set_sub_flags(arm_vm, op2, arm_vm->regs[op->op1.num], res);

	arm_vm->regs[15] += 4;
	arm_vm->regs[op->dest.num] = res;
}

static void prv_set_add_flags(subtilis_arm_vm_t *arm_vm, int32_t op1,
			      int32_t op2, int32_t res)
{
	prv_reset_flags(arm_vm);
	arm_vm->zero_flag = res == 0;
	arm_vm->negative_flag = res < 0;
	arm_vm->carry_flag = (op1 < 0 && op2 < 0) || (op1 < 0 && res >= 0) ||
			     (op2 < 0 && res >= 0);
	if (((op1 < 0 && op2 < 0) || (op1 >= 0 && op2 >= 0)) &&
	    ((op1 >= 0 && res < 0) || (op1 < 0 && res >= 0)))
		arm_vm->overflow_flag = true;
}

static void prv_process_adc(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t carry;
	int32_t res;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	carry = arm_vm->carry_flag ? 1 : 0;
	res = arm_vm->regs[op->op1.num] + op2 + carry;
	if (op->status)
		prv_set_add_flags(arm_vm, arm_vm->regs[op->op1.num],
				  op2 + carry, res);
	arm_vm->regs[op->dest.num] = res;
	arm_vm->regs[15] += 4;
}

static void prv_process_add(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1.num] + op2;
	if (op->status)
		prv_set_add_flags(arm_vm, arm_vm->regs[op->op1.num], op2, res);
	arm_vm->regs[15] += 4;
	arm_vm->regs[op->dest.num] = res;
}

static void prv_set_status_flags(subtilis_arm_vm_t *arm_vm, int32_t op1,
				 int32_t op2)
{
	prv_set_sub_flags(arm_vm, op1, op2, op1 - op2);
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
	int32_t op2_old;
	int32_t res;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1.num] | op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg.num];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest.num] = res;
	arm_vm->regs[15] += 4;
}

static size_t prv_compute_stran_addr(subtilis_arm_vm_t *arm_vm,
				     subtilis_arm_stran_instr_t *op,
				     subtilis_error_t *err)
{
	int32_t offset;
	size_t addr;

	if (op->offset.type == SUBTILIS_ARM_OP2_I32) {
		offset = op->offset.op.integer;
		if (offset > 4096) {
			subtilis_error_set_assertion_failed(err);
			return 0;
		}
	} else {
		offset = prv_eval_op2(arm_vm, false, &op->offset, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	if (op->subtract)
		offset = -offset;

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

static void prv_set_fpa_f32(subtilis_arm_vm_t *arm_vm, size_t reg, float val)
{
	arm_vm->fregs[reg].val.real32 = val;
	arm_vm->fregs[reg].size = 4;
}

static void prv_set_fpa_f64(subtilis_arm_vm_t *arm_vm, size_t reg, double val)
{
	arm_vm->fregs[reg].val.real64 = val;
	arm_vm->fregs[reg].size = 8;
}

static size_t prv_compute_fpa_stran_addr(subtilis_arm_vm_t *arm_vm,
					 subtilis_fpa_stran_instr_t *op,
					 subtilis_error_t *err)
{
	int32_t offset;
	size_t addr;

	offset = op->offset;
	if (offset > 256) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}
	offset *= 4;
	if (op->subtract)
		offset = -offset;

	if (op->pre_indexed) {
		addr = arm_vm->regs[op->base.num] + offset;
		if (op->write_back)
			arm_vm->regs[op->base.num] = (int32_t)addr;
	} else {
		addr = arm_vm->regs[op->base.num];
		if (op->write_back)
			arm_vm->regs[op->base.num] += offset;
	}

	addr -= 0x8000;

	if (addr + op->size > arm_vm->mem_size) {
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
	int32_t op2_old;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (op->status) {
		prv_set_and_flags(arm_vm, op2);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg.num];
			prv_set_shift_flags(arm_vm, op2_old, op2,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest.num] = op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_mvn(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t op2_old;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	op2 = prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (op->status) {
		prv_set_and_flags(arm_vm, op2);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg.num];
			prv_set_shift_flags(arm_vm, op2_old, -op2,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest.num] = ~op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_mul(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mul_instr_t *op, subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	arm_vm->regs[op->dest.num] =
	    arm_vm->regs[op->rm.num] * arm_vm->regs[op->rs.num];
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

static void prv_process_b(subtilis_arm_vm_t *arm_vm,
			  subtilis_arm_br_instr_t *op, subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->link)
		arm_vm->regs[14] = arm_vm->regs[15];

	arm_vm->regs[15] += (op->target.offset * 4) + 8;
	if (prv_calc_pc(arm_vm) >= arm_vm->code_size)
		subtilis_error_set_assertion_failed(err);
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
	case 0x11:
		arm_vm->quit = true;
		break;
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

static void prv_process_stm(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mtran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;
	size_t i;
	int after = 0;
	int before = 0;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = arm_vm->regs[op->op0.num] - 0x8000;
	switch (op->type) {
	case SUBTILIS_ARM_MTRAN_FA:
	case SUBTILIS_ARM_MTRAN_IB:
		before = 4;
		break;
	case SUBTILIS_ARM_MTRAN_FD:
	case SUBTILIS_ARM_MTRAN_DB:
		before = -4;
		break;
	case SUBTILIS_ARM_MTRAN_EA:
	case SUBTILIS_ARM_MTRAN_IA:
		after = 4;
		break;
	case SUBTILIS_ARM_MTRAN_ED:
	case SUBTILIS_ARM_MTRAN_DA:
		after = -4;
		break;
	}

	for (i = 0; i < 15; i++) {
		if (((1 << i) & op->reg_list) == 0)
			continue;
		addr += before;
		if (addr + 4 > arm_vm->mem_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*((int32_t *)&arm_vm->memory[addr]) = arm_vm->regs[i];
		addr += after;
	}

	if (op->write_back)
		arm_vm->regs[op->op0.num] = addr + 0x8000;

	arm_vm->regs[15] += 4;
}

static void prv_process_ldm(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mtran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;
	int i;
	int after = 0;
	int before = 0;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = arm_vm->regs[op->op0.num] - 0x8000;

	switch (op->type) {
	case SUBTILIS_ARM_MTRAN_FA:
	case SUBTILIS_ARM_MTRAN_IB:
		after = -4;
		break;
	case SUBTILIS_ARM_MTRAN_FD:
	case SUBTILIS_ARM_MTRAN_IA:
		after = 4;
		break;
	case SUBTILIS_ARM_MTRAN_EA:
	case SUBTILIS_ARM_MTRAN_DB:
		before = -4;
		break;
	case SUBTILIS_ARM_MTRAN_ED:
	case SUBTILIS_ARM_MTRAN_DA:
		before = 4;
		break;
	}

	for (i = 14; i >= 0; i--) {
		if (((1 << i) & op->reg_list) == 0)
			continue;
		addr += before;
		if (addr + 4 > arm_vm->mem_size) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		arm_vm->regs[i] = *((int32_t *)&arm_vm->memory[addr]);
		addr += after;
	}

	if (op->write_back)
		arm_vm->regs[op->op0.num] = addr + 0x8000;

	// TODO: Is this correct? We dont do this for other loads

	if (!((1 << 15) & op->reg_list))
		arm_vm->regs[15] += 4;
}

static void prv_process_fpa_ldf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_stran_instr_t *op,
				subtilis_error_t *err)
{
	size_t addr;
	uint32_t *ptr;
	uint64_t dbl;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_fpa_stran_addr(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (op->size == 4) {
		prv_set_fpa_f32(arm_vm, op->dest.num,
				*((float *)&arm_vm->memory[addr]));
	} else if (op->size == 8) {
		if (arm_vm->reverse_fpa_consts) {
			ptr = (uint32_t *)&arm_vm->memory[addr];
			dbl = ptr[0];
			dbl = (dbl << 32) | ptr[1];
			prv_set_fpa_f64(arm_vm, op->dest.num,
					*((double *)&dbl));
		} else {
			prv_set_fpa_f64(arm_vm, op->dest.num,
					*((double *)&arm_vm->memory[addr]));
		}
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_stf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_stran_instr_t *op,
				subtilis_error_t *err)
{
	size_t addr;
	uint32_t *ptr;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_fpa_stran_addr(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (op->size == 4) {
		*((float *)&arm_vm->memory[addr]) =
		    arm_vm->fregs[op->dest.num].val.real32;
	} else if (op->size == 8) {
		if (arm_vm->reverse_fpa_consts) {
			ptr =
			    (uint32_t *)&arm_vm->fregs[op->dest.num].val.real64;
			*((uint32_t *)&arm_vm->memory[addr]) = ptr[1];
			*((uint32_t *)&arm_vm->memory[addr + 4]) = ptr[0];
		} else {
			*((double *)&arm_vm->memory[addr]) =
			    arm_vm->fregs[op->dest.num].val.real64;
		}
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_mvf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	double imm;
	size_t dest;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	dest = op->dest.num;

	if (op->size == 4) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_set_fpa_f32(arm_vm, dest, (float)imm);
		} else {
			prv_set_fpa_f32(
			    arm_vm, dest,
			    arm_vm->fregs[op->op2.reg.num].val.real32);
		}
	} else if (op->size == 8) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_set_fpa_f64(arm_vm, dest, imm);
		} else {
			prv_set_fpa_f64(
			    arm_vm, dest,
			    arm_vm->fregs[op->op2.reg.num].val.real64);
		}
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_mnf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	size_t dest;

	prv_process_fpa_mvf(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dest = op->dest.num;

	if (op->size == 4)
		prv_set_fpa_f32(arm_vm, dest, -arm_vm->fregs[dest].val.real32);
	else if (op->size == 8)
		prv_set_fpa_f64(arm_vm, dest, -arm_vm->fregs[dest].val.real64);
}

static void prv_process_fpa_dyadic(subtilis_arm_vm_t *arm_vm,
				   subtilis_fpa_data_instr_t *op,
				   float (*f32_op)(float, float),
				   double (*f64_op)(double, double),
				   subtilis_error_t *err)
{
	size_t dest;
	size_t op1;
	double imm;
	float res32;
	double res64;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	dest = op->dest.num;
	op1 = op->op1.num;

	if (op->size == 4) {
		res32 = arm_vm->fregs[op1].val.real32;
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			res32 = f32_op(res32, (float)imm);
		} else {
			res32 = f32_op(
			    res32, arm_vm->fregs[op->op2.reg.num].val.real32);
		}
		prv_set_fpa_f32(arm_vm, dest, res32);
	} else if (op->size == 8) {
		res64 = arm_vm->fregs[op1].val.real64;
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			res64 = f64_op(res64, imm);
		} else {
			res64 = f64_op(
			    res64, arm_vm->fregs[op->op2.reg.num].val.real64);
		}
		prv_set_fpa_f64(arm_vm, dest, res64);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static float prv_process_fpa_add_real32(float a, float b) { return a + b; }

static double prv_process_fpa_add_real64(double a, double b) { return a + b; }

static void prv_process_fpa_adf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_add_real32,
			       prv_process_fpa_add_real64, err);
}

static float prv_process_fpa_mul_real32(float a, float b) { return a * b; }

static double prv_process_fpa_mul_real64(double a, double b) { return a * b; }

static void prv_process_fpa_muf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_mul_real32,
			       prv_process_fpa_mul_real64, err);
}

static float prv_process_fpa_sub_real32(float a, float b) { return a - b; }

static double prv_process_fpa_sub_real64(double a, double b) { return a - b; }

static void prv_process_fpa_suf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_sub_real32,
			       prv_process_fpa_sub_real64, err);
}

static float prv_process_fpa_rsub_real32(float a, float b) { return b - a; }

static double prv_process_fpa_rsub_real64(double a, double b) { return b - a; }

static void prv_process_fpa_rsf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_rsub_real32,
			       prv_process_fpa_rsub_real64, err);
}

static float prv_process_fpa_div_real32(float a, float b) { return a / b; }

static double prv_process_fpa_div_real64(double a, double b) { return a / b; }

static void prv_process_fpa_dvf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_div_real32,
			       prv_process_fpa_div_real64, err);
}

static float prv_process_fpa_rdiv_real32(float a, float b) { return b / a; }

static double prv_process_fpa_rdiv_real64(double a, double b) { return b / a; }

static void prv_process_fpa_rdf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_rdiv_real32,
			       prv_process_fpa_rdiv_real64, err);
}

static void prv_process_fpa_fix(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_tran_instr_t *op,
				subtilis_error_t *err)
{
	double val;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (arm_vm->fregs[op->op2.reg.num].size == 4)
		val = (double)arm_vm->fregs[op->op2.reg.num].val.real32;
	else
		val = arm_vm->fregs[op->op2.reg.num].val.real64;
	if (op->rounding == SUBTILIS_FPA_ROUNDING_ZERO)
		arm_vm->regs[op->dest.num] = (int32_t)val;
	else
		arm_vm->regs[op->dest.num] = (int32_t)round(val);

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_flt(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_tran_instr_t *op,
				subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->size == 4) {
		prv_set_fpa_f32(arm_vm, op->dest.num,
				(float)arm_vm->regs[op->op2.reg.num]);
	} else if (op->size == 8) {
		prv_set_fpa_f64(arm_vm, op->dest.num,
				(double)arm_vm->regs[op->op2.reg.num]);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_set_double_sub_flags(subtilis_arm_vm_t *arm_vm, double op1,
				     double op2, double res)
{
	prv_reset_flags(arm_vm);
	arm_vm->zero_flag = res == 0;
	arm_vm->negative_flag = res < 0;
	arm_vm->carry_flag = (op1 < 0 && op2 >= 0) || (op1 < 0 && res >= 0) ||
			     (op2 >= 0 && res >= 0);
	if (((op1 < 0 && op2 >= 0) || (op1 >= 0 && op2 < 0)) &&
	    ((op1 >= 0 && res < 0) || (op1 < 0 && res >= 0)))
		arm_vm->overflow_flag = true;
}

static void prv_process_fpa_cmf_gen(subtilis_arm_vm_t *arm_vm,
				    subtilis_fpa_cmp_instr_t *op,
				    bool negate_op2, subtilis_error_t *err)
{
	double op1;
	double op2;
	double res;
	size_t size;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	size = arm_vm->fregs[op->dest.num].size;
	if (size == 4) {
		op1 = (double)arm_vm->fregs[op->dest.num].val.real32;
	} else if (size == 8) {
		op1 = arm_vm->fregs[op->dest.num].val.real64;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (op->immediate) {
		op2 = subtilis_fpa_extract_imm(op->op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		size = arm_vm->fregs[op->op2.reg.num].size;
		if (size == 4) {
			op2 = (double)arm_vm->fregs[op->op2.reg.num].val.real32;
		} else if (size == 8) {
			op2 = arm_vm->fregs[op->op2.reg.num].val.real64;
		} else {
			subtilis_error_set_assertion_failed(err);
			return;
		}
	}

	if (negate_op2)
		op2 = -op2;

	res = op1 - op2;
	prv_set_double_sub_flags(arm_vm, op1, op2, res);

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_cmf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_cmp_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_cmf_gen(arm_vm, op, false, err);
}

static void prv_process_fpa_cnf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_cmp_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_cmf_gen(arm_vm, op, true, err);
}

void subtilis_arm_vm_run(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			 subtilis_error_t *err)
{
	size_t pc;
	subtilis_arm_instr_t instr;

	arm_vm->quit = false;
	arm_vm->negative_flag = false;
	arm_vm->zero_flag = false;
	arm_vm->carry_flag = false;
	arm_vm->overflow_flag = false;
	arm_vm->regs[15] = 0x8000 + 8;

	pc = prv_calc_pc(arm_vm);
	while (!arm_vm->quit && pc < arm_vm->code_size) {
		subtilis_arm_disass(&instr, ((uint32_t *)arm_vm->memory)[pc],
				    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		switch (instr.type) {
		case SUBTILIS_ARM_INSTR_AND:
			prv_process_and(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_EOR:
			prv_process_eor(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_SUB:
			prv_process_sub(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_RSB:
			prv_process_rsb(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_ADD:
			prv_process_add(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_ADC:
			prv_process_adc(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_CMP:
			prv_process_cmp(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_CMN:
			prv_process_cmn(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_ORR:
			prv_process_orr(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MOV:
			prv_process_mov(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MVN:
			prv_process_mvn(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MUL:
			prv_process_mul(arm_vm, &instr.operands.mul, err);
			break;
		case SUBTILIS_ARM_INSTR_LDR:
			prv_process_ldr(arm_vm, &instr.operands.stran, err);
			break;
		case SUBTILIS_ARM_INSTR_STR:
			prv_process_str(arm_vm, &instr.operands.stran, err);
			break;
		case SUBTILIS_ARM_INSTR_B:
			prv_process_b(arm_vm, &instr.operands.br, err);
			break;
		case SUBTILIS_ARM_INSTR_SWI:
			prv_process_swi(arm_vm, b, &instr.operands.swi, err);
			break;
		case SUBTILIS_ARM_INSTR_STM:
			prv_process_stm(arm_vm, &instr.operands.mtran, err);
			break;
		case SUBTILIS_ARM_INSTR_LDM:
			prv_process_ldm(arm_vm, &instr.operands.mtran, err);
			break;
		case SUBTILIS_FPA_INSTR_LDF:
			prv_process_fpa_ldf(arm_vm, &instr.operands.fpa_stran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_STF:
			prv_process_fpa_stf(arm_vm, &instr.operands.fpa_stran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_MVF:
			prv_process_fpa_mvf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_MNF:
			prv_process_fpa_mnf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_ADF:
			prv_process_fpa_adf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_MUF:
			prv_process_fpa_muf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_SUF:
			prv_process_fpa_suf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_RSF:
			prv_process_fpa_rsf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_DVF:
			prv_process_fpa_dvf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_RDF:
			prv_process_fpa_rdf(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_FLT:
			prv_process_fpa_flt(arm_vm, &instr.operands.fpa_tran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_FIX:
			prv_process_fpa_fix(arm_vm, &instr.operands.fpa_tran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_CMF:
			prv_process_fpa_cmf(arm_vm, &instr.operands.fpa_cmp,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_CNF:
			prv_process_fpa_cnf(arm_vm, &instr.operands.fpa_cmp,
					    err);
			break;
		default:
			printf("instr type %d\n", instr.type);
			subtilis_error_set_assertion_failed(err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		/*
		 *		subtilis_arm_disass_dump(
		 *		   (uint8_t *)&((uint32_t *)arm_vm->memory)[pc],
		 *4);
		 *		printf("r=%d d=%d t=%d q=%d s=%d\n",
		 *arm_vm->regs[0],
		 *		       arm_vm->regs[1], arm_vm->regs[2],
		 *arm_vm->regs[3],
		 *		       arm_vm->regs[4]);
		 *		printf("zf=%d cf=%d n=%d ov=%d\n",
		 *arm_vm->zero_flag,
		 *		       arm_vm->carry_flag,
		 *arm_vm->negative_flag,
		 *		       arm_vm->overflow_flag);
		 */
		pc = prv_calc_pc(arm_vm);
	}
}
