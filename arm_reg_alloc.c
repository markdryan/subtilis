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

#include <limits.h>
#include <stdlib.h>

#include "arm_fpa_dist.h"
#include "arm_int_dist.h"
#include "arm_reg_alloc.h"

#define SUBTILIS_ARM_REG_MIN_INT_REGS 4
#define SUBTILIS_ARM_REG_MAX_INT_REGS 11

#define SUBTILIS_ARM_REG_MAX_FPA_REGS 6

/* clang-format off */
typedef void (*subtilis_arm_reg_spill_imm_t)(subtilis_arm_section_t *s,
					     subtilis_arm_op_t *current,
					     subtilis_arm_instr_type_t itype,
					     subtilis_arm_ccode_type_t ccode,
					     subtilis_arm_reg_t dest,
					     subtilis_arm_reg_t base,
					     subtilis_arm_reg_t spill_reg,
					     int32_t offset,
					     subtilis_error_t *err);
typedef void (*subtilis_arm_reg_stran_imm_t)(subtilis_arm_section_t *s,
					     subtilis_arm_op_t *current,
					     subtilis_arm_instr_type_t itype,
					     subtilis_arm_ccode_type_t ccode,
					     subtilis_arm_reg_t dest,
					     subtilis_arm_reg_t base,
					     int32_t offset,
					     subtilis_error_t *err);

/* clang-format on */

struct subtilis_arm_reg_class_t_ {
	size_t max_regs;
	size_t *phys_to_virt;
	int *next;
	size_t vr_reg_count;
	int32_t *spilt_regs;
	size_t *spill_stack;
	size_t spill_top;
	size_t spill_max;
	size_t reg_size;
	int32_t max_offset;
	subtilis_arm_reg_spill_imm_t spill_imm;
	subtilis_arm_reg_stran_imm_t stran_imm;
};

typedef struct subtilis_arm_reg_class_t_ subtilis_arm_reg_class_t;

struct subtilis_arm_reg_ud_t_ {
	subtilis_arm_reg_class_t *int_regs;
	subtilis_arm_reg_class_t *fpa_regs;
	subtilis_arm_section_t *arm_s;
	size_t instr_count;
	subtlis_arm_walker_t int_dist_walker;
	subtlis_arm_walker_t fpa_dist_walker;
	subtilis_dist_data_t dist_data;
};

typedef struct subtilis_arm_reg_ud_t_ subtilis_arm_reg_ud_t;

static subtilis_arm_reg_class_t *
prv_new_regs(subtilis_arm_section_t *arm_s, size_t max_regs, size_t reg_start,
	     size_t reg_size, size_t args, int32_t max_offset,
	     subtilis_arm_reg_spill_imm_t spill_imm,
	     subtilis_arm_reg_stran_imm_t stran_imm, subtilis_error_t *err)
{
	size_t i;
	size_t index;
	subtilis_arm_reg_class_t *regs;
	size_t reg_count = arm_s->reg_counter;
	size_t reg_args = 4;

	regs = calloc(sizeof(*regs), 1);
	if (!regs) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	regs->phys_to_virt = malloc(sizeof(*regs->phys_to_virt) * max_regs);
	if (!regs->phys_to_virt) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	regs->next = malloc(sizeof(*regs->next) * max_regs);
	if (!regs->next) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	if (args <= reg_args) {
		reg_args = args;
		regs->spill_top = 0;
	} else {
		regs->spill_top = args - reg_args;
	}

	regs->spilt_regs = malloc(reg_count * sizeof(int32_t));
	if (!regs->spilt_regs) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	regs->spill_stack = malloc(reg_count * sizeof(size_t));
	if (!regs->spill_stack) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	regs->max_regs = max_regs;
	regs->spill_max = regs->spill_top;
	regs->reg_size = reg_size;
	regs->spill_imm = spill_imm;
	regs->stran_imm = stran_imm;
	regs->max_offset = max_offset;

	for (i = 0; i < reg_count; i++) {
		regs->spilt_regs[i] = -1;
		regs->spill_stack[i] = i * reg_size;
	}
	regs->vr_reg_count = reg_count;

	for (i = 0; i < reg_args; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = i + reg_start;
	}

	for (; i < max_regs; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = INT_MAX;
	}

	for (i = reg_args; i < args; i++) {
		index = i - reg_args;
		regs->spilt_regs[i + reg_start] = index * reg_size;
	}

	return regs;

on_error:

	free(regs->spilt_regs);
	free(regs->next);
	free(regs->phys_to_virt);
	free(regs);

	return NULL;
}

static void prv_free_regs(subtilis_arm_reg_class_t *regs)
{
	if (regs) {
		free(regs->spill_stack);
		free(regs->spilt_regs);
		free(regs->next);
		free(regs->phys_to_virt);
		free(regs);
	}
}

static void prv_init_arm_reg_ud(subtilis_arm_reg_ud_t *ud,
				subtilis_arm_section_t *arm_s,
				subtilis_error_t *err)
{
	ud->int_regs = prv_new_regs(arm_s, SUBTILIS_ARM_REG_MAX_INT_REGS,
				    SUBTILIS_IR_REG_TEMP_START, sizeof(int32_t),
				    arm_s->stype->int_regs, 4095,
				    subtilis_arm_insert_stran_spill_imm,
				    subtilis_arm_insert_stran_imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->fpa_regs = prv_new_regs(arm_s, SUBTILIS_ARM_REG_MAX_FPA_REGS, 0,
				    sizeof(double), arm_s->stype->fp_regs, 1023,
				    subtilis_fpa_insert_stran_spill_imm,
				    subtilis_fpa_insert_stran_imm, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		prv_free_regs(ud->int_regs);
		return;
	}

	ud->instr_count = 0;
	ud->arm_s = arm_s;

	subtilis_init_int_dist_walker(&ud->int_dist_walker, &ud->dist_data);
	subtilis_init_fpa_dist_walker(&ud->fpa_dist_walker, &ud->dist_data);
}

static int prv_calculate_dist(subtilis_arm_reg_ud_t *ud, size_t reg_num,
			      subtilis_arm_op_t *op,
			      subtlis_arm_walker_t *walker)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	if (op->next == SIZE_MAX)
		return -1;

	op = &ud->arm_s->op_pool->ops[op->next];

	ud->dist_data.reg_num = reg_num;
	ud->dist_data.last_used = ud->instr_count + 1;

	subtilis_arm_walk_from(ud->arm_s, walker, op, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		return ud->dist_data.last_used;
	else
		return -1;
}

static void prv_free_arm_reg_ud(subtilis_arm_reg_ud_t *ud)
{
	if (!ud)
		return;

	prv_free_regs(ud->fpa_regs);
	prv_free_regs(ud->int_regs);
}

static size_t prv_virt_to_phys(subtilis_arm_reg_class_t *regs,
			       subtilis_arm_reg_t *reg)
{
	size_t i;
	size_t retval = INT_MAX;

	for (i = 0; i < regs->max_regs; i++) {
		if (regs->phys_to_virt[i] == reg->num)
			return i;
	}

	return retval;
}

static void prv_load_spilled_reg(subtilis_arm_section_t *arm_s,
				 subtilis_arm_op_t *current,
				 subtilis_arm_reg_class_t *regs,
				 subtilis_arm_reg_t virt,
				 subtilis_arm_reg_t phys, subtilis_error_t *err)
{
	subtilis_arm_reg_t base;
	int32_t offset = regs->spilt_regs[virt.num];

	if (offset == INT_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (regs->spill_top == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	offset += arm_s->locals;

	base.type = SUBTILIS_ARM_REG_FIXED;
	base.num = 11;
	if (offset > regs->max_offset || offset < -regs->max_offset) {
		regs->spill_imm(arm_s, current, SUBTILIS_ARM_INSTR_LDR,
				SUBTILIS_ARM_CCODE_AL, phys, base, phys, offset,
				err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		regs->stran_imm(arm_s, current, SUBTILIS_ARM_INSTR_LDR,
				SUBTILIS_ARM_CCODE_AL, phys, base, offset, err);
	}

	/* TODO need to update next */

	regs->phys_to_virt[phys.num] = virt.num;
	regs->spilt_regs[virt.num] = INT_MAX;
	regs->spill_top--;
	regs->spill_stack[regs->spill_top] = offset;
	regs->next[phys.num] = -1;
}

static void prv_spill_reg(subtilis_arm_section_t *arm_s,
			  subtilis_arm_op_t *current, size_t assigned,
			  subtilis_arm_reg_class_t *int_regs,
			  subtilis_arm_reg_class_t *regs,
			  subtilis_arm_reg_t reg, subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_reg_t spill_reg;
	int32_t offset;
	subtilis_arm_reg_t base;

	if (regs->spill_top == regs->vr_reg_count) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	offset = (int32_t)regs->spill_stack[regs->spill_top++] + arm_s->locals;
	if (regs->spill_max < regs->spill_top)
		regs->spill_max = regs->spill_top;

	regs->spilt_regs[assigned] = offset;

	base.type = SUBTILIS_ARM_REG_FIXED;
	base.num = 11;
	if (offset > regs->max_offset || offset < -regs->max_offset) {
		/*
		 * We need to find an integer register to act as our base
		 * when spilling the contents of reg_num.  So if we're
		 * currently spilling the contents of a floating point
		 * register we still need to use the int_regs structure
		 * to use as our base.
		 */

		for (i = 0; i < int_regs->max_regs; i++)
			if ((int_regs->phys_to_virt[i] == INT_MAX) &&
			    (int_regs != regs || i != reg.num))
				break;

		spill_reg.type = SUBTILIS_ARM_REG_FIXED;
		if (i == int_regs->max_regs) {
			subtilis_arm_insert_push(arm_s, current,
						 SUBTILIS_ARM_CCODE_AL, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			spill_reg.num = 0;
		} else {
			spill_reg.num = i;
		}
		regs->spill_imm(arm_s, current, SUBTILIS_ARM_INSTR_STR,
				SUBTILIS_ARM_CCODE_AL, reg, base, spill_reg,
				offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (i == int_regs->max_regs) {
			subtilis_arm_insert_pop(arm_s, current,
						SUBTILIS_ARM_CCODE_AL, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} else {
		regs->stran_imm(arm_s, current, SUBTILIS_ARM_INSTR_STR,
				SUBTILIS_ARM_CCODE_AL, reg, base, offset, err);
	}

	/* TODO need to update next */
}

static void prv_allocate_fixed(subtilis_arm_section_t *arm_s,
			       subtilis_arm_op_t *current,
			       subtilis_arm_reg_class_t *int_regs,
			       subtilis_arm_reg_class_t *regs,
			       subtilis_arm_reg_t *reg, subtilis_error_t *err)
{
	size_t assigned;

	assigned = regs->phys_to_virt[reg->num];
	if (assigned != INT_MAX) {
		prv_spill_reg(arm_s, current, assigned, int_regs, regs, *reg,
			      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_allocate_floating(subtilis_arm_section_t *arm_s,
				  subtilis_arm_op_t *current,
				  subtilis_arm_reg_class_t *int_regs,
				  subtilis_arm_reg_class_t *regs,
				  subtilis_arm_reg_t *reg,
				  subtilis_error_t *err)
{
	subtilis_arm_reg_t target_reg;
	int i;
	size_t next = regs->max_regs;
	size_t max_next = -1;

	/* Virtual register is not already assigned. */

	for (i = regs->max_regs - 1; i >= 0; i--)
		if (regs->phys_to_virt[i] == INT_MAX)
			break;

	target_reg.type = SUBTILIS_ARM_REG_FIXED;
	if (i >= 0) {
		target_reg.num = (size_t)i;
	} else {
		/* No free physical regs.  Need to spill. */

		for (i = 0; i < regs->max_regs; i++)
			if (regs->next[i] > max_next) {
				max_next = regs->next[i];
				next = i;
			}
		if (next == regs->max_regs) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		target_reg.num = next;
		prv_spill_reg(arm_s, current, next, int_regs, regs, target_reg,
			      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	*reg = target_reg;
}

static void prv_allocate(subtilis_arm_section_t *arm_s,
			 subtilis_arm_op_t *current,
			 subtilis_arm_reg_class_t *int_regs,
			 subtilis_arm_reg_class_t *regs,
			 subtilis_arm_reg_t *reg, subtilis_error_t *err)
{
	size_t virt_num = reg->num;

	if (reg->type == SUBTILIS_ARM_REG_FIXED) {
		if (reg->num >= regs->max_regs)
			return;
		prv_allocate_fixed(arm_s, current, int_regs, regs, reg, err);
	} else {
		prv_allocate_floating(arm_s, current, int_regs, regs, reg, err);
	}

	regs->phys_to_virt[reg->num] = virt_num;
}

/* Returns true if register is of fixed use, e.g., R13. */

static bool prv_ensure(subtilis_arm_section_t *arm_s,
		       subtilis_arm_op_t *current,
		       subtilis_arm_reg_class_t *int_regs,
		       subtilis_arm_reg_class_t *regs, subtilis_arm_reg_t *reg,
		       subtilis_error_t *err)
{
	size_t assigned;
	subtilis_arm_reg_t target_reg;

	if (reg->type == SUBTILIS_ARM_REG_FIXED) {
		/*
		 * Register has fixed use and is unavailable to user's code
		 * Physical register has already been allocated, e.g., r12, r13
		 */
		if (reg->num >= regs->max_regs)
			return true;

		assigned = regs->phys_to_virt[reg->num];
		if (assigned != reg->num) {
			prv_allocate_fixed(arm_s, current, int_regs, regs, reg,
					   err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(arm_s, current, regs, *reg, *reg,
					     err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
		}
	} else {
		assigned = prv_virt_to_phys(regs, reg);
		if (assigned != INT_MAX) {
			reg->num = assigned;
			reg->type = SUBTILIS_ARM_REG_FIXED;
		} else {
			prv_allocate_floating(arm_s, current, int_regs, regs,
					      &target_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(arm_s, current, regs, *reg,
					     target_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			*reg = target_reg;
		}
	}

	return false;
}

static void prv_alloc_label(void *user_data, subtilis_arm_op_t *op,
			    size_t label, subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	ud->instr_count++;
}

static subtilis_arm_reg_t *prv_ensure_op2_reg(subtilis_arm_reg_ud_t *ud,
					      subtilis_arm_op_t *op,
					      int *dist_op2,
					      subtilis_arm_reg_t *reg,
					      subtilis_error_t *err)
{
	size_t vreg_op2;
	bool fixed_reg;

	vreg_op2 = reg->num;
	fixed_reg =
	    prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs, reg, err);
	if (fixed_reg || (err->type != SUBTILIS_ERROR_OK))
		return NULL;

	*dist_op2 = prv_calculate_dist(ud, vreg_op2, op, &ud->int_dist_walker);
	if (*dist_op2 == -1)
		ud->int_regs->phys_to_virt[reg->num] = INT_MAX;

	return reg;
}

static subtilis_arm_reg_t *prv_ensure_op2(subtilis_arm_reg_ud_t *ud,
					  subtilis_arm_op_t *op,
					  subtilis_arm_op2_t *op2,
					  int *dist_op2, subtilis_error_t *err)
{
	if (op2->type == SUBTILIS_ARM_OP2_I32)
		return NULL;

	if (op2->type == SUBTILIS_ARM_OP2_REG)
		return prv_ensure_op2_reg(ud, op, dist_op2, &op2->op.reg, err);
	else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED)
		return prv_ensure_op2_reg(ud, op, dist_op2, &op2->op.shift.reg,
					  err);

	return NULL;
}

static subtilis_arm_reg_t *prv_ensure_op2_shift_reg(subtilis_arm_reg_ud_t *ud,
						    subtilis_arm_op_t *op,
						    subtilis_arm_op2_t *op2,
						    int *dist_op2,
						    subtilis_error_t *err)
{
	if ((op2->type != SUBTILIS_ARM_OP2_SHIFTED) || !op2->op.shift.shift_reg)
		return NULL;

	return prv_ensure_op2_reg(ud, op, dist_op2, &op2->op.shift.shift.reg,
				  err);
}

static void prv_allocate_dest(subtilis_arm_reg_ud_t *ud, subtilis_arm_op_t *op,
			      subtilis_arm_reg_t *dest, subtilis_error_t *err)
{
	int dist_dest;
	size_t vreg_dest = dest->num;

	if ((dest->type == SUBTILIS_ARM_REG_FIXED) &&
	    (vreg_dest >= SUBTILIS_ARM_REG_MAX_INT_REGS))
		return;

	prv_allocate(ud->arm_s, op, ud->int_regs, ud->int_regs, dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_dest = prv_calculate_dist(ud, vreg_dest, op, &ud->int_dist_walker);
	if (dist_dest == -1)
		ud->int_regs->phys_to_virt[dest->num] = INT_MAX;
	ud->int_regs->next[dest->num] = dist_dest;
}

static void prv_allocate_fpa_dest(subtilis_arm_reg_ud_t *ud,
				  subtilis_arm_op_t *op,
				  subtilis_arm_reg_t *dest,
				  subtilis_error_t *err)
{
	int dist_dest;
	size_t vreg_dest = dest->num;

	prv_allocate(ud->arm_s, op, ud->int_regs, ud->fpa_regs, dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dist_dest = prv_calculate_dist(ud, vreg_dest, op, &ud->fpa_dist_walker);
	if (dist_dest == -1)
		ud->fpa_regs->phys_to_virt[dest->num] = INT_MAX;
	ud->fpa_regs->next[dest->num] = dist_dest;
}

/*
 * TWO ISSUES TO FIX
 *
 * 1. Spill doesn't seem to work properly
 * 2. Will inserting new registers mess up our distance count, probably?
 */

static void prv_alloc_mov_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	int dist_op2;
	int dist_op2_shift;
	subtilis_arm_reg_t *reg;
	subtilis_arm_reg_t *shift_reg;
	subtilis_arm_reg_ud_t *ud = user_data;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	shift_reg =
	    prv_ensure_op2_shift_reg(ud, op, &instr->op2, &dist_op2_shift, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (reg)
		ud->int_regs->next[reg->num] = dist_op2;

	if (shift_reg)
		ud->int_regs->next[shift_reg->num] = dist_op2_shift;

	ud->instr_count++;
}

static void prv_alloc_data_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	int dist_op1;
	int dist_op2;
	int dist_op2_shift;
	size_t vreg_op1;
	bool fixed_reg_op1;
	subtilis_arm_reg_ud_t *ud = user_data;
	subtilis_arm_reg_t *reg = NULL;
	subtilis_arm_reg_t *shift_reg = NULL;

	vreg_op1 = instr->op1.num;
	fixed_reg_op1 = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				   &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	shift_reg =
	    prv_ensure_op2_shift_reg(ud, op, &instr->op2, &dist_op2_shift, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1) {
		dist_op1 =
		    prv_calculate_dist(ud, vreg_op1, op, &ud->int_dist_walker);
		if (dist_op1 == -1)
			ud->int_regs->phys_to_virt[instr->op1.num] = INT_MAX;
	}

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1)
		ud->int_regs->next[instr->op1.num] = dist_op1;
	if (reg)
		ud->int_regs->next[reg->num] = dist_op2;
	if (shift_reg)
		ud->int_regs->next[shift_reg->num] = dist_op2_shift;

	ud->instr_count++;
}

static void prv_alloc_mul_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_mul_instr_t *instr,
				subtilis_error_t *err)
{
	int dist_rm;
	int dist_rs;
	size_t vreg_rm;
	bool fixed_reg_rm;
	size_t vreg_rs;
	bool fixed_reg_rs;
	subtilis_arm_reg_ud_t *ud = user_data;

	vreg_rm = instr->rm.num;
	fixed_reg_rm = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				  &instr->rm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs = instr->rs.num;
	fixed_reg_rs = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				  &instr->rs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rm) {
		dist_rm =
		    prv_calculate_dist(ud, vreg_rm, op, &ud->int_dist_walker);
		if (dist_rm == -1)
			ud->int_regs->phys_to_virt[instr->rm.num] = INT_MAX;
	}

	if (!fixed_reg_rs) {
		dist_rs =
		    prv_calculate_dist(ud, vreg_rs, op, &ud->int_dist_walker);
		if (dist_rs == -1)
			ud->int_regs->phys_to_virt[instr->rs.num] = INT_MAX;
	}

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rm)
		ud->int_regs->next[instr->rm.num] = dist_rm;
	if (!fixed_reg_rs)
		ud->int_regs->next[instr->rs.num] = dist_rs;

	ud->instr_count++;
}

static void prv_alloc_stran_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_stran_instr_t *instr,
				  subtilis_error_t *err)
{
	int dist_dest;
	int dist_base;
	int dist_op2;
	size_t vreg_dest;
	size_t vreg_base;
	bool fixed_reg_dest;
	bool fixed_reg_base;
	subtilis_arm_reg_t *reg = NULL;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (type == SUBTILIS_ARM_INSTR_STR) {
		vreg_dest = instr->dest.num;
		fixed_reg_dest = prv_ensure(ud->arm_s, op, ud->int_regs,
					    ud->int_regs, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	vreg_base = instr->base.num;
	fixed_reg_base = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				    &instr->base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->offset, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_base) {
		dist_base =
		    prv_calculate_dist(ud, vreg_base, op, &ud->int_dist_walker);
		if (dist_base == -1)
			ud->int_regs->phys_to_virt[instr->base.num] = INT_MAX;
	}

	if (type == SUBTILIS_ARM_INSTR_STR) {
		if (!fixed_reg_dest) {
			dist_dest = prv_calculate_dist(ud, vreg_dest, op,
						       &ud->int_dist_walker);
			if (dist_dest == -1)
				ud->int_regs->phys_to_virt[instr->dest.num] =
				    INT_MAX;
			ud->int_regs->next[instr->dest.num] = dist_dest;
		}
	} else {
		prv_allocate_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (!fixed_reg_base)
		ud->int_regs->next[instr->base.num] = dist_base;
	if (reg)
		ud->int_regs->next[reg->num] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_mtran_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	/*
	 * TODO: Again mtran is not currently been used as a general
	 * instruction.  We're just inserting it into the code after
	 * register allocation has happened to implement a stack.
	 */

	ud->instr_count++;
}

static void prv_alloc_br_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_br_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	ud->instr_count++;
}

static void prv_alloc_swi_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_swi_instr_t *instr,
				subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_reg_t reg;
	subtilis_arm_reg_ud_t *ud = user_data;

	/* SWIs can only use the first 10 regs */

	reg.type = SUBTILIS_ARM_REG_FIXED;
	for (i = 0; i < 10; i++) {
		if ((1 << i) & instr->reg_mask) {
			reg.num = i;
			prv_allocate_dest(ud, op, &reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	ud->instr_count++;
}

static void prv_alloc_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_ldrc_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

static void prv_alloc_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	int dist_op1;
	int dist_op2;
	int dist_op2_shift;
	size_t vreg_op1;
	bool fixed_reg_op1;
	subtilis_arm_reg_ud_t *ud = user_data;
	subtilis_arm_reg_t *reg = NULL;
	subtilis_arm_reg_t *shift_reg = NULL;

	vreg_op1 = instr->op1.num;
	fixed_reg_op1 = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				   &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	shift_reg =
	    prv_ensure_op2_shift_reg(ud, op, &instr->op2, &dist_op2_shift, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1) {
		dist_op1 =
		    prv_calculate_dist(ud, vreg_op1, op, &ud->int_dist_walker);
		if (dist_op1 == -1)
			ud->int_regs->phys_to_virt[instr->op1.num] = INT_MAX;

		ud->int_regs->next[instr->op1.num] = dist_op1;
	}
	if (reg)
		ud->int_regs->next[reg->num] = dist_op2;
	if (shift_reg)
		ud->int_regs->next[shift_reg->num] = dist_op2_shift;

	ud->instr_count++;
}

static void prv_alloc_fpa_data_dyadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	int dist_op1;
	int dist_op2;
	size_t vreg_op1;
	size_t vreg_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

	vreg_op1 = instr->op1.num;
	(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs, &instr->op1,
			 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg.num;
		(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
				 &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 =
		    prv_calculate_dist(ud, vreg_op2, op, &ud->fpa_dist_walker);
		if (dist_op2 == -1)
			ud->fpa_regs->phys_to_virt[instr->op2.reg.num] =
			    INT_MAX;
	}

	dist_op1 = prv_calculate_dist(ud, vreg_op1, op, &ud->fpa_dist_walker);
	if (dist_op1 == -1)
		ud->fpa_regs->phys_to_virt[instr->op1.num] = INT_MAX;

	prv_allocate_fpa_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->fpa_regs->next[instr->op1.num] = dist_op1;
	if (!instr->immediate)
		ud->fpa_regs->next[instr->op2.reg.num] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_fpa_data_monadic_instr(void *user_data,
					     subtilis_arm_op_t *op,
					     subtilis_arm_instr_type_t type,
					     subtilis_fpa_data_instr_t *instr,
					     subtilis_error_t *err)
{
	int dist_op2;
	size_t vreg_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg.num;
		(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
				 &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 =
		    prv_calculate_dist(ud, vreg_op2, op, &ud->fpa_dist_walker);
		if (dist_op2 == -1)
			ud->fpa_regs->phys_to_virt[instr->op2.reg.num] =
			    INT_MAX;
	}

	prv_allocate_fpa_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!instr->immediate)
		ud->fpa_regs->next[instr->op2.reg.num] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_stran_instr_t *instr,
				      subtilis_error_t *err)
{
	int dist_dest;
	int dist_base;
	size_t vreg_dest;
	size_t vreg_base;
	bool fixed_reg_base;
	subtilis_arm_reg_ud_t *ud = user_data;

	if (type == SUBTILIS_FPA_INSTR_STF) {
		vreg_dest = instr->dest.num;
		(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
				 &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	vreg_base = instr->base.num;
	fixed_reg_base = prv_ensure(ud->arm_s, op, ud->int_regs, ud->int_regs,
				    &instr->base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_base) {
		dist_base =
		    prv_calculate_dist(ud, vreg_base, op, &ud->int_dist_walker);
		if (dist_base == -1)
			ud->int_regs->phys_to_virt[instr->base.num] = INT_MAX;
	}

	if (type == SUBTILIS_FPA_INSTR_STF) {
		dist_dest =
		    prv_calculate_dist(ud, vreg_dest, op, &ud->fpa_dist_walker);
		if (dist_dest == -1)
			ud->fpa_regs->phys_to_virt[instr->dest.num] = INT_MAX;
		ud->fpa_regs->next[instr->dest.num] = dist_dest;
	} else {
		prv_allocate_fpa_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (!fixed_reg_base)
		ud->int_regs->next[instr->base.num] = dist_base;

	ud->instr_count++;
}

static void prv_alloc_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	size_t vreg_op2;
	int dist_op2;
	subtilis_arm_reg_ud_t *ud = user_data;
	bool fixed_op2 = true;

	if (type != SUBTILIS_FPA_INSTR_FLT) {
		if (!instr->immediate) {
			vreg_op2 = instr->op2.reg.num;
			prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
				   &instr->op2.reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			dist_op2 = prv_calculate_dist(ud, vreg_op2, op,
						      &ud->fpa_dist_walker);
			if (dist_op2 == -1)
				ud->fpa_regs->phys_to_virt[instr->op2.reg.num] =
				    INT_MAX;
		}

		prv_allocate_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		if (!instr->immediate) {
			vreg_op2 = instr->op2.reg.num;
			fixed_op2 =
			    prv_ensure(ud->arm_s, op, ud->int_regs,
				       ud->int_regs, &instr->op2.reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			dist_op2 = prv_calculate_dist(ud, vreg_op2, op,
						      &ud->int_dist_walker);
			if (dist_op2 == -1)
				ud->int_regs->phys_to_virt[instr->op2.reg.num] =
				    INT_MAX;
		}

		prv_allocate_fpa_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_op2)
			ud->int_regs->next[instr->op2.reg.num] = dist_op2;
	}

	ud->instr_count++;
}

static void prv_alloc_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	int dist_dest;
	int dist_op2;
	size_t vreg_dest;
	size_t vreg_op2;
	subtilis_arm_reg_ud_t *ud = user_data;

	vreg_dest = instr->dest.num;
	(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
			 &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!instr->immediate) {
		vreg_op2 = instr->op2.reg.num;
		(void)prv_ensure(ud->arm_s, op, ud->int_regs, ud->fpa_regs,
				 &instr->op2.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dist_op2 =
		    prv_calculate_dist(ud, vreg_op2, op, &ud->fpa_dist_walker);
		if (dist_op2 == -1)
			ud->fpa_regs->phys_to_virt[instr->op2.reg.num] =
			    INT_MAX;
		ud->fpa_regs->next[instr->op2.reg.num] = dist_op2;
	}

	dist_dest = prv_calculate_dist(ud, vreg_dest, op, &ud->fpa_dist_walker);
	if (dist_dest == -1)
		ud->fpa_regs->phys_to_virt[instr->dest.num] = INT_MAX;
	ud->fpa_regs->next[instr->dest.num] = dist_dest;

	ud->instr_count++;
}

static void prv_alloc_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_arm_reg_ud_t *ud = user_data;

	prv_allocate_fpa_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

size_t subtilis_arm_reg_alloc(subtilis_arm_section_t *arm_s,
			      subtilis_error_t *err)
{
	subtlis_arm_walker_t walker;
	subtilis_arm_reg_ud_t ud;
	size_t retval;

	prv_init_arm_reg_ud(&ud, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	walker.user_data = &ud;
	walker.label_fn = prv_alloc_label;
	walker.data_fn = prv_alloc_data_instr;
	walker.mul_fn = prv_alloc_mul_instr;
	walker.cmp_fn = prv_alloc_cmp_instr;
	walker.mov_fn = prv_alloc_mov_instr;
	walker.stran_fn = prv_alloc_stran_instr;
	walker.mtran_fn = prv_alloc_mtran_instr;
	walker.br_fn = prv_alloc_br_instr;
	walker.swi_fn = prv_alloc_swi_instr;
	walker.ldrc_fn = prv_alloc_ldrc_instr;
	walker.fpa_data_monadic_fn = prv_alloc_fpa_data_monadic_instr;
	walker.fpa_data_dyadic_fn = prv_alloc_fpa_data_dyadic_instr;
	walker.fpa_stran_fn = prv_alloc_fpa_stran_instr;
	walker.fpa_tran_fn = prv_alloc_fpa_tran_instr;
	walker.fpa_cmp_fn = prv_alloc_fpa_cmp_instr;
	walker.fpa_ldrc_fn = prv_alloc_fpa_ldrc_instr;

	subtilis_arm_walk(arm_s, &walker, err);

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_s->reg_counter = 16;

	retval = (ud.int_regs->spill_max * sizeof(int32_t)) +
		 (ud.fpa_regs->spill_max * sizeof(double));

	prv_free_arm_reg_ud(&ud);

	return retval;

cleanup:

	prv_free_arm_reg_ud(&ud);

	return 0;
}

static bool prv_is_reg_used_before(subtilis_arm_reg_ud_t *ud, size_t reg_num,
				   subtilis_arm_op_t *from,
				   subtilis_arm_op_t *to)
{
	subtilis_error_t err;
	size_t i;

	ud->dist_data.reg_num = reg_num;

	do {
		subtilis_error_init(&err);
		ud->dist_data.last_used = 0;
		subtilis_arm_walk_from_to(ud->arm_s, &ud->int_dist_walker, from,
					  to, &err);
		if (err.type == SUBTILIS_ERROR_OK)
			return false;
		else if (ud->dist_data.last_used == -1)
			return true;

		/*
		 * We arrive here if reg_num is read from but not written to
		 * by one instruction in the region.  We need to keep checking.
		 * from the subsequent instruction.
		 */

		for (i = 0; i < ud->dist_data.last_used + 1; i++) {
			if ((from == to) || (from->next == SIZE_MAX))
				return false;

			from = &ud->arm_s->op_pool->ops[from->next];
		}
	} while (true);

	return false;
}

size_t subtilis_arm_regs_used_before(subtilis_arm_section_t *arm_s,
				     subtilis_arm_op_t *op,
				     subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_reg_ud_t ud;
	subtilis_arm_op_t *from;
	size_t reg_list = 0;

	prv_init_arm_reg_ud(&ud, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	from = &arm_s->op_pool->ops[arm_s->first_op];

	for (i = SUBTILIS_ARM_REG_MIN_INT_REGS;
	     i <= SUBTILIS_ARM_REG_MAX_INT_REGS; i++) {
		if (prv_is_reg_used_before(&ud, i, from, op))
			reg_list |= 1 << i;
	}

	prv_free_arm_reg_ud(&ud);

	return reg_list;
}

static bool prv_is_reg_used_after(subtilis_arm_reg_ud_t *ud, size_t reg_num,
				  subtilis_arm_op_t *op)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	ud->dist_data.reg_num = reg_num;
	ud->dist_data.last_used = ud->instr_count + 1;

	subtilis_arm_walk_from(ud->arm_s, &ud->int_dist_walker, op, &err);

	/*
	 * Check that reg_num is used and that the  first usage of
	 * reg_num is not a write.
	 */

	return (err.type != SUBTILIS_ERROR_OK) &&
	       (ud->dist_data.last_used != -1);
}

size_t subtilis_arm_regs_used_after(subtilis_arm_section_t *arm_s,
				    subtilis_arm_op_t *op,
				    subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_reg_ud_t ud;
	size_t reg_list = 0;

	prv_init_arm_reg_ud(&ud, arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	for (i = SUBTILIS_ARM_REG_MIN_INT_REGS;
	     i <= SUBTILIS_ARM_REG_MAX_INT_REGS; i++) {
		if (prv_is_reg_used_after(&ud, i, op))
			reg_list |= 1 << i;
	}

	prv_free_arm_reg_ud(&ud);

	return reg_list;
}

void subtilis_arm_save_regs(subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err)
{
	size_t i;
	size_t cs;
	size_t stm;
	size_t ldm;
	size_t start;
	size_t end;
	size_t regs_used;
	subtilis_arm_mtran_instr_t *mtran;

	for (i = 0; i < arm_s->call_site_count; i++) {
		regs_used = 0;
		cs = arm_s->call_sites[i].call_site;
		stm = arm_s->call_sites[i].stm_site;
		end = arm_s->op_pool->ops[stm].prev;
		if (end != SIZE_MAX) {
			regs_used = subtilis_arm_regs_used_before(
			    arm_s, &arm_s->op_pool->ops[end], err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		ldm = arm_s->op_pool->ops[cs].next;
		if (ldm == SIZE_MAX) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		start = arm_s->op_pool->ops[ldm].next;
		if (start == SIZE_MAX)
			continue;

		regs_used &= subtilis_arm_regs_used_after(
		    arm_s, &arm_s->op_pool->ops[start], err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		mtran = &arm_s->op_pool->ops[stm].op.instr.operands.mtran;
		mtran->reg_list |= regs_used;
		mtran = &arm_s->op_pool->ops[ldm].op.instr.operands.mtran;
		mtran->reg_list |= regs_used;
	}
}
