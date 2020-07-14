/*
 * Copyright (c) 2020 Mark Ryan
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

#include "arm_heap.h"

const int32_t subtilis_arm_heap_min_slot_shift = 5;

/* clang-format off */
const int32_t subtilis_arm_heap_min_slot_size =
	1 << subtilis_arm_heap_min_slot_shift;

/* clang-format on */
const size_t subtilis_arm_heap_max_slot = 12; /* Index of final slot */
const size_t subtilis_arm_heap_max_slots = subtilis_arm_heap_max_slot + 1;

/*
 * We need to ensure that there's enough heap space available to
 * have an entry in the final slot.
 */

const uint32_t subtilis_arm_heap_min_size(void)
{
	return (1 << (subtilis_arm_heap_min_slot_shift +
		      subtilis_arm_heap_max_slot)) +
	       subtilis_arm_heap_max_slots * sizeof(int32_t);
}

/*
 * Let's give ourselves 13 slots.  If the smallest slot is
 * 32 bytes, allocations in slot 11 will be 1 << 16, which is
 * 64Kb.  Any allocations larger than this will come from slot
 * 12 which will always be fragmented.  We may want to increase
 * the number of slots on more modern hardware, perhaps even
 * on the RiscPC.  I guess it needs to be configurable on a
 * per platform and perhaps per project basis.
 * The heap will start with the 13 slots and then we'll have
 * the actual heap data.  Each heap block contains an 8 byte
 * header.  This header is invisible to the callers of the
 * heap API.  So a heap block looks like this.
 *
 *
 * | Block Size     | 0
 * | Next Block     | 4
 * | Data           | 8 to block size
 */

/*
 * The init code will be inlined into the preamble.  This is not a builtin
 * function.  There's not really much point in making it a function as it
 * will only be called once, at program startup.
 *
 * R1 is the start of the heap
 * R3 is the heap_size, which  must be a multiple of
 * subtilis_arm_heap_min_slot_size
 *
 * This code will be inserted into the preamble where no register allocation
 * takes place so we need to use fixed registers in this code.
 */

void subtilis_arm_heap_init(subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	size_t loop_label;
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	const subtilis_arm_reg_t heap_start = 1;
	const subtilis_arm_reg_t slots_counter = 2;
	const subtilis_arm_reg_t heap_size = 3;
	const subtilis_arm_reg_t zero = 4;

	loop_label = arm_s->label_counter++;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 slots_counter, subtilis_arm_heap_max_slot,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, zero, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Loop to zero out the first subtilis_arm_heap_max_slot slots.
	 * When the heap is first initialised there's only one block in
	 * the final slot that holds all the data.
	 */

	subtilis_arm_section_add_label(arm_s, loop_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 slots_counter, slots_counter, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = zero;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = slots_counter;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, slots_counter, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->target.label = loop_label;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 heap_start, heap_start,
				 subtilis_arm_heap_max_slots * 4, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, heap_size,
				 heap_size, subtilis_arm_heap_max_slots * 4,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * The size of the first and only slot must be divisible by the
	 * size of the smallest slot, currently 32 bytes.
	 */

	subtilis_arm_add_data_imm(
	    arm_s, SUBTILIS_ARM_INSTR_BIC, SUBTILIS_ARM_CCODE_AL, false,
	    heap_size, heap_size, (1 << subtilis_arm_heap_min_slot_shift) - 1,
	    err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Initialise heap with on entry in the final slot.
	 */

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = heap_start;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = true;
	stran->byte = false;

	/*
	 * Set up one and only block.  Start by storing its size
	 */

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = heap_size;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 0;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	/*
	 * Zero out the next pointer.
	 */

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = zero;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;
}

/*
 * Returns the slot number for allocation size value, in the register ret.
 * scratch is used as workspace and is corrupted.
 */

static void prv_get_slot(subtilis_arm_section_t *arm_s,
			 subtilis_arm_reg_t value, subtilis_arm_reg_t ret,
			 subtilis_arm_reg_t scratch, subtilis_error_t *err)
{
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_instr_t *instr;
	int32_t i;
	int32_t masks[5] = {0, 0x2, 0xc, 0xf0, 0xff00};

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, ret, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, value,
				 value, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch,
				 0xff000000, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
				  SUBTILIS_ARM_CCODE_AL, false, scratch,
				  scratch, 0xff0000, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_TST,
			     SUBTILIS_ARM_CCODE_AL, value, scratch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 4; i >= 0; --i) {
		instr = subtilis_arm_section_add_instr(
		    arm_s, SUBTILIS_ARM_INSTR_MOV, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		datai = &instr->operands.data;
		datai->ccode = SUBTILIS_ARM_CCODE_NE;
		datai->status = false;
		datai->dest = value;
		datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
		datai->op2.op.shift.shift_reg = false;
		datai->op2.op.shift.reg = value;
		datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
		datai->op2.op.shift.shift.integer = 1 << i;

		subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_ORR,
					  SUBTILIS_ARM_CCODE_NE, false, ret,
					  ret, 1 << i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (i == 0)
			break;

		subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TST,
					 SUBTILIS_ARM_CCODE_AL, value, masks[i],
					 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, ret, ret,
				 subtilis_arm_heap_min_slot_shift - 1, err);
}

static void prv_slot12_alloc(subtilis_arm_section_t *arm_s, size_t good_label,
			     size_t bad_label, size_t exact_slot_size_label,
			     subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	const subtilis_arm_reg_t ret_val = 0;
	subtilis_arm_data_instr_t *datai;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t segment = 1;
	const subtilis_arm_reg_t first_entry = 3;
	const subtilis_arm_reg_t next_ptr = 4;
	const subtilis_arm_reg_t block_size = 5;
	const subtilis_arm_reg_t scratch2 = 6;
	const subtilis_arm_reg_t split_block_size = 7;
	const subtilis_arm_reg_t split_slot = 8;
	const subtilis_arm_reg_t scratch3 = 9;
	const subtilis_arm_reg_t scratch = 10;
	const subtilis_arm_reg_t max_size_of_block = 10;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, next_ptr, first_entry,
				   4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, scratch2, first_entry,
				   0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = split_block_size;
	datai->op1 = scratch2;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = block_size;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, next_ptr, heap_start,
				   subtilis_arm_heap_max_slot * sizeof(int32_t),
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, split_block_size, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_EQ, false, ret_val,
				 first_entry, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = good_label;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch,
				 split_block_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = segment;
	datai->op1 = first_entry;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = block_size;

	prv_get_slot(arm_s, scratch, split_slot, scratch3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, split_slot,
				 subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_GT, false,
				 split_slot, subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->target.label = exact_slot_size_label;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch3,
				 subtilis_arm_heap_min_slot_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = scratch;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = true;
	datai->op2.op.shift.reg = scratch3;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	datai->op2.op.shift.shift.reg = split_slot;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, scratch, split_block_size,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = exact_slot_size_label;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 split_slot, split_slot, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = max_size_of_block;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = false;
	datai->op2.op.shift.reg = max_size_of_block;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	datai->op2.op.shift.shift.integer = 1;
}

static void prv_split_slot12_block(subtilis_arm_section_t *arm_s,
				   size_t split_slot12_block_label,
				   size_t exact_slot_size_label,
				   subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t segment = 1;
	const subtilis_arm_reg_t ptr = 4;
	const subtilis_arm_reg_t split_block_size = 7;
	const subtilis_arm_reg_t split_slot = 8;
	const subtilis_arm_reg_t max_size_of_block = 10;

	size_t find_next_slot_label = arm_s->label_counter++;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = ptr;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = split_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, max_size_of_block,
				   segment, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, ptr, segment, 4,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = segment;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = split_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = segment;
	datai->op1 = segment;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = max_size_of_block;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_SUB, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = split_block_size;
	datai->op1 = split_block_size;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = max_size_of_block;

	subtilis_arm_section_add_label(arm_s, find_next_slot_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 split_slot, split_slot, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = max_size_of_block;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = false;
	datai->op2.op.shift.reg = max_size_of_block;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
	datai->op2.op.shift.shift.integer = 1;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, split_block_size,
			     max_size_of_block, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = exact_slot_size_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_LT;
	br->link = false;
	br->target.label = find_next_slot_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->target.label = split_slot12_block_label;
}

static void prv_exact_slot_size(subtilis_arm_section_t *arm_s,
				size_t good_label, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t ret_val = 0;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t split_block_start = 1;
	const subtilis_arm_reg_t first_entry = 3;
	const subtilis_arm_reg_t slot_start = 4;
	const subtilis_arm_reg_t block_size = 5;
	const subtilis_arm_reg_t split_block_size = 7;
	const subtilis_arm_reg_t split_slot = 8;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, block_size,
				   first_entry, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, split_block_size,
				   split_block_start, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = slot_start;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = split_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, slot_start,
				   split_block_start, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = split_block_start;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = split_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, ret_val,
				 first_entry, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->target.label = good_label;
}

static void prv_big_alloc(subtilis_arm_section_t *arm_s, size_t good_label,
			  size_t bad_label, size_t slot12_alloc_label,
			  size_t move_block_to_start_label,
			  subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t requested_size = 1;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t last_slot = 2;
	const subtilis_arm_reg_t first_entry = 3;
	const subtilis_arm_reg_t size_of_block = 4;
	const subtilis_arm_reg_t block_size = 5;

	size_t big_alloc_loop_label = arm_s->label_counter++;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, last_slot,
				 subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = first_entry;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = last_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, first_entry, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = bad_label;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, size_of_block,
				   first_entry, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, size_of_block,
			     requested_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_GE, false,
				 block_size, requested_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->target.label = slot12_alloc_label;

	subtilis_arm_section_add_label(arm_s, big_alloc_loop_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, block_size,
				   first_entry, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, block_size, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = bad_label;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, size_of_block,
				   block_size, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, size_of_block,
			     requested_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->target.label = move_block_to_start_label;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 first_entry, block_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->target.label = big_alloc_loop_label;
}

static void prv_move_block_to_start(subtilis_arm_section_t *arm_s,
				    size_t good_label, size_t bad_label,
				    size_t slot12_alloc_label,
				    subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t requested_size = 1;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t last_slot = 2;
	const subtilis_arm_reg_t previous_block = 3;
	const subtilis_arm_reg_t block_to_move = 5;
	const subtilis_arm_reg_t next_ptr = 6;
	const subtilis_arm_reg_t first_block = 7;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, next_ptr,
				   block_to_move, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = first_block;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = last_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, next_ptr,
				   previous_block, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, first_block,
				   block_to_move, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = block_to_move;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = last_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 previous_block, block_to_move, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 block_to_move, requested_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_AL;
	br->link = false;
	br->target.label = slot12_alloc_label;
}

static void prv_search_slots(subtilis_arm_section_t *arm_s, size_t good_label,
			     size_t bad_label, subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t next_ptr = 1;
	const subtilis_arm_reg_t desired_slot = 2;
	const subtilis_arm_reg_t non_empty_slot = 3;
	const subtilis_arm_reg_t next_slot = 4;

	size_t search_slots_loop_label = arm_s->label_counter++;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, next_slot,
				 desired_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, search_slots_loop_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, next_slot,
				 next_slot, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, next_slot,
				 subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = bad_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = non_empty_slot;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = next_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, non_empty_slot, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = search_slots_loop_label;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, next_ptr,
				   non_empty_slot, 4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = next_ptr;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = next_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;
}

static void prv_split_small_block(subtilis_arm_section_t *arm_s,
				  size_t good_label, subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t ret_val = 0;
	const subtilis_arm_reg_t slot_number = 2;
	const subtilis_arm_reg_t slot_ptr = 3;
	const subtilis_arm_reg_t next_slot = 4;
	const subtilis_arm_reg_t block_size = 5;
	const subtilis_arm_reg_t min_slot_size = 8;
	const subtilis_arm_reg_t zero = 9;

	size_t split_up_block_label = arm_s->label_counter++;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 min_slot_size, subtilis_arm_heap_min_slot_size,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, zero, 0,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, split_up_block_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, next_slot,
				 next_slot, 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = block_size;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = true;
	datai->op2.op.shift.reg = min_slot_size;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	datai->op2.op.shift.shift.reg = next_slot;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, block_size, slot_ptr,
				   0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = slot_ptr;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = next_slot;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, zero, slot_ptr, 4,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_ADD, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = slot_ptr;
	datai->op1 = slot_ptr;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = block_size;

	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_AL, next_slot, slot_number,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GT;
	br->link = false;
	br->target.label = split_up_block_label;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, block_size, slot_ptr,
				   0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, ret_val,
				 slot_ptr, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* We fall through to good label here. */
}

/*
 * Another inline function to be inserted in the backends heap allocation code.
 * On entry:
 * R0 - start of heap
 * R1 - size of block to allocate
 *
 * If this function detects an error it generates a branch to bad_label.  If
 * it is successful it generates a branch to good_label.
 */

void subtilis_arm_heap_alloc(subtilis_arm_section_t *arm_s, size_t good_label,
			     size_t bad_label, subtilis_error_t *err)
{
	subtilis_arm_br_instr_t *br;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_stran_instr_t *stran;
	const subtilis_arm_reg_t heap_start = 0;
	const subtilis_arm_reg_t ret_val = 0;
	const subtilis_arm_reg_t requested_size = 1;
	const subtilis_arm_reg_t slot_number = 2;
	const subtilis_arm_reg_t scratch1 = 3;
	const subtilis_arm_reg_t first_entry = 3;
	const subtilis_arm_reg_t next_ptr = 4;
	const subtilis_arm_reg_t block_size = 5;
	const subtilis_arm_reg_t scratch2 = 6;
	const subtilis_arm_reg_t scratch = 10;

	size_t big_alloc_label = arm_s->label_counter++;
	size_t search_slots_label = arm_s->label_counter++;
	size_t slot12_alloc_label = arm_s->label_counter++;
	size_t exact_slot_size_label = arm_s->label_counter++;
	size_t split_slot12_block_label = arm_s->label_counter++;
	size_t move_block_to_start_label = arm_s->label_counter++;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 requested_size, requested_size, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_TST,
				 SUBTILIS_ARM_CCODE_AL, requested_size,
				 subtilis_arm_heap_min_slot_size - 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_data_imm(arm_s, SUBTILIS_ARM_INSTR_BIC,
				  SUBTILIS_ARM_CCODE_NE, false, requested_size,
				  requested_size,
				  subtilis_arm_heap_min_slot_size - 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_NE, false,
				 requested_size, requested_size,
				 subtilis_arm_heap_min_slot_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch,
				 requested_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_get_slot(arm_s, scratch, slot_number, scratch1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, slot_number,
				 subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_GE;
	br->link = false;
	br->target.label = big_alloc_label;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = first_entry;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = slot_number;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, first_entry, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_NE, next_ptr, first_entry,
				   4, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_NE;
	stran->dest = next_ptr;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = slot_number;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_NE, false, ret_val,
				 first_entry, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_NE;
	br->link = false;
	br->target.label = good_label;

	subtilis_arm_add_stran_imm(
	    arm_s, SUBTILIS_ARM_INSTR_LDR, SUBTILIS_ARM_CCODE_AL, first_entry,
	    heap_start, subtilis_arm_heap_max_slot * sizeof(int32_t), false,
	    err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, first_entry, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	br = &instr->operands.br;
	br->ccode = SUBTILIS_ARM_CCODE_EQ;
	br->link = false;
	br->target.label = search_slots_label;

	subtilis_arm_add_add_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false,
				 block_size, slot_number,
				 subtilis_arm_heap_min_slot_shift, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, scratch2,
				 1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.data;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->status = false;
	datai->dest = block_size;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.shift_reg = true;
	datai->op2.op.shift.reg = scratch2;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	datai->op2.op.shift.shift.reg = block_size;

	subtilis_arm_section_add_label(arm_s, slot12_alloc_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_slot12_alloc(arm_s, good_label, bad_label, exact_slot_size_label,
			 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, split_slot12_block_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_split_slot12_block(arm_s, split_slot12_block_label,
			       exact_slot_size_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, exact_slot_size_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_exact_slot_size(arm_s, good_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, big_alloc_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_big_alloc(arm_s, good_label, bad_label, slot12_alloc_label,
		      move_block_to_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, move_block_to_start_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_move_block_to_start(arm_s, good_label, bad_label,
				slot12_alloc_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, search_slots_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_search_slots(arm_s, good_label, bad_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_split_small_block(arm_s, good_label, err);
}

void subtilis_arm_heap_free(subtilis_arm_section_t *arm_s,
			    subtilis_arm_reg_t heap_start,
			    subtilis_arm_reg_t block, subtilis_error_t *err)
{
	subtilis_arm_stran_instr_t *stran;
	subtilis_arm_instr_t *instr;
	const subtilis_arm_reg_t block_size = 3;
	const subtilis_arm_reg_t ptr = 4;
	const subtilis_arm_reg_t slot_number = 8;
	const subtilis_arm_reg_t scratch = 9;

	subtilis_arm_add_sub_imm(arm_s, SUBTILIS_ARM_CCODE_AL, false, block,
				 block, 8, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_AL, block_size, block, 0,
				   false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_get_slot(arm_s, block_size, slot_number, scratch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_cmp_imm(arm_s, SUBTILIS_ARM_INSTR_CMP,
				 SUBTILIS_ARM_CCODE_AL, slot_number,
				 subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_mov_imm(arm_s, SUBTILIS_ARM_CCODE_GT, false,
				 slot_number, subtilis_arm_heap_max_slot, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = ptr;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = slot_number;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;

	stran->ccode = SUBTILIS_ARM_CCODE_AL;
	stran->dest = block;
	stran->base = heap_start;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.reg = slot_number;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.shift_reg = false;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;

	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_STR,
				   SUBTILIS_ARM_CCODE_AL, ptr, block, 4, false,
				   err);
}
