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

#include "arm_reg_alloc.h"
#include "fpa_alloc.h"

#include <limits.h>

static void prv_alloc_fpa_data_dyadic_instr(void *user_data,
					    subtilis_arm_op_t *op,
					    subtilis_arm_instr_type_t type,
					    subtilis_fpa_data_instr_t *instr,
					    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_data_monadic_instr(void *user_data,
					     subtilis_arm_op_t *op,
					     subtilis_arm_instr_type_t type,
					     subtilis_fpa_data_instr_t *instr,
					     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_stran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_tran_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				    subtilis_arm_instr_type_t type,
				    subtilis_fpa_cmp_instr_t *instr,
				    subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_ldrc_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

static void prv_alloc_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_fpa_cptran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_alloc_init_walker(subtlis_arm_walker_t *walker,
				    void *user_data)
{
	walker->fpa_data_monadic_fn = prv_alloc_fpa_data_monadic_instr;
	walker->fpa_data_dyadic_fn = prv_alloc_fpa_data_dyadic_instr;
	walker->fpa_stran_fn = prv_alloc_fpa_stran_instr;
	walker->fpa_tran_fn = prv_alloc_fpa_tran_instr;
	walker->fpa_cmp_fn = prv_alloc_fpa_cmp_instr;
	walker->fpa_ldrc_fn = prv_alloc_fpa_ldrc_instr;
	walker->fpa_cptran_fn = prv_alloc_fpa_cptran_instr;
}
