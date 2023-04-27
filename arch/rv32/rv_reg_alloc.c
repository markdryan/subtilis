/*
 * Copyright (c) 2023 Mark Ryan
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

#include "rv_reg_alloc.h"

#include "../../common/bitset.h"
#include "../../common/prespilt_offsets.h"
#include "../../common/regs_used_virt.h"

#include "rv_int_dist.h"
#include "rv_int_used.h"
#include "rv_real_dist.h"
#include "rv_real_used.h"
#include "rv_sub_section.h"
#include "rv_walker.h"

/*
 * The other thing that needs documenting is the layout of the stack.  It should
 * look something like this.  It needs to be 8 byte aligned on entry to each
 * function
 *
 * |------------------|     ------------------------------------------- *
 * |   int arg n      |     This section is optional and only exists
 * |------------------|     if there are more than 4 integer arguments.
 * |   int arg n-1    |     Arguments are pushed in reverse order.
 * |----------------- |     Each argument consumes 4 bytes.
 * |   int arg 4      |
 *
 * There's an optional 4 bytes spare here to make sure we're aligned.
 * This happens if we have an odd number of spilled arguments.
 *
 * |------------------|     -------------------------------------------
 * |   real arg n     |     This section is optional and only exists
 * |                  |     if there are more than 4 real arguments.
 * |------------------|     Arguments are pushed in reverse order.
 * |   real arg n-1   |     Each argument consumes 8 bytes.
 * |                  |
 * |----------------- |
 * |   real arg 4     |
 * |                  |
 * |----------------- |     -------------------------------------------*
 * |   x1             |     Return address of calling function
 * |------------------|     -------------------------------------------
 * |   x8             |     Stack frame of current function
 * |----------------- |     -------------------------------------------
 *
 * Stack pointer starts here at entry into callee.  There may be an
 * unused 4 bytes here, if the size of the split data + local data is
 * not a multiple of 8.
 *
 * |----------------- |     -------------------------------------------
 * |  spilt int reg 1 |     This section is optional and exists if the
 * |----------------- |     register allocator needed to spill any
 * |  spilt int reg n |     virtual integer registers. 4 bytes per reg.
 * |----------------- |     -------------------------------------------
 * | spilt real reg 1 |     This section is optional and exists if the
 * |                  |     register allocator needed to spill any
 * |----------------- |     virtual real registers.  Each spilt
 * | spilt real reg n |     register consumes 8 bytes.
 * |                  |
 * |----------------- |     -------------------------------------------
 * | prespilt real    |     This section is optional and exists if the
 * | reg1             |     register allocator needed to spill any
 * |----------------- |     virtual real registers at the end of a
 * | prespilt real    |     basic block.  Each spilt register consumes
 * | reg n            |     8 bytes.
 * |----------------- |     -------------------------------------------
 *
 * We may have a dummy 4 bytes here to make sure that the real split
 * section is 8 byte aligned.
 *
 * | prespilt int reg1|     This section is optional and exists if the
 * |----------------- |     register allocator needed to spill any
 * | prespilt int reg2|     virtual int registers at the end of a
 * |----------------- |     basic block.  Each spilt register consumes
 * | prespilt int regn|     4 bytes
 * |----------------- |     -------------------------------------------
 * |                  |     Area reserved for storing local data, i.e.,
 * |                  |     local variables that are too large to fit
 * |  Local data      |     into a virtual register e.g., a string or
 * |                  |     an array.  This block is always 8 byte
 * |                  |     aligned.
 * |----------------- |     -------------------------------------------
 *
 */

/* clang-format off */
typedef void (*subtilis_rv_reg_spill_imm_t)(subtilis_rv_section_t *s,
					    subtilis_rv_op_t *current,
					    subtilis_rv_reg_t dest,
					    subtilis_rv_reg_t base,
					    subtilis_rv_reg_t spill_reg,
					    int32_t offset,
					    subtilis_error_t *err);
typedef void (*subtilis_rv_reg_stran_imm_t)(subtilis_rv_section_t *s,
					    subtilis_rv_op_t *current,
					    subtilis_rv_reg_t dest,
					    subtilis_rv_reg_t base,
					    int32_t offset,
					    subtilis_error_t *err);

/* clang-format on */

typedef enum {
	SUBTILIS_RV_SPILL_POINT_LOAD,
	SUBTILIS_RV_SPILL_POINT_STORE,
} subtilis_rv_spill_point_type_t;

struct subtilis_rv_spill_point_t_ {
	subtilis_rv_spill_point_type_t type;
	size_t pos;
	int32_t offset;
	subtilis_rv_reg_t phys;
};

typedef struct subtilis_rv_spill_point_t_ subtilis_rv_spill_point_t;

struct subtilis_rv_reg_class_t_ {
	size_t first_free;
	size_t max_regs;
	size_t *phys_to_virt;
	int *next;
	size_t vr_reg_count;
	int32_t *spilt_regs;
	size_t *spill_stack;
	size_t spill_top;
	size_t spill_max;
	size_t reg_size;
	size_t spilt_args;
	subtilis_rv_reg_stran_imm_t load_far;
	subtilis_rv_reg_spill_imm_t store_far;
	subtilis_rv_reg_stran_imm_t load_near;
	subtilis_rv_reg_stran_imm_t store_near;
	subtilis_rv_spill_point_t *spill_points;
	size_t spill_points_count;
	size_t spill_points_max;
	subtilis_rv_walker_t dist_walker;
	subtilis_rv_walker_t used_walker;
};

typedef struct subtilis_rv_reg_class_t_ subtilis_rv_reg_class_t;

struct subtilis_rv_reg_ud_t_ {
	size_t basic_block_spill;
	subtilis_rv_reg_class_t *int_regs;
	subtilis_rv_reg_class_t *real_regs;
	subtilis_rv_section_t *rv_s;
	size_t instr_count;
	subtilis_dist_data_t dist_data;
	subtilis_rv_op_t **ss_terminators;
	size_t current_ss;
	size_t max_ss;
};

typedef struct subtilis_rv_reg_ud_t_ subtilis_rv_reg_ud_t;

/* clang-format off */
static subtilis_rv_reg_class_t *prv_new_regs(
	size_t reg_count, size_t max_regs, size_t first_free, size_t reg_size,
	size_t args, subtilis_rv_reg_stran_imm_t load_far,
	subtilis_rv_reg_spill_imm_t store_far,
	subtilis_rv_reg_stran_imm_t load_near,
	subtilis_rv_reg_stran_imm_t store_near,
	subtilis_error_t *err)

/* clang-format on */
{
	size_t i;
	size_t index;
	subtilis_rv_reg_class_t *regs;
	size_t reg_args = RV_MAX_REG_ARGS;

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
	regs->first_free = first_free;
	regs->max_regs = max_regs;
	regs->spill_max = regs->spill_top;
	regs->reg_size = reg_size;
	regs->load_far = load_far;
	regs->store_far = store_far;
	regs->load_near = load_near;
	regs->store_near = store_near;

	for (i = 0; i < reg_count; i++) {
		regs->spilt_regs[i] = -1;
		regs->spill_stack[i] = i * reg_size;
	}
	regs->vr_reg_count = reg_count;

	/*
	 * We pre-allocate the restricted registers to themselves.
	 * The distance caculation is incorrect but this shouldn't matter
	 * as these registers will be excluded from register allocation.
	 */

	for (i = 0; i < first_free; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = i;
	}

	for (; i < SUBTILIS_RV_REG_A0; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = INT_MAX;
	}

	for (; i < SUBTILIS_RV_REG_A0 + reg_args; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = (i + max_regs) - SUBTILIS_RV_REG_A0;
	}

	for (; i < max_regs; i++) {
		regs->next[i] = -1;
		regs->phys_to_virt[i] = INT_MAX;
	}

	for (i = reg_args; i < args; i++) {
		index = i - reg_args;
		regs->spilt_regs[i + max_regs] = index * reg_size;
	}

	if (i > reg_args) {
		regs->spilt_args = i - reg_args;
		regs->spill_top = regs->spilt_args;
		regs->spill_max = regs->spill_top;
		for (i = 0; i < regs->spilt_args; i++)
			regs->spill_stack[i] = INT_MAX;
	} else {
		regs->spilt_args = 0;
	}

	regs->spill_points = NULL;
	regs->spill_points_count = 0;
	regs->spill_points_max = 0;

	return regs;

on_error:

	free(regs->spilt_regs);
	free(regs->next);
	free(regs->phys_to_virt);
	free(regs);

	return NULL;
}

static void prv_free_regs(subtilis_rv_reg_class_t *regs)
{
	if (regs) {
		free(regs->spill_points);
		free(regs->spill_stack);
		free(regs->spilt_regs);
		free(regs->next);
		free(regs->phys_to_virt);
		free(regs);
	}
}


static void prv_init_rv_reg_ud(subtilis_rv_reg_ud_t *ud,
			       subtilis_rv_section_t *rv_s,
			       subtilis_error_t *err)
{
	size_t max_int_regs;
	size_t max_real_regs;

	subtilis_rv_section_max_regs(rv_s, &max_int_regs, &max_real_regs);

	ud->int_regs = prv_new_regs(
	    max_int_regs, SUBTILIS_RV_REG_MAX_INT_REGS,
	    SUBTILIS_RV_INT_FIRST_FREE, sizeof(int32_t),
	    rv_s->stype->int_regs, subtilis_rv_insert_lw_helper,
	    subtilis_rv_insert_sw_helper, subtilis_rv_section_insert_lw,
	    subtilis_rv_section_insert_sw, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->real_regs = prv_new_regs(
		max_real_regs, SUBTILIS_RV_REG_MAX_REAL_REGS,
		SUBTILIS_RV_REAL_FIRST_FREE, sizeof(double),
		rv_s->stype->fp_regs,
		subtilis_rv_insert_ld_helper,
		subtilis_rv_insert_sd_helper, subtilis_rv_section_insert_ld,
		subtilis_rv_section_insert_sd, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		prv_free_regs(ud->int_regs);
		return;
	}

	ud->instr_count = 0;
	ud->rv_s = rv_s;
	ud->ss_terminators = NULL;
	ud->current_ss = 0;
	ud->max_ss = 0;

	subtilis_rv_init_dist_walker(&ud->int_regs->dist_walker,
				     &ud->dist_data);
	subtilis_rv_init_real_dist_walker(&ud->real_regs->dist_walker,
					  &ud->dist_data);
	subtilis_rv_init_used_walker(&ud->int_regs->used_walker,
				     &ud->dist_data);
	subtilis_rv_init_real_used_walker(&ud->real_regs->used_walker,
					  &ud->dist_data);
}

static void prv_free_rv_reg_ud(subtilis_rv_reg_ud_t *ud)
{
	if (!ud)
		return;

	prv_free_regs(ud->real_regs);
	prv_free_regs(ud->int_regs);
	free(ud->ss_terminators);
}

static void prv_sub_section_int_links(subtilis_rv_reg_ud_t *ud,
				      subtilis_bitset_t *int_save,
				      subtilis_prespilt_offsets_t *offsets,
				      subtilis_rv_op_t *op,
				      subtilis_error_t *err)
{
	int j;
	int32_t offset;
	subtilis_rv_reg_t base;
	subtilis_rv_reg_t reg = SIZE_MAX;

	for (j = 0; j <= int_save->max_value; j++) {
		if (!subtilis_bitset_isset(int_save, j))
			continue;
		offset = subtilis_prespilt_int_offset(offsets, j, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		offset += ud->rv_s->locals;
		base = SUBTILIS_RV_REG_LOCAL;
		if (offset > 2047 || offset < -2048) {
			reg = subtilis_rv_acquire_new_reg(ud->rv_s);
			subtilis_rv_insert_offset_helper(ud->rv_s, op, base,
							 reg, offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			offset = 0;
			base = reg;
		}
		subtilis_rv_section_insert_sw(ud->rv_s, op, base, j, offset,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_sub_section_real_links(subtilis_rv_reg_ud_t *ud,
				       subtilis_bitset_t *real_save,
				       subtilis_prespilt_offsets_t *offsets,
				       subtilis_rv_op_t *op,
				       subtilis_error_t *err)
{
	int j;
	int32_t offset;
	subtilis_rv_reg_t base;
	subtilis_rv_reg_t reg = SIZE_MAX;

	for (j = 0; j <= real_save->max_value; j++) {
		if (!subtilis_bitset_isset(real_save, j))
			continue;
		offset = subtilis_prespilt_real_offset(offsets, j, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		offset += ud->rv_s->locals;
		base = SUBTILIS_RV_REG_LOCAL;
		if (offset > 2047 || offset < -2048) {
			reg = subtilis_rv_acquire_new_reg(ud->rv_s);
			subtilis_rv_insert_offset_helper(ud->rv_s, op, base,
							 reg, offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			offset = 0;
			base = reg;
		}
		subtilis_rv_section_insert_sd(ud->rv_s, op, base, j, offset,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_init_two_links(subtilis_rv_reg_ud_t *ud, subtilis_rv_ss_t *ss,
			       subtilis_prespilt_offsets_t *offsets,
			       size_t ss_index, subtilis_error_t *err)
{
	subtilis_rv_ss_link_t *link1;
	subtilis_rv_ss_link_t *link2;
	subtilis_rv_op_t *op1;
	subtilis_rv_op_t *op2;
	subtilis_bitset_t common_save;
	subtilis_bitset_t link1_save;
	subtilis_bitset_t link2_save;

	subtilis_bitset_init(&common_save);
	subtilis_bitset_init(&link1_save);
	subtilis_bitset_init(&link2_save);

	link1 = &ss->links[0];
	link2 = &ss->links[1];
	op1 = &ud->rv_s->op_pool->ops[link1->op];
	op2 = &ud->rv_s->op_pool->ops[link2->op];
	ud->ss_terminators[ss_index] = op2;
	if ((op1->type != SUBTILIS_RV_OP_INSTR) ||
	    (op1->op.instr.etype != SUBTILIS_RV_B_TYPE) ||
	    (op2->type != SUBTILIS_RV_OP_LABEL)) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	subtilis_compute_save_sets(&link1->int_save, &link2->int_save,
				   &common_save, &link1_save,
				   &link2_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * Need to be a bit careful here.  Adding ops can invalidate our
	 * pointers to op1 and op2 so we need to refresh them before using
	 * them.
	 */

	/*
	 * TODO:
	 * These are split up because ideally we onlt want to spill
	 * the live regs for link1 if we take the branch and spill the
	 * live regs for link2 if we don't.  We always want to spill
	 * the common regs.  In the ARM backend, we apply the
	 * conditional code of the branch to the link1_save registers.
	 * This way we only do the store if we're going to take the branch.
	 * There are no condition codes in RV so for the time being we do
	 * the link1 stores, even if we're not taking the branch.
	 * We could avoid this with a second jump, but not sure
	 * if it's worth fixing as ultimately we'll switch to a global
	 * register allocator.  Interesting though.
	 *
	 * At least if we take the branch we don't store the registers
	 * for the link we don't take.
	 */

	op1 = &ud->rv_s->op_pool->ops[link1->op];
	prv_sub_section_int_links(ud, &common_save, offsets, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1 = &ud->rv_s->op_pool->ops[link1->op];
	prv_sub_section_int_links(ud, &link1_save, offsets, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2 = &ud->rv_s->op_pool->ops[link2->op];
	prv_sub_section_int_links(ud, &link2_save, offsets, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_reset(&common_save);
	subtilis_bitset_reset(&link1_save);
	subtilis_bitset_reset(&link2_save);

	subtilis_compute_save_sets(&link1->real_save, &link2->real_save,
			      &common_save, &link1_save, &link2_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1 = &ud->rv_s->op_pool->ops[link1->op];
	prv_sub_section_real_links(ud, &common_save, offsets, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1 = &ud->rv_s->op_pool->ops[link1->op];
	prv_sub_section_real_links(ud, &link1_save, offsets, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op2 = &ud->rv_s->op_pool->ops[link2->op];
	prv_sub_section_real_links(ud, &link2_save, offsets, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:

	subtilis_bitset_free(&link2_save);
	subtilis_bitset_free(&link1_save);
	subtilis_bitset_free(&common_save);
}

static void prv_init_sub_section_links(subtilis_rv_reg_ud_t *ud,
				       subtilis_rv_ss_t *ss,
				       subtilis_prespilt_offsets_t *offsets,
				       size_t ss_index, subtilis_error_t *err)
{
	subtilis_rv_ss_link_t *link;
	subtilis_rv_op_t *op;

	if (ss->num_links == 0)
		return;

	if (ss->num_links == 1) {
		link = &ss->links[0];
		op = &ud->rv_s->op_pool->ops[link->op];
		ud->ss_terminators[ss_index] = op;
		prv_sub_section_int_links(ud, &link->int_save, offsets, op,
					  err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		prv_sub_section_real_links(ud, &link->real_save, offsets,
					   op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		return;
	}

	if (ss->num_links != 2) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prv_init_two_links(ud, ss, offsets, ss_index, err);
}

static void prv_init_sub_section(subtilis_rv_reg_ud_t *ud,
				 subtilis_rv_ss_t *ss,
				 subtilis_prespilt_offsets_t *offsets,
				 subtilis_error_t *err)
{
	int i;
	int32_t offset;
	subtilis_rv_op_t *op;

	op = &ud->rv_s->op_pool->ops[ss->load_spill];
	if (op->next == ud->rv_s->last_op)
		return;
	op = &ud->rv_s->op_pool->ops[op->next];

	for (i = 0; i <= ss->int_inputs.max_value; i++) {
		if (!subtilis_bitset_isset(&ss->int_inputs, i))
			continue;
		offset = subtilis_prespilt_int_offset(offsets, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		offset += ud->rv_s->locals;

		subtilis_rv_insert_lw_helper(ud->rv_s, op, i,
					     SUBTILIS_RV_REG_LOCAL,
					     offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	for (i = 0; i <= ss->real_inputs.max_value; i++) {
		if (!subtilis_bitset_isset(&ss->real_inputs, i))
			continue;
		offset = subtilis_prespilt_real_offset(offsets, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		offset += ud->rv_s->locals;

		subtilis_rv_insert_ld_helper(ud->rv_s, op, i,
					     SUBTILIS_RV_REG_LOCAL,
					     offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_link_basic_blocks(subtilis_rv_reg_ud_t *ud,
				  subtilis_error_t *err)
{
	subtilis_rv_subsections_t sss;
	subtilis_prespilt_offsets_t offsets;
	size_t i;

	subtilis_rv_subsections_init(&sss);
	subtilis_prespilt_offsets_init(&offsets);

	subtilis_rv_subsections_calculate(&sss, ud->rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	ud->max_ss = sss.count;
	ud->ss_terminators = calloc(sss.count, sizeof(*ud->ss_terminators));
	if (!ud->ss_terminators) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	ud->basic_block_spill = 
		subtilis_prespilt_calculate(&offsets, &sss.int_save,
					    &sss.real_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * We now need to insert instructions to spill the live virtual
	 * registers at the end of each basic block.
	 */

	prv_init_sub_section_links(ud, &sss.sub_sections[0], &offsets, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 1; i < sss.count; i++) {
		prv_init_sub_section(ud, &sss.sub_sections[i], &offsets, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		prv_init_sub_section_links(ud, &sss.sub_sections[i], &offsets,
					   i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	subtilis_prespilt_free(&offsets);
	subtilis_rv_subsections_free(&sss);
}

static void prv_insert_spill_code_store(subtilis_rv_section_t *rv_s,
					subtilis_rv_reg_t reg,
					subtilis_rv_op_t *current,
					subtilis_rv_reg_class_t *int_regs,
					subtilis_rv_reg_class_t *regs,
					int32_t offset, subtilis_error_t *err)
{
	size_t i;
	subtilis_rv_reg_t spill_reg;

	if (offset > SUBTILIS_RV_MAX_OFFSET ||
	    offset < -SUBTILIS_RV_MIN_OFFSET) {
		/*
		 * We need to find an integer register to act as our base
		 * when spilling the contents of reg_num.  So if we're
		 * currently spilling the contents of a floating point
		 * register we still need to use the int_regs structure
		 * to use as our base.
		 */

		for (i = int_regs->first_free; i < int_regs->max_regs; i++)
			if ((int_regs->phys_to_virt[i] == INT_MAX) &&
			    (int_regs != regs || i != reg))
				break;

		if (i == int_regs->max_regs) {
			/*
			 * TODO: Don't think this code has ever been tested.
			 */

			spill_reg = SUBTILIS_RV_REG_T7;
			subtilis_rv_section_insert_sw(rv_s, current,
						      SUBTILIS_RV_REG_STACK,
						      spill_reg,-4,
						      err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else {
			spill_reg = i;
		}
		regs->store_far(rv_s, current, SUBTILIS_RV_REG_LOCAL, reg,
				spill_reg, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (i == int_regs->max_regs) {
			subtilis_rv_section_insert_lw(rv_s, current, spill_reg,
						      SUBTILIS_RV_REG_STACK, -4,
						      err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} else {
		regs->store_near(rv_s, current, SUBTILIS_RV_REG_LOCAL, reg,
				 offset, err);
	}
}

static void prv_insert_spill_code(subtilis_rv_reg_ud_t *ud,
				  subtilis_rv_reg_class_t *regs,
				  int32_t offset, int32_t arg_offset,
				  subtilis_error_t *err)
{
	size_t i;
	subtilis_rv_spill_point_t *sp;
	subtilis_rv_op_t *current;
	int32_t basic_offset;
	subtilis_rv_section_t *rv_s = ud->rv_s;

	for (i = 0; i < regs->spill_points_count; i++) {
		sp = &regs->spill_points[i];
		current = &rv_s->op_pool->ops[sp->pos];
		if (sp->offset < regs->spilt_args * regs->reg_size)
			basic_offset = arg_offset;
		else
			basic_offset = offset;
		basic_offset += sp->offset;
		if (sp->type == SUBTILIS_RV_SPILL_POINT_LOAD) {
			regs->load_far(rv_s, current, sp->phys,
				       SUBTILIS_RV_REG_LOCAL, basic_offset,
				       err);
		} else {
			prv_insert_spill_code_store(rv_s, sp->phys, current,
						    ud->int_regs, regs,
						    basic_offset, err);
		}
	}
}

static size_t prv_virt_to_phys(subtilis_rv_reg_class_t *regs,
			       subtilis_rv_reg_t *reg)
{
	size_t i;
	size_t retval = INT_MAX;

	for (i = regs->first_free; i < regs->max_regs; i++) {
		if (regs->phys_to_virt[i] == *reg)
			return i;
	}

	return retval;
}

static void prv_add_spill_point(subtilis_rv_reg_class_t *regs,
				subtilis_rv_spill_point_type_t type, size_t pos,
				int32_t offset, subtilis_rv_reg_t phys,
				subtilis_error_t *err)
{
	subtilis_rv_spill_point_t *sp;
	size_t new_max;

	if (regs->spill_points_count == regs->spill_points_max) {
		new_max = regs->spill_points_count + SUBTILIS_CONFIG_SSS_GRAN;
		sp = realloc(regs->spill_points, new_max * sizeof(*sp));
		if (!sp) {
			subtilis_error_set_oom(err);
			return;
		}
		regs->spill_points = sp;
		regs->spill_points_max = new_max;
	}
	sp = &regs->spill_points[regs->spill_points_count++];
	sp->type = type;
	sp->pos = pos;
	sp->offset = offset;
	sp->phys = phys;
}

static void prv_load_spilled_reg(subtilis_rv_reg_ud_t *ud,
				 subtilis_rv_op_t *current,
				 subtilis_rv_reg_class_t *regs,
				 subtilis_rv_reg_t virt,
				 subtilis_rv_reg_t phys, subtilis_error_t *err)
{
	int i;
	size_t pos;
	subtilis_rv_section_t *rv_s = ud->rv_s;
	int32_t basic_offset = regs->spilt_regs[virt];

	if (basic_offset == INT_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (regs->spill_top == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (current->prev == SIZE_MAX)
		pos = rv_s->first_op;
	else
		pos = rv_s->op_pool->ops[current->prev].next;

	prv_add_spill_point(regs, SUBTILIS_RV_SPILL_POINT_LOAD, pos,
			    basic_offset, phys, err);

	regs->phys_to_virt[phys] = virt;
	regs->spilt_regs[virt] = INT_MAX;
	i = basic_offset / regs->reg_size;
	regs->spill_stack[i] = basic_offset;
	if (i == regs->spill_top - 1) {
		regs->spill_top--;
		for (i = i - 1; i >= 0; i--) {
			if (regs->spill_stack[i] == INT_MAX)
				break;
			regs->spill_top--;
		}
	}
	regs->next[phys] = -1;
}

static void prv_spill_reg(subtilis_rv_reg_ud_t *ud, subtilis_rv_op_t *current,
			  size_t assigned, subtilis_rv_reg_class_t *int_regs,
			  subtilis_rv_reg_class_t *regs,
			  subtilis_rv_reg_t reg, subtilis_error_t *err)
{
	size_t i;
	int32_t offset;
	size_t pos;
	subtilis_rv_section_t *rv_s = ud->rv_s;

	if (regs->spill_top == regs->vr_reg_count) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	for (i = 0; i < regs->spill_top; i++)
		if (regs->spill_stack[i] != INT_MAX)
			break;

	offset = (int32_t)regs->spill_stack[i];
	regs->spill_stack[i] = INT_MAX;
	if (i == regs->spill_top) {
		regs->spill_top++;
		if (regs->spill_max < regs->spill_top)
			regs->spill_max = regs->spill_top;
	}

	regs->spilt_regs[assigned] = offset;

	if (current->prev == SIZE_MAX)
		pos = rv_s->first_op;
	else
		pos = rv_s->op_pool->ops[current->prev].next;

	prv_add_spill_point(regs, SUBTILIS_RV_SPILL_POINT_STORE, pos, offset,
			    reg, err);
}

static void prv_allocate_fixed(subtilis_rv_reg_ud_t *ud,
			       subtilis_rv_op_t *current,
			       subtilis_rv_reg_class_t *int_regs,
			       subtilis_rv_reg_class_t *regs,
			       subtilis_rv_reg_t *reg, subtilis_error_t *err)
{
	size_t assigned;

	assigned = regs->phys_to_virt[*reg];

	/*
	 * If the fixed register is already assigned to itself there's
	 * no need to spill it.
	 */

	if (assigned != INT_MAX && assigned != *reg)
		prv_spill_reg(ud, current, assigned, int_regs, regs, *reg, err);
}

static void
prv_allocate_floating(subtilis_rv_reg_ud_t *ud, subtilis_rv_op_t *current,
		      subtilis_rv_reg_class_t *int_regs,
		      subtilis_rv_reg_class_t *regs, subtilis_rv_reg_t *reg,
		      subtilis_error_t *err)
{
	subtilis_rv_reg_t target_reg;
	int i;
	int next = regs->max_regs;
	int max_next = -1;

	/* Virtual register is not already assigned. */

	/*
	 * TODO maybe we should choose a better ordering here for rv.
	 * but this should be regs specific as it is not needed for the
	 * floating point registers.  What we have here is fine for
	 * floating point, but sub optimal for rv32i.
	 */

	for (i = regs->max_regs - 1; i >= regs->first_free; i--)
		if (regs->phys_to_virt[i] == INT_MAX)
			break;

	if (i >= 0) {
		target_reg = (size_t)i;
	} else {
		/* No free physical regs.  Need to spill. */

		for (i = regs->first_free; i < regs->max_regs; i++)
			if (regs->next[i] > max_next) {
				max_next = regs->next[i];
				next = i;
			}
		if (next == regs->max_regs) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		target_reg = next;
		prv_spill_reg(ud, current, regs->phys_to_virt[next], int_regs,
			      regs, target_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	*reg = target_reg;
}

static void prv_rv_reg_alloc_alloc(subtilis_rv_reg_ud_t *ud,
				   subtilis_rv_op_t *current,
				   subtilis_rv_reg_class_t *int_regs,
				   subtilis_rv_reg_class_t *regs,
				   subtilis_rv_reg_t *reg,
				   subtilis_error_t *err)
{
	size_t assigned;
	size_t virt_num = *reg;

	if (*reg < regs->max_regs) {
		if (*reg < regs->first_free)
			return;
		prv_allocate_fixed(ud, current, int_regs, regs, reg, err);
	} else {
		assigned = prv_virt_to_phys(regs, reg);
		if (assigned != INT_MAX) {
			*reg = assigned;
		} else {
			prv_allocate_floating(ud, current, int_regs, regs, reg,
					      err);
		}
	}

	if (err->type != SUBTILIS_ERROR_OK)
		return;

	regs->phys_to_virt[*reg] = virt_num;
}

/* Returns true if register is of fixed use, e.g., X0. */

static bool prv_rv_reg_alloc_ensure(subtilis_rv_reg_ud_t *ud,
				    subtilis_rv_op_t *current,
				    subtilis_rv_reg_class_t *int_regs,
				    subtilis_rv_reg_class_t *regs,
				    subtilis_rv_reg_t *reg,
				    subtilis_error_t *err)
{
	size_t assigned;
	subtilis_rv_reg_t target_reg;

	if (*reg < regs->max_regs) {
		/*
		 * Register has fixed use and is unavailable to user's code
		 * Physical register has already been allocated, e.g., x8, x1
		 */

		if (*reg < regs->first_free)
			return true;

		assigned = regs->phys_to_virt[*reg];
		if (assigned != *reg) {
			prv_allocate_fixed(ud, current, int_regs, regs, reg,
					   err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(ud, current, regs, *reg, *reg,
					     err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
		}
	} else {
		assigned = prv_virt_to_phys(regs, reg);
		if (assigned != INT_MAX) {
			*reg = assigned;
		} else {
			target_reg = *reg;
			prv_allocate_floating(ud, current, int_regs, regs,
					      &target_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_load_spilled_reg(ud, current, regs, *reg,
					     target_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			*reg = target_reg;
		}
	}

	return false;
}

static void prv_check_current_ss(subtilis_rv_reg_ud_t *ud,
				 subtilis_rv_op_t *op, subtilis_error_t *err)
{
	if (op != ud->ss_terminators[ud->current_ss])
		return;

	if (ud->current_ss == ud->max_ss) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ud->current_ss++;
}

static int prv_rv_reg_alloc_calculate_dist(subtilis_rv_reg_ud_t *ud,
					   size_t reg_num,
					   subtilis_rv_op_t *op,
					   subtilis_rv_walker_t *walker,
					   subtilis_rv_reg_class_t *regs)
{
	subtilis_error_t err;
	subtilis_rv_op_t *to;

	subtilis_error_init(&err);

	if (op->next == SIZE_MAX)
		return -1;

	op = &ud->rv_s->op_pool->ops[op->next];

	/*
	 * If we're calculating the distance for a virtual register
	 * we only need to go to the end of the basic block and not
	 * to the end of the function.  For fixed registers we need
	 * to go to the end of the function as these registers aren't
	 * currently preserved and restored as we move between basic
	 * blocks, although perhaps they should be.
	 */

	if (ud->ss_terminators && reg_num >= regs->max_regs)
		to = ud->ss_terminators[ud->current_ss];
	else
		to = NULL;
	ud->dist_data.reg_num = reg_num;
	ud->dist_data.last_used = ud->instr_count + 1;

	subtilis_rv_walk_from_to(ud->rv_s, walker, op, to, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		return ud->dist_data.last_used;
	else
		return -1;
}

static void prv_rv_reg_alloc_alloc_int_dest(subtilis_rv_reg_ud_t *ud,
					    subtilis_rv_op_t *op,
					    subtilis_rv_reg_t *dest,
					    subtilis_error_t *err)
{
	int dist_dest;
	size_t vreg_dest = *dest;

	if (vreg_dest < SUBTILIS_RV_INT_FIRST_FREE)
		return;

	prv_rv_reg_alloc_alloc(ud, op, ud->int_regs, ud->int_regs, dest, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dist_dest = prv_rv_reg_alloc_calculate_dist(
	    ud, vreg_dest, op, &ud->int_regs->dist_walker, ud->int_regs);
	if (dist_dest == -1)
		ud->int_regs->phys_to_virt[*dest] = INT_MAX;
	ud->int_regs->next[*dest] = dist_dest;
}

static void prv_rv_reg_alloc_alloc_fp_dest(subtilis_rv_reg_ud_t *ud,
					   subtilis_rv_op_t *op,
					   subtilis_rv_reg_t *dest,
					   subtilis_error_t *err)
{
	int dist_dest;
	size_t vreg_dest = *dest;

	prv_rv_reg_alloc_alloc(ud, op, ud->int_regs, ud->real_regs, dest,
			       err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dist_dest = prv_rv_reg_alloc_calculate_dist(
	    ud, vreg_dest, op, &ud->real_regs->dist_walker, ud->real_regs);
	if (dist_dest == -1)
		ud->real_regs->phys_to_virt[*dest] = INT_MAX;
	ud->real_regs->next[*dest] = dist_dest;
}

static void prv_alloc_label(void *user_data, subtilis_rv_op_t *op, size_t label,
			    subtilis_error_t *err)
{
	subtilis_rv_reg_ud_t *ud = user_data;

	prv_check_current_ss(ud, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

static void prv_alloc_directive(void *user_data, subtilis_rv_op_t *op,
				subtilis_error_t *err)
{
}

static void prv_alloc_r(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_rtype_t *r, subtilis_error_t *err)
{
	int dist_rs1;
	int dist_rs2;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	size_t vreg_rs2;
	bool fixed_reg_rs2;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = r->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &r->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs2 = r->rs2;
	fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &r->rs2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs1 == -1)
			ud->int_regs->phys_to_virt[r->rs1] = INT_MAX;
	}

	if (!fixed_reg_rs2) {
		dist_rs2 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs2, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs2 == -1)
			ud->int_regs->phys_to_virt[r->rs2] = INT_MAX;
	}

	prv_rv_reg_alloc_alloc_int_dest(ud, op, &r->rd, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1)
		ud->int_regs->next[r->rs1] = dist_rs1;
	if (!fixed_reg_rs2)
		ud->int_regs->next[r->rs2] = dist_rs2;

	ud->instr_count++;
}

static void prv_alloc_i(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_itype_t *i, subtilis_error_t *err)
{
	int dist_rs1;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	subtilis_rv_reg_t ret;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = i->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &i->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs1 == -1)
			ud->int_regs->phys_to_virt[i->rs1] = INT_MAX;
	}

	prv_rv_reg_alloc_alloc_int_dest(ud, op, &i->rd, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * We need to make sure that A0 or FA0 are marked as being
	 * allocated when we return from a function.  Otherwise we
	 * can get an assertion when we ensure them later in the code.
	 */

	if (itype == SUBTILIS_RV_JALR) {
		if (i->link_type == SUBTILIS_RV_JAL_LINK_INT) {
			ret = SUBTILIS_RV_REG_A0;
			prv_rv_reg_alloc_alloc_int_dest(ud, op, &ret, err);
		}
		else if (i->link_type == SUBTILIS_RV_JAL_LINK_REAL) {
			ret = SUBTILIS_RV_REG_FA0;
			prv_rv_reg_alloc_alloc_fp_dest(ud, op, &ret, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (!fixed_reg_rs1)
		ud->int_regs->next[i->rs1] = dist_rs1;

	ud->instr_count++;
}

static void prv_alloc_sb(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_sbtype_t *sb, subtilis_error_t *err)
{
	int dist_rs1;
	int dist_rs2;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	size_t vreg_rs2;
	bool fixed_reg_rs2;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = sb->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &sb->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs2 = sb->rs2;
	fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->int_regs, &sb->rs2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs1 == -1)
			ud->int_regs->phys_to_virt[sb->rs1] = INT_MAX;
	}

	if (!fixed_reg_rs2) {
		dist_rs2 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs2, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs2 == -1)
			ud->int_regs->phys_to_virt[sb->rs2] = INT_MAX;
	}

	if (!fixed_reg_rs1)
		ud->int_regs->next[sb->rs1] = dist_rs1;
	if (!fixed_reg_rs2)
		ud->int_regs->next[sb->rs2] = dist_rs2;

	ud->instr_count++;
}

static void prv_alloc_uj(void *user_data, subtilis_rv_op_t *op,
			 subtilis_rv_instr_type_t itype,
			 subtilis_rv_instr_encoding_t etype,
			 rv_ujtype_t *uj, subtilis_error_t *err)
{
	subtilis_rv_reg_t ret;
	subtilis_rv_reg_ud_t *ud = user_data;

	prv_rv_reg_alloc_alloc_int_dest(ud, op, &uj->rd, err);

	/*
	 * We need to make sure that A0 or FA0 are marked as being
	 * allocated when we return from a function.  Otherwise we
	 * can get an assertion when we ensure them later in the code.
	 */

	if (itype == SUBTILIS_RV_JAL) {
		if (uj->link_type == SUBTILIS_RV_JAL_LINK_INT) {
			ret = SUBTILIS_RV_REG_A0;
			prv_rv_reg_alloc_alloc_int_dest(ud, op, &ret, err);
		}
		else if (uj->link_type == SUBTILIS_RV_JAL_LINK_REAL) {
			ret = SUBTILIS_RV_REG_FA0;
			prv_rv_reg_alloc_alloc_fp_dest(ud, op, &ret, err);
		}
	}
}

static void prv_alloc_real_r(void *user_data, subtilis_rv_op_t *op,
			     subtilis_rv_instr_type_t itype,
			     subtilis_rv_instr_encoding_t etype,
			     rv_rrtype_t *rr, subtilis_error_t *err)
{
	int dist_rs1;
	int dist_rs2;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	size_t vreg_rs2;
	bool fixed_reg_rs2;

	subtilis_rv_reg_ud_t *ud = user_data;

	switch (itype) {
	case SUBTILIS_RV_FCVT_W_S:
	case SUBTILIS_RV_FCVT_WU_S:
	case SUBTILIS_RV_FCVT_W_D:
	case SUBTILIS_RV_FCVT_WU_D:
	case SUBTILIS_RV_FMV_X_W:
	case SUBTILIS_RV_FCLASS_S:
	case SUBTILIS_RV_FCLASS_D:
		/*
		 * These all have a floating point rs1 and an integer
		 * rd.
		 */

		vreg_rs1 = rr->rs1;
		fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1) {
			dist_rs1 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs1, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs1 == -1)
				ud->real_regs->phys_to_virt[rr->rs1] = INT_MAX;
		}

		prv_rv_reg_alloc_alloc_int_dest(ud, op, &rr->rd, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1)
			ud->real_regs->next[rr->rs1] = dist_rs1;
		break;
	case SUBTILIS_RV_FEQ_S:
	case SUBTILIS_RV_FLT_S:
	case SUBTILIS_RV_FLE_S:
	case SUBTILIS_RV_FEQ_D:
	case SUBTILIS_RV_FLT_D:
	case SUBTILIS_RV_FLE_D:
		/*
		 * These all have two floating point sources, rs1 and rs2
		 * and an integer rd.
		 */

		vreg_rs1 = rr->rs1;
		fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		vreg_rs2 = rr->rs2;
		fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1) {
			dist_rs1 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs1, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs1 == -1)
				ud->real_regs->phys_to_virt[rr->rs1] = INT_MAX;
		}

		if (!fixed_reg_rs2) {
			dist_rs2 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs2, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs2 == -1)
				ud->real_regs->phys_to_virt[rr->rs2] = INT_MAX;
		}

		prv_rv_reg_alloc_alloc_int_dest(ud, op, &rr->rd, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1)
			ud->real_regs->next[rr->rs1] = dist_rs1;

		if (!fixed_reg_rs2)
			ud->real_regs->next[rr->rs2] = dist_rs2;
		break;
	case SUBTILIS_RV_FSQRT_S:
	case SUBTILIS_RV_FSQRT_D:
	case SUBTILIS_RV_FCVT_S_D:
	case SUBTILIS_RV_FCVT_D_S:
		/*
		 * These all have rd and rs1 that are float registers.
		 */

		vreg_rs1 = rr->rs1;
		fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1) {
			dist_rs1 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs1, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs1 == -1)
				ud->real_regs->phys_to_virt[rr->rs1] = INT_MAX;
		}

		prv_rv_reg_alloc_alloc_fp_dest(ud, op, &rr->rd, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1)
			ud->real_regs->next[rr->rs1] = dist_rs1;
		break;
	case SUBTILIS_RV_FCVT_S_W:
	case SUBTILIS_RV_FCVT_S_WU:
	case SUBTILIS_RV_FCVT_D_W:
	case SUBTILIS_RV_FCVT_D_WU:
	case SUBTILIS_RV_FMV_W_X:
		/*
		 * rd is a floating point register, rs1 is an int.
		 */

		vreg_rs1 = rr->rs1;
		fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->int_regs, &rr->rs1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1) {
			dist_rs1 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs1, op, &ud->int_regs->dist_walker,
				ud->int_regs);
			if (dist_rs1 == -1)
				ud->int_regs->phys_to_virt[rr->rs1] = INT_MAX;
		}

		prv_rv_reg_alloc_alloc_fp_dest(ud, op, &rr->rd, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1)
			ud->int_regs->next[rr->rs1] = dist_rs1;
		break;
	default:
		/*
		 * Everything else has three floating point registers.
		 */

		vreg_rs1 = rr->rs1;
		fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		vreg_rs2 = rr->rs2;
		fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
			ud, op, ud->int_regs, ud->real_regs, &rr->rs2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1) {
			dist_rs1 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs1, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs1 == -1)
				ud->real_regs->phys_to_virt[rr->rs1] = INT_MAX;
		}

		if (!fixed_reg_rs2) {
			dist_rs2 = prv_rv_reg_alloc_calculate_dist(
				ud, vreg_rs2, op, &ud->real_regs->dist_walker,
				ud->real_regs);
			if (dist_rs2 == -1)
				ud->real_regs->phys_to_virt[rr->rs2] = INT_MAX;
		}

		prv_rv_reg_alloc_alloc_fp_dest(ud, op, &rr->rd, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!fixed_reg_rs1)
			ud->real_regs->next[rr->rs1] = dist_rs1;

		if (!fixed_reg_rs2)
			ud->real_regs->next[rr->rs2] = dist_rs2;

		break;
	}

	ud->instr_count++;
}

static void prv_alloc_real_r4(void *user_data, subtilis_rv_op_t *op,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_instr_encoding_t etype,
			      rv_r4type_t *r4, subtilis_error_t *err)
{
	int dist_rs1;
	int dist_rs2;
	int dist_rs3;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	size_t vreg_rs2;
	bool fixed_reg_rs2;
	size_t vreg_rs3;
	bool fixed_reg_rs3;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = r4->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->real_regs, &r4->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs2 = r4->rs2;
	fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->real_regs, &r4->rs2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs3 = r4->rs3;
	fixed_reg_rs3 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->real_regs, &r4->rs3, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->real_regs->dist_walker,
			ud->real_regs);
		if (dist_rs1 == -1)
			ud->real_regs->phys_to_virt[r4->rs1] = INT_MAX;
	}

	if (!fixed_reg_rs2) {
		dist_rs2 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs2, op, &ud->real_regs->dist_walker,
			ud->real_regs);
		if (dist_rs2 == -1)
			ud->real_regs->phys_to_virt[r4->rs2] = INT_MAX;
	}

	if (!fixed_reg_rs3) {
		dist_rs3 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs3, op, &ud->real_regs->dist_walker,
			ud->real_regs);
		if (dist_rs3 == -1)
			ud->real_regs->phys_to_virt[r4->rs3] = INT_MAX;
	}

	prv_rv_reg_alloc_alloc_fp_dest(ud, op, &r4->rd, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1)
		ud->real_regs->next[r4->rs1] = dist_rs1;
	if (!fixed_reg_rs2)
		ud->real_regs->next[r4->rs2] = dist_rs2;
	if (!fixed_reg_rs3)
		ud->real_regs->next[r4->rs3] = dist_rs3;

	ud->instr_count++;
}

static void prv_alloc_real_i(void *user_data, subtilis_rv_op_t *op,
			     subtilis_rv_instr_type_t itype,
			     subtilis_rv_instr_encoding_t etype,
			     rv_itype_t *i, subtilis_error_t *err)
{
	int dist_rs1;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = i->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
		ud, op, ud->int_regs, ud->int_regs, &i->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs1 == -1)
			ud->int_regs->phys_to_virt[i->rs1] = INT_MAX;
	}

	prv_rv_reg_alloc_alloc_fp_dest(ud, op, &i->rd, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1)
		ud->int_regs->next[i->rs1] = dist_rs1;

	ud->instr_count++;
}

static void prv_alloc_real_s(void *user_data, subtilis_rv_op_t *op,
			     subtilis_rv_instr_type_t itype,
			     subtilis_rv_instr_encoding_t etype,
			     rv_sbtype_t *sb, subtilis_error_t *err)
{
	int dist_rs1;
	int dist_rs2;
	size_t vreg_rs1;
	bool fixed_reg_rs1;
	size_t vreg_rs2;
	bool fixed_reg_rs2;
	subtilis_rv_reg_ud_t *ud = user_data;

	vreg_rs1 = sb->rs1;
	fixed_reg_rs1 = prv_rv_reg_alloc_ensure(
		ud, op, ud->int_regs, ud->int_regs, &sb->rs1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	vreg_rs2 = sb->rs2;
	fixed_reg_rs2 = prv_rv_reg_alloc_ensure(
	    ud, op, ud->int_regs, ud->real_regs, &sb->rs2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!fixed_reg_rs1) {
		dist_rs1 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs1, op, &ud->int_regs->dist_walker,
			ud->int_regs);
		if (dist_rs1 == -1)
			ud->int_regs->phys_to_virt[sb->rs1] = INT_MAX;
	}

	if (!fixed_reg_rs2) {
		dist_rs2 = prv_rv_reg_alloc_calculate_dist(
			ud, vreg_rs2, op, &ud->real_regs->dist_walker,
			ud->real_regs);
		if (dist_rs2 == -1)
			ud->real_regs->phys_to_virt[sb->rs2] = INT_MAX;
	}

	if (!fixed_reg_rs1)
		ud->int_regs->next[sb->rs1] = dist_rs1;
	if (!fixed_reg_rs2)
		ud->real_regs->next[sb->rs2] = dist_rs2;

	ud->instr_count++;
}

static void prv_alloc_ldrc_f(void *user_data, subtilis_rv_op_t *op,
			     subtilis_rv_instr_type_t itype,
			     subtilis_rv_instr_encoding_t etype,
			     rv_ldrctype_t *ldrc, subtilis_error_t *err)
{
	subtilis_rv_reg_ud_t *ud = user_data;

	prv_rv_reg_alloc_alloc_fp_dest(ud, op, &ldrc->rd, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_rv_reg_alloc_alloc_int_dest(ud, op, &ldrc->rd2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->instr_count++;
}

size_t subtilis_rv_reg_alloc(subtilis_rv_section_t *rv_s,
			      subtilis_error_t *err)
{
	subtilis_rv_walker_t walker;
	subtilis_rv_reg_ud_t ud;
	int32_t offset;
	int32_t adjusted_offset;
	int32_t arg_offset;
	int32_t int_spill_space;
	int32_t real_spill_space;
	int32_t stack_space;
	int32_t stack_rem;
	int32_t arg_adjust;

	prv_init_rv_reg_ud(&ud, rv_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	/*
	 * Compute basic blocks and spill live registers at the end
	 * of each block.  We don't need to unspill registers as the
	 * allocation phase will do this for us.
	 */

	prv_link_basic_blocks(&ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	walker.user_data = &ud;
	walker.label_fn = prv_alloc_label;
	walker.directive_fn = prv_alloc_directive;
	walker.r_fn = prv_alloc_r;
	walker.i_fn = prv_alloc_i;
	walker.sb_fn = prv_alloc_sb;
	walker.uj_fn = prv_alloc_uj;
	walker.real_r_fn = prv_alloc_real_r;
	walker.real_r4_fn = prv_alloc_real_r4;
	walker.real_i_fn = prv_alloc_real_i;
	walker.real_s_fn = prv_alloc_real_s;
	walker.real_ldrc_f_fn = prv_alloc_ldrc_f;

	/*
	 * Perform the register allocation and record the positions in the
	 * code where spill code needs to be inserted, if for example we've
	 * run out of registers.  We don't insert the spill code on the fly
	 * for fear that it will predujice our distance calculations.
	 */

	subtilis_rv_walk(rv_s, &walker, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * See the description of the stack layout above to understand
	 * this code.  The register allocators assume they are sole
	 * owners of the stack and that any arguments of their type
	 * are the first offsets spilled.  This assumption is invalid
	 * if more than one type of argument is passed on the stack or
	 * virtual registers of any type are actually spilled.  So we
	 * need to adjust the offsets of the spill code before we
	 * insert it.  There are two offsets one for spilled registers
	 * and one for spilled arguments.  The register allocators
	 * treat spilled arguments as being contiguous with their spilled
	 * registers but this is not necessarily the case, hence the
	 * two offsets.
	 */

	/*
	 * Note the space allocated to spill arguments by the register
	 * allocators won't actually be used as their values are stored
	 * further up the stack.
	 */

	/*
	 * There are two types of spills, spills that occur to preserve live
	 * registers between basic blocks and spills that occur when we actually
	 * run out of physical registers.
	 */

	/*
	 * We subtract the space for arguments pushed on the stack as they
	 * are not stored in our spill space but on the stack area of the
	 * previous function call.
	 */

	int_spill_space = (ud.int_regs->spill_max - ud.int_regs->spilt_args) *
			  ud.int_regs->reg_size;

	real_spill_space =
	    (ud.real_regs->spill_max - ud.real_regs->spilt_args) *
	    ud.real_regs->reg_size;

	stack_space = int_spill_space + real_spill_space +
		ud.basic_block_spill + rv_s->locals;

	/*
	 * The stack is always 8 byte aligned when we enter a procedure.  If
	 * we spill an odd number of integer arguments, it will no longer be
	 * 8 byte aligned, so we need an adjustment.
	 */

	arg_adjust = 0;
	if (ud.int_regs->spilt_args & 1)
		arg_adjust += ud.int_regs->reg_size;

	/*
	 * Let's make sure that the stack is always 8 byte aligned.
	 */

	stack_rem = stack_space & 7;
	if (stack_rem > 0) {
		stack_space -= stack_rem;
		stack_space += 8;
	}

	offset = ud.basic_block_spill + rv_s->locals;

	/*
	 * The register allocators assign offsets to spilled registers after
	 * they have assigned offsets to any arguments passed on the stack.
	 * However, such arguments are not in reality stored as part of the
	 * virtual registers spill space, so we need to subtract these
	 * arguments to correctly compute the adjusted offset.
	 */

	/*
	 * These offsets are relative to x8 after the stack has been reduced,
	 * i.e., after it's been initialised to x2 - stack_space.
	 */

	adjusted_offset =
	    offset - (ud.real_regs->spilt_args * ud.real_regs->reg_size);
	arg_offset = stack_space + 8;
	prv_insert_spill_code(&ud, ud.real_regs, adjusted_offset, arg_offset,
			      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	offset += real_spill_space;
	adjusted_offset =
	    offset - (ud.int_regs->spilt_args * ud.int_regs->reg_size);
	arg_offset += ud.real_regs->spilt_args * ud.real_regs->reg_size +
		arg_adjust;

	prv_insert_spill_code(&ud, ud.int_regs, adjusted_offset, arg_offset,
			      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	rv_s->reg_counter = 32;
	rv_s->freg_counter = 32;

	prv_free_rv_reg_ud(&ud);

	return stack_space;

cleanup:

	prv_free_rv_reg_ud(&ud);

	return 0;

}

