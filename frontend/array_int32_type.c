/*
 * Copyright (c) 2019 Mark Ryan
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

#include "array_int32_type.h"
#include "array_type.h"

static size_t prv_size(const subtilis_type_t *type)
{
	return subtilis_array_type_size(type);
}

static subtilis_exp_t *prv_data_size(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	subtilis_exp_t *two;

	two = subtilis_exp_new_int32(2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return subtilis_type_if_lsl(p, e, two, err);
}

static subtilis_exp_t *prv_zero(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "zero on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static void prv_zero_reg(subtilis_parser_t *p, size_t reg,
			 subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "zero reg on arrays",
					 p->l->stream->name, p->l->line);
}

static subtilis_exp_t *prv_exp_to_var(subtilis_parser_t *p, subtilis_exp_t *e,
				      subtilis_error_t *err)
{
	return e;
}

static void prv_indexed_write(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t *e,
			      subtilis_exp_t **indices, size_t index_count,
			      subtilis_error_t *err)
{
	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_array_write(p, var_name, type, mem_reg, loc, e, indices,
			     index_count, err);
}

static subtilis_exp_t *
prv_indexed_read(subtilis_parser_t *p, const char *var_name,
		 const subtilis_type_t *type, size_t mem_reg, size_t loc,
		 subtilis_exp_t **indices, size_t index_count,
		 subtilis_error_t *err)
{
	subtilis_exp_t *offset;
	subtilis_exp_t *e;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_type_if_load_from_mem(p, &subtilis_type_integer,
					   offset->exp.ir_op.reg, 0, err);
	subtilis_exp_delete(offset);
	return e;
}

static subtilis_exp_t *prv_unary_minus(subtilis_parser_t *p, subtilis_exp_t *e,
				       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "unary - on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_add(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "+ on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "* on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_and(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "AND on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_or(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "OR on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "EOR on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_not(subtilis_parser_t *p, subtilis_exp_t *e,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "NOT on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
			      subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "= on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, bool swapped,
			       subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "- on arrays", p->l->stream->name,
					 p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_div(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "DIV on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "MOD on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, "<< on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

static subtilis_exp_t *prv_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
			       subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_error_set_not_supported(err, ">>> on arrays",
					 p->l->stream->name, p->l->line);
	return NULL;
}

/* clang-format off */
subtilis_type_if subtilis_type_array_int32 = {
	.is_const = false,
	.size = prv_size,
	.data_size = prv_data_size,
	.zero = prv_zero,
	.zero_reg = prv_zero_reg,
	.exp_to_var = prv_exp_to_var,
	.copy_var = NULL,
	.dup = NULL,
	.assign_reg = NULL,
	.assign_mem = NULL,
	.indexed_write = prv_indexed_write,
	.indexed_read = prv_indexed_read,
	.load_mem = NULL,
	.to_int32 = NULL,
	.to_float64 = NULL,
	.unary_minus = prv_unary_minus,
	.add = prv_add,
	.mul = prv_mul,
	.and = prv_and,
	.or = prv_or,
	.eor = prv_eor,
	.not = prv_not,
	.eq = prv_eq,
	.neq = prv_neq,
	.sub = prv_sub,
	.div = prv_div,
	.mod = prv_mod,
	.gt = NULL,
	.lte = NULL,
	.lt = NULL,
	.gte = NULL,
	.lsl = prv_lsl,
	.lsr = prv_lsr,
	.asr = prv_asr,
};

/* clang-format on */
