/*
 * Copyright (c) 2022 Mark Ryan
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

#include "array_type.h"
#include "builtins_helper.h"
#include "builtins_ir.h"
#include "rec_type.h"
#include "reference_type.h"
#include "type_if.h"

static void prv_rec_type_zero(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t mem_reg, size_t loc, bool memset,
			      subtilis_error_t *err);

static void prv_rec_memset_zero(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t mem_reg,
				size_t loc, subtilis_error_t *err)

{
	size_t size = subtilis_type_rec_size(type);
	subtilis_exp_t *size_exp = NULL;
	subtilis_exp_t *val_exp = NULL;

	size_exp = subtilis_exp_new_int32((int32_t)size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	size_exp = subtilis_type_if_exp_to_var(p, size_exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val_exp = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	val_exp = subtilis_type_if_exp_to_var(p, val_exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	mem_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (size & 3)
		subtilis_builtin_memset_i8(p, mem_reg, size_exp->exp.ir_op.reg,
					   val_exp->exp.ir_op.reg, err);
	else
		subtilis_builtin_memset_i32(p, mem_reg, size_exp->exp.ir_op.reg,
					    val_exp->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(val_exp);
	subtilis_exp_delete(size_exp);
}

void subtilis_rec_type_init_array(subtilis_parser_t *p,
				  const subtilis_type_t *type,
				  const char *var_name, size_t mem_reg,
				  size_t loc, subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_operand_t op;
	subtilis_exp_t *e[SUBTILIS_MAX_DIMENSIONS] = {NULL};
	const subtilis_type_array_t *arr = &type->params.array;

	for (i = 0; i < arr->num_dims; i++) {
		e[i] = subtilis_exp_new_int32(arr->dims[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op.reg = mem_reg;
	subtilis_array_type_allocate(p, var_name, type, loc, e, op, false, err);

cleanup:
	for (i = 0; i < arr->num_dims; i++)
		subtilis_exp_delete(e[i]);
}

void subtilis_rec_type_init_vector(subtilis_parser_t *p,
				   const subtilis_type_t *type, size_t mem_reg,
				   size_t loc, int32_t vec_dim,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_exp_t *e = NULL;

	e = subtilis_exp_new_int32(vec_dim, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op.reg = mem_reg;
	subtilis_array_type_vector_alloc(p, loc, type, e, op, false, err);
	subtilis_exp_delete(e);
}

void subtilis_rec_type_init_field(subtilis_parser_t *p,
				  const subtilis_type_t *type,
				  subtilis_type_field_t *field, size_t mem_reg,
				  size_t loc, bool memset,
				  subtilis_error_t *err)
{
	subtilis_exp_t *e;

	if (subtilis_type_if_is_array(type)) {
		subtilis_rec_type_init_array(p, type, field->name, mem_reg, loc,
					     err);
	} else if (subtilis_type_if_is_vector(type) && (field->vec_dim >= 0)) {
		subtilis_rec_type_init_vector(p, type, mem_reg, loc,
					      field->vec_dim, err);
	} else if (subtilis_type_if_is_reference(type)) {
		subtilis_type_if_zero_ref(p, type, mem_reg, loc, false, err);
	} else if (type->type == SUBTILIS_TYPE_REC) {
		prv_rec_type_zero(p, type, mem_reg, loc, memset, err);
	} else if (subtilis_type_if_is_numeric(type) ||
		   (type->type == SUBTILIS_TYPE_FN)) {
		e = subtilis_type_if_zero(p, type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_type_if_assign_to_mem(p, mem_reg, loc, e, err);
	} else {
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_uninit_fields(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t mem_reg, size_t loc, subtilis_error_t *err)
{
	size_t i;
	const subtilis_type_rec_t *rec;
	const subtilis_type_t *field;
	uint32_t offset;

	/*
	 * We only need to initialise fields that cannot be zero initialised.
	 */

	rec = &type->params.rec;
	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		if (subtilis_type_if_is_numeric(field) ||
		    field->type == SUBTILIS_TYPE_STRING)
			continue;
		offset = subtilis_type_rec_field_offset_id(rec, i);
		subtilis_rec_type_init_field(p, field, &rec->fields[i], mem_reg,
					     offset + loc, true, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_init_fields(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t mem_reg, size_t loc, subtilis_error_t *err)
{
	size_t i;
	const subtilis_type_rec_t *rec;
	const subtilis_type_t *field;
	uint32_t offset;

	rec = &type->params.rec;
	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		offset = subtilis_type_rec_field_offset_id(rec, i);
		subtilis_rec_type_init_field(p, field, &rec->fields[i], mem_reg,
					     offset + loc, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_rec_type_zero(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t mem_reg, size_t loc, bool memset,
			      subtilis_error_t *err)
{
	if (memset)
		prv_uninit_fields(p, type, mem_reg, loc, err);
	else
		prv_init_fields(p, type, mem_reg, loc, err);
}

void subtilis_rec_type_zero_body(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, size_t zero_fill_size,
				 subtilis_error_t *err)
{
	bool memset = false;

	/*
	 * If the record and any nested records contain more than 16 bytes
	 * of data that can be zero initialised, we'll just memset the whole
	 * thing.
	 */

	if (zero_fill_size > 16) {
		prv_rec_memset_zero(p, type, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memset = true;
	}

	prv_rec_type_zero(p, type, mem_reg, loc, memset, err);
}

void subtilis_rec_type_zero(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t mem_reg, size_t loc, bool push,
			    subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t zfs = subtilis_type_rec_zero_fill_size(type);

	if (subtilis_type_rec_size(type) - zfs < 16) {
		subtilis_rec_type_zero_body(p, type, mem_reg, loc, zfs, err);
	} else {
		op1.reg = mem_reg;
		op2.integer = loc;
		mem_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		loc = 0;
		subtilis_builtin_ir_rec_zero(p, type, mem_reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (push && subtilis_type_rec_need_deref(type))
		subtilis_reference_type_push_reference(p, type, mem_reg, loc,
						       err);
}

void subtilis_type_rec_copy_ref(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t dest_reg,
				size_t loc, size_t src_reg,
				subtilis_error_t *err)
{
	size_t i;
	uint32_t offset;
	subtilis_exp_t *e;
	subtilis_exp_t *e1;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t ptr_reg;
	const subtilis_type_t *field_type;
	const subtilis_type_rec_t *rec = &type->params.rec;

	/*
	 * We only need to initialise fields that cannot be zero initialised.
	 */

	for (i = 0; i < rec->num_fields; i++) {
		offset = subtilis_type_rec_field_offset_id(rec, i);
		field_type = &rec->field_types[i];
		if (field_type->type == SUBTILIS_TYPE_REC) {
			op1.reg = src_reg;
			op2.integer = offset;
			ptr_reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op1, op2,
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;

			subtilis_rec_type_copy(p, field_type, dest_reg,
					       loc + offset, ptr_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (subtilis_type_if_is_reference(field_type)) {
			ptr_reg = subtilis_reference_get_pointer(p, src_reg,
								 offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			e = subtilis_exp_new_var(field_type, ptr_reg, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_ref(p, field_type, dest_reg,
						    loc + offset, e, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (field_type->type == SUBTILIS_TYPE_FN ||
			   subtilis_type_if_is_numeric(field_type)) {
			e = subtilis_type_if_load_from_mem(
			    p, field_type, src_reg, offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			e1 = subtilis_exp_new_var(field_type, e->exp.ir_op.reg,
						  err);
			subtilis_exp_delete(e);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_type_if_assign_to_mem(p, dest_reg,
						       loc + offset, e1, err);
		} else {
			subtilis_error_set_assertion_failed(err);
			return;
		}
	}
}

void subtilis_rec_type_copy(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t dest_reg, size_t loc, size_t src_reg,
			    subtilis_error_t *err)
{
	size_t size_reg;
	subtilis_ir_operand_t op;
	size_t zfs;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	if (subtilis_type_rec_is_scalar(type) &&
	    (subtilis_type_rec_zero_fill_size(type) > 16)) {
		dest_reg =
		    subtilis_reference_get_pointer(p, dest_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		op.integer = subtilis_type_rec_size(type);
		size_reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_reference_type_memcpy_dest(p, dest_reg, src_reg,
						    size_reg, err);
		return;
	}

	zfs = subtilis_type_rec_zero_fill_size(type);
	if (subtilis_type_rec_size(type) - zfs < 16) {
		subtilis_type_rec_copy_ref(p, type, dest_reg, loc, src_reg,
					   err);
	} else {
		op1.reg = dest_reg;
		op2.integer = loc;
		dest_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_builtin_ir_rec_copy(p, type, dest_reg, src_reg, err);
	}
}
