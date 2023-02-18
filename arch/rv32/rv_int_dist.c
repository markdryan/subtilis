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

#include "rv_int_dist.h"

static void prv_dist_r(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_rtype_t *r, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((r->rs1 == ud->reg_num) || (r->rs2 == ud->reg_num)) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (r->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_i(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_itype_t *i, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (i->rs1 == ud->reg_num) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	if (i->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_sb(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_sbtype_t *sb, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if ((sb->rs1 == ud->reg_num) || (sb->rs2 == ud->reg_num)) {
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_uj(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_ujtype_t *uj, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (uj->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_label_type(void *user_data, subtilis_rv_op_t *op,
				subtilis_rv_instr_type_t itype,
				subtilis_rv_instr_encoding_t etype,
				rv_labeltype_t *label, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (label->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_dist_label(void *user_data, subtilis_rv_op_t *op, size_t label,
			   subtilis_error_t *err)
{
}

static void prv_dist_directive(void *user_data, subtilis_rv_op_t *op,
			       subtilis_error_t *err)
{
}

void subtilis_rv_init_dist_walker(subtilis_rv_walker_t *walker,
				  void *user_data)
{
	walker->user_data = user_data;
	walker->label_fn = prv_dist_label;
	walker->directive_fn = prv_dist_directive;
	walker->r_fn = prv_dist_r;
	walker->i_fn = prv_dist_i;
	walker->sb_fn = prv_dist_sb;
	walker->uj_fn = prv_dist_uj;
	walker->label_type_fn = prv_dist_label_type;
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

void subtilis_rv_regs_used_afterv(subtilis_rv_section_t *rv_s,
				  subtilis_rv_op_t *from,
				  subtilis_rv_op_t *to, size_t int_args,
				  size_t real_args, size_t count,
				  subtilis_regs_used_virt_t *used,
				  subtilis_error_t *err)
{
	size_t i;
	subtilis_dist_data_t dist_data;
	subtilis_rv_walker_t walker;

	subtilis_rv_init_dist_walker(&walker, &dist_data);
	count++;

	for (i = SUBTILIS_RV_INT_VIRT_REG_START; i < int_args; i++) {
		if (prv_is_reg_used_after_to(rv_s, &dist_data, i, count,
					     &walker, from, to)) {
			subtilis_bitset_set(&used->int_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	for (i = SUBTILIS_RV_INT_VIRT_REG_START; i < real_args; i++) {
		if (prv_is_reg_used_after_to(rv_s, &dist_data, i, count,
					     &walker, from, to)) {
			subtilis_bitset_set(&used->real_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}
