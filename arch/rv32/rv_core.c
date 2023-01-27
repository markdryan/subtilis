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
	return 0;
}

subtilis_rv_reg_t subtilis_rv_acquire_new_freg(subtilis_rv_section_t *s)
{
	return 0;
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
void subtilis_rv_section_delete(subtilis_rv_section_t *s)
{

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

}

void subtilis_rv_prog_delete(subtilis_rv_prog_t *prog)
{

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
	uj->rd = 0;
	uj->imm = offset;
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

	if (imm & 1) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	i = &instr->operands.i;
	i->rd = rd;
	i->rs1 = rs1;
	i->imm = imm;
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
	uj->imm = imm >> 12;
}

void
subtilis_rv_section_mv(subtilis_rv_section_t *s,
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
		subtilis_rv_section_add_addi(s, rd, 0, lower, err);
}
