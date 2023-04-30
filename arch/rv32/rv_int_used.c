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

	if ((itype == SUBTILIS_RV_JALR) &&
	    (i->link_type == SUBTILIS_RV_JAL_LINK_INT) &&
	    (ud->reg_num == SUBTILIS_RV_REG_A0)) {
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

	/*
	 * There's an implicit overwrite of A0.
	 */

	if ((itype == SUBTILIS_RV_JAL) &&
	    (uj->link_type == SUBTILIS_RV_JAL_LINK_INT) &&
	    (ud->reg_num == SUBTILIS_RV_REG_A0)) {
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

static void prv_dist_real_r(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_rrtype_t *rr, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	switch (itype) {
	case SUBTILIS_RV_FCVT_W_S:
	case SUBTILIS_RV_FCVT_WU_S:
	case SUBTILIS_RV_FCVT_W_D:
	case SUBTILIS_RV_FCVT_WU_D:
	case SUBTILIS_RV_FMV_X_W:
	case SUBTILIS_RV_FEQ_S:
	case SUBTILIS_RV_FLT_S:
	case SUBTILIS_RV_FLE_S:
	case SUBTILIS_RV_FCLASS_S:
	case SUBTILIS_RV_FEQ_D:
	case SUBTILIS_RV_FLT_D:
	case SUBTILIS_RV_FLE_D:
	case SUBTILIS_RV_FCLASS_D:
		if (ud->reg_num == rr->rd) {
			ud->last_used = -1;
			subtilis_error_set_walker_failed(err);
			return;
		}
		break;
	default:
		break;
	}

	ud->last_used++;
}

static void prv_dist_real_r4(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_r4type_t *r4, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_real_i(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_itype_t *i, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_real_s(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_sbtype_t *sb, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	ud->last_used++;
}

static void prv_dist_ldrc_f(void *user_data, subtilis_rv_op_t *op,
			    subtilis_rv_instr_type_t itype,
			    subtilis_rv_instr_encoding_t etype,
			    rv_ldrctype_t *ldrc, subtilis_error_t *err)
{
	subtilis_dist_data_t *ud = user_data;

	if (ud->reg_num == ldrc->rd2) {
		ud->last_used = -1;
		subtilis_error_set_walker_failed(err);
		return;
	}

	ud->last_used++;
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
	walker->real_r_fn = prv_dist_real_r;
	walker->real_r4_fn = prv_dist_real_r4;
	walker->real_i_fn = prv_dist_real_i;
	walker->real_s_fn = prv_dist_real_s;
	walker->real_ldrc_f_fn = prv_dist_ldrc_f;
}
