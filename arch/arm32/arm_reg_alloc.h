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

#ifndef __SUBTILIS_ARM_REG_ALLOC_H
#define __SUBTILIS_ARM_REG_ALLOC_H

#include "../../common/bitset.h"
#include "arm_core.h"
#include "arm_walker.h"

struct subtilis_dist_data_t_ {
	size_t reg_num;
	int last_used;
};

typedef struct subtilis_dist_data_t_ subtilis_dist_data_t;

struct subtilis_regs_used_t_ {
	size_t int_regs;
	size_t real_regs;
};

typedef struct subtilis_regs_used_t_ subtilis_regs_used_t;

struct subtilis_regs_used_virt_t_ {
	subtilis_bitset_t int_regs;
	subtilis_bitset_t real_regs;
};

typedef struct subtilis_regs_used_virt_t_ subtilis_regs_used_virt_t;

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

typedef enum {
	SUBTILIS_SPILL_POINT_LOAD,
	SUBTILIS_SPILL_POINT_STORE,
} subtilis_spill_point_type_t;

struct subtilis_spill_point_t_ {
	subtilis_spill_point_type_t type;
	size_t pos;
	int32_t offset;
	subtilis_arm_reg_t phys;
};

typedef struct subtilis_spill_point_t_ subtilis_spill_point_t;

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
	size_t spilt_args;
	subtilis_arm_reg_spill_imm_t spill_imm;
	subtilis_arm_reg_stran_imm_t stran_imm;
	subtilis_arm_instr_type_t store_type;
	subtilis_arm_instr_type_t load_type;
	subtilis_arm_fp_is_fixed_t is_fixed;
	subtilis_spill_point_t *spill_points;
	size_t spill_points_count;
	size_t spill_points_max;
	subtlis_arm_walker_t dist_walker;
	subtlis_arm_walker_t used_walker;
};

typedef struct subtilis_arm_reg_class_t_ subtilis_arm_reg_class_t;

struct subtilis_arm_reg_ud_t_ {
	size_t basic_block_spill;
	subtilis_arm_reg_class_t *int_regs;
	subtilis_arm_reg_class_t *real_regs;
	subtilis_arm_section_t *arm_s;
	size_t instr_count;
	subtilis_dist_data_t dist_data;
	subtilis_arm_op_t **ss_terminators;
	size_t current_ss;
	size_t max_ss;
};

typedef struct subtilis_arm_reg_ud_t_ subtilis_arm_reg_ud_t;

void subtilis_regs_used_virt_init(subtilis_regs_used_virt_t *regs_usedv);
void subtilis_regs_used_virt_free(subtilis_regs_used_virt_t *regs_usedv);

size_t subtilis_arm_reg_alloc(subtilis_arm_section_t *arm_s,
			      subtilis_error_t *err);

void subtilis_arm_regs_used_before(subtilis_arm_section_t *arm_s,
				   subtilis_arm_op_t *op, size_t int_args,
				   size_t real_args,
				   subtilis_regs_used_t *regs_used,
				   subtilis_error_t *err);
void subtilis_arm_regs_used_after(subtilis_arm_section_t *arm_s,
				  subtilis_arm_op_t *op, size_t int_args,
				  size_t real_args,
				  subtilis_regs_used_t *regs_used,
				  subtilis_error_t *err);
void subtilis_arm_regs_used_before_from_tov(subtilis_arm_section_t *arm_s,
					    subtilis_arm_op_t *from,
					    subtilis_arm_op_t *op,
					    size_t int_args, size_t real_args,
					    subtilis_regs_used_virt_t *used,
					    subtilis_error_t *err);
void subtilis_arm_regs_used_afterv(subtilis_arm_section_t *arm_s,
				   subtilis_arm_op_t *from,
				   subtilis_arm_op_t *to, size_t int_args,
				   size_t real_args, size_t count,
				   subtilis_regs_used_virt_t *used,
				   subtilis_error_t *err);

void subtilis_arm_save_regs(subtilis_arm_section_t *arm_s,
			    subtilis_error_t *err);

bool subtilis_arm_reg_alloc_ensure(subtilis_arm_reg_ud_t *ud,
				   subtilis_arm_op_t *current,
				   subtilis_arm_reg_class_t *int_regs,
				   subtilis_arm_reg_class_t *regs,
				   subtilis_arm_reg_t *reg,
				   subtilis_error_t *err);
void subtilis_arm_reg_alloc_alloc(subtilis_arm_reg_ud_t *ud,
				  subtilis_arm_op_t *current,
				  subtilis_arm_reg_class_t *int_regs,
				  subtilis_arm_reg_class_t *regs,
				  subtilis_arm_reg_t *reg,
				  subtilis_arm_reg_t restricted,
				  subtilis_error_t *err);
void subtilis_arm_reg_alloc_alloc_fp_dest(subtilis_arm_reg_ud_t *ud,
					  subtilis_arm_op_t *op,
					  subtilis_arm_reg_t *dest,
					  subtilis_error_t *err);

int subtilis_arm_reg_alloc_calculate_dist(subtilis_arm_reg_ud_t *ud,
					  size_t reg_num, subtilis_arm_op_t *op,
					  subtlis_arm_walker_t *walker,
					  subtilis_arm_reg_class_t *regs);
void subtilis_arm_reg_alloc_alloc_dest(subtilis_arm_reg_ud_t *ud,
				       subtilis_arm_op_t *op,
				       subtilis_arm_reg_t *dest,
				       subtilis_error_t *err);

#endif
