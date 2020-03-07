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

#include <stdlib.h>
#include <string.h>

#include "string_type.h"
#include "string_type_if.h"

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return e;
}

/* clang-format off */
subtilis_type_if subtilis_type_if_string = {
	.is_const = false,
	.is_numeric = false,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = subtilis_string_type_size,
	.data_size = prv_data_size,
	.zero = NULL,
	.zero_ref = subtilis_string_type_zero_ref,
	.new_ref = subtilis_string_type_new_ref,
	.assign_ref = subtilis_string_type_assign_ref,
	.zero_reg = NULL,
	.array_of = NULL,
	.element_type = NULL,
	.exp_to_var = NULL,
	.copy_var = NULL,
	.dup = NULL,
	.assign_reg = NULL,
	.assign_mem = NULL,
	.indexed_write = NULL,
	.indexed_add = NULL,
	.indexed_sub = NULL,
	.indexed_read = NULL,
	.load_mem = NULL,
	.to_int32 = NULL,
	.to_float64 = NULL,
	.unary_minus = NULL,
	.add = NULL,
	.mul = NULL,
	.and = NULL,
	.or = NULL,
	.eor = NULL,
	.not = NULL,
	.eq = NULL,
	.neq = NULL,
	.sub = NULL,
	.div = NULL,
	.mod = NULL,
	.gt = NULL,
	.lte = NULL,
	.lt = NULL,
	.gte = NULL,
	.lsl = NULL,
	.lsr = NULL,
	.asr = NULL,
	.abs = NULL,
	.call = NULL,
	.ret = NULL,
};

/* clang-format on */
