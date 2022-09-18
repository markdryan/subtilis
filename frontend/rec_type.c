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
#include "parser_exp.h"
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

	subtilis_builtin_bzero(p, mem_reg, loc, size, err);
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
	size_t zfs = subtilis_type_rec_zero_fill_size(type);

	if (subtilis_type_rec_size(type) - zfs < 16) {
		subtilis_rec_type_zero_body(p, type, mem_reg, loc, zfs, err);
	} else {
		mem_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
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
				size_t loc, size_t src_reg, bool new_rec,
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
					       loc + offset, ptr_reg, new_rec,
					       err);
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
			if (new_rec) {
				if (subtilis_type_if_is_array(field_type) ||
				    subtilis_type_if_is_vector(field_type)) {
					subtilis_array_type_init_field(
					    p, field_type, dest_reg,
					    loc + offset, e, err);
				} else {
					subtilis_reference_type_init_ref(
					    p, dest_reg, loc + offset,
					    e->exp.ir_op.reg, true, true, err);
					subtilis_exp_delete(e);
				}
			} else {
				subtilis_type_if_assign_ref(
				    p, field_type, dest_reg, loc + offset, e,
				    err);
			}
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

static void prv_memcpy_struct(subtilis_parser_t *p, const subtilis_type_t *type,
			      size_t dest_reg, size_t loc, size_t src_reg,
			      subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	size_t size_reg;

	dest_reg = subtilis_reference_get_pointer(p, dest_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op.integer = subtilis_type_rec_size(type);
	size_reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_memcpy_dest(p, dest_reg, src_reg, size_reg,
					    err);
}

void subtilis_rec_type_tmp_copy(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t dest_reg,
				size_t loc, size_t src_reg,
				subtilis_error_t *err)
{
	if (subtilis_type_rec_is_scalar(type) &&
	    subtilis_type_rec_zero_fill_size(type) <= 16)
		subtilis_type_rec_copy_ref(p, type, dest_reg, loc, src_reg,
					   true, err);
	else
		prv_memcpy_struct(p, type, dest_reg, loc, src_reg, err);
}

void subtilis_rec_type_copy(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t dest_reg, size_t loc, size_t src_reg,
			    bool new_rec, subtilis_error_t *err)
{
	size_t zfs;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	zfs = subtilis_type_rec_zero_fill_size(type);
	if (subtilis_type_rec_is_scalar(type) && (zfs > 16)) {
		prv_memcpy_struct(p, type, dest_reg, loc, src_reg, err);
		return;
	}

	if (subtilis_type_rec_size(type) - zfs < 16) {
		subtilis_type_rec_copy_ref(p, type, dest_reg, loc, src_reg,
					   new_rec, err);
	} else {
		op1.reg = dest_reg;
		op2.integer = loc;
		dest_reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op1, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_builtin_ir_rec_copy(p, type, dest_reg, src_reg,
					     new_rec, err);
	}
}

static char *prv_make_ref_fn_name(const subtilis_type_t *type, const char *base,
				  subtilis_error_t *err)
{
	char *fn_name;
	int name_len;
	const subtilis_type_rec_t *rec = &type->params.rec;

	name_len = strlen(rec->name);
	fn_name = malloc(name_len + 1 + 1 + strlen(base));
	if (!fn_name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	fn_name[0] = '_';
	strcpy(&fn_name[1], rec->name);
	strcpy(&fn_name[name_len + 1], base);

	return fn_name;
}

char *subtilis_rec_type_deref_fn_name(const subtilis_type_t *type,
				      subtilis_error_t *err)
{
	return prv_make_ref_fn_name(type, "_deref", err);
}

char *subtilis_rec_type_ref_fn_name(const subtilis_type_t *type,
				    subtilis_error_t *err)
{
	return prv_make_ref_fn_name(type, "_ref", err);
}

void subtilis_rec_type_deref(subtilis_parser_t *p, const subtilis_type_t *type,
			     size_t mem_reg, size_t loc, subtilis_error_t *err)
{
	size_t base_reg;

	if (!subtilis_type_rec_need_deref(type))
		return;

	base_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_builtin_call_ir_rec_deref(p, type, base_reg, err);
}

void subtilis_rec_type_ref(subtilis_parser_t *p, const subtilis_type_t *type,
			   size_t mem_reg, size_t loc, subtilis_error_t *err)
{
	if (subtilis_type_rec_need_ref_fn(type)) {
		mem_reg = subtilis_reference_get_pointer(p, mem_reg, loc, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_builtin_call_ir_rec_ref(p, type, mem_reg, err);
		return;
	}
	subtilis_rec_type_ref_gen(p, type, mem_reg, loc, err);
}

void subtilis_rec_type_ref_gen(subtilis_parser_t *p,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, subtilis_error_t *err)
{
	bool check_size;
	size_t i;
	size_t offset;
	subtilis_type_t *field;
	const subtilis_type_rec_t *rec = &type->params.rec;

	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];

		if (!subtilis_type_if_is_reference(field) &&
		    (field->type != SUBTILIS_TYPE_REC))
			continue;

		offset = loc + subtilis_type_rec_field_offset_id(rec, i);
		if (subtilis_type_if_is_reference(field)) {
			check_size = !subtilis_type_if_is_array(field);
			subtilis_reference_type_ref(p, mem_reg, offset,
						    check_size, err);
		} else {
			subtilis_rec_type_ref(p, field, mem_reg, offset, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

void subtilis_rec_type_copy_ret(subtilis_parser_t *p, const subtilis_type_t *t,
				size_t dest_reg, size_t source_reg,
				subtilis_error_t *err)
{
	subtilis_rec_type_tmp_copy(p, t, dest_reg, 0, source_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (!subtilis_type_rec_need_deref(t))
		return;

	/*
	 * It's important that this comes last as the reference we're
	 * copying might be left on the stack of a previous function.
	 * We don't want to overwrite the data before we've copied it.
	 */

	subtilis_reference_push_ref(p, t, dest_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_inc_cleanup_stack(p, t, err);
}

/*
 * Used when returning RECs from functions.  Here we just want to
 * copy the pointer to the REC into the integer register used to
 * return a value from the function and potentially ref it.
 */

void subtilis_rec_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
				     subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	bool ref_needed = subtilis_type_rec_need_deref(&e->type);

	op0.reg = reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, e->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (ref_needed)
		subtilis_rec_type_ref(p, &e->type, reg, 0, err);

cleanup:
	subtilis_exp_delete(e);
}

static void prv_rec_swap_32(subtilis_parser_t *p, size_t size, size_t reg1,
			    size_t reg2, subtilis_error_t *err)
{
	subtilis_ir_operand_t start;
	subtilis_ir_operand_t body;
	subtilis_ir_operand_t end;
	subtilis_ir_operand_t four;
	subtilis_ir_operand_t sizeop;
	subtilis_ir_operand_t counter;
	subtilis_ir_operand_t condee;
	subtilis_ir_operand_t ptr1;
	subtilis_ir_operand_t ptr2;

	/*
	 * We're just going to do a memcpy, copying 4 bytes at a time
	 * until we run out of space.  Copying the fields individually
	 * will generate too much code and doing a memcpy might require
	 * too much stack data, as we'd need a temporary buffer the size
	 * of the type.
	 */

	start.label = subtilis_ir_section_new_label(p->current);
	body.label = subtilis_ir_section_new_label(p->current);
	end.label = subtilis_ir_section_new_label(p->current);
	ptr1.reg = reg1;
	ptr2.reg = reg2;

	sizeop.integer = (int32_t)size;
	counter.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr1, sizeop, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_label(p->current, start.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	four.integer = 4;
	condee.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LT_I32, ptr1, counter, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  condee, body, end, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_label(p->current, body.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_swap_int32_mem(p, ptr1.reg, ptr2.reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr1, ptr1, four, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, ptr2, ptr2, four, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_label(p->current, end.label, err);
}

void subtilis_rec_type_swap(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t reg1, size_t reg2, subtilis_error_t *err)
{
	subtilis_ir_operand_t ptr1;
	subtilis_ir_operand_t ptr2;
	size_t offset;
	size_t i;
	size_t size = subtilis_type_rec_size(type);

	offset = size - (size & 3);
	if (size >= 4) {
		ptr1.reg = reg1;
		ptr2.reg = reg2;
		reg1 = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV, ptr1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		reg2 = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV, ptr2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		prv_rec_swap_32(p, offset, reg1, reg2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	i = 0;
	for (; offset < size; offset++, i++) {
		subtilis_exp_swap_int8_mem(p, reg1, reg2, i, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}
