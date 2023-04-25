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

#include "rv_used.h"

#include "rv_int_dist.h"
#include "rv_int_used.h"
#include "rv_real_dist.h"
#include "rv_real_used.h"


static bool prv_is_reg_used_before(subtilis_rv_section_t *rv_s,
				   subtilis_dist_data_t *used_data,
				   size_t reg_num,
				   subtilis_rv_walker_t *used_walker,
				   subtilis_rv_op_t *from,
				   subtilis_rv_op_t *to)
{
	subtilis_error_t err;

	subtilis_error_init(&err);
	used_data->reg_num = reg_num;
	used_data->last_used = 0;
	subtilis_rv_walk_from_to(rv_s, used_walker, from, to, &err);
	if ((err.type == SUBTILIS_ERROR_OK) || (used_data->last_used != -1))
		return false;

	return true;
}

void subtilis_rv_int_regs_used_before_from_tov(subtilis_rv_section_t *rv_s,
					       subtilis_rv_op_t *from,
					       subtilis_rv_op_t *op,
					       size_t int_args,
					       subtilis_regs_used_virt_t *used,
					       subtilis_error_t *err)
{
	size_t i;
	subtilis_dist_data_t used_data;
	subtilis_rv_walker_t walker;

	subtilis_rv_init_used_walker(&walker, &used_data);

	for (i = SUBTILIS_RV_REG_MAX_INT_REGS; i < int_args; i++) {
		if (prv_is_reg_used_before(rv_s, &used_data, i, &walker, from,
					   op)) {
			subtilis_bitset_set(&used->int_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

void subtilis_rv_real_regs_used_before_from_tov(subtilis_rv_section_t *rv_s,
						subtilis_rv_op_t *from,
						subtilis_rv_op_t *op,
						size_t real_args,
						subtilis_regs_used_virt_t *used,
						subtilis_error_t *err)
{
	size_t i;
	subtilis_dist_data_t used_data;
	subtilis_rv_walker_t walker;

	subtilis_rv_init_real_used_walker(&walker, &used_data);

	for (i = SUBTILIS_RV_REG_MAX_REAL_REGS; i < real_args; i++) {
		if (prv_is_reg_used_before(rv_s, &used_data, i, &walker, from,
					   op)) {
			subtilis_bitset_set(&used->real_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

static bool prv_is_reg_used_after_to(subtilis_rv_section_t *rv_s,
				     subtilis_dist_data_t *dist_data,
				     size_t reg_num, size_t count,
				     subtilis_rv_walker_t *dist_walker,
				     subtilis_rv_op_t *from,
				     subtilis_rv_op_t *to)
{
	subtilis_error_t err;

	subtilis_error_init(&err);

	dist_data->reg_num = reg_num;
	dist_data->last_used = count + 1;

	subtilis_rv_walk_from_to(rv_s, dist_walker, from, to, &err);

	/*
	 * Check that reg_num is used and that the  first usage of
	 * reg_num is not a write.
	 */

	return (err.type != SUBTILIS_ERROR_OK) &&
	       (dist_data->last_used != -1);
}


void subtilis_rv_int_regs_used_afterv(subtilis_rv_section_t *rv_s,
				      subtilis_rv_op_t *from,
				      subtilis_rv_op_t *to, size_t int_args,
				      size_t count,
				      subtilis_regs_used_virt_t *used,
				      subtilis_error_t *err)
{
	size_t i;
	subtilis_dist_data_t dist_data;
	subtilis_rv_walker_t walker;

	subtilis_rv_init_dist_walker(&walker, &dist_data);
	count++;

	for (i = SUBTILIS_RV_REG_MAX_INT_REGS; i < int_args; i++) {
		if (prv_is_reg_used_after_to(rv_s, &dist_data, i, count,
					     &walker, from, to)) {
			subtilis_bitset_set(&used->int_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

void subtilis_rv_real_regs_used_afterv(subtilis_rv_section_t *rv_s,
				       subtilis_rv_op_t *from,
				       subtilis_rv_op_t *to,
				       size_t real_args,
				       size_t count,
				       subtilis_regs_used_virt_t *used,
				       subtilis_error_t *err)
{
	size_t i;
	subtilis_dist_data_t dist_data;
	subtilis_rv_walker_t walker;

	subtilis_rv_init_real_dist_walker(&walker, &dist_data);
	count++;

	for (i = SUBTILIS_RV_REG_MAX_REAL_REGS; i < real_args; i++) {
		if (prv_is_reg_used_after_to(rv_s, &dist_data, i, count,
					     &walker, from, to)) {
			subtilis_bitset_set(&used->real_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

