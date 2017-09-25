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

#include "buffer.h"
#include "error.h"
#include "ir.h"

#ifndef __SUBTILIS_EXP_H
#define __SUBTILIS_EXP_H

typedef enum {
	SUBTILIS_EXP_CONST_INTEGER,
	SUBTILIS_EXP_CONST_REAL,
	SUBTILIS_EXP_CONST_STRING,
	SUBTILIS_EXP_INTEGER,
	SUBTILIS_EXP_REAL,
	SUBTILIS_EXP_STRING,
} subtilis_exp_type_t;

union subtilis_exp_operand_t_ {
	subtilis_ir_operand_t ir_op;
	subtilis_buffer_t str;
};

typedef union subtilis_exp_operand_t_ subtilis_exp_operand_t;

struct subtilis_exp_t_ {
	subtilis_exp_type_t type;
	subtilis_exp_operand_t exp;
};

typedef struct subtilis_exp_t_ subtilis_exp_t;

subtilis_exp_t *subtilis_exp_new_var(subtilis_exp_type_t type, unsigned int reg,
				     subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_int32(int32_t integer, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_real(double real, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_new_str(subtilis_buffer_t *str,
				     subtilis_error_t *err);

typedef subtilis_exp_t *(*subtilis_exp_fn_t)(subtilis_ir_program_t *,
					     subtilis_exp_t *, subtilis_exp_t *,
					     subtilis_error_t *err);

/* These functions assume ownership of a1 and a2 */
subtilis_exp_t *subtilis_exp_add(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_sub(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_mul(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_div(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_and(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_or(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_eor(subtilis_ir_program_t *p, subtilis_exp_t *a1,
				 subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_unary_minus(subtilis_ir_program_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err);
subtilis_exp_t *subtilis_exp_not(subtilis_ir_program_t *p, subtilis_exp_t *e,
				 subtilis_error_t *err);

void subtilis_exp_delete(subtilis_exp_t *e);

#endif
