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

#include "rv_int_used.h"

static void prv_used_r(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_rtype_t *r, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (r->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_i(void *user_data, subtilis_rv_op_t *op,
		       subtilis_rv_instr_type_t itype,
		       subtilis_rv_instr_encoding_t etype,
		       rv_itype_t *i, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (i->rd == ud->reg_num) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
}

static void prv_used_sb(void *user_data, subtilis_rv_op_t *op,
			subtilis_rv_instr_type_t itype,
			subtilis_rv_instr_encoding_t etype,
			rv_sbtype_t *sb, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_used_uj(void *user_data, subtilis_rv_op_t *op,
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

static void prv_used_label(void *user_data, subtilis_rv_op_t *op, size_t label,
			   subtilis_error_t *err)
{
}

static void prv_used_directive(void *user_data, subtilis_rv_op_t *op,
			       subtilis_error_t *err)
{
}

void subtilis_rv_init_used_walker(subtilis_rv_walker_t *walker,
				  void *user_data)
{
	walker->user_data = user_data;
	walker->label_fn = prv_used_label;
	walker->directive_fn = prv_used_directive;
	walker->r_fn = prv_used_r;
	walker->i_fn = prv_used_i;
	walker->sb_fn = prv_used_sb;
	walker->uj_fn = prv_used_uj;
}


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

void subtilis_rv_regs_used_before_from_tov(subtilis_rv_section_t *rv_s,
					   subtilis_rv_op_t *from,
					   subtilis_rv_op_t *op,
					   size_t int_args, size_t real_args,
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

	for (i = SUBTILIS_RV_REG_MAX_REAL_REGS; i < real_args; i++) {
		if (prv_is_reg_used_before(rv_s, &used_data, i, &walker, from,
					   op)) {
			subtilis_bitset_set(&used->real_regs, i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}
