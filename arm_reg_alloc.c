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

#include "arm_reg_alloc.h"
#include "arm_walker.h"

#define SUBTILIS_ARM_REG_MAX_REGS 11

struct subtilis_arm_reg_class_t_ {
	size_t phys_to_virt[SUBTILIS_ARM_REG_MAX_REGS];
	int next[SUBTILIS_ARM_REG_MAX_REGS];
	size_t vr_reg_count;
	int32_t *spilt_regs;
	size_t *spill_stack;
	size_t spill_top;
	size_t spill_max;
};

typedef struct subtilis_arm_reg_class_t_ subtilis_arm_reg_class_t;

struct subtilis_dist_data_t_ {
	size_t reg_num;
	int last_used;
};

typedef struct subtilis_dist_data_t_ subtilis_dist_data_t;

struct subtilis_arm_reg_ud_t_ {
	subtilis_arm_reg_class_t int_regs;
	subtilis_arm_program_t *arm_p;
	size_t instr_count;
	subtlis_arm_walker_t dist_walker;
	subtilis_dist_data_t dist_data;
};

typedef struct subtilis_arm_reg_ud_t_ subtilis_arm_reg_ud_t;

static void prv_init_int_regs(subtilis_arm_reg_class_t *regs, size_t reg_count,
			      subtilis_error_t *err)
{
	size_t i;

	regs->spilt_regs = malloc(reg_count * sizeof(int32_t));
	if (!regs->spilt_regs) {
		subtilis_error_set_oom(err);
		return;
	}
	regs->spill_stack = malloc(reg_count * sizeof(size_t));
	if (!regs->spill_stack) {
		free(regs->spilt_regs);
		subtilis_error_set_oom(err);
		return;
	}

	for (i = 0; i < reg_count; i++) {
		regs->spilt_regs[i] = -1;
		regs->spill_stack[i] = i * 4;
	}
	regs->spill_top = 0;
	regs->spill_max = 0;
	regs->vr_reg_count = reg_count;

	for (i = 0; i < SUBTILIS_ARM_REG_MAX_REGS; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = INT_MAX;
	}
}

static void prv_free_int_regs(subtilis_arm_reg_class_t *regs)
{
	if (regs) {
		free(regs->spill_stack);
		free(regs->spilt_regs);
	}
}

static void prv_dist_handle_op2(subtilis_dist_data_t *ud,
				subtilis_arm_op2_t *op2, subtilis_error_t *err)
{
	if (op2->type == SUBTILIS_ARM_OP2_REG) {
		if (op2->op.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}
	ud->last_used++;
}

static void prv_dist_mov_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_data_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_data_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (instr->op1.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_stran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_stran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (type == SUBTILIS_ARM_INSTR_LDR) {
		if (instr->dest.num == ud->reg_num) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
	} else {
		if (instr->dest.num == ud->reg_num) {
			subtilis_error_set_walker_failed(err);
			return;
		}
	}

	if (instr->base.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->offset, err);
}

static void prv_dist_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mtran_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_dist_br_instr(void *user_data, subtilis_arm_op_t *op,
			      subtilis_arm_instr_type_t type,
			      subtilis_arm_br_instr_t *instr,
			      subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_swi_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_swi_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((1 << ud->reg_num) & instr->reg_mask) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_ldrc_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->dest.num == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_cmp_instr(void *user_data, subtilis_arm_op_t *op,
			       subtilis_arm_instr_type_t type,
			       subtilis_arm_data_instr_t *instr,
			       subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (instr->op1.num == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	prv_dist_handle_op2(ud, &instr->op2, err);
}

static void prv_dist_label(void *user_data, subtilis_arm_op_t *op, size_t label,
			   subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_init_arm_reg_ud(subtilis_arm_reg_ud_t *ud,
				subtilis_arm_program_t *arm_p,
				subtilis_error_t *err)
{
	prv_init_int_regs(&ud->int_regs, arm_p->reg_counter, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count = 0;
	ud->arm_p = arm_p;

	ud->dist_walker.user_data = &ud->dist_data;
	ud->dist_walker.label_fn = prv_dist_label;
	ud->dist_walker.data_fn = prv_dist_data_instr;
	ud->dist_walker.cmp_fn = prv_dist_cmp_instr;
	ud->dist_walker.mov_fn = prv_dist_mov_instr;
	ud->dist_walker.stran_fn = prv_dist_stran_instr;
	ud->dist_walker.mtran_fn = prv_dist_mtran_instr;
	ud->dist_walker.br_fn = prv_dist_br_instr;
	ud->dist_walker.swi_fn = prv_dist_swi_instr;
	ud->dist_walker.ldrc_fn = prv_dist_ldrc_instr;
}

static int prv_calculate_dist(subtilis_arm_reg_ud_t *ud, size_t reg_num,
			      subtilis_arm_op_t *op)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	if (op->next == SIZE_MAX)
		return -1;

	op = &ud->arm_p->pool->ops[op->next];

	ud->dist_data.reg_num = reg_num;
	ud->dist_data.last_used = ud->instr_count + 1;

	subtilis_arm_walk_from(ud->arm_p, &ud->dist_walker, op, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		return ud->dist_data.last_used;
	else
		return -1;
}

static void prv_free_arm_reg_ud(subtilis_arm_reg_ud_t *ud)
{
	if (ud)
		prv_free_int_regs(&ud->int_regs);
}

static size_t prv_virt_to_phys(subtilis_arm_reg_class_t *regs,
			       subtilis_arm_reg_t *reg)
{
	size_t i;
	size_t retval = INT_MAX;

	for (i = 0; i < SUBTILIS_ARM_REG_MAX_REGS; i++) {
		if (regs->phys_to_virt[i] == reg->num)
			return i;
	}

	return retval;
}

static void prv_load_spilled_reg(subtilis_arm_program_t *arm_p,
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

	offset += arm_p->globals;

	base.type = SUBTILIS_ARM_REG_FIXED;
	base.num = 12;
	if (offset > 4095 || offset < -4095) {
		subtilis_arm_insert_stran_spill_imm(
		    arm_p, current, SUBTILIS_ARM_INSTR_LDR,
		    SUBTILIS_ARM_CCODE_AL, phys, base, phys, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		subtilis_arm_insert_stran_imm(
		    arm_p, current, SUBTILIS_ARM_INSTR_LDR,
		    SUBTILIS_ARM_CCODE_AL, phys, base, offset, err);
	}

	/* TODO need to update next */

	regs->phys_to_virt[phys.num] = virt.num;
	regs->spilt_regs[virt.num] = INT_MAX;
	regs->spill_top--;
	regs->spill_stack[regs->spill_top] = offset;
	regs->next[phys.num] = -1;
}

static void prv_spill_reg(subtilis_arm_program_t *arm_p,
			  subtilis_arm_op_t *current, size_t assigned,
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

	offset = (int32_t)regs->spill_stack[regs->spill_top++] + arm_p->globals;
	if (regs->spill_max < regs->spill_top)
		regs->spill_max = regs->spill_top;

	regs->spilt_regs[assigned] = offset;

	base.type = SUBTILIS_ARM_REG_FIXED;
	base.num = 12;
	if (offset > 4095 || offset < -4095) {
		for (i = 0; i < SUBTILIS_ARM_REG_MAX_REGS; i++)
			if ((regs->phys_to_virt[i] == INT_MAX) &&
			    (i != reg.num))
				break;

		spill_reg.type = SUBTILIS_ARM_REG_FIXED;
		if (i == SUBTILIS_ARM_REG_MAX_REGS) {
			subtilis_arm_insert_push(arm_p, current,
						 SUBTILIS_ARM_CCODE_AL, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			spill_reg.num = 0;
		} else {
			spill_reg.num = i;
		}
		subtilis_arm_insert_stran_spill_imm(
		    arm_p, current, SUBTILIS_ARM_INSTR_STR,
		    SUBTILIS_ARM_CCODE_AL, reg, base, spill_reg, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (i == SUBTILIS_ARM_REG_MAX_REGS) {
			subtilis_arm_insert_pop(arm_p, current,
						SUBTILIS_ARM_CCODE_AL, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} else {
		subtilis_arm_insert_stran_imm(
		    arm_p, current, SUBTILIS_ARM_INSTR_STR,
		    SUBTILIS_ARM_CCODE_AL, reg, base, offset, err);
	}

	/* TODO need to update next */
}

static void prv_allocate_fixed(subtilis_arm_program_t *arm_p,
			       subtilis_arm_op_t *current,
			       subtilis_arm_reg_class_t *regs,
			       subtilis_arm_reg_t *reg, subtilis_error_t *err)
{
	size_t assigned;

	assigned = regs->phys_to_virt[reg->num];
	if (assigned != INT_MAX) {
		prv_spill_reg(arm_p, current, assigned, regs, *reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_allocate_floating(subtilis_arm_program_t *arm_p,
				  subtilis_arm_op_t *current,
				  subtilis_arm_reg_class_t *regs,
				  subtilis_arm_reg_t *reg,
				  subtilis_error_t *err)
{
	subtilis_arm_reg_t target_reg;
	size_t next = SUBTILIS_ARM_REG_MAX_REGS;
	size_t max_next = -1;
	int i;

	/* Virtual register is not already assigned. */

	for (i = SUBTILIS_ARM_REG_MAX_REGS - 1; i >= 0; i--)
		if (regs->phys_to_virt[i] == INT_MAX)
			break;

	target_reg.type = SUBTILIS_ARM_REG_FIXED;
	if (i >= 0) {
		target_reg.num = (size_t)i;
	} else {
		/* No free physical regs.  Need to spill. */

		for (i = 0; i < SUBTILIS_ARM_REG_MAX_REGS; i++)
			if (regs->next[i] > max_next) {
				max_next = regs->next[i];
				next = i;
			}
		if (next == SUBTILIS_ARM_REG_MAX_REGS) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		target_reg.num = next;
		prv_spill_reg(arm_p, current, next, regs, target_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	*reg = target_reg;
}

static void prv_allocate(subtilis_arm_program_t *arm_p,
			 subtilis_arm_op_t *current,
			 subtilis_arm_reg_class_t *regs,
			 subtilis_arm_reg_t *reg, subtilis_error_t *err)
{
	size_t virt_num = reg->num;

	if (reg->type == SUBTILIS_ARM_REG_FIXED) {
		if (reg->num >= SUBTILIS_ARM_REG_MAX_REGS)
			return;
		prv_allocate_fixed(arm_p, current, regs, reg, err);
	} else {
		prv_allocate_floating(arm_p, current, regs, reg, err);
	}

	regs->phys_to_virt[reg->num] = virt_num;
}

/* Returns true if register is of fixed use, e.g., R13. */

static bool prv_ensure(subtilis_arm_program_t *arm_p,
		       subtilis_arm_op_t *current,
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
		if (reg->num >= SUBTILIS_ARM_REG_MAX_REGS)
			return true;

		assigned = regs->phys_to_virt[reg->num];
		if (assigned != reg->num) {
			prv_allocate_fixed(arm_p, current, regs, reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(arm_p, current, regs, *reg, *reg,
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
			prv_allocate_floating(arm_p, current, regs, &target_reg,
					      err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(arm_p, current, regs, *reg,
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

static subtilis_arm_reg_t *prv_ensure_op2(subtilis_arm_reg_ud_t *ud,
					  subtilis_arm_op_t *op,
					  subtilis_arm_op2_t *op2,
					  int *dist_op2, subtilis_error_t *err)
{
	size_t vreg_op2;
	bool fixed_reg;
	subtilis_arm_reg_t *reg = NULL;

	if (op2->type == SUBTILIS_ARM_OP2_I32)
		return NULL;

	if (op2->type == SUBTILIS_ARM_OP2_REG)
		reg = &op2->op.reg;
	else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED)
		reg = &op2->op.shift.reg;

	vreg_op2 = reg->num;
	fixed_reg = prv_ensure(ud->arm_p, op, &ud->int_regs, reg, err);
	if (fixed_reg || (err->type != SUBTILIS_ERROR_OK))
		return NULL;

	*dist_op2 = prv_calculate_dist(ud, vreg_op2, op);
	if (*dist_op2 == -1)
		ud->int_regs.phys_to_virt[reg->num] = INT_MAX;

	return reg;
}

static void prv_allocate_dest(subtilis_arm_reg_ud_t *ud, subtilis_arm_op_t *op,
			      subtilis_arm_reg_t *dest, subtilis_error_t *err)
{
	int dist_dest;
	size_t vreg_dest = dest->num;

	if ((dest->type == SUBTILIS_ARM_REG_FIXED) &&
	    (vreg_dest >= SUBTILIS_ARM_REG_MAX_REGS))
		return;

	prv_allocate(ud->arm_p, op, &ud->int_regs, dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_dest = prv_calculate_dist(ud, vreg_dest, op);
	if (dist_dest == -1)
		ud->int_regs.phys_to_virt[dest->num] = INT_MAX;
	ud->int_regs.next[dest->num] = dist_dest;
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
	subtilis_arm_reg_t *reg;
	subtilis_arm_reg_ud_t *ud = user_data;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (reg)
		ud->int_regs.next[reg->num] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_data_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	int dist_op1;
	int dist_op2;
	size_t vreg_op1;
	bool fixed_reg_op1;
	subtilis_arm_reg_ud_t *ud = user_data;
	subtilis_arm_reg_t *reg = NULL;

	vreg_op1 = instr->op1.num;
	fixed_reg_op1 =
	    prv_ensure(ud->arm_p, op, &ud->int_regs, &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1) {
		dist_op1 = prv_calculate_dist(ud, vreg_op1, op);
		if (dist_op1 == -1)
			ud->int_regs.phys_to_virt[instr->op1.num] = INT_MAX;
	}

	prv_allocate_dest(ud, op, &instr->dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1)
		ud->int_regs.next[instr->op1.num] = dist_op1;
	if (reg)
		ud->int_regs.next[reg->num] = dist_op2;

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
		fixed_reg_dest =
		    prv_ensure(ud->arm_p, op, &ud->int_regs, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	vreg_base = instr->base.num;
	fixed_reg_base =
	    prv_ensure(ud->arm_p, op, &ud->int_regs, &instr->base, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->offset, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_base) {
		dist_base = prv_calculate_dist(ud, vreg_base, op);
		if (dist_base == -1)
			ud->int_regs.phys_to_virt[instr->base.num] = INT_MAX;
	}

	if (type == SUBTILIS_ARM_INSTR_STR) {
		if (!fixed_reg_dest) {
			dist_dest = prv_calculate_dist(ud, vreg_dest, op);
			if (dist_dest == -1)
				ud->int_regs.phys_to_virt[instr->dest.num] =
				    INT_MAX;
			ud->int_regs.next[instr->dest.num] = dist_dest;
		}
	} else {
		prv_allocate_dest(ud, op, &instr->dest, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (!fixed_reg_base)
		ud->int_regs.next[instr->base.num] = dist_base;
	if (reg)
		ud->int_regs.next[reg->num] = dist_op2;

	ud->instr_count++;
}

static void prv_alloc_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_mtran_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
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
	size_t vreg_op1;
	bool fixed_reg_op1;
	subtilis_arm_reg_ud_t *ud = user_data;
	subtilis_arm_reg_t *reg = NULL;

	vreg_op1 = instr->op1.num;
	fixed_reg_op1 =
	    prv_ensure(ud->arm_p, op, &ud->int_regs, &instr->op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = prv_ensure_op2(ud, op, &instr->op2, &dist_op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_op1) {
		dist_op1 = prv_calculate_dist(ud, vreg_op1, op);
		if (dist_op1 == -1)
			ud->int_regs.phys_to_virt[instr->op1.num] = INT_MAX;

		ud->int_regs.next[instr->op1.num] = dist_op1;
	}
	if (reg)
		ud->int_regs.next[reg->num] = dist_op2;

	ud->instr_count++;
}

size_t subtilis_arm_reg_alloc(subtilis_arm_program_t *arm_p,
			      subtilis_error_t *err)
{
	subtlis_arm_walker_t walker;
	subtilis_arm_reg_ud_t ud;
	size_t retval;

	prv_init_arm_reg_ud(&ud, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	walker.user_data = &ud;
	walker.label_fn = prv_alloc_label;
	walker.data_fn = prv_alloc_data_instr;
	walker.cmp_fn = prv_alloc_cmp_instr;
	walker.mov_fn = prv_alloc_mov_instr;
	walker.stran_fn = prv_alloc_stran_instr;
	walker.mtran_fn = prv_alloc_mtran_instr;
	walker.br_fn = prv_alloc_br_instr;
	walker.swi_fn = prv_alloc_swi_instr;
	walker.ldrc_fn = prv_alloc_ldrc_instr;

	subtilis_arm_walk(arm_p, &walker, err);

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p->reg_counter = 16;

	retval = ud.int_regs.spill_max;

	prv_free_arm_reg_ud(&ud);

	return retval;

cleanup:

	prv_free_arm_reg_ud(&ud);

	return 0;
}
