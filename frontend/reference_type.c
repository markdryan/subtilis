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

#include "array_type.h"
#include "expression.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "type_if.h"

void subtilis_reference_type_init_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      bool check_size, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t copy;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t gtzero;

	op0.reg = source_reg;

	op1.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_REFERENCE_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, copy, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (check_size) {
		zero.label = subtilis_ir_section_new_label(p->current);
		gtzero.label = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, copy,
						  gtzero, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, gtzero.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	op1.integer = SUBTIILIS_REFERENCE_DATA_OFF;
	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_REFERENCE_DATA_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, copy, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.integer = SUBTIILIS_REFERENCE_DESTRUCTOR_OFF;
	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = dest_mem_reg;
	op2.integer = dest_loc + SUBTIILIS_REFERENCE_DESTRUCTOR_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, copy, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (check_size)
		subtilis_ir_section_add_label(p->current, zero.label, err);
}

/*
 * This function is called when initialising string parameters of a function
 * and a procedure.  As we're only setting up the procedure at this point
 * we don't want to use the normal reference copying parameters as the
 * cleanup stack isn't yet set up.
 */

void subtilis_reference_type_copy_ref(subtilis_parser_t *p, size_t dest_mem_reg,
				      size_t dest_loc, size_t source_reg,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t store_op;
	subtilis_ir_operand_t op2;

	subtilis_reference_type_init_ref(p, dest_mem_reg, dest_loc, source_reg,
					 false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	store_op.reg = dest_mem_reg;
	if (dest_loc > 0) {
		op2.integer = dest_loc;
		op2.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, store_op, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		op2.reg = dest_mem_reg;
	}

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PUSH_I32, op2, err);
}

/*
 * Initialise a temporary reference on the caller's stack with the contents
 * of a reference variable on the callee's stack.  This is called directly after
 * the callee's ret has executed so his stack is still presevered.
 *
 * TODO: This is a bit risky though.  Might be better, and faster, to allocate
 * space for this variable on the caller's stack.
 */

void subtilis_reference_type_copy_ret(subtilis_parser_t *p,
				      const subtilis_type_t *t, size_t dest_reg,
				      size_t source_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t d;
	subtilis_ir_operand_t s;

	d.reg = dest_reg;
	s.reg = source_reg;

	op2.integer = SUBTIILIS_REFERENCE_SIZE_OFF;
	op0.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, s, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, d, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op1.reg = subtilis_reference_get_data(p, source_reg, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = SUBTIILIS_REFERENCE_DATA_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op1, d, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = SUBTIILIS_REFERENCE_DESTRUCTOR_OFF;
	op1.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, s, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op1, d, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * It's important that this comes last as the reference we're
	 * copying might be left on the stack of a previous function.
	 * We don't want to overwrite the data before we've copied it.
	 */

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PUSH_I32, d, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_inc_cleanup_stack(p, t, err);
}

void subtilis_reference_type_ref(subtilis_parser_t *p, size_t mem_reg,
				 size_t loc, bool check_size,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t gtzero;

	op0.reg = mem_reg;
	if (check_size) {
		op1.integer = loc + SUBTIILIS_REFERENCE_SIZE_OFF;
		size.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		zero.label = subtilis_ir_section_new_label(p->current);
		gtzero.label = subtilis_ir_section_new_label(p->current);
		subtilis_ir_section_add_instr_reg(p->current,
						  SUBTILIS_OP_INSTR_JMPC, size,
						  gtzero, zero, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, gtzero.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	op1.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;
	op0.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_REF,
					     op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (check_size)
		subtilis_ir_section_add_label(p->current, zero.label, err);
}

/*
 * Used when returning reference types from functions.  Here we just want to
 * copy the pointer to the reference type into the integer register used to
 * return a value from the function and potentially ref it.
 */

void subtilis_reference_type_assign_to_reg(subtilis_parser_t *p, size_t reg,
					   subtilis_exp_t *e, bool check_size,
					   subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;

	op0.reg = reg;
	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      op0, e->exp.ir_op, err);
	subtilis_exp_delete(e);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_ref(p, reg, 0, check_size, err);
}

void subtilis_reference_type_push_reference(subtilis_parser_t *p,
					    const subtilis_type_t *type,
					    size_t reg, size_t loc,
					    subtilis_error_t *err)
{
	subtilis_ir_operand_t store_op;
	subtilis_ir_operand_t op2;

	store_op.reg = reg;

	if (loc > 0) {
		op2.integer = loc;
		op2.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_ADDI_I32, store_op, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		op2.reg = reg;
	}

	subtilis_ir_section_add_instr_no_reg(
	    p->current, SUBTILIS_OP_INSTR_PUSH_I32, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_inc_cleanup_stack(p, type, err);
}

void subtilis_reference_type_new_ref(subtilis_parser_t *p,
				     const subtilis_type_t *type,
				     size_t dest_mem_reg, size_t dest_loc,
				     size_t source_reg, bool check_size,
				     subtilis_error_t *err)
{
	subtilis_reference_type_init_ref(p, dest_mem_reg, dest_loc, source_reg,
					 check_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_push_reference(p, type, dest_mem_reg, dest_loc,
					       err);
}

static void prv_call_deref(subtilis_parser_t *p, subtilis_ir_operand_t address,
			   subtilis_error_t *err)
{
	if (p->caps & SUBTILIS_BACKEND_HAVE_ALLOC)
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_DEREF, address, err);
	else
		(void)subtilis_parser_call_1_arg_fn(
		    p, "_deref", address.reg, SUBTILIS_BUILTINS_DEREF,
		    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_void, true,
		    err);
}

void subtilis_reference_type_assign_ref(subtilis_parser_t *p,
					size_t dest_mem_reg, size_t dest_loc,
					size_t source_reg,
					subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t copy;

	op0.reg = dest_mem_reg;
	op1.integer = dest_loc + SUBTIILIS_REFERENCE_DATA_OFF;

	copy.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_call_deref(p, copy, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_init_ref(p, dest_mem_reg, dest_loc, source_reg,
					 false, err);
}

size_t subtilis_reference_get_pointer(subtilis_parser_t *p, size_t reg,
				      size_t offset, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	size_t dest;

	if (offset == 0)
		return reg;

	op0.reg = reg;
	op1.integer = (int32_t)offset;
	dest = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return dest;
}

size_t subtilis_reference_get_data(subtilis_parser_t *p, size_t reg, size_t loc,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	op0.reg = reg;
	op1.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;

	return subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, op0, op1, err);
}

void subtilis_reference_set_data(subtilis_parser_t *p, size_t reg,
				 size_t store_reg, size_t loc,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	op0.reg = reg;
	op1.reg = store_reg;
	op2.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);
}

void subtilis_reference_type_memcpy(subtilis_parser_t *p, size_t mem_reg,
				    size_t loc, size_t src_reg, size_t size_reg,
				    subtilis_error_t *err)
{
	size_t dest_reg;

	dest_reg = subtilis_reference_get_data(p, mem_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_reference_type_memcpy_dest(p, dest_reg, src_reg, size_reg,
					    err);
}

void subtilis_reference_type_memcpy_dest(subtilis_parser_t *p, size_t dest_reg,
					 size_t src_reg, size_t size_reg,
					 subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memcpy[] = "_memcpy";

	name = malloc(sizeof(memcpy));
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name, memcpy);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = dest_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = src_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = size_reg;

	(void)subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMCPY, NULL,
				    args, &subtilis_type_void, 3, true, err);
}

static void prv_ensure_cleanup_stack(subtilis_parser_t *p,
				     bool destructor_needed,
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

	/*
	 * Extra cleanup code is needed at the end of the function if the
	 * function contains a variable that has a destructor, e.g., an
	 * array of strings.  We only want to add this code if necessary.
	 */

	p->current->destructor_needed =
	    p->current->destructor_needed || destructor_needed;
}

void subtilis_reference_inc_cleanup_stack(subtilis_parser_t *p,
					  const subtilis_type_t *type,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t dest;

	prv_ensure_cleanup_stack(p, subtilis_type_if_destructor(type) != 0,
				 err);
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

size_t subtilis_reference_type_raw_alloc(subtilis_parser_t *p, size_t size_reg,
					 subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_exp_t *e;
	subtilis_ir_operand_t size_op;

	size_op.reg = size_reg;

	if (p->caps & SUBTILIS_BACKEND_HAVE_ALLOC) {
		op.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_ALLOC, size_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;

		subtilis_exp_handle_errors(p, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	} else {
		e = subtilis_parser_call_1_arg_fn(
		    p, "_alloc", size_reg, SUBTILIS_BUILTINS_ALLOC,
		    SUBTILIS_IR_REG_TYPE_INTEGER, &subtilis_type_integer, true,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
		op.reg = e->exp.ir_op.reg;
		subtilis_exp_delete(e);
	}

	return op.reg;
}

size_t subtilis_reference_type_alloc(subtilis_parser_t *p,
				     const subtilis_type_t *type, size_t loc,
				     size_t store_reg, size_t size_reg,
				     bool push, subtilis_error_t *err)
{
	subtilis_ir_operand_t op;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t destructor;
	subtilis_ir_operand_t size_op;
	subtilis_ir_operand_t store_op;

	size_op.reg = size_reg;
	store_op.reg = store_reg;

	op1.integer = loc + SUBTIILIS_REFERENCE_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32, size_op,
					  store_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;
	op.reg = subtilis_reference_type_raw_alloc(p, size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (push) {
		subtilis_reference_type_push_reference(p, type, store_reg, loc,
						       err);
		if (err->type != SUBTILIS_ERROR_OK)
			return SIZE_MAX;
	}

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op, store_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = (int32_t)subtilis_type_if_destructor(type);
	destructor.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op1.integer = loc + SUBTIILIS_REFERENCE_DESTRUCTOR_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  destructor, store_op, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return op.reg;
}

static size_t prv_resize_with_realloc(subtilis_parser_t *p, size_t loc,
				      size_t store_reg, size_t data_reg,
				      size_t size_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	size_t dest_reg = p->current->reg_counter++;

	op0.reg = data_reg;
	op1.reg = size_reg;
	op2.reg = dest_reg;
	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_REALLOC,
					  op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op0.reg = size_reg;
	op1.reg = store_reg;
	op2.integer = loc + SUBTIILIS_REFERENCE_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op0.reg = dest_reg;
	op1.reg = store_reg;
	op2.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, op0, op1, op2, err);

	return dest_reg;
}

static size_t prv_resize_with_malloc(subtilis_parser_t *p, size_t loc,
				     size_t store_reg, size_t data_reg,
				     size_t old_size_reg, size_t new_size_reg,
				     size_t delta_reg, subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t store;
	subtilis_ir_operand_t free_space;
	subtilis_ir_operand_t block_size;
	subtilis_ir_operand_t old_size;
	subtilis_ir_operand_t new_size;
	subtilis_ir_operand_t alloc_needed;
	subtilis_ir_operand_t alloc_needed_label;
	subtilis_ir_operand_t alloc_end_label;
	subtilis_ir_operand_t no_alloc_needed_label;
	subtilis_ir_operand_t dest;
	subtilis_ir_operand_t data;
	subtilis_ir_operand_t delta;
	subtilis_ir_operand_t new_block;

	alloc_needed_label.label = subtilis_ir_section_new_label(p->current);
	no_alloc_needed_label.label = subtilis_ir_section_new_label(p->current);
	alloc_end_label.label = subtilis_ir_section_new_label(p->current);

	data.reg = data_reg;
	dest.reg = p->current->reg_counter++;
	old_size.reg = old_size_reg;
	new_size.reg = new_size_reg;
	store.reg = store_reg;
	delta.reg = delta_reg;

	op0.reg = data_reg;
	free_space.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_BLOCK_FREE, op0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	block_size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_ADD_I32, old_size, free_space, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	alloc_needed.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_GT_I32, new_size, block_size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  alloc_needed, alloc_needed_label,
					  no_alloc_needed_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, alloc_needed_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	new_block.reg = subtilis_reference_type_raw_alloc(p, new_size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_reference_type_memcpy_dest(p, new_block.reg, data_reg,
					    old_size_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	/*
	 * Here we call deref directly as we already have the pointer to the
	 * block and we don't want to call any destructors.
	 */

	prv_call_deref(p, data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op2.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  new_block, store, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      dest, new_block, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_no_reg(p->current, SUBTILIS_OP_INSTR_JMP,
					     alloc_end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, no_alloc_needed_label.label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_BLOCK_ADJUST, data, delta, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_instr_no_reg2(p->current, SUBTILIS_OP_INSTR_MOV,
					      dest, data, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	subtilis_ir_section_add_label(p->current, alloc_end_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	op2.integer = loc + SUBTIILIS_REFERENCE_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(p->current,
					  SUBTILIS_OP_INSTR_STOREO_I32,
					  new_size, store, op2, err);

	return dest.reg;
}

size_t subtilis_reference_type_realloc(subtilis_parser_t *p, size_t loc,
				       size_t store_reg, size_t data_reg,
				       size_t old_size_reg, size_t new_size_reg,
				       size_t delta_reg, subtilis_error_t *err)
{
	if (p->caps & SUBTILIS_BACKEND_HAVE_ALLOC)
		return prv_resize_with_realloc(p, loc, store_reg, data_reg,
					       new_size_reg, err);

	return prv_resize_with_malloc(p, loc, store_reg, data_reg, old_size_reg,
				      new_size_reg, delta_reg, err);
}

static void prv_deref(subtilis_parser_t *p, size_t mem_reg, size_t loc,
		      bool destructor, subtilis_error_t *err)
{
	subtilis_ir_operand_t el_start;
	subtilis_ir_operand_t destruct;
	subtilis_ir_operand_t offset;
	subtilis_ir_operand_t size;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t ref_count;
	subtilis_ir_operand_t non_zero;
	subtilis_ir_operand_t zero;
	subtilis_ir_operand_t no_destruct_label;
	subtilis_ir_operand_t destruct_label;

	non_zero.label = subtilis_ir_section_new_label(p->current);
	zero.label = subtilis_ir_section_new_label(p->current);

	el_start.reg = mem_reg;
	op2.integer = SUBTIILIS_REFERENCE_SIZE_OFF + loc;
	size.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  size, non_zero, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_ir_section_add_label(p->current, non_zero.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op2.integer = loc + SUBTIILIS_REFERENCE_DATA_OFF;
	offset.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * TODO: This is temporary to handle arrays of strings.  Ultimately,
	 * we'll need a more comprehensive solution to deal with structures
	 * and arrays of structures, but first we'll need function pointer
	 * support.
	 */

	if (destructor) {
		destruct_label.label =
		    subtilis_ir_section_new_label(p->current);
		no_destruct_label.label =
		    subtilis_ir_section_new_label(p->current);
		op2.integer = loc + SUBTIILIS_REFERENCE_DESTRUCTOR_OFF;
		destruct.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_LOADO_I32, el_start, op2,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ref_count.reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_GETREF, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op2.integer = 1;
		ref_count.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_EQI_I32, ref_count, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		destruct.reg = subtilis_ir_section_add_instr(
		    p->current, SUBTILIS_OP_INSTR_AND_I32, destruct, ref_count,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_instr_reg(
		    p->current, SUBTILIS_OP_INSTR_JMPC, destruct,
		    destruct_label, no_destruct_label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current, destruct_label.label,
					      err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_array_type_deref_els(p, offset.reg, size.reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_ir_section_add_label(p->current,
					      no_destruct_label.label, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_call_deref(p, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, zero.label, err);
}

void subtilis_reference_type_pop_and_deref(subtilis_parser_t *p,
					   bool ref_of_ref,
					   subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t dest;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_POP_I32, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_deref(p, reg, 0, ref_of_ref, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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

void subtilis_reference_type_deref(subtilis_parser_t *p, size_t mem_reg,
				   size_t loc, subtilis_error_t *err)
{
	prv_deref(p, mem_reg, loc, false, err);
}

void subtilis_reference_deallocate_refs(subtilis_parser_t *p,
					subtilis_ir_operand_t load_reg,
					subtilis_symbol_table_t *st,
					size_t level, subtilis_error_t *err)
{
	size_t i;
	bool ref_of_ref;
	const subtilis_symbol_t *s;
	subtilis_symbol_level_t *l = &st->levels[level];

	for (i = 0; i < l->size; i++) {
		s = l->symbols[i];
		if (subtilis_type_if_is_numeric(&s->t))
			continue;

		ref_of_ref = !(subtilis_type_if_destructor(&s->t) == 0);
		subtilis_reference_type_pop_and_deref(p, ref_of_ref, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}
