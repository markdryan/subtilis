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

#ifndef __SUBTILIS_ARM_WALKER_H
#define __SUBTILIS_ARM_WALKER_H

#include <stddef.h>

#include "arm_core.h"
#include "error.h"

typedef struct subtlis_arm_walker_t_ subtlis_arm_walker_t;

struct subtlis_arm_walker_t_ {
	void *user_data;
	void (*label_fn)(void *user_data, subtilis_arm_op_t *op, size_t label,
			 subtilis_error_t *err);
	void (*data_fn)(void *user_data, subtilis_arm_op_t *op,
			subtilis_arm_instr_type_t type,
			subtilis_arm_data_instr_t *instr,
			subtilis_error_t *err);
	void (*cmp_fn)(void *user_data, subtilis_arm_op_t *op,
		       subtilis_arm_instr_type_t type,
		       subtilis_arm_data_instr_t *instr, subtilis_error_t *err);
	void (*mov_fn)(void *user_data, subtilis_arm_op_t *op,
		       subtilis_arm_instr_type_t type,
		       subtilis_arm_data_instr_t *instr, subtilis_error_t *err);
	void (*stran_fn)(void *user_data, subtilis_arm_op_t *op,
			 subtilis_arm_instr_type_t type,
			 subtilis_arm_stran_instr_t *instr,
			 subtilis_error_t *err);
	void (*mtran_fn)(void *user_data, subtilis_arm_op_t *op,
			 subtilis_arm_instr_type_t type,
			 subtilis_arm_mtran_instr_t *instr,
			 subtilis_error_t *err);
	void (*br_fn)(void *user_data, subtilis_arm_op_t *op,
		      subtilis_arm_instr_type_t type,
		      subtilis_arm_br_instr_t *instr, subtilis_error_t *err);
	void (*swi_fn)(void *user_data, subtilis_arm_op_t *op,
		       subtilis_arm_instr_type_t type,
		       subtilis_arm_swi_instr_t *instr, subtilis_error_t *err);
	void (*ldrc_fn)(void *user_data, subtilis_arm_op_t *op,
			subtilis_arm_instr_type_t type,
			subtilis_arm_ldrc_instr_t *instr,
			subtilis_error_t *err);
};

void subtilis_arm_walk(subtilis_arm_program_t *arm_p,
		       subtlis_arm_walker_t *walker, subtilis_error_t *err);
void subtilis_arm_walk_from(subtilis_arm_program_t *arm_p,
			    subtlis_arm_walker_t *walker, subtilis_arm_op_t *op,
			    subtilis_error_t *err);
#endif
