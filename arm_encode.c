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

#include "arm_encode.h"
#include "arm_link.h"
#include "arm_walker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUBTILIS_ENCODER_BACKPATCH_GRAN 128

typedef enum {
	SUBTILIS_ARM_ENCODE_BP_BR,
	SUBTILIS_ARM_ENCODE_BP_LDR,
} subtilis_arm_encode_bp_type_t;

struct subtilis_arm_encode_bp_t_ {
	size_t label;
	size_t code_index;
	subtilis_arm_encode_bp_type_t type;
};

typedef struct subtilis_arm_encode_bp_t_ subtilis_arm_encode_bp_t;

struct subtilis_arm_encode_ud_t_ {
	size_t *label_offsets;
	size_t max_labels;
	uint32_t *code;
	size_t words_written;
	subtilis_arm_encode_bp_t *back_patches;
	size_t back_patch_count;
	size_t max_patch_count;
	subtilis_arm_link_t *link;
};

typedef struct subtilis_arm_encode_ud_t_ subtilis_arm_encode_ud_t;

static void prv_free_encode_ud(subtilis_arm_encode_ud_t *ud)
{
	subtilis_arm_link_delete(ud->link);
	free(ud->back_patches);
	free(ud->code);
	free(ud->label_offsets);
}

static void prv_init_encode_ud(subtilis_arm_encode_ud_t *ud,
			       subtilis_arm_prog_t *arm_p,
			       subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_section_t *arm_s;
	size_t max_label_offsets = 0;
	size_t constants = 0;

	memset(ud, 0, sizeof(*ud));

	for (i = 0; i < arm_p->num_sections; i++) {
		arm_s = arm_p->sections[i];
		if (arm_s->label_counter > max_label_offsets)
			max_label_offsets = arm_s->label_counter;
		constants += arm_s->constants.ui32_count;
	}

	ud->label_offsets = malloc(sizeof(size_t) * max_label_offsets);
	if (!ud->label_offsets) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	ud->code = malloc(sizeof(uint32_t) * (arm_p->op_pool->len + constants));
	if (!ud->code) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	ud->link = subtilis_arm_link_new(arm_p->num_sections, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return;

on_error:

	prv_free_encode_ud(ud);
}

static void prv_reset_encode_ud(subtilis_arm_encode_ud_t *ud,
				subtilis_arm_section_t *arm_s)
{
	ud->max_labels = arm_s->label_counter;
	ud->back_patch_count = 0;
}

static void prv_add_back_patch(subtilis_arm_encode_ud_t *ud, size_t label,
			       size_t code_index,
			       subtilis_arm_encode_bp_type_t type,
			       subtilis_error_t *err)
{
	size_t new_max;
	subtilis_arm_encode_bp_t *new_bp;

	if (ud->back_patch_count == ud->max_patch_count) {
		new_max = ud->max_patch_count + SUBTILIS_ENCODER_BACKPATCH_GRAN;
		new_bp = realloc(ud->back_patches,
				 new_max * sizeof(subtilis_arm_encode_bp_t));
		if (!new_bp) {
			subtilis_error_set_oom(err);
			return;
		}
		ud->max_patch_count = new_max;
		ud->back_patches = new_bp;
	}
	new_bp = &ud->back_patches[ud->back_patch_count++];
	new_bp->label = label;
	new_bp->code_index = code_index;
	new_bp->type = type;
}

static void prv_write_file(subtilis_arm_encode_ud_t *ud, const char *fname,
			   subtilis_error_t *err)
{
	FILE *fp;

	fp = fopen(fname, "w");
	if (!fp) {
		subtilis_error_set_file_open(err, fname);
		return;
	}

	if (fwrite(ud->code, sizeof(uint32_t), ud->words_written, fp) <
	    ud->words_written) {
		subtilis_error_set_file_write(err);
		goto fail;
	}

	if (fclose(fp) != 0)
		subtilis_error_set_file_close(err);

	return;

fail:

	(void)fclose(fp);
}

static uint32_t prv_convert_shift(subtilis_arm_shift_type_t type,
				  subtilis_error_t *err)
{
	switch (type) {
	case SUBTILIS_ARM_SHIFT_LSL:
		return 0;
	case SUBTILIS_ARM_SHIFT_RRX:
		subtilis_error_set_assertion_failed(err);
		return 0;
	default:
		return ((uint32_t)type) - 1;
	}
}

static void prv_encode_data_op2(subtilis_arm_op2_t *op2, uint32_t *word,
				subtilis_error_t *err)
{
	uint32_t shift;

	if (op2->type == SUBTILIS_ARM_OP2_REG) {
		if (op2->op.reg.num > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.reg.num;
	} else if (op2->type == SUBTILIS_ARM_OP2_I32) {
		if (op2->op.integer & 0xfffff000) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.integer;
		*word |= 1 << 25;
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg.num > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		if (op2->op.shift.shift_reg &&
		    op2->op.shift.shift.reg.num > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		*word |= op2->op.shift.reg.num;
		if (op2->op.shift.shift_reg) {
			*word |= 1 << 4;
			*word |= (op2->op.shift.shift.reg.num & 15) << 8;
		} else {
			*word |= (op2->op.shift.shift.integer & 31) << 7;
		}
		shift = prv_convert_shift(op2->op.shift.type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*word |= shift << 5;
	}
}

static void prv_encode_data_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_data_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	if ((instr->dest.num > 15) || (instr->op1.num > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= type << 21;
	if (instr->status)
		word |= 1 << 20;
	word |= instr->op1.num << 16;
	word |= instr->dest.num << 12;
	prv_encode_data_op2(&instr->op2, &word, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_mul_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_mul_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0x90;

	if ((instr->dest.num > 15) || (instr->rm.num > 15) ||
	    (instr->rs.num > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (type == SUBTILIS_ARM_INSTR_MLA) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	if (instr->status)
		word |= 1 << 20;
	word |= instr->dest.num << 16;
	word |= instr->rs.num << 8;
	word |= instr->rm.num;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_mov_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	prv_encode_data_instr(user_data, op, type, instr, err);
}

static void prv_encode_stran_op2(subtilis_arm_op2_t *op2, uint32_t *word,
				 subtilis_error_t *err)
{
	uint32_t shift;

	if (op2->type == SUBTILIS_ARM_OP2_REG) {
		if (op2->op.reg.num > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= 1 << 25;
		*word |= op2->op.reg.num;
	} else if (op2->type == SUBTILIS_ARM_OP2_I32) {
		if (op2->op.integer > 4095) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.integer;
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg.num > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		*word |= op2->op.shift.reg.num;
		*word |= 1 << 25;
		*word |= (op2->op.shift.shift.integer & 31) << 7;
		shift = prv_convert_shift(op2->op.shift.type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*word |= shift << 5;
	}
}

static void prv_encode_stran_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_arm_stran_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	if ((instr->base.num > 15) || (instr->dest.num > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= 1 << 26;
	if (instr->pre_indexed)
		word |= 1 << 24;

	if (instr->write_back)
		word |= 1 << 21;
	if (!instr->subtract)
		word |= 1 << 23;
	if (type == SUBTILIS_ARM_INSTR_LDR)
		word |= 1 << 20;
	word |= instr->base.num << 16;
	word |= instr->dest.num << 12;
	prv_encode_stran_op2(&instr->offset, &word, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_mtran_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_arm_mtran_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;
	subtilis_arm_mtran_type_t m_type;

	word |= instr->ccode << 28;
	word |= 0x4 << 25;

	m_type = instr->type;
	if (type == SUBTILIS_ARM_INSTR_STM) {
		switch (m_type) {
		case SUBTILIS_ARM_MTRAN_FA:
			m_type = SUBTILIS_ARM_MTRAN_IB;
			break;
		case SUBTILIS_ARM_MTRAN_FD:
			m_type = SUBTILIS_ARM_MTRAN_DB;
			break;
		case SUBTILIS_ARM_MTRAN_EA:
			m_type = SUBTILIS_ARM_MTRAN_IA;
			break;
		case SUBTILIS_ARM_MTRAN_ED:
			m_type = SUBTILIS_ARM_MTRAN_DA;
			break;
		default:
			break;
		}
	} else {
		switch (m_type) {
		case SUBTILIS_ARM_MTRAN_FA:
			m_type = SUBTILIS_ARM_MTRAN_DA;
			break;
		case SUBTILIS_ARM_MTRAN_FD:
			m_type = SUBTILIS_ARM_MTRAN_IA;
			break;
		case SUBTILIS_ARM_MTRAN_EA:
			m_type = SUBTILIS_ARM_MTRAN_DB;
			break;
		case SUBTILIS_ARM_MTRAN_ED:
			m_type = SUBTILIS_ARM_MTRAN_IB;
			break;
		default:
			break;
		}
	}

	switch (m_type) {
	case SUBTILIS_ARM_MTRAN_IA:
		word |= 1 << 23;
		break;
	case SUBTILIS_ARM_MTRAN_IB:
		word |= 1 << 23;
		word |= 1 << 24;
		break;
	case SUBTILIS_ARM_MTRAN_DA:
		break;
	case SUBTILIS_ARM_MTRAN_DB:
		word |= 1 << 24;
		break;
	default:
		break;
	}

	/* TODO: Do not support writing to status bits. */

	if (instr->write_back)
		word |= 1 << 21;

	if (type == SUBTILIS_ARM_INSTR_LDM)
		word |= 1 << 20;

	word |= instr->op0.num << 16;
	word |= instr->reg_list & 0xffff;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_br_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_br_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	word |= instr->ccode << 28;
	word |= 0x5 << 25;
	if (instr->link) {
		word |= 1 << 24;
		subtilis_arm_link_add(ud->link, ud->words_written, err);
		word |= instr->target.label;
	} else {
		prv_add_back_patch(ud, instr->target.label, ud->words_written,
				   SUBTILIS_ARM_ENCODE_BP_BR, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->code[ud->words_written++] = word;
}

static void prv_encode_swi_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_swi_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	word |= instr->ccode << 28;
	word |= 0xf << 24;
	word |= 0x0fffffff & instr->code;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_ldrc_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_stran_instr_t stran;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_add_back_patch(ud, instr->label, ud->words_written,
			   SUBTILIS_ARM_ENCODE_BP_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran.ccode = instr->ccode;
	stran.dest = instr->dest;
	stran.base.type = SUBTILIS_ARM_REG_FIXED;
	stran.base.num = 15;
	stran.offset.type = SUBTILIS_ARM_OP2_I32;
	stran.offset.op.integer = 0;
	stran.pre_indexed = true;
	stran.write_back = false;
	stran.subtract = false;
	prv_encode_stran_instr(user_data, op, SUBTILIS_ARM_INSTR_LDR, &stran,
			       err);
}

static void prv_encode_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	prv_encode_data_instr(user_data, op, type, instr, err);
}

static void prv_encode_label(void *user_data, subtilis_arm_op_t *op,
			     size_t label, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;

	if (label >= ud->max_labels) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ud->label_offsets[label] = ud->words_written;
}

static void prv_apply_back_patches(subtilis_arm_encode_ud_t *ud,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_bp_t *bp;
	size_t i;
	int32_t dist;

	for (i = 0; i < ud->back_patch_count; i++) {
		bp = &ud->back_patches[i];
		dist = (ud->label_offsets[bp->label] - bp->code_index) - 2;
		switch (bp->type) {
		case SUBTILIS_ARM_ENCODE_BP_BR:
			if ((dist < -(1 << 23)) || (dist > ((1 << 23) - 1))) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			dist &= 0xffffff;
			break;
		case SUBTILIS_ARM_ENCODE_BP_LDR:
			if (dist < 0)
				dist = -dist;
			dist *= 4;
			if (dist > 4096) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			break;
		}
		ud->code[bp->code_index] |= dist;
	}
}

static void prv_arm_encode(subtilis_arm_section_t *arm_s,
			   subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	subtlis_arm_walker_t walker;
	size_t i;

	walker.user_data = ud;
	walker.label_fn = prv_encode_label;
	walker.data_fn = prv_encode_data_instr;
	walker.mul_fn = prv_encode_mul_instr;
	walker.cmp_fn = prv_encode_cmp_instr;
	walker.mov_fn = prv_encode_mov_instr;
	walker.stran_fn = prv_encode_stran_instr;
	walker.mtran_fn = prv_encode_mtran_instr;
	walker.br_fn = prv_encode_br_instr;
	walker.swi_fn = prv_encode_swi_instr;
	walker.ldrc_fn = prv_encode_ldrc_instr;

	subtilis_arm_walk(arm_s, &walker, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < arm_s->constants.ui32_count; i++) {
		ud->label_offsets[arm_s->constants.ui32[i].label] =
		    ud->words_written;
		ud->code[ud->words_written++] =
		    arm_s->constants.ui32[i].integer;
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_apply_back_patches(ud, err);
}

static void prv_encode_prog(subtilis_arm_prog_t *arm_p,
			    subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s;
	size_t i;

	for (i = 0; i < arm_p->num_sections; i++) {
		arm_s = arm_p->sections[i];
		subtilis_arm_link_section(ud->link, i, ud->words_written);
		prv_reset_encode_ud(ud, arm_s);
		prv_arm_encode(arm_s, ud, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_link_link(ud->link, ud->code, ud->words_written, err);
}

void subtilis_arm_encode(subtilis_arm_prog_t *arm_p, const char *fname,
			 subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t ud;

	prv_init_encode_ud(&ud, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(arm_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_write_file(&ud, fname, err);

cleanup:

	prv_free_encode_ud(&ud);
}

uint32_t *subtilis_arm_encode_buf(subtilis_arm_prog_t *arm_p,
				  size_t *words_written, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t ud;
	uint32_t *retval = NULL;

	prv_init_encode_ud(&ud, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(arm_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	*words_written = ud.words_written;
	retval = ud.code;
	ud.code = NULL;

cleanup:

	prv_free_encode_ud(&ud);

	return retval;
}
