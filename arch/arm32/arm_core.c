/*
 * Copyright (c) 2017 Mark Ryan
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arm_core.h"

/* clang-format off */
static const uint32_t subitlis_arm_imm_mask[] = {
	0x000000ff,
	0xc000003f,
	0xf000000f,
	0xfc000003,
	0xff000000,
	0xff000000 >> 2,
	0xff000000 >> 4,
	0xff000000 >> 6,
	0xff000000 >> 8,
	0xff000000 >> 10,
	0xff000000 >> 12,
	0xff000000 >> 14,
	0xff000000 >> 16,
	0xff000000 >> 18,
	0xff000000 >> 20,
	0xff000000 >> 22,
};

/* clang-format on */

bool subtilis_arm_is_fixed(size_t reg)
{
	return reg < SUBTILIS_ARM_INT_VIRT_REG_START;
}

subtilis_arm_op_pool_t *subtilis_arm_op_pool_new(subtilis_error_t *err)
{
	subtilis_arm_op_pool_t *pool = malloc(sizeof(subtilis_arm_op_pool_t));

	if (!pool) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	pool->len = 0;
	pool->max_len = 0;
	pool->ops = NULL;
	return pool;
}

void subtilis_arm_op_pool_reset(subtilis_arm_op_pool_t *pool) { pool->len = 0; }

void subtilis_arm_op_pool_delete(subtilis_arm_op_pool_t *pool)
{
	size_t i;

	if (pool) {
		for (i = 0; i < pool->len; i++)
			if (pool->ops[i].type == SUBTILIS_ARM_OP_STRING)
				free(pool->ops[i].op.str);
		free(pool->ops);
		free(pool);
	}
}

size_t subtilis_arm_op_pool_alloc(subtilis_arm_op_pool_t *pool,
				  subtilis_error_t *err)
{
	subtilis_arm_op_t *new_ops;
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
	new_ops = realloc(pool->ops, new_max * sizeof(subtilis_arm_op_t));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return SIZE_MAX;
	}
	pool->max_len = new_max;
	pool->ops = new_ops;
	pool->len++;
	return new_op;
}

subtilis_arm_reg_t subtilis_arm_acquire_new_reg(subtilis_arm_section_t *s)
{
	return subtilis_arm_ir_to_arm_reg(s->reg_counter++);
}

subtilis_arm_reg_t subtilis_arm_acquire_new_freg(subtilis_arm_section_t *s)
{
	return subtilis_arm_ir_to_freg(s->freg_counter++);
}

/* clang-format off */
subtilis_arm_section_t *subtilis_arm_section_new(subtilis_arm_op_pool_t *pool,
						 subtilis_type_section_t *stype,
						 size_t reg_counter,
						 size_t freg_counter,
						 size_t label_counter,
						 size_t locals,
						 const subtilis_settings_t *set,
						 subtilis_error_t *err)
/* clang-format on */

{
	subtilis_arm_section_t *s = calloc(1, sizeof(*s));

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

	return s;
}

/* The register counters in the arm_section are very confusing.  They always
 * count virtual registers as used in the IR.  To map them to ARM registers
 * one needs to call a function, e.g., subtilis_arm_ir_to_arm_reg.  Sometimes
 * you need to know the maximum number of registers used and the counters
 * won't tell you this.  This function will.  We always return at least
 * 16 for the integer registers as we're always going to use PC.
 */

void subtilis_arm_section_max_regs(subtilis_arm_section_t *s, size_t *int_regs,
				   size_t *real_regs)
{
	*int_regs = subtilis_arm_ir_to_arm_reg(s->reg_counter);
	if (*int_regs < SUBTILIS_ARM_INT_VIRT_REG_START)
		*int_regs = SUBTILIS_ARM_INT_VIRT_REG_START;
	*real_regs = subtilis_arm_ir_to_freg(s->freg_counter);
}

static void prv_free_constants(subtilis_arm_constants_t *constants)
{
	free(constants->ui32);
	free(constants->real);
}

void subtilis_arm_section_delete(subtilis_arm_section_t *s)
{
	if (!s)
		return;

	subtilis_type_section_delete(s->stype);
	free(s->ret_sites);
	free(s->call_sites);
	prv_free_constants(&s->constants);
	free(s);
}

/* clang-format off */
void subtilis_arm_section_add_call_site(subtilis_arm_section_t *s,
					size_t stm_site, size_t ldm_site,
					size_t stf_site, size_t ldf_site,
					size_t int_args, size_t real_args,
					size_t op, size_t *int_arg_ops,
					size_t *real_arg_ops,
					subtilis_error_t *err)
/* clang-format on */

{
	size_t new_max;
	subtilis_arm_call_site_t *site;
	subtilis_arm_call_site_t *new_call_sites;
	size_t i;

	if (s->call_site_count == s->max_call_site_count) {
		new_max = s->call_site_count + SUBTILIS_CONFIG_PROC_GRAN;
		new_call_sites =
		    realloc(s->call_sites, new_max * sizeof(*new_call_sites));
		if (!new_call_sites) {
			subtilis_error_set_oom(err);
			return;
		}
		s->call_sites = new_call_sites;
		s->max_call_site_count = new_max;
	}
	site = &s->call_sites[s->call_site_count++];
	site->stm_site = stm_site;
	site->ldm_site = ldm_site;
	site->stf_site = stf_site;
	site->ldf_site = ldf_site;
	site->int_args = int_args;
	site->real_args = real_args;
	site->call_site = op;

	if (int_args > 4)
		for (i = 0; i < int_args - 4; i++)
			site->int_arg_ops[i] = int_arg_ops[i];

	if (real_args > 4)
		for (i = 0; i < real_args - 4; i++)
			site->real_arg_ops[i] = real_arg_ops[i];
}

void subtilis_arm_section_add_ret_site(subtilis_arm_section_t *s, size_t op,
				       subtilis_error_t *err)
{
	size_t new_max;
	size_t *new_ret_sites;

	if (s->ret_site_count == s->max_ret_site_count) {
		new_max = s->ret_site_count + SUBTILIS_CONFIG_PROC_GRAN;
		new_ret_sites =
		    realloc(s->ret_sites, new_max * sizeof(*new_ret_sites));
		if (!new_ret_sites) {
			subtilis_error_set_oom(err);
			return;
		}
		s->ret_sites = new_ret_sites;
		s->max_ret_site_count = new_max;
	}
	s->ret_sites[s->ret_site_count++] = op;
}

subtilis_arm_prog_t *subtilis_arm_prog_new(size_t max_sections,
					   subtilis_arm_op_pool_t *op_pool,
					   subtilis_string_pool_t *string_pool,
					   subtilis_constant_pool_t *cnst_pool,
					   const subtilis_settings_t *settings,
					   subtilis_error_t *err)
{
	double dummy_float = 1.0;
	uint32_t *lower_word = (uint32_t *)((void *)&dummy_float);
	subtilis_arm_prog_t *arm_p = malloc(sizeof(*arm_p));

	if (!arm_p) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	arm_p->sections = malloc(sizeof(*arm_p->sections) * max_sections);
	if (!arm_p->sections) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}

	arm_p->num_sections = 0;
	arm_p->max_sections = max_sections;
	arm_p->op_pool = op_pool;
	arm_p->string_pool = subtilis_string_pool_clone(string_pool);
	arm_p->constant_pool = subtilis_constant_pool_clone(cnst_pool);

	/* Slightly weird but on ARM FPA the words of a double are big endian */
	arm_p->reverse_fpa_consts = (*lower_word) == 0;
	arm_p->settings = settings;

	return arm_p;

cleanup:
	subtilis_arm_prog_delete(arm_p);
	return NULL;
}

/* clang-format off */
subtilis_arm_section_t *
subtilis_arm_prog_section_new(subtilis_arm_prog_t *prog,
			      subtilis_type_section_t *stype,
			      size_t reg_counter, size_t freg_counter,
			      size_t label_counter, size_t locals,
			      subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_section_t *arm_s;

	if (prog->num_sections == prog->max_sections) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	arm_s = subtilis_arm_section_new(prog->op_pool, stype, reg_counter,
					 freg_counter, label_counter, locals,
					 prog->settings, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	prog->sections[prog->num_sections++] = arm_s;
	return arm_s;
}

void subtilis_arm_prog_append_section(subtilis_arm_prog_t *prog,
				      subtilis_arm_section_t *arm_s,
				      subtilis_error_t *err)
{
	if (prog->num_sections == prog->max_sections) {
		subtilis_arm_section_delete(arm_s);
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prog->sections[prog->num_sections++] = arm_s;
}

void subtilis_arm_prog_delete(subtilis_arm_prog_t *prog)
{
	size_t i;

	if (!prog)
		return;

	if (prog->sections) {
		for (i = 0; i < prog->num_sections; i++)
			subtilis_arm_section_delete(prog->sections[i]);
		free(prog->sections);
	}
	subtilis_constant_pool_delete(prog->constant_pool);
	subtilis_string_pool_delete(prog->string_pool);
	free(prog);
}

subtilis_arm_reg_t subtilis_arm_ir_to_arm_reg(size_t ir_reg)
{
	subtilis_arm_reg_t arm_reg;

	switch (ir_reg) {
	case SUBTILIS_IR_REG_GLOBAL:
		arm_reg = 12;
		break;
	case SUBTILIS_IR_REG_LOCAL:
		arm_reg = 11;
		break;
	case SUBTILIS_IR_REG_STACK:
		arm_reg = 13;
		break;
	default:
		ir_reg = ir_reg - SUBTILIS_IR_REG_TEMP_START +
			 SUBTILIS_ARM_INT_VIRT_REG_START;
		arm_reg = ir_reg;
		break;
	}

	return arm_reg;
}

subtilis_arm_reg_t subtilis_arm_ir_to_freg(size_t ir_reg)
{
	subtilis_arm_reg_t arm_reg;

	arm_reg = ir_reg + SUBTILIS_ARM_FPA_VIRT_REG_START;

	return arm_reg;
}

static subtilis_arm_op_t *prv_append_op(subtilis_arm_section_t *s,
					subtilis_error_t *err)
{
	size_t ptr;
	subtilis_arm_op_t *op;
	size_t prev = s->last_op;

	ptr = subtilis_arm_op_pool_alloc(s->op_pool, err);
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

static subtilis_arm_op_t *prv_insert_op(subtilis_arm_section_t *s,
					subtilis_arm_op_t *pos,
					subtilis_error_t *err)
{
	size_t ptr;
	size_t old_ptr;
	subtilis_arm_op_t *op;

	ptr = subtilis_arm_op_pool_alloc(s->op_pool, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (pos->prev == SIZE_MAX)
		old_ptr = s->first_op;
	else
		old_ptr = s->op_pool->ops[pos->prev].next;

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

void subtilis_arm_section_add_label(subtilis_arm_section_t *s, size_t label,
				    subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op->type = SUBTILIS_ARM_OP_LABEL;
	op->op.label = label;
	s->label_counter++;
}

void subtilis_arm_section_insert_label(subtilis_arm_section_t *s, size_t label,
				       subtilis_arm_op_t *pos,
				       subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_insert_op(s, pos, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op->type = SUBTILIS_ARM_OP_LABEL;
	op->op.label = label;
	s->label_counter++;
}

static void prv_add_ui32_constant(subtilis_arm_section_t *s, size_t label,
				  uint32_t num, bool link_time,
				  subtilis_error_t *err)
{
	subtilis_arm_ui32_constant_t *c;
	subtilis_arm_ui32_constant_t *new_constants;
	size_t new_max;

	if (s->constants.ui32_count == s->constants.max_ui32) {
		new_max = s->constants.max_ui32 + 64;
		new_constants =
		    realloc(s->constants.ui32,
			    new_max * sizeof(subtilis_arm_ui32_constant_t));
		if (!new_constants) {
			subtilis_error_set_oom(err);
			return;
		}
		s->constants.max_ui32 = new_max;
		s->constants.ui32 = new_constants;
	}
	c = &s->constants.ui32[s->constants.ui32_count++];
	c->integer = num;
	c->label = label;
	c->link_time = link_time;
}

subtilis_arm_instr_t *
subtilis_arm_section_add_instr(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t type,
			       subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op->type = SUBTILIS_ARM_OP_INSTR;
	op->op.instr.type = type;
	return &op->op.instr;
}

/* clang-format off */
subtilis_arm_instr_t *
subtilis_arm_section_insert_instr(subtilis_arm_section_t *s,
				  subtilis_arm_op_t *pos,
				  subtilis_arm_instr_type_t type,
				  subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_op_t *op;

	op = prv_insert_op(s, pos, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	op->type = SUBTILIS_ARM_OP_INSTR;
	op->op.instr.type = type;
	return &op->op.instr;
}

subtilis_arm_instr_t *subtilis_arm_section_dup_instr(subtilis_arm_section_t *s,
						     subtilis_error_t *err)
{
	subtilis_arm_op_t *src;
	subtilis_arm_op_t *op;

	if (s->len == 0) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	src = &s->op_pool->ops[s->last_op];
	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	*op = *src;
	op->next = SIZE_MAX;

	return &op->op.instr;
}

static void prv_encode_imm(uint32_t imm, size_t mask_index, uint32_t *encoded)
{
	switch (mask_index) {
	case 0:
		*encoded = imm;
		break;
	case 1:
		*encoded = (imm >> 30) | (((imm & 0x3F)) << 2);
		break;
	case 2:
		*encoded = (imm >> 28) | (((imm & 0xF)) << 4);
		break;
	case 3:
		*encoded = (imm >> 26) | (((imm & 0x3)) << 6);
		break;
	default:
		*encoded = imm >> (24 - ((mask_index - 4) * 2));
		break;
	}

	*encoded |= mask_index << 8;
}

/*
 * Sometimes if we cannot encode  the exact immediate, the next encodable
 * number is acceptable.  We can use this for our stack frame.  If for
 * example the size of the local variables and spill registers was
 * 516, we could reserve 520 bytes instead.  The stack frame can then
 * be set up with a single instruction.  Otherwise, we'd need a ldr
 * followed by a sub.
 */

uint32_t subtilis_arm_encode_nearest(int32_t num, subtilis_error_t *err)
{
	bool can_encode;
	uint32_t encoded;
	size_t i;
	size_t new_num;
	size_t mask;
	size_t shift;

	can_encode = subtilis_arm_encode_imm(num, &encoded);
	if (can_encode)
		return encoded;

	if (((uint32_t)num) >= ((uint32_t)0xff000000)) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	/* First first bit that's set */

	for (i = 31; i > 7; i--)
		if (num & (1 << i))
			break;

	if (i == 7) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	shift = i - 7;

	if (shift & 1)
		shift++;
	mask = 0xff << shift;
	new_num = (((uint32_t)num) & mask) + (1 << shift);
	can_encode = subtilis_arm_encode_imm(new_num, &encoded);
	if (!can_encode) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	return encoded;
}

bool subtilis_arm_encode_imm(int32_t num, uint32_t *encoded)
{
	size_t i;
	uint32_t imm = (uint32_t)num;

	if (imm == 0) {
		*encoded = 0;
		return true;
	}

	for (i = 0; i < sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t); i++) {
		if ((imm & ~subitlis_arm_imm_mask[i]) == 0)
			break;
	}
	if (i == sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t))
		return false;

	prv_encode_imm(imm, i, encoded);

	return true;
}

bool subtilis_arm_encode_lvl2_imm(int32_t num, uint32_t *encoded1,
				  uint32_t *encoded2)
{
	size_t i;
	uint32_t rem;
	uint32_t imm = (uint32_t)num;

	for (i = 0; i < sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t); i++) {
		rem = imm & ~subitlis_arm_imm_mask[i];
		if (subtilis_arm_encode_imm(rem, encoded2))
			break;
	}
	if (i == sizeof(subitlis_arm_imm_mask) / sizeof(uint32_t))
		return false;

	prv_encode_imm(imm - rem, i, encoded1);

	return true;
}

static size_t prv_add_data_imm_ldr(subtilis_arm_section_t *s,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest, int32_t op2,
				   subtilis_error_t *err)
{
	subtilis_arm_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label;
	size_t i;

	for (i = 0; i < s->constants.ui32_count; i++)
		if (s->constants.ui32[i].integer == op2)
			break;

	if (i < s->constants.ui32_count) {
		label = s->constants.ui32[i].label;
	} else {
		label = s->label_counter++;
		prv_add_ui32_constant(s, label, (uint32_t)op2, false, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;
	}

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ldrc = &instr->operands.ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
	ldrc->link_time = false;
	return label;
}

size_t subtilis_arm_insert_data_imm_ldr(subtilis_arm_section_t *s,
					subtilis_arm_op_t *current,
					subtilis_arm_ccode_type_t ccode,
					subtilis_arm_reg_t dest, int32_t op2,
					subtilis_error_t *err)
{
	subtilis_arm_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label = s->label_counter++;

	prv_add_ui32_constant(s, label, (uint32_t)op2, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	instr = subtilis_arm_section_insert_instr(s, current,
						  SUBTILIS_ARM_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ldrc = &instr->operands.ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
	ldrc->link_time = false;
	return label;
}

size_t subtilis_add_explicit_ldr(subtilis_arm_section_t *s,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest, int32_t op2,
				 bool link_time, subtilis_error_t *err)
{
	subtilis_arm_ldrc_instr_t *ldrc;
	subtilis_arm_instr_t *instr;
	size_t label = s->label_counter++;

	prv_add_ui32_constant(s, label, (uint32_t)op2, link_time, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_LDRC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ldrc = &instr->operands.ldrc;
	ldrc->ccode = ccode;
	ldrc->dest = dest;
	ldrc->label = label;
	ldrc->link_time = link_time;

	return label;
}

size_t subtilis_add_data_imm_ldr_datai(subtilis_arm_section_t *s,
				       subtilis_arm_instr_type_t itype,
				       subtilis_arm_ccode_type_t ccode,
				       bool status, subtilis_arm_reg_t dest,
				       subtilis_arm_reg_t op1, int32_t op2,
				       subtilis_error_t *err)
{
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t ldr_dest;
	size_t label;

	ldr_dest = subtilis_arm_acquire_new_reg(s);

	label = prv_add_data_imm_ldr(s, ccode, ldr_dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.op.reg = ldr_dest;
	return label;
}

void subtilis_arm_add_addsub_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op1, int32_t op2,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_data_instr_t *datai2;
	bool can_encode;
	uint32_t encoded;
	uint32_t encoded2 = 0;

	can_encode = subtilis_arm_encode_imm(op2, &encoded);
	/* TODO why does it matter if the op2 is less than 0? */
	if ((!can_encode) && (op2 < 0)) {
		can_encode = subtilis_arm_encode_imm(-op2, &encoded);
		if (can_encode) {
			op2 = -op2;
			itype = alt_type;
		}
	}

	if (!can_encode) {
		if (status && (ccode != SUBTILIS_ARM_CCODE_AL)) {
			(void)subtilis_add_data_imm_ldr_datai(
			    s, itype, ccode, status, dest, op1, op2, err);
			return;
		}

		can_encode =
		    subtilis_arm_encode_lvl2_imm(op2, &encoded, &encoded2);

		if ((!can_encode) && (op2 < 0)) {
			can_encode = subtilis_arm_encode_lvl2_imm(
			    -op2, &encoded, &encoded2);
			if (can_encode) {
				op2 = -op2;
				itype = alt_type;
			}
		}
	}

	if (!can_encode) {
		(void)subtilis_add_data_imm_ldr_datai(s, itype, ccode, status,
						      dest, op1, op2, err);
		return;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->ccode = ccode;
	datai->dest = encoded2 == 0 ? dest : subtilis_arm_acquire_new_reg(s);
	datai->op1 = op1;
	datai->op2.op.integer = encoded;

	if (encoded2 == 0)
		return;

	instr = subtilis_arm_section_dup_instr(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai2 = &instr->operands.data;
	datai2->op1 = datai->dest;
	datai2->op2.op.integer = encoded2;
	datai2->dest = dest;
}

void subtilis_arm_add_rsub_imm(subtilis_arm_section_t *s,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	bool can_encode;
	uint32_t encoded;
	subtilis_arm_reg_t neg_dest;
	subtilis_arm_instr_type_t itype = SUBTILIS_ARM_INSTR_RSB;

	can_encode = subtilis_arm_encode_imm(op2, &encoded);
	if ((!can_encode) && (op2 < 0) &&
	    !(status && (ccode != SUBTILIS_ARM_CCODE_AL))) {
		can_encode = subtilis_arm_encode_imm(-op2, &encoded);
		if (can_encode) {
			neg_dest = subtilis_arm_acquire_new_reg(s);
			subtilis_arm_add_rsub_imm(s, ccode, false, neg_dest,
						  op1, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			op1 = neg_dest;
			op2 = -op2;
			itype = SUBTILIS_ARM_INSTR_SUB;
		}
	}

	if (!can_encode) {
		(void)subtilis_add_data_imm_ldr_datai(s, itype, ccode, status,
						      dest, op1, op2, err);
		return;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2.op.integer = encoded;
}

void subtilis_arm_add_mul_imm(subtilis_arm_section_t *s,
			      subtilis_arm_ccode_type_t ccode, bool status,
			      subtilis_arm_reg_t dest, subtilis_arm_reg_t rm,
			      int32_t rs, subtilis_error_t *err)
{
	subtilis_arm_reg_t mov_dest;
	subtilis_arm_instr_t *instr;
	subtilis_arm_mul_instr_t *mul;

	if (rm == dest) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	mov_dest = subtilis_arm_acquire_new_reg(s);
	subtilis_arm_add_mov_imm(s, ccode, false, mov_dest, rs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* TODO: A whole pile of optimisations here */

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_MUL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	mul = &instr->operands.mul;
	mul->status = status;
	mul->ccode = ccode;
	mul->dest = dest;
	mul->rm = rm;
	mul->rs = mov_dest;
}

void subtilis_arm_add_mul(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t rm,
			  subtilis_arm_reg_t rs, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_reg_t tmp_reg;
	subtilis_arm_mul_instr_t *mul;

	if (dest == rm) {
		if (dest == rs) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		tmp_reg = rs;
		rs = rm;
		rm = tmp_reg;
	}

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_MUL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	mul = &instr->operands.mul;
	mul->status = status;
	mul->ccode = ccode;
	mul->dest = dest;
	mul->rm = rm;
	mul->rs = rs;
}

void subtilis_arm_add_data_imm(subtilis_arm_section_t *s,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t op1,
			       int32_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	uint32_t encoded;
	subtilis_arm_reg_t mov_dest;
	subtilis_arm_op2_t data_opt2;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		mov_dest = subtilis_arm_acquire_new_reg(s);
		subtilis_arm_add_mov_imm(s, ccode, false, mov_dest, op2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		data_opt2.type = SUBTILIS_ARM_OP2_REG;
		data_opt2.op.reg = mov_dest;
	} else {
		data_opt2.type = SUBTILIS_ARM_OP2_I32;
		data_opt2.op.integer = encoded;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2 = data_opt2;
}

void subtilis_arm_add_cmov(subtilis_arm_section_t *s, subtilis_arm_reg_t dest,
			   subtilis_arm_reg_t op1, subtilis_arm_reg_t op2,
			   subtilis_arm_reg_t op3, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_cmov_instr_t *cmov;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_CMOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cmov = &instr->operands.cmov;
	cmov->dest = dest;
	cmov->op1 = op1;
	cmov->op2 = op2;
	cmov->op3 = op3;
	cmov->fused = false;
}

void subtilis_arm_add_cmov_fused(subtilis_arm_section_t *s,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op2, subtilis_arm_reg_t op3,
				 subtilis_arm_ccode_type_t true_cond,
				 subtilis_arm_ccode_type_t false_cond,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_cmov_instr_t *cmov;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_CMOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cmov = &instr->operands.cmov;
	cmov->dest = dest;
	cmov->op2 = op2;
	cmov->op3 = op3;
	cmov->fused = true;
	cmov->true_cond = true_cond;
	cmov->false_cond = false_cond;
}

void subtilis_arm_add_swi(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode, size_t code,
			  uint32_t read_mask, uint32_t write_mask,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_swi_instr_t *swi;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_SWI, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	swi = &instr->operands.swi;
	swi->ccode = ccode;
	swi->code = code;
	swi->reg_read_mask = read_mask;
	swi->reg_write_mask = write_mask;
}

void subtilis_arm_add_movmvn_reg(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t op2, subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = op2;
}

void subtilis_arm_add_movmvn_imm(subtilis_arm_section_t *s,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_instr_type_t alt_type,
				 subtilis_arm_ccode_type_t ccode, bool status,
				 subtilis_arm_reg_t dest, int32_t op2,
				 subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	uint32_t encoded = 0;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		if (subtilis_arm_encode_imm(~op2, &encoded)) {
			itype = alt_type;
		} else {
			(void)prv_add_data_imm_ldr(s, ccode, dest, op2, err);
			return;
		}
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->status = status;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op2.op.integer = encoded;
}

void subtilis_arm_add_stran_imm(subtilis_arm_section_t *s,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				subtilis_arm_reg_t base, int32_t offset,
				bool byte, subtilis_error_t *err)
{
	subtilis_arm_op2_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	bool subtract = false;

	if (offset < 0) {
		offset = -offset;
		subtract = true;
	}

	if (offset > 4095) {
		op2.op.reg = subtilis_arm_acquire_new_reg(s);
		(void)prv_add_data_imm_ldr(s, ccode, op2.op.reg, offset, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		op2.type = SUBTILIS_ARM_OP2_REG;
	} else {
		op2.type = SUBTILIS_ARM_OP2_I32;
		op2.op.integer = offset;
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = op2;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->byte = byte;
	stran->subtract = subtract;
}

void subtilis_arm_add_push(subtilis_arm_section_t *s,
			   subtilis_arm_ccode_type_t ccode, size_t reg_num,
			   subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = reg_num;
	stran->base = 13;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = true;
	stran->write_back = true;
	stran->subtract = true;
	stran->byte = false;
}

void subtilis_arm_add_pop(subtilis_arm_section_t *s,
			  subtilis_arm_ccode_type_t ccode, size_t reg_num,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	instr = subtilis_arm_section_add_instr(s, SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = reg_num;
	stran->base = 13;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;
}

void subtilis_arm_insert_push(subtilis_arm_section_t *s,
			      subtilis_arm_op_t *current,
			      subtilis_arm_ccode_type_t ccode, size_t reg_num,
			      subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	instr = subtilis_arm_section_insert_instr(s, current,
						  SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = reg_num;
	stran->base = 13;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = true;
	stran->write_back = true;
	stran->subtract = true;
	stran->byte = false;
}

void subtilis_arm_insert_pop(subtilis_arm_section_t *s,
			     subtilis_arm_op_t *current,
			     subtilis_arm_ccode_type_t ccode, size_t reg_num,
			     subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	instr = subtilis_arm_section_insert_instr(s, current,
						  SUBTILIS_ARM_INSTR_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = reg_num;
	stran->base = 13;
	stran->offset.type = SUBTILIS_ARM_OP2_I32;
	stran->offset.op.integer = 4;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->subtract = false;
	stran->byte = false;
}

/* clang-format off */
void subtilis_arm_insert_stran_spill_imm(subtilis_arm_section_t *s,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_op2_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;

	op2.op.reg = spill_reg;
	(void)subtilis_arm_insert_data_imm_ldr(s, current, ccode, op2.op.reg,
					       offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	op2.type = SUBTILIS_ARM_OP2_REG;

	instr = subtilis_arm_section_insert_instr(s, current, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = op2;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = false;
	stran->byte = false;
}

void subtilis_arm_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err)
{
	subtilis_arm_op2_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	bool subtract = false;

	if (offset < 0) {
		offset = -offset;
		subtract = true;
	}

	if (offset > 4095) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	op2.type = SUBTILIS_ARM_OP2_I32;
	op2.op.integer = offset;

	instr = subtilis_arm_section_insert_instr(s, current, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = op2;
	stran->pre_indexed = true;
	stran->write_back = false;
	stran->subtract = subtract;
	stran->byte = false;
}

void subtilis_arm_add_cmp_imm(subtilis_arm_section_t *s,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_arm_reg_t op1, int32_t op2,
			      subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_reg_t dest;
	uint32_t encoded = 0;

	if (!subtilis_arm_encode_imm(op2, &encoded)) {
		if (subtilis_arm_encode_imm(-op2, &encoded)) {
			itype = SUBTILIS_ARM_INSTR_CMN;
		} else {
			dest = 0;
			(void)subtilis_add_data_imm_ldr_datai(
			    s, itype, ccode, true, dest, op1, op2, err);
			return;
		}
	}

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->op1 = op1;
	datai->op2.type = SUBTILIS_ARM_OP2_I32;
	datai->op2.op.integer = encoded;
	datai->status = true;
}

void subtilis_arm_add_cmp(subtilis_arm_section_t *s,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_arm_reg_t op1, subtilis_arm_reg_t op2,
			  subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->op1 = op1;
	datai->op2.type = SUBTILIS_ARM_OP2_REG;
	datai->op2.op.reg = op2;
	datai->status = true;
}

void subtilis_arm_add_mtran(subtilis_arm_section_t *s,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_reg_t op0, size_t reg_list,
			    subtilis_arm_mtran_type_t type, bool write_back,
			    subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_mtran_instr_t *mtran;

	instr = subtilis_arm_section_add_instr(s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	mtran = &instr->operands.mtran;
	mtran->ccode = ccode;
	mtran->op0 = op0;
	mtran->reg_list = reg_list;
	mtran->type = type;
	mtran->write_back = write_back;
}

void subtilis_arm_add_byte(subtilis_arm_section_t *s, uint8_t byte,
			   subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_BYTE;
	op->op.byte = byte;
}

void subtilis_arm_add_two_bytes(subtilis_arm_section_t *s, uint16_t two_bytes,
				subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_TWO_BYTE;
	op->op.two_bytes = two_bytes;
}

void subtilis_arm_add_four_bytes(subtilis_arm_section_t *s, uint32_t four_bytes,
				 subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_FOUR_BYTE;
	op->op.four_bytes = four_bytes;
}

void subtilis_arm_add_double(subtilis_arm_section_t *s, double dbl,
			     subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_DOUBLE;
	op->op.dbl = dbl;
}

void subtilis_arm_add_doubler(subtilis_arm_section_t *s, double dbl,
			      subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_DOUBLER;
	op->op.dbl = dbl;
}

void subtilis_arm_add_string(subtilis_arm_section_t *s, const char *str,
			     subtilis_error_t *err)
{
	subtilis_arm_op_t *op;

	op = prv_append_op(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op->type = SUBTILIS_ARM_OP_STRING;
	op->op.str = malloc(strlen(str) + 1);
	if (!op->op.str) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(op->op.str, str);
}

void subtilis_arm_restore_stack(subtilis_arm_section_t *arm_s,
				size_t stack_space, subtilis_error_t *err)
{
	size_t i;
	size_t rs;
	subtilis_arm_data_instr_t *datai;

	for (i = 0; i < arm_s->ret_site_count; i++) {
		rs = arm_s->ret_sites[i];
		datai = &arm_s->op_pool->ops[rs].op.instr.operands.data;
		datai->op2.op.integer = stack_space;
	}
}
