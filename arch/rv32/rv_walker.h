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

#ifndef SUBTILIS_RV_WALKER_H
#define SUBTILIS_RV_WALKER_H

#include <stddef.h>

#include "../../common/error.h"
#include "rv32_core.h"

struct subtilis_rv_walker_t_ {
	void *user_data;
	void (*directive_fn)(void *user_data, subtilis_rv_op_t *op,
			     subtilis_error_t *err);
	void (*label_fn)(void *user_data, subtilis_rv_op_t *op, size_t label,
			 subtilis_error_t *err);
	void (*r_fn)(void *user_data, subtilis_rv_op_t *op,
		     subtilis_rv_instr_type_t itype,
		     subtilis_rv_instr_encoding_t etype,
		     rv_rtype_t *r, subtilis_error_t *err);
	void (*i_fn)(void *user_data, subtilis_rv_op_t *op,
		     subtilis_rv_instr_type_t itype,
		     subtilis_rv_instr_encoding_t etype,
		     rv_itype_t *i, subtilis_error_t *err);
	void (*sb_fn)(void *user_data, subtilis_rv_op_t *op,
		      subtilis_rv_instr_type_t itype,
		      subtilis_rv_instr_encoding_t etype,
		      rv_sbtype_t *sb, subtilis_error_t *err);
	void (*uj_fn)(void *user_data, subtilis_rv_op_t *op,
		      subtilis_rv_instr_type_t itype,
		      subtilis_rv_instr_encoding_t etype,
		      rv_ujtype_t *uj, subtilis_error_t *err);
};

typedef struct subtilis_rv_walker_t_ subtilis_rv_walker_t;

void subtilis_rv_walk(subtilis_rv_section_t *rv_s,
		      subtilis_rv_walker_t *walker, subtilis_error_t *err);
void subtilis_rv_walk_from(subtilis_rv_section_t *rv_s,
			   subtilis_rv_walker_t *walker, subtilis_rv_op_t *op,
			   subtilis_error_t *err);
void subtilis_rv_walk_from_to(subtilis_rv_section_t *rv_s,
			      subtilis_rv_walker_t *walker,
			      subtilis_rv_op_t *from, subtilis_rv_op_t *to,
			      subtilis_error_t *err);

#endif
