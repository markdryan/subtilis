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

#include "reference_type.h"
#include "string_type.h"
#include "string_type_if.h"
#include "symbol_table.h"

static subtilis_exp_t *prv_exp_to_var_const(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	size_t reg;
	const subtilis_symbol_t *s;
	subtilis_type_t type;

	type.type = SUBTILIS_TYPE_STRING;

	s = subtilis_symbol_table_insert_tmp(p->local_st, &type, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_new_ref(p, &e->type, SUBTILIS_IR_REG_LOCAL, s->loc,
				     e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&s->t, reg, err);

cleanup:

	subtilis_exp_delete(e);

	return NULL;
}

/* clang-format off */
subtilis_type_if subtilis_type_if_const_string = {
	.is_const = true,
	.is_numeric = false,
	.is_integer = false,
	.param_type = SUBTILIS_IR_REG_TYPE_INTEGER,
	.size = NULL,
	.data_size = NULL,
	.zero = NULL,
	.zero_ref = NULL,
	.new_ref = NULL,
	.assign_ref = NULL,
	.zero_reg = NULL,
	.array_of = NULL,
	.element_type = NULL,
	.exp_to_var = prv_exp_to_var_const,
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
	.print = subtilis_string_type_print_const,
};

/* clang-format on */

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return e;
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
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
	.exp_to_var = prv_exp_to_var,
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
	.print = subtilis_string_type_print,
};

/* clang-format on */
