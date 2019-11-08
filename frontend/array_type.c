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
#include "expression.h"
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
 * | Size of DIM 1     |           8 |
 *  ----------------------------------
 * | Size of DIM n     | 4 * n * 4   |
 *  ----------------------------------
 */

#define SUBTIILIS_ARRAY_SIZE_OFF 0
#define SUBTIILIS_ARRAY_DATA_OFF 4
#define SUBTIILIS_ARRAY_DIMS_OFF 8

size_t subtilis_array_type_size(const subtilis_type_t *type)
{
	size_t size;

	/*
	 * TODO: Size appropriately for 64 bit builds.
	 */

	/*
	 * We need, on 32 bit builds,
	 * 4 bytes for the pointer
	 * 4 bytes for the size
	 * 4 bytes for each dimension unknown at compile time.
	 */

	size = 4 + 4 + type->params.array.num_dims * 4;

	return size;
}

/* Does not consume elements in e */

void subtilis_array_type_init(subtilis_parser_t *p,
			      const subtilis_type_t *element_type,
			      subtilis_type_t *type, subtilis_exp_t **e,
			      size_t dims, subtilis_error_t *err)
{
	size_t i;

	if (dims > SUBTILIS_MAX_DIMENSIONS) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_type_if_array_of(p, element_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	type->params.array.num_dims = dims;
	for (i = 0; i < dims; i++) {
		if (e[i]->type.type == SUBTILIS_TYPE_CONST_INTEGER)
			type->params.array.dims[i] = e[i]->exp.ir_op.integer;
		else
			type->params.array.dims[i] = SUBTILIS_DYNAMIC_DIMENSION;
	}
}

static void prv_memset_array(subtilis_parser_t *p, size_t base_reg,
			     size_t size_reg, size_t val_reg,
			     subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memset[] = "_memseti32";

	name = malloc(sizeof(memset));
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name, memset);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = base_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = size_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = val_reg;

	(void)subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMSETI32, NULL,
				    args, &subtilis_type_void, 3, err);
}

void subtlis_array_type_allocate(subtilis_parser_t *p, const char *var_name,
				 subtilis_type_t *type, size_t loc,
				 subtilis_exp_t **e,
				 subtilis_ir_operand_t store_reg,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_ir_operand_t op1;
	size_t i;
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *zero = NULL;
	int32_t offset;

	if (loc + subtilis_array_type_size(type) > 0x7fffffff) {
		subtilis_error_set_assertion_failed(err);
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

	/*
	 * We need to zero out the pointer in case alloc fails.  Our variable
	 * has been created at this point, so the cleanup code will attempt to
	 * free it, unless the pointer is zero.
	 */

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = loc + SUBTIILIS_ARRAY_DATA_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  zero->exp.ir_op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_data_size(p, type, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = loc + SUBTIILIS_ARRAY_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, sizee->exp.ir_op,
	    store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.integer = loc + SUBTIILIS_ARRAY_DATA_OFF;
	op.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_ALLOC, sizee->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op, store_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_memset_array(p, op.reg, sizee->exp.ir_op.reg, zero->exp.ir_op.reg,
			 err);

cleanup:

	subtilis_exp_delete(zero);
	subtilis_exp_delete(sizee);
}

void subtlis_array_type_deallocate_nc(subtilis_parser_t *p, size_t loc,
				      subtilis_ir_operand_t load_reg,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t offset;

	op1.integer = (int32_t)loc + SUBTIILIS_ARRAY_DATA_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, load_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_DEREF, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

void subtlis_array_type_deallocate(subtilis_parser_t *p, size_t loc,
				   subtilis_ir_operand_t load_reg,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t free_label;
	subtilis_ir_operand_t skip_label;

	free_label.label = subtilis_ir_section_new_label(p->current);
	skip_label.label = subtilis_ir_section_new_label(p->current);

	op1.integer = (int32_t)loc + SUBTIILIS_ARRAY_DATA_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, load_reg, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero.integer = 0;
	zero.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_NEQI_I32, offset, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  zero, free_label, skip_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, free_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_DEREF, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, skip_label.label, err);
}

/* Does not consume e.  Does consume  max_dim */

static void prv_check_dynamic_dim(subtilis_parser_t *p, subtilis_exp_t *e,
				  subtilis_exp_t *max_dim,
				  subtilis_error_t *err)
{
	subtilis_ir_operand_t error_label;
	subtilis_ir_operand_t ok_label;
	subtilis_exp_t *zero = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *maxe = NULL;

	if (p->current->array_access == SIZE_MAX) {
		error_label.label = subtilis_ir_section_new_label(p->current);
		p->current->array_access = error_label.label;
	} else {
		error_label.label = p->current->array_access;
	}

	ok_label.label = subtilis_ir_section_new_label(p->current);

	edup = subtilis_type_if_dup(e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (max_dim) {
		maxe = subtilis_type_if_dup(e, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	zero = subtilis_exp_new_int32(0, err);
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
	 * TODO: This whole thing is 32 bit specific.  At least the pointer
	 * needs to be platform specific although we may still keep maximum
	 * array sizes to 32 bits.
	 */

	/* TODO:
	 * I don't see any way to detect whether the array dimensions overflow
	 * 32 bits. For constants we can use 64 bit integers to store the size.
	 * This would then limit us to using 32 bits for the array size, even on
	 * 64 bit builds (unless we resort to instrinsics) as there
	 * are no 128 bit integers in C.  For the ARM2 backend, there is
	 * no way to detect overflow of dynamic vars.  This could be done on
	 * StrongARM using UMULL.  Perhaps we could execute different code on
	 * SA here.  For now, be careful with those array bounds!
	 */

	for (i = 0; i < index_count; i++) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER) {
			non_const_dim = true;
			continue;
		}
		if (e[i]->exp.ir_op.integer <= 0) {
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
				prv_check_dynamic_dim(p, e[i], NULL, err);
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

/* Does not consume e. */

static subtilis_exp_t *prv_check_dynamic_index(subtilis_parser_t *p,
					       subtilis_exp_t *e,
					       int32_t dim_size, int32_t offset,
					       size_t mem_reg,
					       subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t reg;
	subtilis_exp_t *maxe = NULL;
	subtilis_exp_t *maxe_dup = NULL;

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
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	maxe_dup = subtilis_type_if_dup(maxe, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	prv_check_dynamic_dim(p, e, maxe_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return maxe;

cleanup:

	subtilis_exp_delete(maxe);

	return NULL;
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

subtilis_exp_t *
subtilis_array_index_calc(subtilis_parser_t *p, const char *var_name,
			  const subtilis_type_t *type, size_t mem_reg,
			  size_t loc, subtilis_exp_t **e, size_t index_count,
			  subtilis_error_t *err)
{
	size_t i;
	int32_t dim_size;
	subtilis_exp_t *one;
	int32_t offset;
	subtilis_exp_t *sizee = NULL;
	subtilis_exp_t *edup = NULL;
	subtilis_exp_t *maxe = NULL;
	int64_t array_size = 0;

	if (index_count != type->params.array.num_dims) {
		subtilis_error_bad_index_count(err, var_name,
					       p->l->stream->name, p->l->line);
		return NULL;
	}

	if (index_count == 1)
		return prv_1d_index_calc(p, var_name, type, mem_reg, loc, e,
					 index_count, err);

	offset = loc + SUBTIILIS_ARRAY_DIMS_OFF;
	for (i = 0; i < index_count - 1; i++) {
		if (e[i]->type.type != SUBTILIS_TYPE_CONST_INTEGER)
			continue;
		if (dim_size == SUBTILIS_DYNAMIC_DIMENSION)
			continue;
		dim_size = type->params.array.dims[i];
		if ((e[i]->exp.ir_op.integer < 0) ||
		    (e[i]->exp.ir_op.integer > dim_size)) {
			subtilis_error_bad_index(
			    err, var_name, p->l->stream->name, p->l->line);
			return NULL;
		}
		array_size += (dim_size + 1) * e[i]->exp.ir_op.integer;
	}

	sizee = subtilis_exp_new_int32((int32_t)array_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	sizee = subtilis_type_if_exp_to_var(p, sizee, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* Need to compute offset array */

	for (i = 0; i < index_count - 1; i++) {
		dim_size = type->params.array.dims[i];
		if ((dim_size != SUBTILIS_DYNAMIC_DIMENSION) &&
		    (e[i]->type.type == SUBTILIS_TYPE_CONST_INTEGER))
			continue;

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
		edup = subtilis_type_if_mul(p, maxe, edup, err);
		maxe = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		sizee = subtilis_type_if_add(p, sizee, edup, err);
		edup = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		offset += sizeof(int32_t);
	}

	prv_check_last_index(p, var_name, e, type, offset, mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	edup = subtilis_type_if_dup(e[i], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	sizee = subtilis_type_if_add(p, edup, sizee, err);
	edup = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return prv_compute_element_address(p, type, mem_reg, loc, sizee, err);

cleanup:

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

	e = subtilis_exp_coerce_type(p, e, el_type, err);
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

	e = subtilis_exp_coerce_type(p, e, el_type, err);
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

	e = subtilis_exp_coerce_type(p, e, el_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_assign_to_mem(p, offset->exp.ir_op.reg, 0, e, err);

cleanup:
	subtilis_exp_delete(offset);
}
