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

#include "../common/error_codes.h"
#include "array_type.h"
#include "builtins_helper.h"
#include "builtins_ir.h"
#include "expression.h"
#include "parser_call.h"
#include "rec_type.h"
#include "reference_type.h"
#include "type_if.h"

/*
 * Each array variable occupies  a fixed amount of space either on the stack or
 * in global memory.  This memory is laid out as follows.
 *
 *  ----------------------------------
 * | Size in Bytes     |           0 |
 *  ----------------------------------
 * | Pointer to Data   |           4 |
 *  ----------------------------------
 * | Pointer to Heap   |           8 |
 *  ----------------------------------
 * | Original Size     |          12 |
 *  ----------------------------------
 * | Size of DIM 1     |          16 |
 *  ----------------------------------
 * | Size of DIM n     | 12 + (n * 4)|
 *  ----------------------------------
 */

/*
 * Vectors have the same layout in memory as a 1 dimensional array.
 *
 *  ----------------------------------
 * | Size in Bytes     |           0 |
 *  ----------------------------------
 * | Pointer to Data   |           4 |
 *  ----------------------------------
 * | Pointer to Heap   |           8 |
 *  ----------------------------------
 * | Original Size     |          12 |
 *  ----------------------------------
 * | Vector Length     |          16 |
 *  ----------------------------------
 */

#define SUBTIILIS_ARRAY_SIZE_OFF SUBTIILIS_REFERENCE_SIZE_OFF
#define SUBTIILIS_ARRAY_DATA_OFF SUBTIILIS_REFERENCE_DATA_OFF
#define SUBTIILIS_ARRAY_HEAP_OFF SUBTIILIS_REFERENCE_HEAP_OFF
#define SUBTIILIS_ARRAY_ORIG_SIZE_OFF SUBTIILIS_REFERENCE_ORIG_SIZE_OFF
#define SUBTIILIS_ARRAY_DIMS_OFF (SUBTIILIS_REFERENCE_ORIG_SIZE_OFF + 4)

size_t subtilis_array_type_size(const subtilis_type_t *type)
{
	size_t size;

	/*
	 * TODO: Size appropriately for 64 bit builds.
	 */

	/*
	 * We need, on 32 bit builds,
	 * 4 bytes for the size
	 * 4 bytes for the data pointer
	 * 4 bytes for the heap pointer
	 * 4 bytes for the destructor
	 * 4 bytes for each dimension.
	 */

	size = SUBTIILIS_REFERENCE_SIZE +
	       (type->params.array.num_dims * sizeof(uint32_t));

	return size;
}

void subtilis_array_type_swap(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_reference_type_swap(p, reg1, reg2,
				     subtilis_array_type_size(type) - 4, err);
}

void subtilis_array_create_1el(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, bool push, subtilis_error_t *err)
{
	subtilis_type_t const_type;
	size_t i;
	subtilis_exp_t **ee;
	subtilis_ir_operand_t store_reg;

	store_reg.reg = mem_reg;
	subtilis_type_init_copy(&const_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ee = malloc(sizeof(*ee) * const_type.params.array.num_dims);
	if (!ee) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	for (i = 0; i < const_type.params.array.num_dims; i++) {
		const_type.params.array.dims[i] = 1;
		ee[i] = subtilis_exp_new_int32(1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_array_type_allocate(p, "temporary array", &const_type, loc, ee,
				     store_reg, push, err);
cleanup:
	for (i = 0; i < const_type.params.array.num_dims; i++)
		free(ee[i]);
	free(ee);

	subtilis_type_free(&const_type);
}

/* Does not consume elements in e */

void subtilis_array_type_init(subtilis_parser_t *p, const char *var_name,
			      const subtilis_type_t *element_type,
			      subtilis_type_t *type, subtilis_exp_t **e,
			      size_t dims, subtilis_error_t *err)
{
	size_t i;

	if (dims > SUBTILIS_MAX_DIMENSIONS) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	type->type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_array_of(element_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	type->params.array.num_dims = dims;
	for (i = 0; i < dims; i++) {
		if (e[i]->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			if (e[i]->exp.ir_op.integer < 0) {
				subtilis_error_bad_dim(err, var_name,
						       p->l->stream->name,
						       p->l->line);
				return;
			}
			type->params.array.dims[i] = e[i]->exp.ir_op.integer;
		} else {
			type->params.array.dims[i] = SUBTILIS_DYNAMIC_DIMENSION;
		}
	}
}

void subtilis_array_type_vector_init(const subtilis_type_t *element_type,
				     subtilis_type_t *type,
				     subtilis_error_t *err)
{
	type->type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_vector_of(element_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	type->params.array.num_dims = 1;
	type->params.array.dims[0] = SUBTILIS_DYNAMIC_DIMENSION;
}

void subtilis_array_type_vector_zero_ref(subtilis_parser_t *p,
					 const subtilis_type_t *type,
					 size_t loc, size_t mem_reg, bool push,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t base;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *neg1 = NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	neg1 = subtilis_exp_new_int32(-1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	neg1 = subtilis_type_if_exp_to_var(p, neg1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_size(p, mem_reg, loc, zero->exp.ir_op.reg,
					 err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_orig_size(p, mem_reg, loc,
					      zero->exp.ir_op.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = ((int32_t)loc) + SUBTIILIS_ARRAY_DIMS_OFF;
	base.reg = mem_reg;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  neg1->exp.ir_op, base, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (push)
		subtilis_reference_type_push_reference(p, type, mem_reg, loc,
						       err);

cleanup:

	subtilis_exp_delete(neg1);
	subtilis_exp_delete(zero);
}

void subtilis_array_zero_buf_i32(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t data_reg,
				 size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t zero;
	size_t zero_reg;

	zero.integer = 0;
	zero_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_builtin_memset_i32(p, data_reg, size_reg, zero_reg, err);
}

void subtilis_array_zero_buf_i8(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t data_reg,
				size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t zero;
	size_t zero_reg;

	zero.integer = 0;
	zero_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_builtin_memset_i8(p, data_reg, size_reg, zero_reg, err);
}

static void prv_clear_new_array(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t loc,
				size_t sizee, subtilis_ir_operand_t store_reg,
				size_t data, subtilis_error_t *err)
{
	subtilis_type_if_zero_buf(p, type, data, sizee, err);
}

static void prv_1d_static_alloc(subtilis_parser_t *p, size_t loc,
				const subtilis_type_t *type, subtilis_exp_t *e,
				subtilis_ir_operand_t store_reg, bool push,
				subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t one;
	subtilis_ir_operand_t size;
	size_t data;
	subtilis_exp_t *sizee = NULL;

	op1.integer = ((int32_t)loc) + SUBTIILIS_ARRAY_DIMS_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  e->exp.ir_op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one.integer = 1;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, e->exp.ir_op, one, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_exp_new_int32_var(size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_type_if_data_size(p, type, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	data = subtilis_reference_type_alloc(p, type, loc, store_reg.reg,
					     sizee->exp.ir_op.reg, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_clear_new_array(p, type, loc, sizee->exp.ir_op.reg, store_reg, data,
			    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (push)
		subtilis_reference_type_push_reference(p, type, store_reg.reg,
						       loc, err);
cleanup:

	subtilis_exp_delete(sizee);
}

static void prv_1d_dynamic_alloc(subtilis_parser_t *p, size_t loc,
				 const subtilis_type_t *type, subtilis_exp_t *e,
				 subtilis_ir_operand_t store_reg, bool push,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t ok_label;
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t condee;

	error_label = subtilis_array_type_error_label(p);
	ok_label.label = subtilis_ir_section_new_label(p->current);

	zero.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GTEI_I32, e->exp.ir_op, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, ok_label, error_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, ok_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_1d_static_alloc(p, loc, type, e, store_reg, push, err);
}

static void prv_dynamic_vector_alloc(subtilis_parser_t *p, size_t loc,
				     const subtilis_type_t *type,
				     subtilis_exp_t *e,
				     subtilis_ir_operand_t store_reg, bool push,
				     subtilis_error_t *err)
{
	subtilis_ir_operand_t neg_label;
	subtilis_ir_operand_t non_neg_label;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t one;
	subtilis_ir_operand_t end_label;
	subtilis_ir_operand_t size;
	size_t data;
	subtilis_exp_t *sizee = NULL;

	end_label.label = subtilis_ir_section_new_label(p->current);
	neg_label.label = subtilis_ir_section_new_label(p->current);
	non_neg_label.label = subtilis_ir_section_new_label(p->current);

	zero.integer = 0;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LTI_I32, e->exp.ir_op, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, neg_label, non_neg_label,
					  err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, neg_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_vector_zero_ref(p, type, loc, store_reg.reg, false,
					    err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, non_neg_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = ((int32_t)loc) + SUBTIILIS_ARRAY_DIMS_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  e->exp.ir_op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	one.integer = 1;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, e->exp.ir_op, one, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_exp_new_int32_var(size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sizee = subtilis_type_if_data_size(p, type, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	data = subtilis_reference_type_alloc(p, type, loc, store_reg.reg,
					     sizee->exp.ir_op.reg, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_clear_new_array(p, type, loc, sizee->exp.ir_op.reg, store_reg, data,
			    err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (push)
		subtilis_reference_type_push_reference(p, type, store_reg.reg,
						       loc, err);

cleanup:

	subtilis_exp_delete(sizee);
}

void subtilis_array_type_vector_alloc(subtilis_parser_t *p, size_t loc,
				      const subtilis_type_t *type,
				      subtilis_exp_t *e,
				      subtilis_ir_operand_t store_reg,
				      bool push, subtilis_error_t *err)
{
	if (e->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		if (e->exp.ir_op.integer < 0) {
			subtilis_array_type_vector_zero_ref(
			    p, type, loc, store_reg.reg, true, err);
			return;
		}

		e = subtilis_type_if_dup(e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		e = subtilis_type_if_exp_to_var(p, e, err);
		if (err->type == SUBTILIS_ERROR_OK)
			prv_1d_static_alloc(p, loc, type, e, store_reg, push,
					    err);
		subtilis_exp_delete(e);
	} else {
		prv_dynamic_vector_alloc(p, loc, type, e, store_reg, push, err);
	}
}

void subtilis_array_type_allocate(subtilis_parser_t *p, const char *var_name,
				  const subtilis_type_t *type, size_t loc,
				  subtilis_exp_t **e,
				  subtilis_ir_operand_t store_reg, bool push,
				  subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_ir_operand_t op1;
	size_t i;
	int32_t offset;
	subtilis_exp_t *sizee = NULL;

	if (loc + subtilis_array_type_size(type) > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/*
	 * We have a separate allocation path for dynamically sized
	 * 1-dimensional arrays.
	 */

	if ((type->params.array.num_dims == 1) &&
	    (type->params.array.dims[0] == SUBTILIS_DYNAMIC_DIMENSION)) {
		prv_1d_dynamic_alloc(p, loc, type, e[0], store_reg, push, err);
		return;
	}

	offset = ((int32_t)loc) + SUBTIILIS_ARRAY_DIMS_OFF;

	sizee = subtilis_array_size_calc(p, var_name, e,
					 type->params.array.num_dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 0; i < type->params.array.num_dims; i++) {
		op1.integer = offset;
		e[i] = subtilis_type_if_exp_to_var(p, e[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_STOREO_I32, e[i]->exp.ir_op,
		    store_reg, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		offset += sizeof(int32_t);
	}

	sizee = subtilis_type_if_data_size(p, type, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op.reg = subtilis_reference_type_alloc(p, type, loc, store_reg.reg,
					       sizee->exp.ir_op.reg, push, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_clear_new_array(p, type, loc, sizee->exp.ir_op.reg, store_reg,
			    op.reg, err);

cleanup:

	subtilis_exp_delete(sizee);
}

static void prv_check_slice_indices(subtilis_parser_t *p,
				    const subtilis_type_t *type,
				    subtilis_exp_t *index1,
				    subtilis_exp_t *index2,
				    subtilis_ir_operand_t error_label,
				    subtilis_error_t *err)
{
	subtilis_ir_operand_t indices_ok;
	subtilis_exp_t *index1_dup = NULL;
	subtilis_exp_t *index2_dup = NULL;
	subtilis_exp_t *condee = NULL;

	if ((index1->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (index2->type.type == SUBTILIS_TYPE_CONST_INTEGER)) {
		if (index1->exp.ir_op.integer > index2->exp.ir_op.integer) {
			subtilis_error_set_bad_slice(
			    err, index1->exp.ir_op.integer,
			    index2->exp.ir_op.integer, p->l->stream->name,
			    p->l->line);
			return;
		}
	} else {
		/* We need to check the slice parameters are okay at runtime. */

		indices_ok.label = subtilis_ir_section_new_label(p->current);

		index1_dup = subtilis_type_if_dup(index1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		index2_dup = subtilis_type_if_dup(index2, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(index1_dup);
			return;
		}

		condee = subtilis_type_if_lte(p, index1_dup, index2_dup, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee->exp.ir_op,
		    indices_ok, error_label, err);
		subtilis_exp_delete(condee);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, indices_ok.label,
					      err);
	}
}

static void
prv_check_slice_index1(subtilis_parser_t *p, const subtilis_type_t *type,
		       subtilis_exp_t *index1, subtilis_exp_t *index2,
		       subtilis_ir_operand_t error_label, subtilis_error_t *err)
{
	subtilis_ir_operand_t index1_ok;
	subtilis_exp_t *index1_dup = NULL;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *condee = NULL;

	if (index1->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		if (index1->exp.ir_op.integer < 0) {
			subtilis_error_set_bad_slice(
			    err, index1->exp.ir_op.integer,
			    index2->exp.ir_op.integer, p->l->stream->name,
			    p->l->line);
			return;
		}
	} else {
		index1_ok.label = subtilis_ir_section_new_label(p->current);
		index1_dup = subtilis_type_if_dup(index1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		zero = subtilis_exp_new_int32(0, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(index1_dup);
			return;
		}

		condee = subtilis_type_if_gte(p, index1_dup, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee->exp.ir_op,
		    index1_ok, error_label, err);
		subtilis_exp_delete(condee);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, index1_ok.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static subtilis_exp_t *prv_calc_max_dim(subtilis_parser_t *p,
					const subtilis_type_t *type, size_t ptr,
					subtilis_error_t *err)
{
	subtilis_exp_t *max_dim = NULL;
	subtilis_exp_t *one;

	if (type->params.array.dims[0] != SUBTILIS_DYNAMIC_DIMENSION)
		max_dim =
		    subtilis_exp_new_int32(type->params.array.dims[0], err);
	else
		max_dim = subtilis_type_if_load_from_mem(
		    p, &subtilis_type_integer, ptr, SUBTIILIS_ARRAY_DIMS_OFF,
		    err);

	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	one = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(max_dim);
		return NULL;
	}

	return subtilis_type_if_add(p, max_dim, one, err);
}

static void prv_check_slice_index2(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   subtilis_exp_t *index1,
				   subtilis_exp_t *index2, size_t ptr,
				   subtilis_ir_operand_t error_label,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t index2_ok;
	subtilis_exp_t *index2_dup = NULL;
	subtilis_exp_t *condee = NULL;
	subtilis_exp_t *max_dim = NULL;

	if ((index2->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (type->params.array.dims[0] != SUBTILIS_DYNAMIC_DIMENSION)) {
		if (index2->exp.ir_op.integer >
		    type->params.array.dims[0] + 1) {
			subtilis_error_set_bad_slice(
			    err, index1->exp.ir_op.integer,
			    index2->exp.ir_op.integer, p->l->stream->name,
			    p->l->line);
			return;
		}
	} else {
		index2_ok.label = subtilis_ir_section_new_label(p->current);

		max_dim = prv_calc_max_dim(p, type, ptr, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		index2_dup = subtilis_type_if_dup(index2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		condee = subtilis_type_if_lte(p, index2_dup, max_dim, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee->exp.ir_op,
		    index2_ok, error_label, err);
		subtilis_exp_delete(condee);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, index2_ok.label, err);
	}
}

static size_t prv_slice_set_sizes(subtilis_parser_t *p,
				  const subtilis_type_t *type,
				  size_t source_reg, size_t dest_reg,
				  subtilis_exp_t *index1,
				  subtilis_exp_t *index2, subtilis_error_t *err)
{
	size_t size_reg;
	size_t orig_size_reg;
	subtilis_exp_t *one = NULL;
	subtilis_exp_t *size = NULL;
	subtilis_exp_t *index1_dup = NULL;
	subtilis_exp_t *index2_dup = NULL;
	subtilis_exp_t *size_dup = NULL;
	subtilis_exp_t *el_count = NULL;

	index1_dup = subtilis_type_if_dup(index1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	index2_dup = subtilis_type_if_dup(index2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size = subtilis_type_if_sub(p, index2_dup, index1_dup, err);
	index1_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size_dup = subtilis_type_if_dup(size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	one = subtilis_exp_new_int32(1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	el_count = subtilis_type_if_sub(p, size_dup, one, err);
	size_dup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size = subtilis_type_if_data_size(p, type, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size = subtilis_type_if_exp_to_var(p, size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size_reg = size->exp.ir_op.reg;
	subtilis_exp_delete(size);
	size = NULL;

	subtilis_reference_type_set_size(p, dest_reg, 0, size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	orig_size_reg =
	    subtilis_reference_type_get_orig_size(p, source_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_set_orig_size(p, dest_reg, 0, orig_size_reg,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, dest_reg, SUBTIILIS_ARRAY_DIMS_OFF,
				       el_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return size_reg;

cleanup:
	subtilis_exp_delete(el_count);
	subtilis_exp_delete(size);
	subtilis_exp_delete(size_dup);
	subtilis_exp_delete(index1_dup);

	return SIZE_MAX;
}

static void prv_slice_set_pointers(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   size_t source_reg, size_t dest_reg,
				   subtilis_exp_t *index1,
				   subtilis_error_t *err)
{
	size_t data_reg;
	subtilis_ir_operand_t heap_reg;
	subtilis_exp_t *offset = NULL;
	subtilis_exp_t *data = NULL;
	subtilis_exp_t *index1_dup = NULL;

	index1_dup = subtilis_type_if_dup(index1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	offset = subtilis_type_if_data_size(p, type, index1_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data_reg = subtilis_reference_get_data(p, source_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	data = subtilis_exp_new_int32_var(data_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((offset->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (offset->exp.ir_op.integer == 0)) {
		subtilis_exp_delete(offset);
		offset = NULL;
		offset = data;
		data = NULL;
	} else {
		offset = subtilis_type_if_add(p, data, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_reference_set_data(p, offset->exp.ir_op.reg, dest_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(offset);
	offset = NULL;

	heap_reg.reg = subtilis_reference_get_heap(p, source_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     heap_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_set_heap(p, heap_reg.reg, dest_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_push_reference(p, type, dest_reg, 0, err);

cleanup:

	subtilis_exp_delete(offset);
}

subtilis_exp_t *subtilis_array_type_slice_vector(subtilis_parser_t *p,
						 const subtilis_type_t *type,
						 size_t mem_reg, size_t loc,
						 subtilis_exp_t *index1,
						 subtilis_exp_t *index2,
						 subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	size_t source_reg;
	size_t dest_reg;
	size_t size_reg;
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t empty;
	subtilis_ir_operand_t not_empty;
	subtilis_exp_t *max_dim = NULL;

	subtilis_ir_operand_t op0;
	char *tmp_name = NULL;

	empty.label = subtilis_ir_section_new_label(p->current);
	not_empty.label = subtilis_ir_section_new_label(p->current);

	error_label = subtilis_array_type_error_label(p);

	/*
	 * index2 may be NULL if the second slice parameter was omitted.
	 */

	/*
	 * Check the indices are sane, i.e., that index1 <= index 2.
	 */

	if (index2) {
		prv_check_slice_indices(p, type, index1, index2, error_label,
					err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	/*
	 * We now know that index1 must be <= index2.  We need to check that
	 * index1 is >= 0.
	 */

	prv_check_slice_index1(p, type, index1, index2, error_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	source_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	/*
	 * Finally, we need to check that index2 <= max_dim + 1.
	 */

	if (index2) {
		prv_check_slice_index2(p, type, index1, index2, source_reg,
				       error_label, err);
	} else {
		index2 = prv_calc_max_dim(p, type, source_reg, err);
		max_dim = index2;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	s = subtilis_symbol_table_insert_tmp(p->local_st, type, &tmp_name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						  s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	size_reg = prv_slice_set_sizes(p, type, source_reg, dest_reg, index1,
				       index2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op0.reg = size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  op0, not_empty, empty, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, not_empty.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_slice_set_pointers(p, type, source_reg, dest_reg, index1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, empty.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(max_dim);
	return subtilis_exp_new_tmp_var(type, dest_reg, tmp_name, err);

cleanup:

	subtilis_exp_delete(max_dim);
	free(tmp_name);

	return NULL;
}

static void prv_check_slice_array_indices(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_exp_t *index1,
					  subtilis_exp_t *index2,
					  subtilis_ir_operand_t error_label,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t indices_ok;
	subtilis_exp_t *index1_dup = NULL;
	subtilis_exp_t *index2_dup = NULL;
	subtilis_exp_t *condee = NULL;

	if ((index1->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (index2->type.type == SUBTILIS_TYPE_CONST_INTEGER)) {
		if (index1->exp.ir_op.integer >= index2->exp.ir_op.integer) {
			subtilis_error_set_bad_slice(
			    err, index1->exp.ir_op.integer,
			    index2->exp.ir_op.integer, p->l->stream->name,
			    p->l->line);
			return;
		}
	} else {
		/* We need to check the slice parameters are okay at runtime. */

		indices_ok.label = subtilis_ir_section_new_label(p->current);

		index1_dup = subtilis_type_if_dup(index1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		index2_dup = subtilis_type_if_dup(index2, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(index1_dup);
			return;
		}

		condee = subtilis_type_if_lt(p, index1_dup, index2_dup, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee->exp.ir_op,
		    indices_ok, error_label, err);
		subtilis_exp_delete(condee);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, indices_ok.label,
					      err);
	}
}

subtilis_exp_t *subtilis_array_type_slice_array(subtilis_parser_t *p,
						const subtilis_type_t *type,
						size_t mem_reg, size_t loc,
						subtilis_exp_t *index1,
						subtilis_exp_t *index2,
						subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	size_t source_reg;
	size_t dest_reg;
	const subtilis_symbol_t *s;
	subtilis_type_t array_type;
	subtilis_exp_t *ret_val;
	subtilis_exp_t *max_dim = NULL;
	char *tmp_name = NULL;

	array_type.type = SUBTILIS_TYPE_VOID;

	error_label = subtilis_array_type_error_label(p);

	/*
	 * index2 may be NULL if the second slice parameter was omitted.
	 */

	/*
	 * Check the indices are sane, i.e., that index1 <= index 2.
	 */

	if (index2) {
		prv_check_slice_array_indices(p, type, index1, index2,
					      error_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	/*
	 * We now know that index1 must be < index2.  We need to check that
	 * index1 is >= 0.
	 */

	prv_check_slice_index1(p, type, index1, index2, error_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	source_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	/*
	 * Finally, we need to check that index2 <= max_dim + 1.
	 */

	if (index2) {
		prv_check_slice_index2(p, type, index1, index2, source_reg,
				       error_label, err);
	} else {
		index2 = prv_calc_max_dim(p, type, source_reg, err);
		max_dim = index2;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_type_init_copy(&array_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if ((index1->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (index2->type.type == SUBTILIS_TYPE_CONST_INTEGER))
		array_type.params.array.dims[0] =
		    (index2->exp.ir_op.integer - index1->exp.ir_op.integer) - 1;
	else
		array_type.params.array.dims[0] = SUBTILIS_DYNAMIC_DIMENSION;
	s = subtilis_symbol_table_insert_tmp(p->local_st, &array_type,
					     &tmp_name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL,
						  s->loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	(void)prv_slice_set_sizes(p, type, source_reg, dest_reg, index1, index2,
				  err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_slice_set_pointers(p, type, source_reg, dest_reg, index1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(max_dim);

	ret_val =
	    subtilis_exp_new_tmp_var(&array_type, dest_reg, tmp_name, err);
	subtilis_type_free(&array_type);
	return ret_val;

cleanup:

	subtilis_exp_delete(max_dim);
	free(tmp_name);
	subtilis_type_free(&array_type);

	return NULL;
}

static size_t prv_new_ref_base(subtilis_parser_t *p, const subtilis_type_t *t,
			       subtilis_ir_operand_t dest_reg,
			       size_t dest_offset,
			       subtilis_ir_operand_t source_reg,
			       size_t source_offset, bool ref,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t soffset;
	subtilis_ir_operand_t doffset;
	subtilis_ir_operand_t ref_start;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t data;
	subtilis_ir_operand_t heap;
	subtilis_ir_operand_t ref_label;
	subtilis_ir_operand_t skip_ref_label;
	size_t i;
	bool check_size = false;

	skip_ref_label.label = SIZE_MAX;

	/* TODO: This is platform specific */

	/* copy destructor + array dimensions */

	size_t ints_to_copy = 1 + (t->params.array.num_dims);

	if (subtilis_type_if_is_vector(t)) {
		check_size = true;
		ref_label.label = subtilis_ir_section_new_label(p->current);
		skip_ref_label.label =
		    subtilis_ir_section_new_label(p->current);
	}

	soffset.integer = SUBTIILIS_ARRAY_SIZE_OFF + source_offset;
	doffset.integer = SUBTIILIS_ARRAY_SIZE_OFF + dest_offset;

	if (doffset.integer > 0) {
		ref_start.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, dest_reg, doffset,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	} else {
		ref_start = dest_reg;
	}

	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, source_reg, soffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32, size,
					  dest_reg, doffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	soffset.integer += sizeof(int32_t);
	doffset.integer += sizeof(int32_t);

	if (check_size) {
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, size, ref_label,
		    skip_ref_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		subtilis_ir_section_add_label(p->current, ref_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	data.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, source_reg, soffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32, data,
					  dest_reg, doffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	soffset.integer += sizeof(int32_t);
	doffset.integer += sizeof(int32_t);

	heap.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, source_reg, soffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32, heap,
					  dest_reg, doffset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (ref) {
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_REF, heap, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	if (check_size) {
		subtilis_ir_section_add_label(p->current, skip_ref_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	soffset.integer += sizeof(int32_t);
	doffset.integer += sizeof(int32_t);

	for (i = 0; i < ints_to_copy; i++) {
		data.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, source_reg,
		    soffset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_STOREO_I32,
						  data, dest_reg, doffset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		soffset.integer += sizeof(int32_t);
		doffset.integer += sizeof(int32_t);
	}

	return ref_start.reg;
}

static void prv_copy_ref_base(subtilis_parser_t *p, const subtilis_type_t *t,
			      subtilis_ir_operand_t dest_reg,
			      size_t dest_offset,
			      subtilis_ir_operand_t source_reg,
			      size_t source_offset, bool ref,
			      subtilis_error_t *err)
{
	size_t ref_start;

	ref_start = prv_new_ref_base(p, t, dest_reg, dest_offset, source_reg,
				     source_offset, ref, err);
	/*
	 * It's important that this comes last as the reference we're
	 * copying might be left on the stack of a previous function.
	 * We don't want to overwrite the data before we've copied it.
	 */

	subtilis_reference_push_ref(p, t, ref_start, err);
}

void subtlis_array_type_copy_param_ref(subtilis_parser_t *p,
				       const subtilis_type_t *t,
				       subtilis_ir_operand_t dest_reg,
				       size_t dest_offset,
				       subtilis_ir_operand_t source_reg,
				       size_t source_offset,
				       subtilis_error_t *err)
{
	prv_copy_ref_base(p, t, dest_reg, dest_offset, source_reg,
			  source_offset, true, err);
}

void subtilis_array_type_create_ref(subtilis_parser_t *p, const char *var_name,
				    const subtilis_symbol_t *s, size_t mem_reg,
				    subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t dest_op;
	subtilis_ir_operand_t source_op;

	subtilis_array_type_match(p, &s->t, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_op.reg = mem_reg;
	source_op.reg = e->exp.ir_op.reg;
	prv_copy_ref_base(p, &s->t, dest_op, s->loc, source_op, 0, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_inc_cleanup_stack(p, &s->t, err);

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_array_type_init_field(subtilis_parser_t *p,
				    const subtilis_type_t *type, size_t mem_reg,
				    size_t loc, subtilis_exp_t *e,
				    subtilis_error_t *err)
{
	subtilis_ir_operand_t dest_reg;
	subtilis_ir_operand_t source_reg;

	dest_reg.reg = mem_reg;
	source_reg.reg = e->exp.ir_op.reg;

	subtilis_array_type_match(p, type, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	(void)prv_new_ref_base(p, type, dest_reg, loc, source_reg, 0, true,
			       err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:

	subtilis_exp_delete(e);
}

void subtilis_array_type_reset_field(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     size_t mem_reg, size_t loc,
				     subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_array_type_match(p, type, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * TODO: I suspect this is incorrect for arrays of RECS which need
	 * dereffing.  Check when we have initialiser for arrays of RECS.
	 */

	subtilis_array_type_assign_ref(p, type, mem_reg, loc, e->exp.ir_op.reg,
				       err);
cleanup:

	subtilis_exp_delete(e);
}

void subtlis_array_type_copy_ret(subtilis_parser_t *p, const subtilis_type_t *t,
				 size_t dest_reg, size_t source_reg,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t d;
	subtilis_ir_operand_t s;

	d.reg = dest_reg;
	s.reg = source_reg;

	prv_copy_ref_base(p, t, d, 0, s, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_inc_cleanup_stack(p, t, err);
}

void subtilis_array_type_match(subtilis_parser_t *p, const subtilis_type_t *t1,
			       const subtilis_type_t *t2, subtilis_error_t *err)
{
	if (!subtilis_type_eq(t1, t2)) {
		if (subtilis_type_if_is_array(t2) ||
		    subtilis_type_if_is_vector(t2))
			subtilis_error_set_array_type_mismatch(
			    err, p->l->stream->name, p->l->line);
		else
			subtilis_error_not_array_or_vector(
			    err, subtilis_type_name(t2), p->l->stream->name,
			    p->l->line);
	}
}

void subtilis_array_type_assign_ref_exp(subtilis_parser_t *p,
					const subtilis_type_t *type,
					size_t mem_reg, size_t loc,
					subtilis_exp_t *e,
					subtilis_error_t *err)
{
	subtilis_type_compare_diag(p, type, &e->type, err);
	if (err->type == SUBTILIS_ERROR_OK)
		subtilis_array_type_assign_ref(p, type, mem_reg, loc,
					       e->exp.ir_op.reg, err);
	subtilis_exp_delete(e);
}

void subtilis_array_type_copy_dims(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   size_t dest_mem_reg, size_t dest_loc,
				   size_t source_reg, subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t copy_reg;
	const subtilis_type_array_t *dest_type = &type->params.array;

	for (i = 0; i < dest_type->num_dims; i++) {
		if (dest_type->dims[i] != SUBTILIS_DYNAMIC_DIMENSION)
			continue;

		op0.reg = source_reg;
		op1.integer = SUBTIILIS_ARRAY_DIMS_OFF + (i * sizeof(int32_t));
		copy_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		op0.reg = copy_reg;
		op1.reg = dest_mem_reg;
		op2.integer =
		    dest_loc + SUBTIILIS_ARRAY_DIMS_OFF + (i * sizeof(int32_t));
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_STOREO_I32,
						  op0, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_array_type_assign_ref(subtilis_parser_t *p,
				    const subtilis_type_t *type,
				    size_t dest_mem_reg, size_t dest_loc,
				    size_t source_reg, subtilis_error_t *err)
{
	bool check_size = subtilis_type_if_is_vector(type);

	subtilis_reference_type_assign_ref(p, dest_mem_reg, dest_loc,
					   source_reg, check_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_copy_dims(p, type, dest_mem_reg, dest_loc,
				      source_reg, err);
}

void subtilis_array_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
				       subtilis_exp_t *e, subtilis_error_t *err)
{
	bool check_size = subtilis_type_if_is_vector(&e->type);

	subtilis_reference_type_assign_to_reg(p, reg, e, check_size, err);
}

/* Does not consume e.  Does consume  max_dim */

static void prv_check_dynamic_dim(subtilis_parser_t *p, subtilis_exp_t *e,
				  int32_t lower_bound, subtilis_exp_t *max_dim,
				  subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *maxe = NULL;

	error_label = subtilis_array_type_error_label(p);
	ok_label.label = subtilis_ir_section_new_label(p->current);

	edup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (max_dim) {
		maxe = subtilis_type_if_dup(e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	zero = subtilis_exp_new_int32(lower_bound, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	edup = subtilis_type_if_gte(p, edup, zero, err);
	zero = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (max_dim) {
		maxe = subtilis_type_if_lte(p, maxe, max_dim, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		max_dim = NULL;
		edup = subtilis_type_if_and(p, edup, maxe, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		maxe = NULL;
	}

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  edup->exp.ir_op, ok_label,
					  error_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, ok_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

cleanup:

	subtilis_exp_delete(max_dim);
	subtilis_exp_delete(maxe);
	subtilis_exp_delete(edup);
}

subtilis_exp_t *subtilis_array_size_calc(subtilis_parser_t *p,
					 const char *var_name,
					 subtilis_exp_t **e, size_t index_count,
					 subtilis_error_t *err)
{
	size_t i;
	bool non_const_dim = false;
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *one = NULL;
	int64_t array_size = 1;

	/*
	 * TODO: This whole thing is 32 bit specific.  At least the
	 * pointer needs to be platform specific although we may still
	 * keep maximum array sizes to 32 bits.
	 */

	/* TODO:
	 * I don't see any way to detect whether the array dimensions
	 * overflow 32 bits. For constants we can use 64 bit integers to
	 * store the size. This would then limit us to using 32 bits for
	 * the array size, even on 64 bit builds (unless we resort to
	 * instrinsics) as there are no 128 bit integers in C.  For the
	 * ARM2 backend, there is no way to detect overflow of dynamic
	 * vars.  This could be done on StrongARM using UMULL.  Perhaps
	 * we could execute different code on SA here.  For now, be
	 * careful with those array bounds!
	 */

	for (i = 0; i < index_count; i++) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER) {
			non_const_dim = true;
			continue;
		}
		if (e[i]->exp.ir_op.integer < 0) {
			subtilis_error_bad_dim(err, var_name,
					       p->l->stream->name, p->l->line);
			return NULL;
		}
		array_size = array_size * (e[i]->exp.ir_op.integer + 1);
	}

	if (array_size > (int64_t)0x7ffffff) {
		subtilis_error_bad_dim(err, var_name, p->l->stream->name,
				       p->l->line);
		return NULL;
	}

	sizee = subtilis_exp_new_int32((int32_t)array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (non_const_dim) {
		sizee = subtilis_type_if_exp_to_var(p, sizee, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		for (i = 0; i < index_count; i++) {
			if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER) {
				prv_check_dynamic_dim(p, e[i], 0, NULL, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				edup = subtilis_type_if_dup(e[i], err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				one = subtilis_exp_new_int32(1, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				edup = subtilis_type_if_add(p, edup, one, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
				sizee =
				    subtilis_type_if_mul(p, sizee, edup, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;
			}
		}
	}

	return sizee;

cleanup:

	subtilis_exp_delete(sizee);

	return NULL;
}

static subtilis_exp_t *prv_get_dynamic_dim_size(subtilis_parser_t *p,
						int32_t dim_size,
						int32_t offset, size_t mem_reg,
						subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t reg;
	subtilis_exp_t *maxe = NULL;

	if (dim_size == SUBTILIS_DYNAMIC_DIMENSION) {
		op0.reg = mem_reg;
		op1.integer = offset;
		reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
		maxe = subtilis_exp_new_int32_var(reg, err);
	} else {
		maxe = subtilis_exp_new_int32(dim_size, err);
	}

	return maxe;
}

/* Does not consume e. */

static subtilis_exp_t *prv_check_dynamic_index(subtilis_parser_t *p,
					       subtilis_exp_t *e,
					       int32_t dim_size, int32_t offset,
					       size_t mem_reg,
					       subtilis_error_t *err)
{
	subtilis_exp_t *maxe;
	subtilis_exp_t *maxe_dup = NULL;

	maxe = prv_get_dynamic_dim_size(p, dim_size, offset, mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	maxe_dup = subtilis_type_if_dup(maxe, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_check_dynamic_dim(p, e, 0, maxe_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return maxe;

cleanup:

	subtilis_exp_delete(maxe);

	return NULL;
}

/*
 * Functions that use arrays will typically contain a lot of error
 * checks and each of these checks can lead to code being called that
 * generates an error. We don't want to duplicate that code for each
 * error, so we create the error generation code once per function when
 * it's first needed and retain the label in the section so it can
 * jumped to when needed.
 */

subtilis_ir_operand_t subtilis_array_type_error_label(subtilis_parser_t *p)
{
	subtilis_ir_operand_t error_label;

	if (p->current->array_access == SIZE_MAX) {
		error_label.label = subtilis_ir_section_new_label(p->current);
		p->current->array_access = error_label.label;
	} else {
		error_label.label = p->current->array_access;
	}

	return error_label;
}

void subtilis_array_gen_index_error_code(subtilis_parser_t *p,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	subtilis_exp_t *ecode = NULL;

	if (p->current->array_access == SIZE_MAX)
		return;

	error_label.label = p->current->array_access;

	subtilis_ir_section_add_label(p->current, error_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ecode = subtilis_exp_new_int32(SUBTILIS_ERROR_CODE_BAD_DIM, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_generate_error(p, ecode, err);
}

static void prv_check_last_index(subtilis_parser_t *p, const char *var_name,
				 subtilis_exp_t **e,
				 const subtilis_type_t *type, int32_t offset,
				 size_t mem_reg, subtilis_error_t *err)
{
	int32_t dim_size;
	size_t i = type->params.array.num_dims - 1;
	subtilis_exp_t *maxe = NULL;

	dim_size = type->params.array.dims[i];
	if ((dim_size != SUBTILIS_DYNAMIC_DIMENSION) &&
	    (e[i]->type.type == SUBTILIS_TYPE_CONST_INTEGER)) {
		if ((e[i]->exp.ir_op.integer < 0) ||
		    (e[i]->exp.ir_op.integer > dim_size)) {
			subtilis_error_bad_index(
			    err, var_name, p->l->stream->name, p->l->line);
			return;
		}
	} else {
		maxe = prv_check_dynamic_index(p, e[i], dim_size, offset,
					       mem_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_exp_delete(maxe);
	}
}

static subtilis_exp_t *prv_compute_element_address(subtilis_parser_t *p,
						   const subtilis_type_t *type,
						   size_t mem_reg, size_t loc,
						   subtilis_exp_t *sizee,
						   subtilis_error_t *err)
{
	subtilis_exp_t *base;

	base =
	    subtilis_type_if_load_from_mem(p, &subtilis_type_integer, mem_reg,
					   loc + SUBTIILIS_ARRAY_DATA_OFF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if ((sizee->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
	    (sizee->exp.ir_op.integer == 0)) {
		subtilis_exp_delete(sizee);
		return base;
	}

	sizee = subtilis_type_if_data_size(p, type, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return subtilis_type_if_add(p, base, sizee, err);

cleanup:

	subtilis_exp_delete(base);

	return NULL;
}

static subtilis_exp_t *
prv_1d_index_calc(subtilis_parser_t *p, const char *var_name,
		  const subtilis_type_t *type, size_t mem_reg, size_t loc,
		  subtilis_exp_t **e, size_t index_count, subtilis_error_t *err)
{
	subtilis_exp_t *sizee = NULL;
	int32_t offset = loc + SUBTIILIS_ARRAY_DIMS_OFF;

	prv_check_last_index(p, var_name, e, type, offset, mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	sizee = subtilis_type_if_dup(e[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return prv_compute_element_address(p, type, mem_reg, loc, sizee, err);
}

static int64_t prv_check_constant_dims(subtilis_parser_t *p, subtilis_exp_t **e,
				       size_t index_count, const char *var_name,
				       const subtilis_type_t *type,
				       int *dyn_start, int32_t *block_size,
				       subtilis_error_t *err)
{
	int i;
	int j;
	int32_t dim_size;
	int64_t prod;
	int32_t bsize = 1;
	int dstart = index_count - 1;
	int64_t array_size = 0;

	for (i = index_count - 1; i >= 0; i--) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER)
			break;
		dim_size = type->params.array.dims[i];
		if (dim_size == SUBTILIS_DYNAMIC_DIMENSION)
			break;

		if ((e[i]->exp.ir_op.integer < 0) ||
		    (e[i]->exp.ir_op.integer > dim_size)) {
			subtilis_error_bad_index(
			    err, var_name, p->l->stream->name, p->l->line);
			return 0;
		}
		dstart--;
	}

	*dyn_start = dstart;

	/*
	 * i now stores the last dimension (working backwards) where the
	 * index and the dimension are both known at compile time.
	 */

	i++;
	for (j = i; j < index_count; j++) {
		dim_size = type->params.array.dims[j] + 1;
		bsize *= dim_size;
	}

	*block_size = bsize;

	for (; i < index_count - 1; i++) {
		prod = e[i]->exp.ir_op.integer;
		for (j = i + 1; j < index_count; j++) {
			dim_size = type->params.array.dims[j] + 1;
			prod *= dim_size;
		}

		array_size += prod;
	}

	return array_size;
}

subtilis_exp_t *
subtilis_array_index_calc(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_exp_t **e, size_t index_count,
			  subtilis_error_t *err)
{
	int i;
	int32_t dim_size;
	size_t last_index;
	subtilis_exp_t *one;
	int32_t offset;
	int32_t last_offset;
	int32_t block_size = 0;
	int dyn_start = 0;
	subtilis_exp_t *blocke = NULL;
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *maxe = NULL;
	subtilis_exp_t *blocke_dup = NULL;
	int64_t array_size;

	if (index_count != type->params.array.num_dims) {
		subtilis_error_bad_index_count(err, var_name,
					       p->l->stream->name, p->l->line);
		return NULL;
	}

	if (index_count == 1)
		return prv_1d_index_calc(p, var_name, type, mem_reg, loc, e,
					 index_count, err);

	offset = loc + SUBTIILIS_ARRAY_DIMS_OFF;

	/*
	 * First we compute the constant part of the array indexing.
	 * This can all be done at compile time if both the indexes and
	 * the dimensions are known to the compiler.  We start with the
	 * final index and work our way toward the first index.  Two
	 * sizes are returned here.
	 *
	 * array_size - the offset in elements from the beginning of the
	 * array we know we need to skip.  This does not include the
	 * offset of the last index, which is added later.
	 *
	 * block_size - the cummulative product elements of all the
	 * trailing dimensions that we've processed. dyn_start -
	 * contains the first index that could not be processed at
	 * compile time.  If dyn_start == indxex_count -1 none of the
	 * indices could be processed at compile time.
	 */

	array_size = prv_check_constant_dims(p, e, index_count, var_name, type,
					     &dyn_start, &block_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	sizee = subtilis_exp_new_int32((int32_t)array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	last_index = index_count - 1;
	last_offset = offset + (sizeof(int32_t) * last_index);
	if (dyn_start == index_count - 1) {
		/*
		 * none of the indices could be processed at compile
		 * time, which means we don't know the block_size for
		 * the last element which the loop below needs.  So we
		 * need to compute it.
		 */

		dim_size = type->params.array.dims[last_index];
		maxe = prv_get_dynamic_dim_size(p, dim_size, last_offset,
						mem_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		one = subtilis_exp_new_int32(1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		blocke = subtilis_type_if_add(p, maxe, one, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		dyn_start--;
	} else {
		blocke = subtilis_exp_new_int32(block_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	offset += (sizeof(int32_t) * dyn_start);

	/* Need to compute  the dynamic portion of the offset array */

	for (i = dyn_start; i >= 0; i--) {
		dim_size = type->params.array.dims[i];

		maxe = prv_check_dynamic_index(p, e[i], dim_size, offset,
					       mem_reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		edup = subtilis_type_if_dup(e[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		one = subtilis_exp_new_int32(1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		maxe = subtilis_type_if_add(p, maxe, one, err);
		one = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		blocke_dup = subtilis_type_if_dup(blocke, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		edup = subtilis_type_if_mul(p, blocke_dup, edup, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		blocke = subtilis_type_if_mul(p, blocke, maxe, err);
		maxe = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		sizee = subtilis_type_if_add(p, sizee, edup, err);
		edup = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		offset -= sizeof(int32_t);
	}

	subtilis_exp_delete(blocke);

	/*
	 * Check and add the offset for the final index.  If it's
	 * constant and the last dimension is constant we'll have
	 * already checked it but hey hum.
	 */

	prv_check_last_index(p, var_name, e, type, last_offset, mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	edup = subtilis_type_if_dup(e[index_count - 1], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_add(p, edup, sizee, err);
	edup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return prv_compute_element_address(p, type, mem_reg, loc, sizee, err);

cleanup:

	subtilis_exp_delete(blocke);
	subtilis_exp_delete(maxe);
	subtilis_exp_delete(edup);
	subtilis_exp_delete(sizee);

	return NULL;
}

void subtilis_array_write(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type,
			  const subtilis_type_t *el_type, size_t mem_reg,
			  size_t loc, subtilis_exp_t *e,
			  subtilis_exp_t **indices, size_t index_count,
			  subtilis_error_t *err)
{
	subtilis_exp_t *offset;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (e->partial_name)
		subtilis_parser_call_add_addr(p, el_type, e, err);
	else
		e = subtilis_type_if_coerce_type(p, e, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, offset->exp.ir_op.reg, 0, e, err);
	e = NULL;

cleanup:

	subtilis_exp_delete(offset);
	subtilis_exp_delete(e);
}

subtilis_exp_t *subtilis_array_read(subtilis_parser_t *p, const char *var_name,
				    const subtilis_type_t *type,
				    const subtilis_type_t *el_type,
				    size_t mem_reg, size_t loc,
				    subtilis_exp_t **indices,
				    size_t index_count, subtilis_error_t *err)
{
	subtilis_exp_t *offset;
	subtilis_exp_t *e;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_type_if_load_from_mem(p, el_type, offset->exp.ir_op.reg, 0,
					   err);
	subtilis_exp_delete(offset);
	return e;
}

void subtilis_array_add(subtilis_parser_t *p, const char *var_name,
			const subtilis_type_t *type,
			const subtilis_type_t *el_type, size_t mem_reg,
			size_t loc, subtilis_exp_t *e, subtilis_exp_t **indices,
			size_t index_count, subtilis_error_t *err)
{
	subtilis_exp_t *offset;
	subtilis_exp_t *cur_val;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cur_val = subtilis_type_if_load_from_mem(p, el_type,
						 offset->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_add(p, cur_val, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_coerce_type(p, e, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, offset->exp.ir_op.reg, 0, e, err);

cleanup:
	subtilis_exp_delete(offset);
}

void subtilis_array_sub(subtilis_parser_t *p, const char *var_name,
			const subtilis_type_t *type,
			const subtilis_type_t *el_type, size_t mem_reg,
			size_t loc, subtilis_exp_t *e, subtilis_exp_t **indices,
			size_t index_count, subtilis_error_t *err)
{
	subtilis_exp_t *offset;
	subtilis_exp_t *cur_val;

	offset = subtilis_array_index_calc(p, var_name, type, mem_reg, loc,
					   indices, index_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cur_val = subtilis_type_if_load_from_mem(p, el_type,
						 offset->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_sub(p, cur_val, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_coerce_type(p, e, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, offset->exp.ir_op.reg, 0, e, err);

cleanup:
	subtilis_exp_delete(offset);
}

static void prv_check_dimension_num(subtilis_parser_t *p, subtilis_exp_t *e,
				    int32_t max_dims, subtilis_error_t *err)
{
	subtilis_exp_t *limit = NULL;

	limit = subtilis_exp_new_int32(max_dims, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_check_dynamic_dim(p, e, 1, limit, err);
}

subtilis_exp_t *subtilis_array_get_dim(subtilis_parser_t *p, subtilis_exp_t *ar,
				       subtilis_exp_t *dim,
				       subtilis_error_t *err)
{
	int32_t requested_dim;
	int32_t dim_index;
	size_t num_dims;
	subtilis_ir_operand_t op;
	subtilis_exp_t *e = NULL;

	/*
	 * This is unfortunate as DIM(a()) may generate one add
	 * instruction for the array reference even though we don't
	 * actually need it. Hopefully, the optimiser will be able to
	 * get rid of it.
	 */

	num_dims = ar->type.params.array.num_dims;
	if (!dim) {
		e = subtilis_exp_new_int32(num_dims, err);
	} else {
		if (dim->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			if ((dim->exp.ir_op.integer <= 0) ||
			    (dim->exp.ir_op.integer > num_dims)) {
				subtilis_error_bad_index(
				    err, "dim", p->l->stream->name, p->l->line);
				goto cleanup;
			}
			dim_index = dim->exp.ir_op.integer - 1;
			requested_dim = ar->type.params.array.dims[dim_index];
			if (requested_dim != SUBTILIS_DYNAMIC_DIMENSION) {
				e = subtilis_exp_new_int32(requested_dim, err);
			} else {
				e = subtilis_type_if_load_from_mem(
				    p, &subtilis_type_integer,
				    ar->exp.ir_op.reg,
				    SUBTIILIS_ARRAY_DIMS_OFF +
					(sizeof(int32_t) * dim_index),
				    err);
			}
		} else {
			prv_check_dimension_num(p, dim, num_dims, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			op.integer = 1;
			dim->exp.ir_op.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_SUBI_I32,
			    dim->exp.ir_op, op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			/* TODO This is int32 specific */
			op.integer = 2;
			dim->exp.ir_op.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_LSLI_I32,
			    dim->exp.ir_op, op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			op.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_ADD_I32,
			    ar->exp.ir_op, dim->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			e = subtilis_type_if_load_from_mem(
			    p, &subtilis_type_integer, op.reg,
			    SUBTIILIS_ARRAY_DIMS_OFF, err);
		}
	}

cleanup:

	subtilis_exp_delete(ar);
	subtilis_exp_delete(dim);

	return e;
}

subtilis_exp_t *subtilis_array_type_dynamic_size(subtilis_parser_t *p,
						 size_t mem_reg, size_t loc,
						 subtilis_error_t *err)
{
	return subtilis_type_if_load_from_mem(
	    p, &subtilis_type_integer, mem_reg, loc + SUBTIILIS_ARRAY_SIZE_OFF,
	    err);
}

void subtilis_array_type_deref_els(subtilis_parser_t *p, size_t data_reg,
				   size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t data_end;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t conde;
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t end;

	start.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);

	/* size_reg must be > 0 */

	op0.reg = data_reg;
	counter.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = size_reg;
	data_end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, counter, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_deref(p, counter.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = subtilis_type_if_size(&subtilis_type_string, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, counter, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	conde.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, counter, data_end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde, start, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end.label, err);
}

void subtilis_array_type_ref_els(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t data_reg,
				 size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t data_end;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t conde;
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t end;

	start.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);

	/* size_reg must be > 0 */

	op0.reg = data_reg;
	counter.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = size_reg;
	data_end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, counter, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type->type == SUBTILIS_TYPE_REC) {
		if (!subtilis_type_rec_need_deref(type)) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		subtilis_rec_type_ref(p, type, counter.reg, 0, err);
	} else {
		if (!subtilis_type_if_is_reference(type)) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		subtilis_reference_type_ref(
		    p, counter.reg, 0, type->type == SUBTILIS_TYPE_STRING, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = subtilis_type_if_size(type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, counter, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	conde.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, counter, data_end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde, start, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end.label, err);
}

void subtilis_array_type_deref_recs(subtilis_parser_t *p, size_t data_reg,
				    size_t size_reg, size_t el_size_reg,
				    size_t el_destruct_reg,
				    subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t data_end;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t conde;
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t end;
	subtilis_ir_arg_t *ir_args;

	start.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);

	/* size_reg must be > 0 */

	op0.reg = data_reg;
	counter.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = size_reg;
	data_end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, counter, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ir_args = malloc(sizeof(*ir_args));
	if (!ir_args) {
		subtilis_error_set_oom(err);
		return;
	}

	ir_args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	ir_args[0].reg = counter.reg;
	ir_args[0].nop = SIZE_MAX;

	subtilis_ir_section_add_call_ptr(p->current, 1, ir_args,
					 el_destruct_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.reg = el_size_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_ADD_I32,
					  counter, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	conde.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, counter, data_end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde, start, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end.label, err);
}

void subtilis_array_type_copy_els(subtilis_parser_t *p,
				  const subtilis_type_t *el_type,
				  size_t data_reg, size_t size_reg,
				  size_t src_reg, bool deref,
				  subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t data_end;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t counter2;
	subtilis_ir_operand_t conde;
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t end;

	start.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);

	/* size_reg must be > 0 */

	op0.reg = data_reg;
	counter.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = src_reg;
	counter2.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOV, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = size_reg;
	data_end.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, counter, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * RECords are a special case.  They're not reference types but
	 * they can hold references.  If we get here, our RECord is not
	 * scalar.
	 */
	if (el_type->type == SUBTILIS_TYPE_REC) {
		subtilis_builtin_ir_rec_copy(p, el_type, counter.reg,
					     counter2.reg, !deref, err);
	} else {
		if (deref) {
			subtilis_reference_type_deref(p, counter.reg, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}

		subtilis_reference_type_init_ref(p, counter.reg, 0,
						 counter2.reg, true, true, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = subtilis_type_if_size(el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, counter, counter, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_ADDI_I32, counter2,
					  counter2, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	conde.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, counter, data_end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  conde, start, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, end.label, err);
}

static size_t prv_grow_wrapper(subtilis_parser_t *p,
			       const subtilis_type_t *el_type,
			       size_t a1_mem_reg, size_t a1_size_reg,
			       size_t gran_reg, size_t a2_size_reg,
			       subtilis_error_t *err)
{
	subtilis_ir_operand_t get_ref_label;
	subtilis_ir_operand_t empty_label;
	subtilis_ir_operand_t ref_label;
	subtilis_ir_operand_t skip_ref_label;
	subtilis_ir_operand_t a1_heap;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t get_ref;
	subtilis_ir_operand_t get_new_ref;
	subtilis_ir_operand_t condee;
	subtilis_exp_t *e;
	size_t dest_reg;
	size_t data_reg;
	bool check_ref = subtilis_type_if_is_reference(el_type) ||
			 ((el_type->type == SUBTILIS_TYPE_REC) &&
			  subtilis_type_rec_need_deref(el_type));

	/*
	 * If we're dealing with a collection of references or structures that
	 * contain references and the reference count of the collection is > 1
	 * before the call to ref_grow and has fallen by 1 after the call to
	 * grow, we know a copy has taken place.  In this case we need to ref
	 * all the copied elements.  We can't do this in ref_grow as this
	 * function is type agnostic.
	 */

	/*
	 * TODO: It may be worth providing a specific builtin for growing
	 * collection of references.  RECs are trickier as they have custom
	 * reffers and dereffers.  But do we really want to duplicate all this
	 * code each time we append a string.
	 */

	if (check_ref) {
		get_ref_label.label = subtilis_ir_section_new_label(p->current);
		empty_label.label = subtilis_ir_section_new_label(p->current);

		op1.integer = 0;
		get_ref.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		condee.reg = a1_size_reg;
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, get_ref_label,
		    empty_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_label(p->current, get_ref_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		op1.reg = a1_mem_reg;
		op2.integer = SUBTIILIS_REFERENCE_HEAP_OFF;
		a1_heap.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_instr_no_reg2(p->current,
						      SUBTILIS_OP_INSTR_GETREF,
						      get_ref, a1_heap, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		subtilis_ir_section_add_label(p->current, empty_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	e = subtilis_builtin_ir_call_ref_grow(p, a1_mem_reg, a1_size_reg,
					      gran_reg, a2_size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;
	dest_reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	if (check_ref) {
		ref_label.label = subtilis_ir_section_new_label(p->current);
		skip_ref_label.label =
		    subtilis_ir_section_new_label(p->current);

		op1.reg = a1_mem_reg;
		op2.integer = SUBTIILIS_REFERENCE_HEAP_OFF;
		a1_heap.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		get_new_ref.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_GETREF, a1_heap, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LT_I32, get_new_ref, get_ref,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, ref_label,
		    skip_ref_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_label(p->current, ref_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		data_reg = subtilis_reference_get_data(p, a1_mem_reg, 0, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		subtilis_array_type_ref_els(p, el_type, data_reg, a1_size_reg,
					    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_ir_section_add_label(p->current, skip_ref_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	return dest_reg;
}

static subtilis_exp_t *prv_check_gran(subtilis_parser_t *p,
				      subtilis_exp_t *gran,
				      subtilis_error_t *err)
{
	char num[32];
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;

	gran = subtilis_type_if_to_int(p, gran, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (gran->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
		if (gran->exp.ir_op.integer <= 0) {
			snprintf(num, sizeof(num), "%d",
				 gran->exp.ir_op.integer);
			subtilis_error_set_expected(err, "positive integer",
						    num, p->l->stream->name,
						    p->l->line);
		}
	} else {
		/* Need a runtime check */

		zero.integer = 0;
		condee.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_GTI_I32, gran->exp.ir_op,
		    zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		error_label = subtilis_array_type_error_label(p);
		ok_label.label = subtilis_ir_section_new_label(p->current);

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, condee, ok_label,
		    error_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		subtilis_ir_section_add_label(p->current, ok_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	}

	return gran;
}

void subtilis_array_append_scalar(subtilis_parser_t *p, subtilis_exp_t *a1,
				  subtilis_exp_t *a2, subtilis_exp_t *gran,
				  subtilis_error_t *err)
{
	subtilis_type_t el_type;
	size_t dest_reg;
	size_t gran_reg;
	subtilis_ir_operand_t a1_size;
	subtilis_ir_operand_t a2_size;
	subtilis_ir_operand_t op1;
	subtilis_exp_t *el_size_e;
	size_t el_size;

	if (!subtilis_type_if_is_vector(&a1->type)) {
		subtilis_error_set_expected(err, "dynamic 1d array",
					    subtilis_type_name(&a1->type),
					    p->l->stream->name, p->l->line);
		goto free_as;
	}

	subtilis_type_if_element_type(p, &a1->type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_as;

	if (a2->partial_name) {
		subtilis_parser_call_add_addr(p, &el_type, a2, err);
	} else {
		a2 = subtilis_type_if_coerce_type(p, a2, &el_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto free_as;
		a2 = subtilis_type_if_exp_to_var(p, a2, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_as;

	el_size = subtilis_type_if_size(&el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_as;

	op1.integer = el_size;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a1_size.reg =
	    subtilis_reference_type_get_size(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_size.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (gran) {
		gran = prv_check_gran(p, gran, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto free_as;
		el_size_e = subtilis_exp_new_int32(el_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto free_as;
		gran = subtilis_type_if_mul(p, gran, el_size_e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto free_as;
		gran = subtilis_type_if_exp_to_var(p, gran, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto free_as;
		gran_reg = gran->exp.ir_op.reg;
	} else {
		gran_reg = a2_size.reg;
	}

	dest_reg = prv_grow_wrapper(p, &el_type, a1->exp.ir_op.reg, a1_size.reg,
				    gran_reg, a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_new_mem(p, dest_reg, 0, a2, err);
	a2 = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2 = subtilis_type_if_load_from_mem(p, &subtilis_type_integer,
					    a1->exp.ir_op.reg,
					    SUBTIILIS_ARRAY_DIMS_OFF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = 1;
	a2->exp.ir_op.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, a2->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, a1->exp.ir_op.reg,
				       SUBTIILIS_ARRAY_DIMS_OFF, a2, err);
	a2 = NULL;

cleanup:
	subtilis_type_free(&el_type);
free_as:
	subtilis_exp_delete(gran);
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
}

static size_t prv_div_by_const(subtilis_parser_t *p, size_t reg,
			       int32_t divisor, subtilis_error_t *err)
{
	subtilis_exp_t *a1;
	subtilis_exp_t *a2 = NULL;

	a1 = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	a2 = subtilis_exp_new_int32(divisor, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(a1);
		return SIZE_MAX;
	}

	a1 = subtilis_type_if_div(p, a1, a2, err);
	reg = a1->exp.ir_op.reg;
	subtilis_exp_delete(a1);

	return reg;
}

static void prv_check_arrays_compat(subtilis_parser_t *p,
				    const subtilis_type_t *el_type,
				    subtilis_exp_t *a1, subtilis_exp_t *a2,
				    subtilis_error_t *err)
{
	subtilis_type_t el_type2;

	if (a1->type.type != a2->type.type) {
		/*
		 * Check to see if a2 is an array.  We can append an
		 * array to a vector, but not the other way around.
		 */

		/*
		 * Number of dimensions aren't important here.  We're
		 * just going to copy all the elements from a2.
		 */

		if (!subtilis_type_if_is_array(&a2->type)) {
			subtilis_error_set_array_type_mismatch(
			    err, p->l->stream->name, p->l->line);
			return;
		}

		subtilis_type_if_element_type(p, &a2->type, &el_type2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (!subtilis_type_eq(el_type, &el_type2))
			subtilis_error_set_array_type_mismatch(
			    err, p->l->stream->name, p->l->line);

		subtilis_type_free(&el_type2);
	}
}

void subtilis_array_append_scalar_array(subtilis_parser_t *p,
					subtilis_exp_t *a1, subtilis_exp_t *a2,
					subtilis_error_t *err)
{
	subtilis_type_t el_type;
	subtilis_ir_operand_t el_size;
	size_t dest_reg;
	subtilis_ir_operand_t a1_size;
	subtilis_ir_operand_t a2_size;
	subtilis_ir_operand_t a2_size_zero;
	subtilis_ir_operand_t a2_size_gt_zero;
	subtilis_ir_operand_t a2_data;
	subtilis_ir_operand_t op1;
	bool a2_dynamic;
	subtilis_exp_t *e;

	a2_size_zero.label = SIZE_MAX;
	el_type.type = SUBTILIS_TYPE_VOID;

	if (!subtilis_type_if_is_vector(&a1->type)) {
		subtilis_error_set_expected(err, "vector",
					    subtilis_type_name(&a1->type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	subtilis_type_if_element_type(p, &a1->type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_check_arrays_compat(p, &el_type, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	el_size.integer = subtilis_type_if_size(&el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_size.reg =
	    subtilis_reference_type_get_size(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_dynamic = subtilis_type_if_is_vector(&a2->type);
	if (a2_dynamic) {
		a2_size_zero.label = subtilis_ir_section_new_label(p->current);
		a2_size_gt_zero.label =
		    subtilis_ir_section_new_label(p->current);

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, a2_size,
		    a2_size_gt_zero, a2_size_zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, a2_size_gt_zero.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	a1_size.reg =
	    subtilis_reference_type_get_size(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_builtin_ir_call_ref_grow(p, a1->exp.ir_op.reg, a1_size.reg,
					      a2_size.reg, a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	dest_reg = e->exp.ir_op.reg;
	subtilis_exp_delete(e);

	a2_data.reg = subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_reference_type_memcpy_dest(p, dest_reg, a2_data.reg,
					    a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(a2);
	a2 = subtilis_type_if_load_from_mem(p, &subtilis_type_integer,
					    a1->exp.ir_op.reg,
					    SUBTIILIS_ARRAY_DIMS_OFF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (el_size.integer > 1) {
		op1.reg =
		    prv_div_by_const(p, a2_size.reg, el_size.integer, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else {
		op1.reg = a2_size.reg;
	}

	a2->exp.ir_op.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a2->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, a1->exp.ir_op.reg,
				       SUBTIILIS_ARRAY_DIMS_OFF, a2, err);
	a2 = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (a2_dynamic)
		subtilis_ir_section_add_label(p->current, a2_size_zero.label,
					      err);

cleanup:
	subtilis_type_free(&el_type);
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
}

void subtilis_array_append_ref_array(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err)
{
	subtilis_type_t el_type;
	subtilis_ir_operand_t el_size;
	size_t dest_reg;
	subtilis_ir_operand_t a1_size;
	subtilis_ir_operand_t a2_size;
	subtilis_ir_operand_t a2_size_zero;
	subtilis_ir_operand_t a2_size_gt_zero;
	subtilis_ir_operand_t a2_data;
	subtilis_ir_operand_t op1;
	bool a2_dynamic;

	a2_size_zero.label = SIZE_MAX;

	if (!subtilis_type_if_is_vector(&a1->type)) {
		subtilis_error_set_expected(err, "vector",
					    subtilis_type_name(&a1->type),
					    p->l->stream->name, p->l->line);
		goto free_as;
	}

	subtilis_type_if_element_type(p, &a1->type, &el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto free_as;

	prv_check_arrays_compat(p, &el_type, a1, a2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	el_size.integer = subtilis_type_if_size(&el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_size.reg =
	    subtilis_reference_type_get_size(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_dynamic = subtilis_type_if_is_vector(&a2->type);
	if (a2_dynamic) {
		a2_size_zero.label = subtilis_ir_section_new_label(p->current);
		a2_size_gt_zero.label =
		    subtilis_ir_section_new_label(p->current);

		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, a2_size,
		    a2_size_gt_zero, a2_size_zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		subtilis_ir_section_add_label(p->current, a2_size_gt_zero.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	a1_size.reg =
	    subtilis_reference_type_get_size(p, a1->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	dest_reg = prv_grow_wrapper(p, &el_type, a1->exp.ir_op.reg, a1_size.reg,
				    a2_size.reg, a2_size.reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2_data.reg = subtilis_reference_get_data(p, a2->exp.ir_op.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_copy_els(p, &el_type, dest_reg, a2_size.reg,
				     a2_data.reg, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(a2);
	a2 = subtilis_type_if_load_from_mem(p, &subtilis_type_integer,
					    a1->exp.ir_op.reg,
					    SUBTIILIS_ARRAY_DIMS_OFF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = prv_div_by_const(p, a2_size.reg, el_size.integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	a2->exp.ir_op.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, a2->exp.ir_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, a1->exp.ir_op.reg,
				       SUBTIILIS_ARRAY_DIMS_OFF, a2, err);
	a2 = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (a2_dynamic)
		subtilis_ir_section_add_label(p->current, a2_size_zero.label,
					      err);

cleanup:
	subtilis_type_free(&el_type);

free_as:
	subtilis_exp_delete(a2);
	subtilis_exp_delete(a1);
}

void subtilis_array_type_dup(subtilis_exp_t *src, subtilis_exp_t *dst,
			     subtilis_error_t *err)
{
	subtilis_type_copy(&dst->type, &src->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (src->temporary) {
		dst->temporary = malloc(strlen(src->temporary) + 1);
		if (!dst->temporary) {
			subtilis_error_set_oom(err);
			return;
		}
		strcpy(dst->temporary, src->temporary);
	} else {
		dst->temporary = NULL;
	}
	dst->exp.ir_op.reg = src->exp.ir_op.reg;
}
