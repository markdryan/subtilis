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

#ifndef __SUBTILIS_ARM_EXPRESSION_H
#define __SUBTILIS_ARM_EXPRESSION_H

#include "../../common/buffer.h"
#include "arm_core.h"

typedef enum {
	SUBTILIS_ARM_EXP_TYPE_FREG,
	SUBTILIS_ARM_EXP_TYPE_REG,
	SUBTILIS_ARM_EXP_TYPE_INT,
	SUBTILIS_ARM_EXP_TYPE_REAL,
	SUBTILIS_ARM_EXP_TYPE_STRING,
	SUBTILIS_ARM_EXP_TYPE_ID,
} subtilis_arm_exp_type_t;

struct subtilis_arm_exp_val_t_ {
	subtilis_arm_exp_type_t type;
	union {
		int32_t integer;
		double real;
		subtilis_buffer_t buf;
		subtilis_arm_reg_t reg;
	} val;
};

typedef struct subtilis_arm_exp_val_t_ subtilis_arm_exp_val_t;

typedef struct subtilis_arm_ass_context_t_ subtilis_arm_ass_context_t;

subtilis_arm_reg_t subtilis_arm_exp_parse_reg(subtilis_arm_ass_context_t *c,
					      const char *id,
					      subtilis_error_t *err);
subtilis_arm_reg_t subtilis_arm_exp_parse_freg(subtilis_arm_ass_context_t *c,
					       const char *id,
					       subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_pri7(subtilis_arm_ass_context_t *c,
					      subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_val_get(subtilis_arm_ass_context_t *c,
						 subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_int32(int32_t val,
						   subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_real(double val,
						  subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_str(subtilis_buffer_t *buf,
						 subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_str_str(const char *str,
						     subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_reg(subtilis_arm_reg_t reg,
						 subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_freg(subtilis_arm_reg_t reg,
						  subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_new_id(const char *id,
						subtilis_error_t *err);
subtilis_arm_exp_val_t *subtilis_arm_exp_dup(subtilis_arm_exp_val_t *val,
					     subtilis_error_t *err);
const char *subtilis_arm_exp_type_name(subtilis_arm_exp_val_t *val);
void subtilis_arm_exp_val_free(subtilis_arm_exp_val_t *val);

/* clang-format off */
subtilis_arm_exp_val_t *subtilis_arm_exp_process_id(
	subtilis_arm_ass_context_t *c, const char *id, subtilis_error_t *err);
/* clang-format on */

#endif
