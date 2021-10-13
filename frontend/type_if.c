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

#include "array_byte_type.h"
#include "array_float64_type.h"
#include "array_int32_type.h"
#include "array_string_type.h"
#include "byte_type_if.h"
#include "float64_type.h"
#include "fn_type.h"
#include "int32_type.h"
#include "local_buffer_type_if.h"
#include "string_type_if.h"
#include "type_if.h"

/* clang-format off */
static subtilis_type_if *prv_type_map[] = {
	&subtilis_type_const_float64,
	&subtilis_type_const_int32,
	&subtilis_type_const_byte,
	&subtilis_type_if_const_string,
	&subtilis_type_float64,
	&subtilis_type_int32,
	&subtilis_type_byte_if,
	&subtilis_type_if_string,
	NULL,
	&subtilis_type_array_float64,
	&subtilis_type_array_int32,
	&subtilis_type_array_byte,
	&subtilis_type_array_string,
	&subtilis_type_vector_float64,
	&subtilis_type_vector_int32,
	&subtilis_type_vector_byte,
	&subtilis_type_vector_string,
	&subtilis_type_if_local_buffer,
	&subtilis_type_fn,
	NULL,
};

/* clang-format on */

static subtilis_exp_t *prv_call_unary_fn(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_type_if_unary_t fn,
					 subtilis_error_t *err)
{
	if (!fn) {
		subtilis_exp_delete(e);
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return fn(p, e, err);
}

static bool prv_order_expressions(subtilis_exp_t **a1, subtilis_exp_t **a2)
{
	subtilis_exp_t *e1;
	subtilis_exp_t *e2;
	bool swap;

	e1 = *a1;
	e2 = *a2;

	swap = !prv_type_map[e2->type.type]->is_const &&
	       prv_type_map[e1->type.type]->is_const;
	if (swap) {
		*a1 = e2;
		*a2 = e1;
	}

	return swap;
}

static subtilis_exp_t *
prv_call_binary_fn(subtilis_parser_t *p, subtilis_exp_t *a1, subtilis_exp_t *a2,
		   subtilis_type_if_binary_t fn, subtilis_error_t *err)
{
	if (!fn) {
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	return fn(p, a1, a2, err);
}

static subtilis_exp_t *
prv_call_binary_nc_fn(subtilis_parser_t *p, subtilis_exp_t *a1,
		      subtilis_exp_t *a2, subtilis_type_if_binary_nc_t fn,
		      bool swapped, subtilis_error_t *err)
{
	if (!fn) {
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return fn(p, a1, a2, swapped, err);
}

/* clang-format off */
static subtilis_exp_t *prv_call_binary_nc_logical_fn(
	subtilis_parser_t *p, subtilis_exp_t *a1, subtilis_exp_t *a2,
	subtilis_type_if_binary_nc_t fn, bool swapped, subtilis_error_t *err)

/* clang-format on */
{
	subtilis_exp_t *e;

	if (!fn) {
		subtilis_exp_delete(a1);
		subtilis_exp_delete(a2);
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	e = fn(p, a1, a2, swapped, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	if (e->type.type == SUBTILIS_TYPE_REAL) {
		e->type.type = SUBTILIS_TYPE_INTEGER;
	} else if ((e->type.type != SUBTILIS_TYPE_INTEGER) &&
		   (e->type.type != SUBTILIS_TYPE_CONST_INTEGER)) {
		subtilis_error_set_assertion_failed(err);
		subtilis_exp_delete(e);
		e = NULL;
	}
	return e;
}

size_t subtilis_type_if_size(const subtilis_type_t *type, subtilis_error_t *err)
{
	subtilis_type_if_size_t fn;

	fn = prv_type_map[type->type]->size;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}
	return fn(type);
}

subtilis_exp_t *subtilis_type_if_data_size(subtilis_parser_t *p,
					   const subtilis_type_t *type,
					   subtilis_exp_t *e,
					   subtilis_error_t *err)
{
	subtilis_type_if_unary_t fn = prv_type_map[type->type]->data_size;

	if (!fn) {
		subtilis_exp_delete(e);
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return fn(p, e, err);
}

subtilis_exp_t *subtilis_type_if_zero(subtilis_parser_t *p,
				      const subtilis_type_t *type,
				      subtilis_error_t *err)
{
	subtilis_type_if_none_t fn;

	fn = prv_type_map[type->type]->zero;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	return fn(p, err);
}

void subtilis_type_if_zero_ref(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, bool push, subtilis_error_t *err)
{
	subtilis_type_if_zeroref_t fn;

	fn = prv_type_map[type->type]->zero_ref;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, type, mem_reg, loc, push, err);
}

void subtilis_type_if_copy_ret(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t dest_reg,
			       size_t source_reg, subtilis_error_t *err)
{
	subtilis_type_if_reg2_t fn;

	fn = prv_type_map[type->type]->copy_ret;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, type, dest_reg, source_reg, err);
}

void subtilis_type_if_new_ref(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t mem_reg, size_t loc, subtilis_exp_t *e,
			      subtilis_error_t *err)
{
	subtilis_type_if_initref_t fn;

	fn = prv_type_map[type->type]->new_ref;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, type, mem_reg, loc, e, err);
}

void subtilis_type_if_assign_ref(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, subtilis_exp_t *e,
				 subtilis_error_t *err)
{
	subtilis_type_if_initref_t fn;

	fn = prv_type_map[type->type]->assign_ref;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, type, mem_reg, loc, e, err);
}

void subtilis_type_if_assign_ref_no_rc(subtilis_parser_t *p,
				       const subtilis_type_t *type,
				       size_t mem_reg, size_t loc,
				       subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_type_if_initref_t fn;

	fn = prv_type_map[type->type]->assign_ref_no_rc;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, type, mem_reg, loc, e, err);
}

subtilis_exp_t *subtilis_type_if_top_bit(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 subtilis_error_t *err)
{
	subtilis_type_if_none_t fn;

	fn = prv_type_map[type->type]->top_bit;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	return fn(p, err);
}

void subtilis_type_if_zero_reg(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t reg,
			       subtilis_error_t *err)
{
	subtilis_type_if_reg_t fn;

	fn = prv_type_map[type->type]->zero_reg;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, reg, err);
}

void subtilis_type_if_array_of(subtilis_parser_t *p,
			       const subtilis_type_t *element_type,
			       subtilis_type_t *type, subtilis_error_t *err)
{
	subtilis_type_if_typeof_t fn;

	if (element_type == type) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[element_type->type]->array_of;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(element_type, type, err);
}

void subtilis_type_if_vector_of(subtilis_parser_t *p,
				const subtilis_type_t *element_type,
				subtilis_type_t *type, subtilis_error_t *err)
{
	subtilis_type_if_typeof_t fn;

	if (element_type == type) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[element_type->type]->vector_of;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(element_type, type, err);
}

void subtilis_type_if_const_of(const subtilis_type_t *type,
			       subtilis_type_t *const_type,
			       subtilis_error_t *err)
{
	subtilis_type_if_typeof_t fn;

	if (const_type == type) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[type->type]->const_of;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(type, const_type, err);
}

void subtilis_type_if_element_type(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_type_t *element_type,
				   subtilis_error_t *err)
{
	subtilis_type_if_typeof_t fn;

	if (element_type == type) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[type->type]->element_type;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(type, element_type, err);
}

subtilis_exp_t *subtilis_type_if_exp_to_var(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->exp_to_var,
				 err);
}

subtilis_exp_t *subtilis_type_if_copy_var(subtilis_parser_t *p,
					  subtilis_exp_t *e,
					  subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->copy_var,
				 err);
}

subtilis_exp_t *subtilis_type_if_dup(subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_type_if_dup_t fn;
	subtilis_exp_t *exp = subtilis_exp_new_empty(&e->type, err);

	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	fn = prv_type_map[e->type.type]->dup;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}
	fn(e, exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return exp;

cleanup:
	subtilis_exp_delete(exp);
	return NULL;
}

void subtilis_type_if_memcpy(subtilis_parser_t *p, subtilis_exp_t *e1,
			     subtilis_exp_t *e2, subtilis_error_t *err)
{
	subtilis_type_if_copy_collection_t fn;

	fn = prv_type_map[e1->type.type]->copy_col;
	if (!fn) {
		subtilis_error_set_expected(err, "string or array",
					    subtilis_type_name(&e1->type),
					    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e1);
		subtilis_exp_delete(e2);
		return;
	}
	fn(p, e1, e2, err);
}

void subtilis_type_if_assign_to_reg(subtilis_parser_t *p, size_t reg,
				    subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_type_if_sizet_exp_t fn;

	fn = prv_type_map[e->type.type]->assign_reg;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, reg, e, err);
}

void subtilis_type_if_assign_to_mem(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	subtilis_type_if_sizet2_exp_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[e->type.type]->assign_mem;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, mem_reg, loc, e, err);
}

void subtilis_type_if_assign_to_new_mem(subtilis_parser_t *p, size_t mem_reg,
					size_t loc, subtilis_exp_t *e,
					subtilis_error_t *err)
{
	subtilis_type_if_sizet2_exp_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[e->type.type]->assign_new_mem;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, mem_reg, loc, e, err);
}

subtilis_exp_t *subtilis_type_if_load_from_mem(subtilis_parser_t *p,
					       const subtilis_type_t *type,
					       size_t mem_reg, size_t loc,
					       subtilis_error_t *err)
{
	subtilis_type_if_load_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	fn = prv_type_map[type->type]->load_mem;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	return fn(p, type, mem_reg, loc, err);
}

void subtilis_type_if_indexed_write(subtilis_parser_t *p, const char *var_name,
				    const subtilis_type_t *type, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_exp_t **indices,
				    size_t index_count, subtilis_error_t *err)
{
	subtilis_type_if_iwrite_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[type->type]->indexed_write;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}
	fn(p, var_name, type, mem_reg, loc, e, indices, index_count, err);
}

void subtilis_type_if_indexed_add(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_exp_t **indices, size_t index_count,
				  subtilis_error_t *err)
{
	subtilis_type_if_iwrite_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[type->type]->indexed_add;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}
	fn(p, var_name, type, mem_reg, loc, e, indices, index_count, err);
}

void subtilis_type_if_indexed_sub(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t mem_reg,
				  size_t loc, subtilis_exp_t *e,
				  subtilis_exp_t **indices, size_t index_count,
				  subtilis_error_t *err)
{
	subtilis_type_if_iwrite_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	fn = prv_type_map[type->type]->indexed_sub;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}
	fn(p, var_name, type, mem_reg, loc, e, indices, index_count, err);
}

subtilis_exp_t *
subtilis_type_if_indexed_read(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, subtilis_exp_t **indices,
			      size_t index_count, subtilis_error_t *err)
{
	subtilis_type_if_iread_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	fn = prv_type_map[type->type]->indexed_read;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		return NULL;
	}
	return fn(p, var_name, type, mem_reg, loc, indices, index_count, err);
}

void subtilis_type_if_array_set(subtilis_parser_t *p, const char *var_name,
				const subtilis_type_t *type, size_t mem_reg,
				size_t loc, subtilis_exp_t *e,
				subtilis_error_t *err)
{
	subtilis_type_if_set_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	fn = prv_type_map[type->type]->set;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	fn(p, var_name, type, mem_reg, loc, e, err);
	return;

cleanup:
	subtilis_exp_delete(e);
}

subtilis_exp_t *
subtilis_type_if_indexed_address(subtilis_parser_t *p, const char *var_name,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, subtilis_exp_t **indices,
				 size_t index_count, subtilis_error_t *err)
{
	subtilis_type_if_iread_t fn;

	if (loc > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	fn = prv_type_map[type->type]->indexed_address;
	if (!fn) {
		subtilis_error_not_array_or_vector(
		    err, var_name, p->l->stream->name, p->l->line);
		return NULL;
	}
	return fn(p, var_name, type, mem_reg, loc, indices, index_count, err);
}

void subtilis_type_if_append(subtilis_parser_t *p, subtilis_exp_t *a1,
			     subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_type_if_copy_collection_t fn;

	fn = prv_type_map[a1->type.type]->append;
	if (!fn) {
		subtilis_error_set_expected(err, "string or 1d array",
					    subtilis_type_name(&a1->type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	fn(p, a1, a2, err);
	return;

cleanup:
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
}

subtilis_exp_t *subtilis_type_if_to_int(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err)
{
	if (!prv_type_map[e->type.type]->to_int32) {
		subtilis_error_set_integer_expected(
		    err, subtilis_type_name(&e->type), p->l->stream->name,
		    p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->to_int32,
				 err);
}

subtilis_exp_t *subtilis_type_if_to_byte(subtilis_parser_t *p,
					 subtilis_exp_t *e,
					 subtilis_error_t *err)
{
	if (subtilis_type_if_is_const(&e->type)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	if (!prv_type_map[e->type.type]->to_byte) {
		subtilis_error_set_byte_expected(
		    err, subtilis_type_name(&e->type), p->l->stream->name,
		    p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->to_byte,
				 err);
}

subtilis_exp_t *subtilis_type_if_to_float64(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->to_float64,
				 err);
}

subtilis_exp_t *subtilis_type_if_zerox(subtilis_parser_t *p, subtilis_exp_t *e,
				       const subtilis_type_t *type,
				       subtilis_error_t *err)
{
	if (!prv_type_map[e->type.type]->zerox) {
		subtilis_error_set_bad_zero_extend(
		    err, subtilis_type_name(&e->type), subtilis_type_name(type),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return prv_type_map[e->type.type]->zerox(p, e, type, err);
}

subtilis_exp_t *subtilis_type_if_to_string(subtilis_parser_t *p,
					   subtilis_exp_t *e,
					   subtilis_error_t *err)
{
	if (!prv_type_map[e->type.type]->to_string) {
		subtilis_error_set_bad_conversion(
		    err, subtilis_type_name(&e->type),
		    subtilis_type_name(&subtilis_type_string),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->to_string,
				 err);
}

subtilis_exp_t *subtilis_type_if_to_hex_string(subtilis_parser_t *p,
					       subtilis_exp_t *e,
					       subtilis_error_t *err)
{
	if (!prv_type_map[e->type.type]->to_hex_string) {
		subtilis_error_set_bad_conversion(
		    err, subtilis_type_name(&e->type),
		    subtilis_type_name(&subtilis_type_string),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}
	return prv_call_unary_fn(
	    p, e, prv_type_map[e->type.type]->to_hex_string, err);
}

subtilis_exp_t *subtilis_type_if_unary_minus(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->unary_minus,
				 err);
}

subtilis_exp_t *subtilis_type_if_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	/*
	 * Irritatingly, string addition is not commutative.
	 */

	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->add, swapped, err);
}

subtilis_exp_t *subtilis_type_if_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->mul,
				  err);
}

subtilis_exp_t *subtilis_type_if_and(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->and,
				  err);
}

subtilis_exp_t *subtilis_type_if_or(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->or,
				  err);
}

subtilis_exp_t *subtilis_type_if_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->eor,
				  err);
}

subtilis_exp_t *subtilis_type_if_not(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->not, err);
}

subtilis_exp_t *subtilis_type_if_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->eq,
				  err);
}

subtilis_exp_t *subtilis_type_if_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	(void)prv_order_expressions(&a1, &a2);
	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->neq,
				  err);
}

subtilis_exp_t *subtilis_type_if_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->sub, swapped, err);
}

subtilis_exp_t *subtilis_type_if_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1 = subtilis_type_if_to_int(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a2);
		return NULL;
	}

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->div,
				  err);
}

subtilis_exp_t *subtilis_type_if_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	a1 = subtilis_type_if_to_int(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a2);
		return NULL;
	}

	a2 = subtilis_type_if_to_int(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->mod,
				  err);
}

subtilis_exp_t *subtilis_type_if_divr(subtilis_parser_t *p, subtilis_exp_t *a1,
				      subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped;

	a1 = subtilis_type_if_to_float64(p, a1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a2);
		return NULL;
	}

	a2 = subtilis_type_if_to_float64(p, a2, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return NULL;
	}

	swapped = prv_order_expressions(&a1, &a2);
	return prv_call_binary_nc_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->divr, swapped, err);
}

subtilis_exp_t *subtilis_type_if_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_logical_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->gt, swapped, err);
}

subtilis_exp_t *subtilis_type_if_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_logical_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->lte, swapped, err);
}

subtilis_exp_t *subtilis_type_if_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_logical_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->lt, swapped, err);
}

subtilis_exp_t *subtilis_type_if_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_logical_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->gte, swapped, err);
}

subtilis_exp_t *subtilis_type_if_pow(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	bool swapped = prv_order_expressions(&a1, &a2);

	return prv_call_binary_nc_fn(
	    p, a1, a2, prv_type_map[a1->type.type]->pow, swapped, err);
}

subtilis_exp_t *subtilis_type_if_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)

{
	if (!subtilis_type_if_is_integer(&a1->type)) {
		a1 = subtilis_type_if_to_int(p, a1, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a2);
			return NULL;
		}
	}

	if (!subtilis_type_if_is_integer(&a2->type)) {
		a2 = subtilis_type_if_to_int(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a1);
			return NULL;
		}
	}

	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->lsl,
				  err);
}

subtilis_exp_t *subtilis_type_if_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)

{
	if (!subtilis_type_if_is_integer(&a1->type)) {
		a1 = subtilis_type_if_to_int(p, a1, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a2);
			return NULL;
		}
	}

	if (!subtilis_type_if_is_integer(&a2->type)) {
		a2 = subtilis_type_if_to_int(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a1);
			return NULL;
		}
	}

	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->lsr,
				  err);
}

subtilis_exp_t *subtilis_type_if_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)

{
	if (!subtilis_type_if_is_integer(&a1->type)) {
		a1 = subtilis_type_if_to_int(p, a1, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a2);
			return NULL;
		}
	}

	if (!subtilis_type_if_is_integer(&a2->type)) {
		a2 = subtilis_type_if_to_int(p, a2, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(a1);
			return NULL;
		}
	}

	return prv_call_binary_fn(p, a1, a2, prv_type_map[a1->type.type]->asr,
				  err);
}

subtilis_exp_t *subtilis_type_if_abs(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->abs, err);
}

subtilis_exp_t *subtilis_type_if_sgn(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err)
{
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->sgn, err);
}

subtilis_exp_t *subtilis_type_if_is_inf(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err)
{
	if (!subtilis_type_if_is_numeric(&e->type)) {
		subtilis_error_set_numeric_expected(
		    err, subtilis_type_name(&e->type), p->l->stream->name,
		    p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}
	return prv_call_unary_fn(p, e, prv_type_map[e->type.type]->is_inf, err);
}

subtilis_exp_t *subtilis_type_if_call(subtilis_parser_t *p,
				      const subtilis_type_t *type,
				      subtilis_ir_arg_t *args, size_t num_args,
				      subtilis_error_t *err)
{
	subtilis_type_if_call_t fn;

	fn = prv_type_map[type->type]->call;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	return fn(p, type, args, num_args, err);
}

void subtilis_type_if_ret(subtilis_parser_t *p, const subtilis_type_t *type,
			  size_t reg, subtilis_error_t *err)
{
	subtilis_type_if_reg_t fn;

	fn = prv_type_map[type->type]->ret;
	if (!fn) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, reg, err);
}

void subtilis_type_if_print(subtilis_parser_t *p, subtilis_exp_t *e,
			    subtilis_error_t *err)
{
	subtilis_type_if_print_t fn;

	fn = prv_type_map[e->type.type]->print;
	if (!fn) {
		subtilis_exp_delete(e);
		subtilis_error_set_assertion_failed(err);
		return;
	}
	fn(p, e, err);
}

bool subtilis_type_if_is_const(const subtilis_type_t *type)
{
	if (prv_type_map[type->type])
		return prv_type_map[type->type]->is_const;
	else
		return false;
}

bool subtilis_type_if_is_numeric(const subtilis_type_t *type)
{
	if (prv_type_map[type->type])
		return prv_type_map[type->type]->is_numeric;
	else
		return false;
}

bool subtilis_type_if_is_integer(const subtilis_type_t *type)
{
	if (prv_type_map[type->type])
		return prv_type_map[type->type]->is_integer;
	else
		return false;
}

bool subtilis_type_if_is_array(const subtilis_type_t *type)
{
	if (prv_type_map[type->type])
		return prv_type_map[type->type]->is_array;
	else
		return false;
}

bool subtilis_type_if_is_vector(const subtilis_type_t *type)
{
	if (prv_type_map[type->type])
		return prv_type_map[type->type]->is_vector;
	else
		return false;
}

bool subtilis_type_if_is_reference(const subtilis_type_t *type)
{
	return (type->type == SUBTILIS_TYPE_STRING) ||
	       prv_type_map[type->type]->is_vector ||
	       prv_type_map[type->type]->is_array;
}

bool subtilis_type_if_is_scalar_ref(const subtilis_type_t *type,
				    subtilis_error_t *err)
{
	subtilis_type_t element_type;
	bool retval;

	if (subtilis_type_if_is_array(type) ||
	    subtilis_type_if_is_vector(type)) {
		prv_type_map[type->type]->element_type(type, &element_type,
						       err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
		retval = subtilis_type_if_is_numeric(&element_type);
		subtilis_type_free(&element_type);
		return retval;
	}

	return (type->type == SUBTILIS_TYPE_STRING);
}

subtilis_exp_t *subtilis_type_if_coerce_type(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     const subtilis_type_t *type,
					     subtilis_error_t *err)
{
	subtilis_type_if_coerce_t fn;

	if (subtilis_type_eq(type, &e->type))
		return e;

	fn = prv_type_map[e->type.type]->coerce;
	if (!fn) {
		subtilis_error_set_bad_conversion(
		    err, subtilis_type_name(&e->type), subtilis_type_name(type),
		    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}

	return fn(p, e, type, err);
}

subtilis_ir_reg_type_t subtilis_type_if_reg_type(const subtilis_type_t *type)
{
	return prv_type_map[type->type]->param_type;
}

size_t subtilis_type_if_destructor(const subtilis_type_t *type)
{
	subtilis_type_if_size_t fn;

	fn = prv_type_map[type->type]->destructor;
	if (!fn)
		return 0;

	return fn(type);
}
