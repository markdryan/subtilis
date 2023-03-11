/*
 * Copyright (c) 2023 Mark Ryan
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

#include "rv32_core.h"

subtilis_rv_op_pool_t *subtilis_rv_op_pool_new(subtilis_error_t *err)
{
	subtilis_rv_op_pool_t *pool = malloc(sizeof(subtilis_rv_op_pool_t));

	if (!pool) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	pool->len = 0;
	pool->max_len = 0;
	pool->ops = NULL;
	return pool;
}

size_t subtilis_rv_op_pool_alloc(subtilis_rv_op_pool_t *pool,
				  subtilis_error_t *err)
{
	subtilis_rv_op_t *new_ops;
	size_t new_max;
	size_t new_op;

	new_op = pool->len;
	if (new_op < pool->max_len) {
		pool->len++;
		return new_op;
	}

	if (pool->max_len > SIZE_MAX - SUBTILIS_CONFIG_PROGRAM_GRAN) {
		subtilis_error_set_oom(err);
		return SIZE_MAX;
	}

	new_max = pool->max_len + SUBTILIS_CONFIG_PROGRAM_GRAN;
	new_ops = realloc(pool->ops, new_max * sizeof(subtilis_rv_op_t));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return SIZE_MAX;
	}
	pool->max_len = new_max;
	pool->ops = new_ops;
	pool->len++;
	return new_op;
}

void subtilis_rv_op_pool_reset(subtilis_rv_op_pool_t *pool)
{
	pool->len = 0;
}

void subtilis_rv_op_pool_delete(subtilis_rv_op_pool_t *pool)
{
	size_t i;

	if (pool) {
		for (i = 0; i < pool->len; i++)
			if (pool->ops[i].type == SUBTILIS_RV_OP_STRING)
				free(pool->ops[i].op.str);
		free(pool->ops);
		free(pool);
	}
}

subtilis_rv_reg_t subtilis_rv_acquire_new_reg(subtilis_rv_section_t *s)
{
	return subtilis_rv_ir_to_rv_reg(s->reg_counter++);
}

subtilis_rv_reg_t subtilis_rv_acquire_new_freg(subtilis_rv_section_t *s)
{
	return subtilis_rv_ir_to_real_reg(s->freg_counter++);
}

/* clang-format off */
subtilis_rv_section_t *subtilis_rv_section_new(subtilis_rv_op_pool_t *pool,
					       subtilis_type_section_t *stype,
					       size_t reg_counter,
					       size_t freg_counter,
					       size_t label_counter,
					       size_t locals,
					       const subtilis_settings_t *set,
					       int32_t start_address,
					       subtilis_error_t *err)
{
	subtilis_rv_section_t *s = calloc(1, sizeof(*s));

	if (!s) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	s->stype = subtilis_type_section_dup(stype);
	s->reg_counter = reg_counter;
	s->freg_counter = freg_counter;
	s->label_counter = label_counter;
	s->first_op = SIZE_MAX;
	s->last_op = SIZE_MAX;
	s->locals = locals;
	s->op_pool = pool;
	s->settings = set;
	s->no_cleanup_label = s->label_counter++;
	s->start_address = start_address;

	return s;
}

subtilis_rv_prog_t *subtilis_rv_prog_new(size_t max_sections,
					 subtilis_rv_op_pool_t *op_pool,
					 subtilis_string_pool_t *string_pool,
					 subtilis_constant_pool_t *cnst_pool,
					 const subtilis_settings_t *settings,
					 int32_t start_address,
					 subtilis_error_t *err)
{
	subtilis_rv_prog_t *rv_p = malloc(sizeof(*rv_p));

	if (!rv_p) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	rv_p->sections = malloc(sizeof(*rv_p->sections) * max_sections);
	if (!rv_p->sections) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	rv_p->num_sections = 0;
	rv_p->max_sections = max_sections;
	rv_p->op_pool = op_pool;
	rv_p->string_pool = subtilis_string_pool_clone(string_pool);
	rv_p->constant_pool = subtilis_constant_pool_clone(cnst_pool);

	rv_p->settings = settings;
	rv_p->start_address = start_address;

	return rv_p;

cleanup:
	subtilis_rv_prog_delete(rv_p);
	return NULL;
}

/* clang-format off */
subtilis_rv_section_t *
subtilis_rv_prog_section_new(subtilis_rv_prog_t *prog,
			     subtilis_type_section_t *stype,
			     size_t reg_counter, size_t freg_counter,
			     size_t label_counter,
			     size_t locals, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s;

	if (prog->num_sections == prog->max_sections) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	rv_s = subtilis_rv_section_new(
	    prog->op_pool, stype, reg_counter, freg_counter, label_counter,
	    locals, prog->settings, prog->start_address, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	prog->sections[prog->num_sections++] = rv_s;
	return rv_s;
}
/* clang-format on */

static void prv_free_constants(subtilis_rv_constants_t *constants)
{
	free(constants->ui32);
	free(constants->real);
}

void subtilis_rv_section_delete(subtilis_rv_section_t *s)
{
	if (!s)
		return;

	subtilis_type_section_delete(s->stype);
	subtilis_sizet_vector_free(&s->ret_sites);
	free(s->call_sites);
	prv_free_constants(&s->constants);
	free(s);
}

void subtilis_rv_prog_append_section(subtilis_rv_prog_t *prog,
				     subtilis_rv_section_t *rv_s,
				     subtilis_error_t *err)
{
	if (prog->num_sections == prog->max_sections) {
		subtilis_rv_section_delete(rv_s);
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prog->sections[prog->num_sections++] = rv_s;
}

void subtilis_rv_section_max_regs(subtilis_rv_section_t *s, size_t *int_regs,
				  size_t *real_regs)
{
	*int_regs = subtilis_rv_ir_to_rv_reg(s->reg_counter);
	*real_regs = subtilis_rv_ir_to_real_reg(s->freg_counter);
}

void subtilis_rv_prog_delete(subtilis_rv_prog_t *prog)
{
	size_t i;

	if (!prog)
		return;

	if (prog->sections) {
		for (i = 0; i < prog->num_sections; i++)
			subtilis_rv_section_delete(prog->sections[i]);
		free(prog->sections);
	}
	subtilis_constant_pool_delete(prog->constant_pool);
	subtilis_string_pool_delete(prog->string_pool);
	free(prog);
}

static subtilis_rv_op_t *prv_append_op(subtilis_rv_section_t *s,
				       subtilis_error_t *err)
{
	size_t ptr;
	subtilis_rv_op_t *op;
	size_t prev = s->last_op;

	ptr = subtilis_rv_op_pool_alloc(s->op_pool, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	op = &s->op_pool->ops[ptr];
	memset(op, 0, sizeof(*op));
	op->next = SIZE_MAX;
	op->prev = prev;
	if (s->first_op == SIZE_MAX)
		s->first_op = ptr;
	else
		s->op_pool->ops[s->last_op].next = ptr;
	s->last_op = ptr;
	s->len++;

	return op;
}

static subtilis_rv_op_t *prv_insert_op(subtilis_rv_section_t *s,
				       subtilis_rv_op_t *pos,
				       subtilis_error_t *err)
{
	size_t ptr;
	size_t old_ptr;
	subtilis_rv_op_t *op;

	/*
	 * We need to figure out the pointer before we do the alloc,
	 * as the alloc may invalidate pos.
	 */

	if (pos->prev == SIZE_MAX)
		old_ptr = s->first_op;
	else
		old_ptr = s->op_pool->ops[pos->prev].next;

	ptr = subtilis_rv_op_pool_alloc(s->op_pool, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	/*
	 * pos may have been invalidated by the alloc.
	 */

	pos = &s->op_pool->ops[old_ptr];
	op = &s->op_pool->ops[ptr];
	memset(op, 0, sizeof(*op));
	op->next = old_ptr;
	op->prev = pos->prev;
	s->op_pool->ops[pos->prev].next = ptr;
	pos->prev = ptr;

	if (old_ptr == s->first_op)
		s->first_op = ptr;
	s->len++;

	return op;
}

subtilis_rv_instr_t *
subtilis_rv_section_add_instr(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_instr_encoding_t etype,
			      subtilis_error_t *err)
{
	subtilis_rv_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op->type = SUBTILIS_RV_OP_INSTR;
	op->op.instr.itype = itype;
	op->op.instr.etype = etype;
	return &op->op.instr;
}

subtilis_rv_instr_t *
subtilis_rv_section_insert_instr(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_instr_encoding_t etype,
				 subtilis_error_t *err)
{
	subtilis_rv_op_t *op;

	op = prv_insert_op(s, pos, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op->type = SUBTILIS_RV_OP_INSTR;
	op->op.instr.itype = itype;
	op->op.instr.etype = etype;
	return &op->op.instr;
}

void
subtilis_rv_section_add_known_jal(subtilis_rv_section_t *s,
				  subtilis_rv_reg_t rd,
				  uint32_t offset,
				  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_ujtype_t *uj;

	instr = subtilis_rv_section_add_instr(s, SUBTILIS_RV_JAL,
					      SUBTILIS_RV_J_TYPE,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (offset & 1) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	uj = &instr->operands.uj;
	uj->rd = rd;
	uj->op.imm = offset;
	uj->is_label = false;
}

void
subtilis_rv_section_add_itype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      uint32_t imm,  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_itype_t *i;

	instr = subtilis_rv_section_add_instr(s, itype,
					      SUBTILIS_RV_I_TYPE,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	i = &instr->operands.i;
	i->rd = rd;
	i->rs1 = rs1;
	i->op.imm = imm;
	i->is_label = false;
}

void
subtilis_rv_section_insert_itype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 subtilis_rv_reg_t rs1,
				 uint32_t imm,  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_itype_t *i;

	instr = subtilis_rv_section_insert_instr(s, pos, itype,
						 SUBTILIS_RV_I_TYPE,
						 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	i = &instr->operands.i;
	i->rd = rd;
	i->rs1 = rs1;
	i->op.imm = imm;
	i->is_label = false;
}

void
subtilis_rv_section_add_utype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      uint32_t imm,  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_ujtype_t *uj;

	instr = subtilis_rv_section_add_instr(s, itype,
					      SUBTILIS_RV_U_TYPE,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	uj = &instr->operands.uj;
	uj->op.imm = imm >> 12;
	uj->rd = rd;
	uj->is_label = false;
}

void
subtilis_rv_section_insert_utype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 uint32_t imm,  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_ujtype_t *uj;

	instr = subtilis_rv_section_insert_instr(s, pos, itype,
						 SUBTILIS_RV_U_TYPE,
						 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	uj = &instr->operands.uj;
	uj->op.imm = imm >> 12;
	uj->rd = rd;
	uj->is_label = false;
}


void
subtilis_rv_section_add_stype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      uint32_t imm,  subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_sbtype_t *sb;

	instr = subtilis_rv_section_add_instr(s, itype,
					      SUBTILIS_RV_S_TYPE,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sb = &instr->operands.sb;
	sb->rs1 = rs1;
	sb->rs2 = rs2;
	sb->op.imm = imm;
	sb->is_label = false;
}

void
subtilis_rv_section_insert_sbtype(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_instr_type_t itype,
				  subtilis_rv_reg_t rs1,
				  subtilis_rv_reg_t rs2,
				  uint32_t imm, subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_sbtype_t *sb;

	instr = subtilis_rv_section_insert_instr(s, pos, itype,
						 SUBTILIS_RV_U_TYPE,
						 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	sb = &instr->operands.sb;
	sb->rs1 = rs1;
	sb->rs2 = rs2;
	sb->op.imm = imm;
	sb->is_label = false;
}

void
subtilis_rv_section_add_rtype(subtilis_rv_section_t *s,
			      subtilis_rv_instr_type_t itype,
			      subtilis_rv_reg_t rd,
			      subtilis_rv_reg_t rs1,
			      subtilis_rv_reg_t rs2,
			      subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_rtype_t *r;

	instr = subtilis_rv_section_add_instr(s, itype,
					      SUBTILIS_RV_R_TYPE,
					      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	r = &instr->operands.r;
	r->rd = rd;
	r->rs1 = rs1;
	r->rs2 = rs2;
}

void
subtilis_rv_section_insert_rtype(subtilis_rv_section_t *s,
				 subtilis_rv_op_t *pos,
				 subtilis_rv_instr_type_t itype,
				 subtilis_rv_reg_t rd,
				 subtilis_rv_reg_t rs1,
				 subtilis_rv_reg_t rs2,
				 subtilis_error_t *err)
{
	subtilis_rv_instr_t *instr;
	rv_rtype_t *r;

	instr = subtilis_rv_section_insert_instr(s, pos, itype,
						 SUBTILIS_RV_U_TYPE,
						 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	r = &instr->operands.r;
	r->rd = rd;
	r->rs1 = rs1;
	r->rs2 = rs2;
}

void
subtilis_rv_section_add_li(subtilis_rv_section_t *s,
			   subtilis_rv_reg_t rd,
			   int32_t imm,  subtilis_error_t *err)
{
	uint32_t lower;

	/*
	 * Then imm will fit in 12 bits.
	 */

	if ((imm > -2048) && (imm < 2048)) {
		subtilis_rv_section_add_addi(s, rd, 0, imm, err);
		return;
	}

	subtilis_rv_section_add_utype(s, SUBTILIS_RV_LUI, rd, (uint32_t) imm,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	lower = ((uint32_t) imm) & 4095;
	if (lower)
		subtilis_rv_section_add_addi(s, rd, rd, lower, err);
}

void
subtilis_rv_section_insert_li(subtilis_rv_section_t *s,
			      subtilis_rv_op_t *pos,
			      subtilis_rv_reg_t rd,
			      int32_t imm,  subtilis_error_t *err)
{
	uint32_t lower;

	/*
	 * Then imm will fit in 12 bits.
	 */

	if ((imm > -2048) && (imm < 2048)) {
		subtilis_rv_section_insert_addi(s, pos, rd, 0, imm, err);
		return;
	}

	subtilis_rv_section_insert_utype(s, pos, SUBTILIS_RV_LUI, rd,
					 (uint32_t) imm, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	lower = ((uint32_t) imm) & 4095;
	if (lower)
		subtilis_rv_section_insert_addi(s, pos, rd, rd, lower, err);
}

subtilis_rv_reg_t subtilis_rv_ir_to_rv_reg(size_t ir_reg)
{
	subtilis_rv_reg_t rv_reg;

	switch (ir_reg) {
	case SUBTILIS_IR_REG_GLOBAL:
		rv_reg = SUBTILIS_RV_REG_GLOBAL;
		break;
	case SUBTILIS_IR_REG_LOCAL:
		rv_reg = SUBTILIS_RV_REG_LOCAL;
		break;
	case SUBTILIS_IR_REG_STACK:
		rv_reg = SUBTILIS_RV_REG_STACK;
		break;
	default:
		rv_reg = ir_reg - SUBTILIS_IR_REG_TEMP_START +
			SUBTILIS_RV_REG_MAX_INT_REGS;
		break;
	}

	return rv_reg;
}

subtilis_rv_reg_t subtilis_rv_ir_to_real_reg(size_t ir_reg)
{
	subtilis_rv_reg_t rv_reg;

	rv_reg = ir_reg + SUBTILIS_RV_REG_MAX_REAL_REGS;

	return rv_reg;
}

void subtilis_rv_section_add_label(subtilis_rv_section_t *s, size_t label,
				    subtilis_error_t *err)
{
	subtilis_rv_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op->type = SUBTILIS_RV_OP_LABEL;
	op->op.label = label;
	s->label_counter++;
}

void subtilis_rv_section_insert_lw(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t dest,
				   subtilis_rv_reg_t base,
				   int32_t offset, subtilis_error_t *err)
{
	subtilis_rv_section_insert_itype(s, pos, SUBTILIS_RV_LW, dest, base,
					 offset, err);
}

void subtilis_rv_section_insert_sw(subtilis_rv_section_t *s,
				   subtilis_rv_op_t *pos,
				   subtilis_rv_reg_t val,
				   subtilis_rv_reg_t base,
				   int32_t offset, subtilis_error_t *err)
{
	subtilis_rv_section_insert_sbtype(s, pos, SUBTILIS_RV_SW, val, base, \
					  offset, err);
}

void subtilis_rv_section_insert_label(subtilis_rv_section_t *s, size_t label,
				      subtilis_rv_op_t *pos,
				      subtilis_error_t *err)
{
	subtilis_rv_op_t *op;

	op = prv_insert_op(s, pos, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op->type = SUBTILIS_RV_OP_LABEL;
	op->op.label = label;
	s->label_counter++;
}

void subtilis_rv_insert_offset_helper(subtilis_rv_section_t *s,
				      subtilis_rv_op_t *pos,
				      subtilis_rv_reg_t base,
				      subtilis_rv_reg_t tmp,
				      int32_t offset, subtilis_error_t *err)
{
	subtilis_rv_section_insert_li(s, pos, tmp, offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_rv_section_insert_add(s, pos, tmp, tmp, base, err);
}

void subtilis_rv_insert_lw_helper(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t dest,
				  subtilis_rv_reg_t base,
				  int32_t offset, subtilis_error_t *err)
{
	if (offset > SUBTILIS_RV_MAX_OFFSET ||
	    offset < SUBTILIS_RV_MIN_OFFSET) {
		subtilis_rv_insert_offset_helper(s, pos, base, dest, offset,
						 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		base = dest;
		offset = 0;
	}
	subtilis_rv_section_insert_lw(s, pos, dest, base, offset, err);
}

void subtilis_rv_insert_sw_helper(subtilis_rv_section_t *s,
				  subtilis_rv_op_t *pos,
				  subtilis_rv_reg_t dest,
				  subtilis_rv_reg_t base,
				  subtilis_rv_reg_t tmp,
				  int32_t offset, subtilis_error_t *err)
{
	if (offset > SUBTILIS_RV_MAX_OFFSET ||
	    offset < -SUBTILIS_RV_MIN_OFFSET) {
		subtilis_rv_insert_offset_helper(s, pos, base, tmp, offset,
						 err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		offset = 0;
	}
	subtilis_rv_section_insert_sw(s, pos, dest, base, offset, err);
}

