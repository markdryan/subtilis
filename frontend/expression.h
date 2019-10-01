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

#include "../common/buffer.h"
#include "../common/error.h"
#include "../common/ir.h"
#include "parser.h"

#ifndef __SUBTILIS_EXP_H
#define __SUBTILIS_EXP_H

union subtilis_exp_operand_t_ {
	subtilis_ir_operand_t ir_op;
	subtilis_buffer_t str;
};

typedef union subtilis_exp_operand_t_ subtilis_exp_operand_t;

struct subtilis_exp_t_ {
	subtilis_type_t type;
	subtilis_exp_operand_t exp;
};

typedef struct subtilis_exp_t_ subtilis_exp_t;

void subtilis_exp_return_default_value(subtilis_parser_t *p,
				       subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_add_call(subtilis_parser_t *p, char *name,
				      subtilis_builtin_type_t ftype,
				      subtilis_type_section_t *stype,
				      subtilis_ir_arg_t *args,
				      const subtilis_type_t *fn_type,
				      size_t num_params, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_var(const subtilis_type_t *type,
				     unsigned int reg, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_int32_var(unsigned int reg,
					   subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_real_var(unsigned int reg,
					  subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_int32(int32_t integer, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_real(double real, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_str(subtilis_buffer_t *str,
				     subtilis_error_t *err);

subtilis_exp_t *subtilis_exp_coerce_type(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 const subtilis_type_t *type,
					 subtilis_error_t *err);
void subtilis_exp_handle_errors(subtilis_parser_t *p, subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_exp_fn_t)(subtilis_parser_t *,
					     subtilis_exp_t *, subtilis_exp_t *,
					     subtilis_error_t *err);

/* These functions assume ownership of a1 and a2 */
subtilis_exp_t *subtilis_exp_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
void subtilis_exp_delete(subtilis_exp_t *e);

/* Consumes e */

void subtilis_exp_generate_error(subtilis_parser_t *p, subtilis_exp_t *e,
				 subtilis_error_t *err);

#endif
