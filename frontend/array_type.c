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

static void prv_ensure_cleanup_stack(subtilis_parser_t *p,
				     subtilis_error_t *err)
{
	subtilis_ir_inst_t *instr;

	/*
	 * This piece is a bit weird.  At the start of the procedure we insert
	 * a NOP.  If we encounter at least one array (local or global) we
	 * replace this nop with a mov instruction that initialises the
	 * cleanup_stack counter to 0.  If there are no arrays delcared in the
	 * procedure, the nop will be removed when we generate code for the
	 * target architecture.
	 */

	if (p->current->cleanup_stack == SIZE_MAX) {
		instr =
		    &p->current->ops[p->current->cleanup_stack_nop]->op.instr;
		p->current->cleanup_stack = p->current->cleanup_stack_reg;
		instr->type = SUBTILIS_OP_INSTR_MOVI_I32;
		instr->operands[0].reg = p->current->cleanup_stack;
		instr->operands[1].integer = 0;
	}
}

static void prv_inc_cleanup_stack(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t dest;

	prv_ensure_cleanup_stack(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * TODO: The part of the register allocator that preserves
	 * live registers between basic blocks can't handle the case
	 * when an instruction uses the same register for both source
	 * and destination.  Ultimately, all that code is going to
	 * dissapear when we have a global register allocator, so for
	 * now we're just going to live with it.
	 */

	op2.integer = 1;
	dest.reg = p->current->cleanup_stack;
	op2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      dest, op2, err);
}

void subtlis_array_type_allocate(subtilis_parser_t *p, const char *var_name,
				 subtilis_type_t *type, size_t loc,
				 subtilis_exp_t **e,
				 subtilis_ir_operand_t store_reg,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
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

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
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

	op2.integer = loc;
	op2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, store_reg, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PUSH_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_inc_cleanup_stack(p, err);
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

static void prv_copy_ref_base(subtilis_parser_t *p, const subtilis_type_t *t,
			      subtilis_ir_operand_t dest_reg,
			      size_t dest_offset,
			      subtilis_ir_operand_t source_reg,
			      size_t source_offset, bool ref,
			      subtilis_error_t *err)
{
	subtilis_ir_operand_t soffset;
	subtilis_ir_operand_t doffset;
	subtilis_ir_operand_t ref_start;
	subtilis_ir_operand_t data;
	size_t i;
	size_t ints_to_copy = 2 + (t->params.array.num_dims);

	soffset.integer = SUBTIILIS_ARRAY_SIZE_OFF + source_offset;
	doffset.integer = SUBTIILIS_ARRAY_SIZE_OFF + dest_offset;

	if (doffset.integer > 0) {
		ref_start.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, dest_reg, doffset,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		ref_start = dest_reg;
	}

	for (i = 0; i < ints_to_copy; i++) {
		data.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, source_reg,
		    soffset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_STOREO_I32,
						  data, dest_reg, doffset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (ref && (soffset.integer ==
			    source_offset + SUBTIILIS_ARRAY_DATA_OFF)) {
			subtilis_ir_section_add_instr_no_reg(
			    p->current, SUBTILIS_OP_INSTR_REF, data, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		soffset.integer += sizeof(int32_t);
		doffset.integer += sizeof(int32_t);
	}

	/*
	 * It's important that this comes last as the reference we're
	 * copying might be left on the stack of a previous function.
	 * We don't want to overwrite the data before we've copied it.
	 */

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PUSH_I32, ref_start, err);
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

	prv_inc_cleanup_stack(p, err);

cleanup:

	subtilis_exp_delete(e);
}

void subtlis_array_type_create_tmp_ref(subtilis_parser_t *p,
				       const subtilis_type_t *t,
				       subtilis_ir_operand_t dest_reg,
				       subtilis_ir_operand_t source_reg,
				       subtilis_error_t *err)
{
	prv_copy_ref_base(p, t, dest_reg, 0, source_reg, 0, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_inc_cleanup_stack(p, err);
}

/*
 * t1 is the target array.  If it has a dynamic dim that's treated as a match.
 */

void subtilis_array_type_match(subtilis_parser_t *p, const subtilis_type_t *t1,
			       const subtilis_type_t *t2, subtilis_error_t *err)
{
	size_t i;

	if (t1->type != t2->type) {
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);
		return;
	}

	if (t1->params.array.num_dims != t2->params.array.num_dims) {
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);
		return;
	}

	for (i = 0; i < t1->params.array.num_dims; i++) {
		if (t1->params.array.dims[i] == -1)
			continue;
		if (t1->params.array.dims[i] != t2->params.array.dims[i]) {
			subtilis_error_set_array_type_mismatch(
			    err, p->l->stream->name, p->l->line);
			return;
		}
	}
}

void subtilis_array_type_assign_ref(subtilis_parser_t *p,
				    const subtilis_type_array_t *dest_type,
				    size_t dest_mem_reg, size_t dest_loc,
				    size_t source_reg, subtilis_error_t *err)
{
	size_t i;
	size_t dest_reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t copy_reg;

	op0.reg = dest_mem_reg;
	op1.integer = dest_loc + SUBTIILIS_ARRAY_DATA_OFF;

	dest_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = dest_reg;
	subtilis_ir_section_add_instr_no_reg(p->current,
					     SUBTILIS_OP_INSTR_DEREF, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = source_reg;
	op1.integer = SUBTIILIS_ARRAY_DATA_OFF;

	copy_reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = copy_reg;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_ARRAY_DATA_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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

void subtilis_array_type_deref(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = mem_reg;
	op1.integer = loc + SUBTIILIS_ARRAY_DATA_OFF;

	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = reg;
	subtilis_ir_section_add_instr_no_reg(p->current,
					     SUBTILIS_OP_INSTR_DEREF, op0, err);
}

void subtilis_array_type_ref(subtilis_parser_t *p, size_t mem_reg, size_t loc,
			     subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = mem_reg;
	op1.integer = loc + SUBTIILIS_ARRAY_DATA_OFF;

	reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op0.reg = reg;
	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     op0, err);
}

void subtilis_array_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
				       subtilis_exp_t *e, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_MOV,
					  op0, e->exp.ir_op, op2, err);
	subtilis_exp_delete(e);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_type_ref(p, reg, 0, err);
}

void subtilis_array_type_pop_and_deref(subtilis_parser_t *p,
				       subtilis_error_t *err)
{
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t dest;

	offset.reg = subtilis_ir_section_add_instr1(
	    p->current, SUBTILIS_OP_INSTR_POP_I32, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = SUBTIILIS_ARRAY_DATA_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, offset, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_DEREF, offset, err);

	if (p->current->cleanup_stack == SIZE_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/*
	 * TODO: The part of the register allocator that preserves
	 * live registers between basic blocks can't handle the case
	 * when an instruction uses the same register for both source
	 * and destination.  Ultimately, all that code is going to dissapear
	 * when we have a global register allocator, so for now we're just
	 * going to live with it.
	 */

	op2.integer = 1;
	dest.reg = p->current->cleanup_stack;
	op2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_SUBI_I32, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      dest, op2, err);
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
	prv_check_dynamic_dim(p, e, 0, maxe_dup, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return maxe;

cleanup:

	subtilis_exp_delete(maxe);

	return NULL;
}

/*
 * Functions that use arrays will typically contain a lot of error checks and
 * each of these checks can lead to code being called that generates an error.
 * We don't want to duplicate that code for each error, so we create the error
 * generation code once per function when it's first needed and retain the label
 * in the section so it can jumped to when needed.
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
		dim_size = type->params.array.dims[i];
		if (dim_size == SUBTILIS_DYNAMIC_DIMENSION)
			continue;
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

subtilis_exp_t *subtilis_array_get_dim(subtilis_parser_t *p,
				       subtilis_exp_t **indices, size_t dims,
				       subtilis_error_t *err)
{
	int32_t requested_dim;
	size_t i;
	int32_t dim_index;
	size_t num_dims;
	subtilis_ir_operand_t op;
	subtilis_exp_t *e = NULL;

	/*
	 * This is unfortunate as DIM(a()) may generate one add instruction
	 * for the array reference even though we don't actually need it.
	 * Hopefully, the optimiser will be able to get rid of it.
	 */

	num_dims = indices[0]->type.params.array.num_dims;
	if (dims == 1) {
		e = subtilis_exp_new_int32(num_dims, err);
	} else if (dims == 2) {
		indices[1] = subtilis_type_if_to_int(p, indices[1], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if (indices[1]->type.type == SUBTILIS_TYPE_CONST_INTEGER) {
			if ((indices[1]->exp.ir_op.integer <= 0) ||
			    (indices[1]->exp.ir_op.integer > num_dims)) {
				subtilis_error_bad_index(
				    err, "dim", p->l->stream->name, p->l->line);
				goto cleanup;
			}
			dim_index = indices[1]->exp.ir_op.integer - 1;
			requested_dim =
			    indices[0]->type.params.array.dims[dim_index];
			if (requested_dim != SUBTILIS_DYNAMIC_DIMENSION) {
				e = subtilis_exp_new_int32(requested_dim, err);
			} else {
				e = subtilis_type_if_load_from_mem(
				    p, &subtilis_type_integer,
				    indices[0]->exp.ir_op.reg,
				    SUBTIILIS_ARRAY_DIMS_OFF +
					(sizeof(int32_t) * dim_index),
				    err);
			}
		} else {
			prv_check_dimension_num(p, indices[1], num_dims, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			op.integer = 1;
			indices[1]->exp.ir_op.reg =
			    subtilis_ir_section_add_instr(
				p->current, SUBTILIS_OP_INSTR_SUBI_I32,
				indices[1]->exp.ir_op, op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			/* TODO This is int32 specific */
			op.integer = 2;
			indices[1]->exp.ir_op.reg =
			    subtilis_ir_section_add_instr(
				p->current, SUBTILIS_OP_INSTR_LSLI_I32,
				indices[1]->exp.ir_op, op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			op.reg = subtilis_ir_section_add_instr(
			    p->current, SUBTILIS_OP_INSTR_ADD_I32,
			    indices[0]->exp.ir_op, indices[1]->exp.ir_op, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			e = subtilis_type_if_load_from_mem(
			    p, &subtilis_type_integer, op.reg,
			    SUBTIILIS_ARRAY_DIMS_OFF, err);
		}
	} else {
		subtilis_error_bad_index_count(err, "dim", p->l->stream->name,
					       p->l->line);
	}

cleanup:

	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);

	return e;
}
