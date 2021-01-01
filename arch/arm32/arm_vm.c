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

#include "../../common/error_codes.h"
#include "../../common/utils.h"
#include "arm_disass.h"
#include "arm_vm.h"

subtilis_arm_vm_t *subtilis_arm_vm_new(uint8_t *code, size_t code_size,
				       size_t mem_size, int32_t start_address,
				       bool vfp, subtilis_error_t *err)
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

	memcpy(arm_vm->memory, code, code_size);
	arm_vm->code_size = code_size / 4;
	arm_vm->mem_size = mem_size;

	arm_vm->reverse_fpa_consts = !vfp && (*lower_word) == 0;
	arm_vm->start_address = start_address;
	arm_vm->vfp = vfp;

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
	return (size_t)(((vm->regs[15] - vm->start_address) - 8) / 4);
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
	addr -= arm_vm->start_address;
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
		return arm_vm->regs[op2->op.reg];
	case SUBTILIS_ARM_OP2_I32:
		val = op2->op.integer;
		if ((val & 0xf00) == 0)
			return val;
		shift = (val & 0xf00) >> 7;
		val &= 0xff;
		return val >> shift | val << (32 - shift);
	case SUBTILIS_ARM_OP2_SHIFTED:
		if (op2->op.shift.shift_reg) {
			shift = arm_vm->regs[op2->op.shift.shift.reg];
			if (shift > 32)
				shift = 32;
		} else {
			shift = op2->op.shift.shift.integer;
		}
		reg = arm_vm->regs[op2->op.shift.reg];
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
			if (shift < 0 || shift > 32) {
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
		shift_val = arm_vm->regs[shift.shift.reg];
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
			arm_vm->carry_flag = (shifted & (1u << 31)) != 0;
		else if (shift_val > 32)
			arm_vm->carry_flag = false;
		break;
	case SUBTILIS_ARM_SHIFT_ASR:
		if (shift_val > 0 && shift_val <= 31)
			arm_vm->carry_flag =
			    (shifted & (1 << (shift_val - 1))) != 0;
		else if (shift_val >= 32)
			arm_vm->carry_flag = (shifted & (1u << 31)) != 0;
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

	res = arm_vm->regs[op->op1] & op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = res;
	arm_vm->regs[15] += 4;
}

static void prv_process_bic(subtilis_arm_vm_t *arm_vm,
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

	op2 = ~prv_eval_op2(arm_vm, op->status, &op->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	res = arm_vm->regs[op->op1] & op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = res;
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

	res = arm_vm->regs[op->op1] ^ op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = res;
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

	res = arm_vm->regs[op->op1] - op2;
	if (op->status)
		prv_set_sub_flags(arm_vm, arm_vm->regs[op->op1], op2, res);
	arm_vm->regs[op->dest] = res;
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

	res = op2 - arm_vm->regs[op->op1];
	if (op->status)
		prv_set_sub_flags(arm_vm, op2, arm_vm->regs[op->op1], res);

	arm_vm->regs[15] += 4;
	arm_vm->regs[op->dest] = res;
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
	res = arm_vm->regs[op->op1] + op2 + carry;
	if (op->status)
		prv_set_add_flags(arm_vm, arm_vm->regs[op->op1], op2 + carry,
				  res);
	arm_vm->regs[op->dest] = res;
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

	res = arm_vm->regs[op->op1] + op2;
	if (op->status)
		prv_set_add_flags(arm_vm, arm_vm->regs[op->op1], op2, res);
	arm_vm->regs[15] += 4;
	arm_vm->regs[op->dest] = res;
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
	op1 = arm_vm->regs[op->op1];
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
	op1 = arm_vm->regs[op->op1];
	prv_set_status_flags(arm_vm, op1, op2);

	arm_vm->regs[15] += 4;
}

static void prv_process_tst(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;
	int32_t op2_old;

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

	res = arm_vm->regs[op->op1] & op2;
	prv_set_and_flags(arm_vm, res);
	if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
		op2_old = arm_vm->regs[op->op2.op.shift.reg];
		prv_set_shift_flags(arm_vm, op2_old, res, op->op2.op.shift);
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_teq(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_data_instr_t *op,
			    subtilis_error_t *err)
{
	int32_t op2;
	int32_t res;
	int32_t op2_old;

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

	res = arm_vm->regs[op->op1] ^ op2;
	prv_set_and_flags(arm_vm, res);
	if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
		op2_old = arm_vm->regs[op->op2.op.shift.reg];
		prv_set_shift_flags(arm_vm, op2_old, res, op->op2.op.shift);
	}

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

	res = arm_vm->regs[op->op1] | op2;
	if (op->status) {
		prv_set_and_flags(arm_vm, res);
		if (op->op2.type == SUBTILIS_ARM_OP2_SHIFTED) {
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, res,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = res;
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
		addr = arm_vm->regs[op->base] + offset;
		if (op->write_back)
			arm_vm->regs[op->base] = (int32_t)addr;
	} else {
		addr = arm_vm->regs[op->base];
		arm_vm->regs[op->base] += offset;
	}

	addr -= arm_vm->start_address;
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

static size_t prv_compute_fp_stran_addr(subtilis_arm_vm_t *arm_vm,
					subtilis_arm_reg_t base, int32_t offset,
					bool subtract, bool pre_indexed,
					bool write_back, size_t size,
					subtilis_error_t *err)
{
	size_t addr;

	if (offset > 256) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}
	offset *= 4;
	if (subtract)
		offset = -offset;

	if (pre_indexed) {
		addr = arm_vm->regs[base] + offset;
		if (write_back)
			arm_vm->regs[base] = (int32_t)addr;
	} else {
		addr = arm_vm->regs[base];
		if (write_back)
			arm_vm->regs[base] += offset;
	}

	addr -= arm_vm->start_address;

	if (addr + size > arm_vm->mem_size) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	return addr;
}

static size_t prv_compute_fpa_stran_addr(subtilis_arm_vm_t *arm_vm,
					 subtilis_fpa_stran_instr_t *op,
					 subtilis_error_t *err)
{
	return prv_compute_fp_stran_addr(arm_vm, op->base, op->offset,
					 op->subtract, op->pre_indexed,
					 op->write_back, op->size, err);
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
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, op2,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = op2;
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
			op2_old = arm_vm->regs[op->op2.op.shift.reg];
			prv_set_shift_flags(arm_vm, op2_old, -op2,
					    op->op2.op.shift);
		}
	}

	arm_vm->regs[op->dest] = ~op2;
	arm_vm->regs[15] += 4;
}

static void prv_process_mul(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mul_instr_t *op, subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	arm_vm->regs[op->dest] = arm_vm->regs[op->rm] * arm_vm->regs[op->rs];
	arm_vm->regs[15] += 4;
}

static void prv_process_mla(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mul_instr_t *op, subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	arm_vm->regs[op->dest] =
	    arm_vm->regs[op->rm] * arm_vm->regs[op->rs] + arm_vm->regs[op->rn];
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
	if (op->byte)
		arm_vm->regs[op->dest] = *((uint8_t *)&arm_vm->memory[addr]);
	else
		arm_vm->regs[op->dest] = *((int32_t *)&arm_vm->memory[addr]);
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
	if (op->byte)
		*((uint8_t *)&arm_vm->memory[addr]) =
		    arm_vm->regs[op->dest] & 255;
	else
		*((int32_t *)&arm_vm->memory[addr]) = arm_vm->regs[op->dest];
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

static void prv_inkey(subtilis_arm_vm_t *arm_vm, subtilis_error_t *err)
{
	if (arm_vm->regs[2] != 0xff)
		arm_vm->regs[1] = getchar();
	else if (arm_vm->regs[1] == 0)
		arm_vm->regs[1] = 2;
	else
		arm_vm->regs[1] = 0;
}

static void prv_os_byte(subtilis_arm_vm_t *arm_vm, subtilis_error_t *err)
{
	switch (arm_vm->regs[0]) {
	case 129:
		prv_inkey(arm_vm, err);
		break;
	case 134: // pos, vpos
		arm_vm->regs[1] = 0;
		arm_vm->regs[2] = 0;
		break;
	default:
		break;
	}
}

static void prv_process_swi(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			    subtilis_arm_swi_instr_t *op, subtilis_error_t *err)
{
	char buf[64];
	uint8_t *addr;
	size_t buf_len;
	size_t ptr;
	size_t code;
	const char *format = "%d";

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	switch (op->code) {
	case 0x11 + 0x20000:
	case 0x11:
		arm_vm->quit = true;
		break;
	case 0x10 + 0x20000:
	case 0x10:
		/* OS_GetEnv */
		arm_vm->regs[0] = 0;
		arm_vm->regs[1] = arm_vm->start_address + arm_vm->mem_size - 4;
		arm_vm->regs[2] = 0;
		break;
	case 0xd4 + 0x20000:
	case 0xd4:
		/* OS_ConvertHex8 */
		format = "%X";
	case 0xdc + 0x20000:
	case 0xdc:
		/* OS_ConvertInteger4  */
		buf_len = sprintf(buf, format, arm_vm->regs[0]);
		if (buf_len + 1 > arm_vm->regs[2]) {
			*((uint32_t *)&arm_vm->memory[arm_vm->mem_size - 4]) =
			    SUBTILIS_ERROR_CODE_BUFFER_OVERFLOW;
			arm_vm->overflow_flag = true;
			arm_vm->regs[0] =
			    arm_vm->mem_size - 4 + arm_vm->start_address;
			break;
		}
		addr =
		    prv_get_vm_address(arm_vm, arm_vm->regs[1], buf_len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(addr, buf, buf_len);
		arm_vm->regs[0] = arm_vm->regs[1];
		arm_vm->regs[1] += buf_len;
		arm_vm->regs[2] -= buf_len;
		break;
	case 0x20000:
	case 0x0:
		/* OS_WriteCh  */
		buf[0] = arm_vm->regs[0] & 0xff;
		buf[1] = 0;
		if (buf[0] == 10 || buf[0] == 13 ||
		    (buf[0] >= 32 && buf[0] <= 127)) {
			subtilis_buffer_append_string(b, buf, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		break;
	case 0x2 + 0x20000:
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
	case 0x46:
	case 0x46 + 0x20000:
		/* OS_WriteN */
		addr = prv_get_vm_address(arm_vm, arm_vm->regs[0],
					  arm_vm->regs[1], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_buffer_append(b, addr, arm_vm->regs[1], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		break;
	case 0x3 + 0x20000:
	case 0x3:
		/* OS_Newline  */
		subtilis_buffer_append_string(b, "\n", err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		break;
	case 0x4 + 0x20000:
	case 0x4:
		/* OS_ReadC  */
		arm_vm->regs[0] = getchar();
		break;
	case 0x6 + 0x20000:
	case 0x6:
		/* OS_Byte */
		prv_os_byte(arm_vm, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		break;
	case 0x7 + 0x20000:
	case 0x7:
		/* OS_Word  */
		if (arm_vm->regs[0] == 1) {
			ptr = arm_vm->regs[1] - arm_vm->start_address;
			*((int32_t *)&arm_vm->memory[ptr]) =
			    subtilis_get_i32_time();
		}
		break;
	case 0x32 + 0x20000:
	case 0x32:
		/* OS_ReadPoint  */
		arm_vm->regs[2] = 0;
		arm_vm->regs[3] = 0;
		arm_vm->regs[4] = 0;
		break;
	default:
		code = op->code;
		code &= ~((size_t)0x20000);
		if (code >= (256 + 32) && code < 512) {
			buf[0] = code - 256;
			buf[1] = 0;
			subtilis_buffer_append_string(b, buf, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		break;
	}

	arm_vm->regs[15] += 4;
}

static size_t prv_write_multiple_reg(subtilis_arm_vm_t *arm_vm,
				     subtilis_arm_mtran_instr_t *op, int i,
				     size_t addr, int after, int before,
				     subtilis_error_t *err)
{
	if (((1 << i) & op->reg_list) == 0)
		return addr;
	addr += before;
	if (addr + 4 > arm_vm->mem_size) {
		subtilis_error_set_assertion_failed(err);
		return -1;
	}
	*((int32_t *)&arm_vm->memory[addr]) = arm_vm->regs[i];
	return addr + after;
}

static void prv_process_stm(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mtran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;
	int i;
	int after = 0;
	int before = 0;
	bool increment = false;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = arm_vm->regs[op->op0] - arm_vm->start_address;
	switch (op->type) {
	case SUBTILIS_ARM_MTRAN_FA:
	case SUBTILIS_ARM_MTRAN_IB:
		increment = true;
		before = 4;
		break;
	case SUBTILIS_ARM_MTRAN_FD:
	case SUBTILIS_ARM_MTRAN_DB:
		before = -4;
		break;
	case SUBTILIS_ARM_MTRAN_EA:
	case SUBTILIS_ARM_MTRAN_IA:
		increment = true;
		after = 4;
		break;
	case SUBTILIS_ARM_MTRAN_ED:
	case SUBTILIS_ARM_MTRAN_DA:
		after = -4;
		break;
	}

	if (increment) {
		for (i = 0; i < 15; i++) {
			addr = prv_write_multiple_reg(arm_vm, op, i, addr,
						      after, before, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} else {
		for (i = 14; i >= 0; i--) {
			addr = prv_write_multiple_reg(arm_vm, op, i, addr,
						      after, before, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	if (op->write_back)
		arm_vm->regs[op->op0] = addr + arm_vm->start_address;

	arm_vm->regs[15] += 4;
}

static size_t prv_read_multiple_reg(subtilis_arm_vm_t *arm_vm,
				    subtilis_arm_mtran_instr_t *op, int i,
				    size_t addr, int after, int before,
				    subtilis_error_t *err)
{
	if (((1 << i) & op->reg_list) == 0)
		return addr;
	addr += before;
	if (addr + 4 > arm_vm->mem_size) {
		subtilis_error_set_assertion_failed(err);
		return -1;
	}
	arm_vm->regs[i] = *((int32_t *)&arm_vm->memory[addr]);
	return addr + after;
}

static void prv_process_ldm(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_mtran_instr_t *op,
			    subtilis_error_t *err)
{
	size_t addr;
	int i;
	int after = 0;
	int before = 0;
	bool increment = false;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = arm_vm->regs[op->op0] - arm_vm->start_address;

	switch (op->type) {
	case SUBTILIS_ARM_MTRAN_FA:
	case SUBTILIS_ARM_MTRAN_DA:
		after = -4;
		break;
	case SUBTILIS_ARM_MTRAN_FD:
	case SUBTILIS_ARM_MTRAN_IA:
		increment = true;
		after = 4;
		break;
	case SUBTILIS_ARM_MTRAN_EA:
	case SUBTILIS_ARM_MTRAN_DB:
		before = -4;
		break;
	case SUBTILIS_ARM_MTRAN_ED:
	case SUBTILIS_ARM_MTRAN_IB:
		increment = true;
		before = 4;
		break;
	}

	if (!increment) {
		for (i = 14; i >= 0; i--) {
			addr = prv_read_multiple_reg(arm_vm, op, i, addr, after,
						     before, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} else {
		for (i = 0; i < 15; i++) {
			addr = prv_read_multiple_reg(arm_vm, op, i, addr, after,
						     before, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	if (op->write_back)
		arm_vm->regs[op->op0] = addr + arm_vm->start_address;

	// TODO: Is this correct? We dont do this for other loads

	if (!((1 << 15) & op->reg_list))
		arm_vm->regs[15] += 4;
}

static void prv_process_msr(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_flags_instr_t *op,
			    subtilis_error_t *err)
{
	uint32_t word;
	uint32_t mask = 0;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->flag_reg == SUBTILIS_ARM_FLAGS_SPSR) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word = op->op2_reg ? arm_vm->regs[op->op.reg] : op->op.integer;
	if (op->fields & SUBTILIS_ARM_FLAGS_FIELD_CONTROL)
		mask |= 0xff;
	if (op->fields & SUBTILIS_ARM_FLAGS_FIELD_EXTENSION)
		mask |= 0xff00;
	if (op->fields & SUBTILIS_ARM_FLAGS_FIELD_STATUS)
		mask |= 0xff0000;
	if (op->fields & SUBTILIS_ARM_FLAGS_FIELD_FLAGS)
		mask |= 0xff000000;

	arm_vm->fpscr = word & mask;

	arm_vm->regs[15] += 4;
}

static void prv_process_mrs(subtilis_arm_vm_t *arm_vm,
			    subtilis_arm_flags_instr_t *op,
			    subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->flag_reg == SUBTILIS_ARM_FLAGS_SPSR) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[op->op.reg] = arm_vm->fpscr;

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_ldf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_stran_instr_t *op,
				subtilis_error_t *err)
{
	size_t addr;
	uint32_t *ptr;
	uint64_t dbl;
	double *dbl_cast;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_fpa_stran_addr(arm_vm, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (op->size == 4) {
		prv_set_fpa_f32(arm_vm, op->dest,
				*((float *)&arm_vm->memory[addr]));
	} else if (op->size == 8) {
		if (arm_vm->reverse_fpa_consts) {
			ptr = (uint32_t *)&arm_vm->memory[addr];
			dbl = ptr[0];
			dbl = (dbl << 32) | ptr[1];
			dbl_cast = (double *)&dbl;
			prv_set_fpa_f64(arm_vm, op->dest, *dbl_cast);
		} else {
			prv_set_fpa_f64(arm_vm, op->dest,
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
		    arm_vm->fregs[op->dest].val.real32;
	} else if (op->size == 8) {
		if (arm_vm->reverse_fpa_consts) {
			ptr = (uint32_t *)&arm_vm->fregs[op->dest].val.real64;
			*((uint32_t *)&arm_vm->memory[addr]) = ptr[1];
			*((uint32_t *)&arm_vm->memory[addr + 4]) = ptr[0];
		} else {
			*((double *)&arm_vm->memory[addr]) =
			    arm_vm->fregs[op->dest].val.real64;
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

	dest = op->dest;

	if (op->size == 4) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_set_fpa_f32(arm_vm, dest, (float)imm);
		} else {
			prv_set_fpa_f32(arm_vm, dest,
					arm_vm->fregs[op->op2.reg].val.real32);
		}
	} else if (op->size == 8) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_set_fpa_f64(arm_vm, dest, imm);
		} else {
			prv_set_fpa_f64(arm_vm, dest,
					arm_vm->fregs[op->op2.reg].val.real64);
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

	dest = op->dest;

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

	dest = op->dest;
	op1 = op->op1;

	if (op->size == 4) {
		res32 = arm_vm->fregs[op1].val.real32;
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			res32 = f32_op(res32, (float)imm);
		} else {
			res32 = f32_op(res32,
				       arm_vm->fregs[op->op2.reg].val.real32);
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
			res64 = f64_op(res64,
				       arm_vm->fregs[op->op2.reg].val.real64);
		}
		prv_set_fpa_f64(arm_vm, dest, res64);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_monadic_dbl(subtilis_arm_vm_t *arm_vm,
					subtilis_fpa_data_instr_t *op,
					double (*f64_op)(double),
					subtilis_error_t *err)
{
	size_t dest;
	double imm;
	float res32;
	double res64;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	dest = op->dest;

	if (op->size == 4) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			res32 = (float)f64_op(imm);
		} else {
			res32 = (float)f64_op(
			    (double)arm_vm->fregs[op->op2.reg].val.real32);
		}
		prv_set_fpa_f32(arm_vm, dest, res32);
	} else if (op->size == 8) {
		if (op->immediate) {
			imm = subtilis_fpa_extract_imm(op->op2, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			res64 = f64_op(imm);
		} else {
			res64 = f64_op(arm_vm->fregs[op->op2.reg].val.real64);
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
	if (!op->immediate) {
		if (((op->size == 4) &&
		     (arm_vm->fregs[op->op2.reg].val.real32 == 0.0)) ||
		    ((op->size == 8) &&
		     (arm_vm->fregs[op->op2.reg].val.real64 == 0.0)))
			arm_vm->fpa_status |= 2;
	}

	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_div_real32,
			       prv_process_fpa_div_real64, err);
}

static float prv_process_fpa_rdiv_real32(float a, float b) { return b / a; }

static double prv_process_fpa_rdiv_real64(double a, double b) { return b / a; }

static void prv_process_fpa_rdf(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	if (((op->size == 4) && (arm_vm->fregs[op->op1].val.real32 == 0.0)) ||
	    ((op->size == 8) && (arm_vm->fregs[op->op1].val.real64 == 0.0)))
		arm_vm->fpa_status |= 2;

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

	if (op->immediate) {
		val = subtilis_fpa_extract_imm(op->op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		if (arm_vm->fregs[op->op2.reg].size == 4)
			val = (double)arm_vm->fregs[op->op2.reg].val.real32;
		else
			val = arm_vm->fregs[op->op2.reg].val.real64;
	}
	if (op->rounding == SUBTILIS_FPA_ROUNDING_ZERO)
		arm_vm->regs[op->dest] = (int32_t)val;
	else if (op->rounding == SUBTILIS_FPA_ROUNDING_MINUS_INFINITY)
		arm_vm->regs[op->dest] = (int32_t)floor(val);
	else if (op->rounding == SUBTILIS_FPA_ROUNDING_PLUS_INFINITY)
		arm_vm->regs[op->dest] = (int32_t)ceil(val);
	else
		arm_vm->regs[op->dest] = (int32_t)round(val);

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
		prv_set_fpa_f32(arm_vm, op->dest,
				(float)arm_vm->regs[op->op2.reg]);
	} else if (op->size == 8) {
		prv_set_fpa_f64(arm_vm, op->dest,
				(double)arm_vm->regs[op->op2.reg]);
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

	size = arm_vm->fregs[op->dest].size;
	if (size == 4) {
		op1 = (double)arm_vm->fregs[op->dest].val.real32;
	} else if (size == 8) {
		op1 = arm_vm->fregs[op->dest].val.real64;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (op->immediate) {
		op2 = subtilis_fpa_extract_imm(op->op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		size = arm_vm->fregs[op->op2.reg].size;
		if (size == 4) {
			op2 = (double)arm_vm->fregs[op->op2.reg].val.real32;
		} else if (size == 8) {
			op2 = arm_vm->fregs[op->op2.reg].val.real64;
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

static void prv_process_fpa_sin(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, sin, err);
}

static void prv_process_fpa_cos(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, cos, err);
}

static void prv_process_fpa_tan(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, tan, err);
}

static void prv_process_fpa_asn(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, asin, err);
}

static void prv_process_fpa_acs(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, acos, err);
}

static void prv_process_fpa_atn(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, atan, err);
}

static void prv_process_fpa_sqr(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, sqrt, err);
}

static void prv_process_fpa_log(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	if (!op->immediate) {
		if (((op->size == 4) &&
		     (arm_vm->fregs[op->op2.reg].val.real32 == 0.0)) ||
		    ((op->size == 8) &&
		     (arm_vm->fregs[op->op2.reg].val.real64 == 0.0)))
			arm_vm->fpa_status |= 2;
		else if (((op->size == 4) &&
			  (arm_vm->fregs[op->op2.reg].val.real32 < 0.0)) ||
			 ((op->size == 8) &&
			  (arm_vm->fregs[op->op2.reg].val.real64 < 0.0)))
			arm_vm->fpa_status |= 1;
	}

	prv_process_fpa_monadic_dbl(arm_vm, op, log10, err);
}

static void prv_process_fpa_ln(subtilis_arm_vm_t *arm_vm,
			       subtilis_fpa_data_instr_t *op,
			       subtilis_error_t *err)
{
	if (!op->immediate) {
		if (((op->size == 4) &&
		     (arm_vm->fregs[op->op2.reg].val.real32 < 0.0)) ||
		    ((op->size == 8) &&
		     (arm_vm->fregs[op->op2.reg].val.real64 < 0.0)))
			arm_vm->fpa_status |= 2;
		else if (((op->size == 4) &&
			  (arm_vm->fregs[op->op2.reg].val.real32 < 0.0)) ||
			 ((op->size == 8) &&
			  (arm_vm->fregs[op->op2.reg].val.real64 < 0.0)))
			arm_vm->fpa_status |= 1;
	}

	prv_process_fpa_monadic_dbl(arm_vm, op, log, err);
}

static void prv_process_fpa_exp(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, exp, err);
}

static void prv_process_fpa_abs(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_monadic_dbl(arm_vm, op, fabs, err);
}

static void prv_process_fpa_wfs(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_cptran_instr_t *op,
				subtilis_error_t *err)
{
	arm_vm->fpa_status = arm_vm->regs[op->dest];
	arm_vm->regs[15] += 4;
}

static void prv_process_fpa_rfs(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_cptran_instr_t *op,
				subtilis_error_t *err)
{
	arm_vm->regs[op->dest] = arm_vm->fpa_status;
	arm_vm->regs[15] += 4;
}

static float prv_process_fpa_pow_real32(float a, float b)
{
	return (float)pow(a, b);
}

static double prv_process_fpa_pow_real64(double a, double b)
{
	return pow(a, b);
}

static void prv_process_fpa_pow(subtilis_arm_vm_t *arm_vm,
				subtilis_fpa_data_instr_t *op,
				subtilis_error_t *err)
{
	prv_process_fpa_dyadic(arm_vm, op, prv_process_fpa_pow_real32,
			       prv_process_fpa_pow_real64, err);
}

static double vfp_round_dbl(subtilis_arm_vm_t *arm_vm, double val)
{
	switch ((arm_vm->fpscr >> 22) & 3) {
	case 0:
		return round(val);
	case 1:
		return ceil(val);
	case 2:
		return floor(val);
	case 3:
		return trunc(val);
	}

	return 0.0;
}

static void prv_vfp_set_double_sub_flags(subtilis_arm_vm_t *arm_vm, double op1,
					 double op2)
{
	arm_vm->fpscr &= 0xfffffff;

	if (op1 == op2) {
		arm_vm->fpscr |= 0x40000000;
		arm_vm->fpscr |= 0x20000000;
	} else if (op1 < op2) {
		arm_vm->fpscr |= 0x80000000;
	} else if (op1 > op2) {
		arm_vm->fpscr |= 0x20000000;
	}
}

static void prv_process_vfp_cmp_gen(subtilis_arm_vm_t *arm_vm, bool dbl,
				    bool zero, subtilis_vfp_cmp_instr_t *op,
				    subtilis_error_t *err)
{
	double op1;
	double op2;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (!dbl)
		op1 = (double)arm_vm->vpfregs.f[op->op1];
	else
		op1 = arm_vm->vpfregs.d[op->op1];

	if (zero)
		op2 = 0.0;
	else if (!dbl)
		op2 = (double)arm_vm->vpfregs.f[op->op2];
	else
		op2 = arm_vm->vpfregs.d[op->op2];

	prv_vfp_set_double_sub_flags(arm_vm, op1, op2);

	arm_vm->regs[15] += 4;
}

static size_t prv_compute_vfp_stran_addr(subtilis_arm_vm_t *arm_vm,
					 subtilis_vfp_stran_instr_t *op,
					 size_t size, subtilis_error_t *err)
{
	return prv_compute_fp_stran_addr(arm_vm, op->base, op->offset,
					 op->subtract, op->pre_indexed,
					 op->write_back, size, err);
}

static void prv_process_vfp_ldf(subtilis_arm_vm_t *arm_vm,
				subtilis_vfp_stran_instr_t *op, size_t size,
				subtilis_error_t *err)
{
	size_t addr;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_vfp_stran_addr(arm_vm, op, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (size == 4) {
		arm_vm->vpfregs.f[op->dest] = *((float *)&arm_vm->memory[addr]);
	} else if (size == 8) {
		arm_vm->vpfregs.d[op->dest] =
		    *((double *)&arm_vm->memory[addr]);
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_stf(subtilis_arm_vm_t *arm_vm,
				subtilis_vfp_stran_instr_t *op, size_t size,
				subtilis_error_t *err)
{
	size_t addr;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	addr = prv_compute_vfp_stran_addr(arm_vm, op, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (size == 4) {
		*((float *)&arm_vm->memory[addr]) = arm_vm->vpfregs.f[op->dest];
	} else if (size == 8) {
		*((double *)&arm_vm->memory[addr]) =
		    arm_vm->vpfregs.d[op->dest];
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_copy(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_copy_instr_t *op,
				 void (*fn)(subtilis_arm_vm_t *,
					    subtilis_vfp_copy_instr_t *,
					    subtilis_error_t *),
				 subtilis_error_t *err)
{
	if (prv_match_ccode(arm_vm, op->ccode))
		fn(arm_vm, op, err);

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fcpys(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] = arm_vm->vpfregs.f[op->src];
}

static void prv_process_vfp_fcpyd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] = arm_vm->vpfregs.d[op->src];
}

static void prv_process_vfp_fnegs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] = -arm_vm->vpfregs.f[op->src];
}

static void prv_process_vfp_fnegd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] = -arm_vm->vpfregs.d[op->src];
}

static void prv_process_vfp_fabss(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	if (arm_vm->vpfregs.f[op->src] < 0)
		arm_vm->vpfregs.f[op->dest] = -arm_vm->vpfregs.f[op->src];
	else
		arm_vm->vpfregs.f[op->dest] = arm_vm->vpfregs.f[op->src];
}

static void prv_process_vfp_fabsd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_copy_instr_t *op,
				  subtilis_error_t *err)
{
	if (arm_vm->vpfregs.d[op->src] < 0)
		arm_vm->vpfregs.d[op->dest] = -arm_vm->vpfregs.d[op->src];
	else
		arm_vm->vpfregs.d[op->dest] = arm_vm->vpfregs.d[op->src];
}

static void prv_process_vfp_tran(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_tran_instr_t *op,
				 void (*fn)(subtilis_arm_vm_t *,
					    subtilis_vfp_tran_instr_t *,
					    subtilis_error_t *),
				 subtilis_error_t *err)
{
	if (prv_match_ccode(arm_vm, op->ccode))
		fn(arm_vm, op, err);

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fsitod(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	int32_t *val = (int32_t *)&arm_vm->vpfregs.f[op->src];

	arm_vm->vpfregs.d[op->dest] = *val;
}

static void prv_process_vfp_fsitos(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	int32_t *val = (int32_t *)&arm_vm->vpfregs.f[op->src];

	arm_vm->vpfregs.f[op->dest] = *val;
}

static void prv_process_vfp_ftosis(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	int32_t val =
	    (int32_t)vfp_round_dbl(arm_vm, arm_vm->vpfregs.f[op->src]);
	int32_t *dest = (int32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftosid(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	int32_t val =
	    (int32_t)vfp_round_dbl(arm_vm, arm_vm->vpfregs.d[op->src]);
	int32_t *dest = (int32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftouis(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	uint32_t val =
	    (uint32_t)vfp_round_dbl(arm_vm, arm_vm->vpfregs.f[op->src]);
	uint32_t *dest = (uint32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftouid(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	uint32_t val =
	    (uint32_t)vfp_round_dbl(arm_vm, arm_vm->vpfregs.d[op->src]);
	uint32_t *dest = (uint32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftosizs(subtilis_arm_vm_t *arm_vm,
				    subtilis_vfp_tran_instr_t *op,
				    subtilis_error_t *err)
{
	uint32_t val = (uint32_t)arm_vm->vpfregs.f[op->src];
	uint32_t *dest = (uint32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftosizd(subtilis_arm_vm_t *arm_vm,
				    subtilis_vfp_tran_instr_t *op,
				    subtilis_error_t *err)
{
	int32_t val = (int32_t)arm_vm->vpfregs.d[op->src];
	int32_t *dest = (int32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftouizs(subtilis_arm_vm_t *arm_vm,
				    subtilis_vfp_tran_instr_t *op,
				    subtilis_error_t *err)
{
	uint32_t val = (uint32_t)arm_vm->vpfregs.f[op->src];
	uint32_t *dest = (uint32_t *)&arm_vm->vpfregs.f[op->dest];

	*dest = val;
}

static void prv_process_vfp_ftouizd(subtilis_arm_vm_t *arm_vm,
				    subtilis_vfp_tran_instr_t *op,
				    subtilis_error_t *err)
{
	uint32_t val = (uint32_t)arm_vm->vpfregs.d[op->src];
	uint32_t *dest = (uint32_t *)&arm_vm->vpfregs.d[op->dest];

	*dest = val;
}

static void prv_process_vfp_fuitod(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	uint32_t val = (uint32_t)arm_vm->vpfregs.f[op->src];

	arm_vm->vpfregs.d[op->dest] = val;
}

static void prv_process_vfp_fuitos(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_tran_instr_t *op,
				   subtilis_error_t *err)
{
	uint32_t val = (uint32_t)arm_vm->vpfregs.f[op->src];

	arm_vm->vpfregs.f[op->dest] = val;
}

static void prv_process_vfp_fmrs(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_cptran_instr_t *op,
				 subtilis_error_t *err)
{
	uint32_t *src;

	if (prv_match_ccode(arm_vm, op->ccode)) {
		src = (uint32_t *)&arm_vm->vpfregs.f[op->src];
		arm_vm->regs[op->dest] = *src;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmsr(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_cptran_instr_t *op,
				 subtilis_error_t *err)
{
	uint32_t *dest;

	if (prv_match_ccode(arm_vm, op->ccode)) {
		dest = (uint32_t *)&arm_vm->vpfregs.f[op->dest];
		*dest = arm_vm->regs[op->src];
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_data(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_data_instr_t *op,
				 void (*fn)(subtilis_arm_vm_t *,
					    subtilis_vfp_data_instr_t *,
					    subtilis_error_t *),
				 subtilis_error_t *err)
{
	if (prv_match_ccode(arm_vm, op->ccode))
		fn(arm_vm, op, err);

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmacs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] +=
	    arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2];
}

static void prv_process_vfp_fmacd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] +=
	    arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2];
}

static void prv_process_vfp_fnmacs(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] +=
	    -(arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2]);
}

static void prv_process_vfp_fnmacd(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] +=
	    -(arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2]);
}

static void prv_process_vfp_fmscs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    (arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2]) -
	    arm_vm->vpfregs.f[op->dest];
}

static void prv_process_vfp_fmscd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    (arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2]) -
	    arm_vm->vpfregs.d[op->dest];
}

static void prv_process_vfp_fnmscs(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    (arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2]) -
	    (-arm_vm->vpfregs.f[op->dest]);
}

static void prv_process_vfp_fnmscd(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    (arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2]) -
	    (-arm_vm->vpfregs.d[op->dest]);
}

static void prv_process_vfp_fmuls(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    (arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2]);
}

static void prv_process_vfp_fmuld(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    (arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2]);
}

static void prv_process_vfp_fnmuls(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    -(arm_vm->vpfregs.f[op->op1] * arm_vm->vpfregs.f[op->op2]);
}

static void prv_process_vfp_fnmuld(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_data_instr_t *op,
				   subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    -(arm_vm->vpfregs.d[op->op1] * arm_vm->vpfregs.d[op->op2]);
}

static void prv_process_vfp_fadds(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    arm_vm->vpfregs.f[op->op1] + arm_vm->vpfregs.f[op->op2];
}

static void prv_process_vfp_faddd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    arm_vm->vpfregs.d[op->op1] + arm_vm->vpfregs.d[op->op2];
}

static void prv_process_vfp_fsubs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.f[op->dest] =
	    arm_vm->vpfregs.f[op->op1] - arm_vm->vpfregs.f[op->op2];
}

static void prv_process_vfp_fsubd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	arm_vm->vpfregs.d[op->dest] =
	    arm_vm->vpfregs.d[op->op1] - arm_vm->vpfregs.d[op->op2];
}

static void prv_process_vfp_fdivs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	if (arm_vm->vpfregs.f[op->op2] == 0.0) {
		arm_vm->fpscr |= 2;
#ifdef INFINITY
		arm_vm->vpfregs.f[op->dest] = INFINITY;
#endif
		return;
	}

	arm_vm->vpfregs.f[op->dest] =
	    arm_vm->vpfregs.f[op->op1] / arm_vm->vpfregs.f[op->op2];
}

static void prv_process_vfp_fdivd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_data_instr_t *op,
				  subtilis_error_t *err)
{
	if (arm_vm->vpfregs.d[op->op2] == 0.0) {
		arm_vm->fpscr |= 2;
#ifdef INFINITY
		arm_vm->vpfregs.f[op->dest] = INFINITY;
#endif
		return;
	}

	arm_vm->vpfregs.d[op->dest] =
	    arm_vm->vpfregs.d[op->op1] / arm_vm->vpfregs.d[op->op2];
}

static void prv_process_vfp_fmrx(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_sysreg_instr_t *op,
				 subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if (op->sysreg != SUBTILIS_VFP_SYSREG_FPSCR) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (op->arm_reg == 15) {
		arm_vm->negative_flag = arm_vm->fpscr & 0x80000000;
		arm_vm->zero_flag = arm_vm->fpscr & 0x40000000;
		arm_vm->carry_flag = arm_vm->fpscr & 0x20000000;
		arm_vm->overflow_flag = arm_vm->fpscr & 0x10000000;
	} else {
		arm_vm->regs[op->arm_reg] = arm_vm->fpscr;
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmxr(subtilis_arm_vm_t *arm_vm,
				 subtilis_vfp_sysreg_instr_t *op,
				 subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	if ((op->sysreg != SUBTILIS_VFP_SYSREG_FPSCR) || (op->arm_reg == 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	arm_vm->fpscr = arm_vm->regs[op->arm_reg];
	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_sqrt(subtilis_arm_vm_t *arm_vm, bool dbl,
				 subtilis_vfp_sqrt_instr_t *op,
				 subtilis_error_t *err)
{
	if (prv_match_ccode(arm_vm, op->ccode)) {
		if (dbl)
			arm_vm->vpfregs.d[op->dest] =
			    sqrt(arm_vm->vpfregs.d[op->op1]);
		else
			arm_vm->vpfregs.f[op->dest] =
			    sqrt(arm_vm->vpfregs.f[op->op1]);
	}

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmrrd(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_tran_dbl_instr_t *op,
				  subtilis_error_t *err)
{
	uint32_t *dbl;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	dbl = (uint32_t *)&arm_vm->vpfregs.d[op->src1];
	arm_vm->regs[op->dest1] = dbl[0];
	arm_vm->regs[op->dest2] = dbl[1];

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmdrr(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_tran_dbl_instr_t *op,
				  subtilis_error_t *err)
{
	uint32_t *dbl;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	dbl = (uint32_t *)&arm_vm->vpfregs.d[op->dest1];
	dbl[0] = arm_vm->regs[op->src1];
	dbl[1] = arm_vm->regs[op->src2];

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmsrr(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_tran_dbl_instr_t *op,
				  subtilis_error_t *err)
{
	uint32_t *flt;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	flt = (uint32_t *)&arm_vm->vpfregs.f[op->dest1];
	*flt = arm_vm->regs[op->src1];
	flt = (uint32_t *)&arm_vm->vpfregs.f[op->dest1 + 1];
	*flt = arm_vm->regs[op->src2];

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fmrrs(subtilis_arm_vm_t *arm_vm,
				  subtilis_vfp_tran_dbl_instr_t *op,
				  subtilis_error_t *err)
{
	uint32_t *flt;

	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	flt = (uint32_t *)&arm_vm->vpfregs.f[op->src1];
	arm_vm->regs[op->dest1] = *flt;
	flt = (uint32_t *)&arm_vm->vpfregs.f[op->src1 + 1];
	arm_vm->regs[op->dest2] = *flt;

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fcvtds(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_cvt_instr_t *op,
				   subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	arm_vm->vpfregs.d[op->dest] = (double)arm_vm->vpfregs.f[op->op1];

	arm_vm->regs[15] += 4;
}

static void prv_process_vfp_fcvtsd(subtilis_arm_vm_t *arm_vm,
				   subtilis_vfp_cvt_instr_t *op,
				   subtilis_error_t *err)
{
	if (!prv_match_ccode(arm_vm, op->ccode)) {
		arm_vm->regs[15] += 4;
		return;
	}

	arm_vm->vpfregs.f[op->dest] = (float)arm_vm->vpfregs.d[op->op1];

	arm_vm->regs[15] += 4;
}

void subtilis_arm_vm_run(subtilis_arm_vm_t *arm_vm, subtilis_buffer_t *b,
			 subtilis_error_t *err)
{
	size_t pc;
	subtilis_arm_instr_t instr;

	arm_vm->fpa_status = 0;
	arm_vm->quit = false;
	arm_vm->negative_flag = false;
	arm_vm->zero_flag = false;
	arm_vm->carry_flag = false;
	arm_vm->overflow_flag = false;
	arm_vm->fpscr = 0;

	arm_vm->regs[15] = arm_vm->start_address + 8;

	pc = prv_calc_pc(arm_vm);
	while (!arm_vm->quit && pc < arm_vm->code_size) {
		subtilis_arm_disass(&instr, ((uint32_t *)arm_vm->memory)[pc],
				    arm_vm->vfp, err);
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
		case SUBTILIS_ARM_INSTR_TST:
			prv_process_tst(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_TEQ:
			prv_process_teq(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_ORR:
			prv_process_orr(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MOV:
			prv_process_mov(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_BIC:
			prv_process_bic(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MVN:
			prv_process_mvn(arm_vm, &instr.operands.data, err);
			break;
		case SUBTILIS_ARM_INSTR_MUL:
			prv_process_mul(arm_vm, &instr.operands.mul, err);
			break;
		case SUBTILIS_ARM_INSTR_MLA:
			prv_process_mla(arm_vm, &instr.operands.mul, err);
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
		case SUBTILIS_ARM_INSTR_MSR:
			prv_process_msr(arm_vm, &instr.operands.flags, err);
			break;
		case SUBTILIS_ARM_INSTR_MRS:
			prv_process_mrs(arm_vm, &instr.operands.flags, err);
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
		case SUBTILIS_FPA_INSTR_SIN:
			prv_process_fpa_sin(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_COS:
			prv_process_fpa_cos(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_TAN:
			prv_process_fpa_tan(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_ASN:
			prv_process_fpa_asn(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_ACS:
			prv_process_fpa_acs(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_ATN:
			prv_process_fpa_atn(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_SQT:
			prv_process_fpa_sqr(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_LOG:
			prv_process_fpa_log(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_LGN:
			prv_process_fpa_ln(arm_vm, &instr.operands.fpa_data,
					   err);
			break;
		case SUBTILIS_FPA_INSTR_EXP:
			prv_process_fpa_exp(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_ABS:
			prv_process_fpa_abs(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_WFS:
			prv_process_fpa_wfs(arm_vm, &instr.operands.fpa_cptran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_RFS:
			prv_process_fpa_rfs(arm_vm, &instr.operands.fpa_cptran,
					    err);
			break;
		case SUBTILIS_FPA_INSTR_POW:
			prv_process_fpa_pow(arm_vm, &instr.operands.fpa_data,
					    err);
			break;
		case SUBTILIS_VFP_INSTR_FSTS:
			prv_process_vfp_stf(arm_vm, &instr.operands.vfp_stran,
					    4, err);
			break;
		case SUBTILIS_VFP_INSTR_FLDS:
			prv_process_vfp_ldf(arm_vm, &instr.operands.vfp_stran,
					    4, err);
			break;
		case SUBTILIS_VFP_INSTR_FSTD:
			prv_process_vfp_stf(arm_vm, &instr.operands.vfp_stran,
					    8, err);
			break;
		case SUBTILIS_VFP_INSTR_FLDD:
			prv_process_vfp_ldf(arm_vm, &instr.operands.vfp_stran,
					    8, err);
			break;
		case SUBTILIS_VFP_INSTR_FCPYS:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fcpys, err);
			break;
		case SUBTILIS_VFP_INSTR_FCPYD:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fcpyd, err);
			break;
		case SUBTILIS_VFP_INSTR_FNEGS:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fnegs, err);
			break;
		case SUBTILIS_VFP_INSTR_FNEGD:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fnegd, err);
			break;
		case SUBTILIS_VFP_INSTR_FABSS:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fabss, err);
			break;
		case SUBTILIS_VFP_INSTR_FABSD:
			prv_process_vfp_copy(arm_vm, &instr.operands.vfp_copy,
					     prv_process_vfp_fabsd, err);
			break;
		case SUBTILIS_VFP_INSTR_FSITOD:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_fsitod, err);
			break;
		case SUBTILIS_VFP_INSTR_FSITOS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_fsitos, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOSIS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftosis, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOSID:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftosid, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOUIS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftouis, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOUID:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftouid, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOSIZS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftosizs, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOSIZD:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftosizd, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOUIZS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftouizs, err);
			break;
		case SUBTILIS_VFP_INSTR_FTOUIZD:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_ftouizd, err);
			break;
		case SUBTILIS_VFP_INSTR_FUITOD:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_fuitod, err);
			break;
		case SUBTILIS_VFP_INSTR_FUITOS:
			prv_process_vfp_tran(arm_vm, &instr.operands.vfp_tran,
					     prv_process_vfp_fuitos, err);
			break;
		case SUBTILIS_VFP_INSTR_FMSR:
			prv_process_vfp_fmsr(arm_vm, &instr.operands.vfp_cptran,
					     err);
			break;
		case SUBTILIS_VFP_INSTR_FMRS:
			prv_process_vfp_fmrs(arm_vm, &instr.operands.vfp_cptran,
					     err);
			break;
		case SUBTILIS_VFP_INSTR_FMACS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmacs, err);
			break;
		case SUBTILIS_VFP_INSTR_FMACD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmacd, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMACS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmacs, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMACD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmacd, err);
			break;
		case SUBTILIS_VFP_INSTR_FMSCS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmscs, err);
			break;
		case SUBTILIS_VFP_INSTR_FMSCD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmscd, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMSCS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmscs, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMSCD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmscd, err);
			break;
		case SUBTILIS_VFP_INSTR_FMULS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmuls, err);
			break;
		case SUBTILIS_VFP_INSTR_FMULD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fmuld, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMULS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmuls, err);
			break;
		case SUBTILIS_VFP_INSTR_FNMULD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fnmuld, err);
			break;
		case SUBTILIS_VFP_INSTR_FADDS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fadds, err);
			break;
		case SUBTILIS_VFP_INSTR_FADDD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_faddd, err);
			break;
		case SUBTILIS_VFP_INSTR_FSUBS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fsubs, err);
			break;
		case SUBTILIS_VFP_INSTR_FSUBD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fsubd, err);
			break;
		case SUBTILIS_VFP_INSTR_FDIVS:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fdivs, err);
			break;
		case SUBTILIS_VFP_INSTR_FDIVD:
			prv_process_vfp_data(arm_vm, &instr.operands.vfp_data,
					     prv_process_vfp_fdivd, err);
			break;
		case SUBTILIS_VFP_INSTR_FCMPS:
		case SUBTILIS_VFP_INSTR_FCMPES:
			prv_process_vfp_cmp_gen(arm_vm, false, false,
						&instr.operands.vfp_cmp, err);
			break;
		case SUBTILIS_VFP_INSTR_FCMPD:
		case SUBTILIS_VFP_INSTR_FCMPED:
			prv_process_vfp_cmp_gen(arm_vm, true, false,
						&instr.operands.vfp_cmp, err);
			break;
		case SUBTILIS_VFP_INSTR_FCMPZS:
		case SUBTILIS_VFP_INSTR_FCMPEZS:
			prv_process_vfp_cmp_gen(arm_vm, false, true,
						&instr.operands.vfp_cmp, err);
			break;
		case SUBTILIS_VFP_INSTR_FCMPZD:
		case SUBTILIS_VFP_INSTR_FCMPEZD:
			prv_process_vfp_cmp_gen(arm_vm, true, true,
						&instr.operands.vfp_cmp, err);
			break;
		case SUBTILIS_VFP_INSTR_FSQRTD:
			prv_process_vfp_sqrt(arm_vm, true,
					     &instr.operands.vfp_sqrt, err);
			break;
		case SUBTILIS_VFP_INSTR_FSQRTS:
			prv_process_vfp_sqrt(arm_vm, false,
					     &instr.operands.vfp_sqrt, err);

			break;
		case SUBTILIS_VFP_INSTR_FMXR:
			prv_process_vfp_fmxr(arm_vm, &instr.operands.vfp_sysreg,
					     err);
			break;
		case SUBTILIS_VFP_INSTR_FMRX:
			prv_process_vfp_fmrx(arm_vm, &instr.operands.vfp_sysreg,
					     err);
			break;
		case SUBTILIS_VFP_INSTR_FMDRR:
			prv_process_vfp_fmdrr(
			    arm_vm, &instr.operands.vfp_tran_dbl, err);
			break;
		case SUBTILIS_VFP_INSTR_FMRRD:
			prv_process_vfp_fmrrd(
			    arm_vm, &instr.operands.vfp_tran_dbl, err);
			break;
		case SUBTILIS_VFP_INSTR_FMSRR:
			prv_process_vfp_fmsrr(
			    arm_vm, &instr.operands.vfp_tran_dbl, err);
			break;
		case SUBTILIS_VFP_INSTR_FMRRS:
			prv_process_vfp_fmrrs(
			    arm_vm, &instr.operands.vfp_tran_dbl, err);
			break;
		case SUBTILIS_VFP_INSTR_FCVTDS:
			prv_process_vfp_fcvtds(arm_vm, &instr.operands.vfp_cvt,
					       err);
			break;
		case SUBTILIS_VFP_INSTR_FCVTSD:
			prv_process_vfp_fcvtsd(arm_vm, &instr.operands.vfp_cvt,
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
		 *4, true);
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
