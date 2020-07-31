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

/*
 * A quick comment on constant encoding.  There are three main types of
 * constants that can be inserted into the code stream, outside of an
 * instruction; 32 bit integer, 64 bit floats and buffers.  Integer and
 * floating point constants are placed in constant pools which appear
 * throughout the code.  Buffer constants are placed at the end of the
 * program and an offset to their location is recorded in a constant pool.
 * If a function is small there's likely to be only one constant pool at
 * the end of the function.  However, if the function is large it's likely
 * to contain multiple constant pools.  The pools appear inline in the
 * function and are skipped with a branch statement during execution.
 *
 * There are two different types of constants.  Constants whose values
 * are known at compile time, and constants whose value are not known
 * until the program is linked (as they're offsets to other locations
 * in the program).  When encodin link time constants we write 0xffff
 * at compile time and then fill the values in a link time.
 */

#define SUBTILIS_ENCODER_BACKPATCH_GRAN 128
#define SUBTILIS_ENCODER_CODE_GRAN 1024

typedef enum {
	SUBTILIS_ARM_ENCODE_BP_LDR,
	SUBTILIS_ARM_ENCODE_BP_LDRF,
} subtilis_arm_encode_const_type_t;

struct subtilis_arm_encode_const_t_ {
	size_t label;
	size_t code_index;
	subtilis_arm_encode_const_type_t type;
};

typedef struct subtilis_arm_encode_const_t_ subtilis_arm_encode_const_t;

struct subtilis_arm_encode_bp_t_ {
	size_t label;
	size_t code_index;
};

typedef struct subtilis_arm_encode_bp_t_ subtilis_arm_encode_bp_t;

struct subtilis_arm_encode_ud_t_ {
	subtilis_arm_section_t *arm_s;
	bool reverse_fpa_consts;
	size_t *label_offsets;
	size_t max_label_offsets;
	size_t max_labels;
	uint32_t *code;
	size_t words_written;
	size_t max_words;
	subtilis_arm_encode_const_t *constants;
	size_t const_count;
	size_t max_const_count;
	size_t int_const_count;
	size_t real_const_count;
	subtilis_arm_encode_bp_t *back_patches;
	size_t back_patch_count;
	size_t max_patch_count;
	subtilis_arm_link_t *link;
	size_t ldrc_real;
	size_t ldrc_int;
};

typedef struct subtilis_arm_encode_ud_t_ subtilis_arm_encode_ud_t;

static void prv_free_encode_ud(subtilis_arm_encode_ud_t *ud)
{
	subtilis_arm_link_delete(ud->link);
	free(ud->back_patches);
	free(ud->constants);
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
	size_t real_constants = 0;

	memset(ud, 0, sizeof(*ud));

	for (i = 0; i < arm_p->num_sections; i++) {
		arm_s = arm_p->sections[i];
		if (arm_s->label_counter > max_label_offsets)
			max_label_offsets = arm_s->label_counter;
		constants += arm_s->constants.ui32_count;
		real_constants += arm_s->constants.real_count;
	}

	/*
	 * This may not be enough memory if we need multiple constant pools.
	 */

	ud->label_offsets = malloc(sizeof(size_t) * max_label_offsets);
	if (!ud->label_offsets) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	ud->max_label_offsets = max_label_offsets;

	/*
	 * This may not be enough memory if we need multiple constant pools.
	 */

	ud->max_words = arm_p->op_pool->len + constants + (real_constants * 2);
	ud->code = malloc(sizeof(uint32_t) * ud->max_words);
	if (!ud->code) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	ud->link = subtilis_arm_link_new(arm_p->num_sections, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	ud->reverse_fpa_consts = arm_p->reverse_fpa_consts;

	return;

on_error:

	prv_free_encode_ud(ud);
}

static void prv_reset_pool_state(subtilis_arm_encode_ud_t *ud)
{
	ud->const_count = 0;
	ud->int_const_count = 0;
	ud->real_const_count = 0;
	ud->ldrc_real = SIZE_MAX;
	ud->ldrc_int = SIZE_MAX;
}

static void prv_ensure_code_size(subtilis_arm_encode_ud_t *ud, size_t words,
				 subtilis_error_t *err)
{
	size_t new_max_words;
	uint32_t *new_code;

	if (ud->words_written + words < ud->max_words)
		return;

	if (words < SUBTILIS_ENCODER_CODE_GRAN)
		new_max_words = SUBTILIS_ENCODER_CODE_GRAN + ud->max_words;
	else
		new_max_words = words + ud->max_words;
	new_code = realloc(ud->code, new_max_words * sizeof(*new_code));
	if (!new_code) {
		subtilis_error_set_oom(err);
		return;
	}

	ud->max_words = new_max_words;
	ud->code = new_code;
}

static void prv_ensure_code(subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	prv_ensure_code_size(ud, 1, err);
}

static void prv_encode_label(void *user_data, subtilis_arm_op_t *op,
			     size_t label, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	size_t new_max_label_offsets;
	size_t *new_label_offsets;

	if (label > ud->max_labels) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if ((label == ud->max_labels) && (label >= ud->max_label_offsets)) {
		new_max_label_offsets = label + SUBTILIS_ENCODER_BACKPATCH_GRAN;
		new_label_offsets = realloc(
		    ud->label_offsets, sizeof(size_t) * new_max_label_offsets);
		if (!new_label_offsets) {
			subtilis_error_set_oom(err);
			return;
		}
		ud->label_offsets = new_label_offsets;
		ud->max_label_offsets = new_max_label_offsets;
	}

	ud->label_offsets[label] = ud->words_written;
}

static void prv_apply_constants(subtilis_arm_encode_ud_t *ud,
				subtilis_error_t *err)
{
	subtilis_arm_encode_const_t *cnst;
	size_t i;
	int32_t dist;

	for (i = 0; i < ud->const_count; i++) {
		cnst = &ud->constants[i];
		dist = (ud->label_offsets[cnst->label] - cnst->code_index) - 2;
		switch (cnst->type) {
		case SUBTILIS_ARM_ENCODE_BP_LDR:
			if (dist < 0)
				dist = -dist;
			dist *= 4;
			if (dist > 4096) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			break;
		case SUBTILIS_ARM_ENCODE_BP_LDRF:
			if (dist < 0)
				dist = -dist;
			if (dist * 4 > 1024) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			break;
		}
		ud->code[cnst->code_index] |= dist;
	}
}

static void prv_flush_constants(subtilis_arm_encode_ud_t *ud,
				subtilis_error_t *err)
{
	size_t i;
	size_t j;
	subtilis_arm_encode_const_t *cnst;
	uint32_t *real_ptr;
	int32_t constant_index;
	subtilis_arm_section_t *arm_s = ud->arm_s;

	for (i = 0; i < ud->const_count; i++) {
		cnst = &ud->constants[i];
		ud->label_offsets[cnst->label] = ud->words_written;
		switch (cnst->type) {
		case SUBTILIS_ARM_ENCODE_BP_LDR:
			for (j = 0; j < arm_s->constants.ui32_count; j++)
				if (arm_s->constants.ui32[j].label ==
				    cnst->label)
					break;
			if (j == arm_s->constants.ui32_count) {
				subtilis_error_set_assertion_failed(err);
				return;
			}

			prv_ensure_code(ud, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (arm_s->constants.ui32[j].link_time) {
				constant_index =
				    arm_s->constants.ui32[j].integer;
				subtilis_arm_link_constant_add(
				    ud->link, cnst->code_index,
				    ud->words_written, constant_index, err);
				ud->code[ud->words_written] = 0xffff;
			} else {
				ud->code[ud->words_written] =
				    arm_s->constants.ui32[j].integer;
			}
			ud->words_written++;
			break;
		case SUBTILIS_ARM_ENCODE_BP_LDRF:
			for (j = 0; j < arm_s->constants.real_count; j++)
				if (arm_s->constants.real[j].label ==
				    cnst->label)
					break;
			if (j == arm_s->constants.real_count) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			real_ptr =
			    (uint32_t *)(void *)&arm_s->constants.real[j].real;
			prv_ensure_code(ud, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (ud->reverse_fpa_consts) {
				ud->code[ud->words_written++] = real_ptr[1];
				prv_ensure_code(ud, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				ud->code[ud->words_written++] = real_ptr[0];
			} else {
				ud->code[ud->words_written++] = real_ptr[0];
				prv_ensure_code(ud, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				ud->code[ud->words_written++] = real_ptr[1];
			}
			break;
		}
	}

	prv_apply_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_reset_pool_state(ud);
}

static void prv_add_back_patch(subtilis_arm_encode_ud_t *ud, size_t label,
			       size_t code_index, subtilis_error_t *err)
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
}

static void prv_check_pool_adj(subtilis_arm_encode_ud_t *ud, size_t adj,
			       subtilis_error_t *err)
{
	uint32_t word = 0;
	bool pool_needed = false;
	size_t pool_end = ud->int_const_count + (ud->real_const_count * 2) +
			  ud->words_written + adj;

	/* We need to leave one byte for the branch statement. */

	pool_needed =
	    (ud->ldrc_real != SIZE_MAX) && ((pool_end - ud->ldrc_real) >= 255);
	if (!pool_needed)
		pool_needed = (ud->ldrc_int != SIZE_MAX) &&
			      ((pool_end - ud->ldrc_int) >= 1023);

	if (!pool_needed)
		return;

	/* We add a B instruction to jump over the pool */

	word |= SUBTILIS_ARM_CCODE_AL << 28;
	word |= 0x5 << 25;
	prv_add_back_patch(ud, ud->max_labels, ud->words_written, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->code[ud->words_written++] = word;

	/* Now we write the constants */

	prv_flush_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* Finally, we add the label for the branch */

	prv_encode_label(ud, NULL, ud->max_labels, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->max_labels++;
}

static void prv_check_pool(subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	prv_check_pool_adj(ud, 0, err);
}

static void prv_reset_encode_ud(subtilis_arm_encode_ud_t *ud,
				subtilis_arm_section_t *arm_s)
{
	ud->arm_s = arm_s;
	ud->max_labels = arm_s->label_counter;
	ud->back_patch_count = 0;
	prv_reset_pool_state(ud);
}

static void prv_add_const(subtilis_arm_encode_ud_t *ud, size_t label,
			  size_t code_index,
			  subtilis_arm_encode_const_type_t type,
			  subtilis_error_t *err)
{
	size_t new_max;
	subtilis_arm_encode_const_t *new_const;

	if (ud->const_count == ud->max_const_count) {
		new_max = ud->max_const_count + SUBTILIS_ENCODER_BACKPATCH_GRAN;
		new_const =
		    realloc(ud->constants,
			    new_max * sizeof(subtilis_arm_encode_const_t));
		if (!new_const) {
			subtilis_error_set_oom(err);
			return;
		}
		ud->max_const_count = new_max;
		ud->constants = new_const;
	}
	if (type == SUBTILIS_ARM_ENCODE_BP_LDR)
		ud->int_const_count++;
	else
		ud->real_const_count++;
	new_const = &ud->constants[ud->const_count++];
	new_const->label = label;
	new_const->code_index = code_index;
	new_const->type = type;
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
		if (op2->op.reg > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.reg;
	} else if (op2->type == SUBTILIS_ARM_OP2_I32) {
		if (op2->op.integer & 0xfffff000) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.integer;
		*word |= 1 << 25;
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		if (op2->op.shift.shift_reg && op2->op.shift.shift.reg > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		*word |= op2->op.shift.reg;
		if (op2->op.shift.shift_reg) {
			*word |= 1 << 4;
			*word |= (op2->op.shift.shift.reg & 15) << 8;
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

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((instr->dest > 15) || (instr->op1 > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= type << 21;
	if (instr->status)
		word |= 1 << 20;
	word |= instr->op1 << 16;
	word |= instr->dest << 12;
	prv_encode_data_op2(&instr->op2, &word, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_ensure_code(ud, err);
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

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((instr->dest > 15) || (instr->rm > 15) || (instr->rs > 15)) {
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
	word |= instr->dest << 16;
	word |= instr->rs << 8;
	word |= instr->rm;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_mov_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_encode_data_instr(user_data, op, type, instr, err);
}

static void prv_encode_stran_op2(subtilis_arm_op2_t *op2, uint32_t *word,
				 subtilis_error_t *err)
{
	uint32_t shift;

	if (op2->type == SUBTILIS_ARM_OP2_REG) {
		if (op2->op.reg > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= 1 << 25;
		*word |= op2->op.reg;
	} else if (op2->type == SUBTILIS_ARM_OP2_I32) {
		if (op2->op.integer > 4095) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		*word |= op2->op.integer;
	} else if (op2->type == SUBTILIS_ARM_OP2_SHIFTED) {
		if (op2->op.shift.reg > 15) {
			subtilis_error_set_assertion_failed(err);
			return;
		}

		*word |= op2->op.shift.reg;
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

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((instr->base > 15) || (instr->dest > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= 1 << 26;
	if (instr->pre_indexed)
		word |= 1 << 24;

	if (instr->byte)
		word |= 1 << 22;
	if (instr->write_back)
		word |= 1 << 21;
	if (!instr->subtract)
		word |= 1 << 23;
	if (type == SUBTILIS_ARM_INSTR_LDR)
		word |= 1 << 20;
	word |= instr->base << 16;
	word |= instr->dest << 12;
	prv_encode_stran_op2(&instr->offset, &word, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_ensure_code(ud, err);
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

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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

	word |= instr->op0 << 16;
	word |= instr->reg_list & 0xffff;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->code[ud->words_written++] = word;
}

static void prv_encode_br_instr(void *user_data, subtilis_arm_op_t *op,
				subtilis_arm_instr_type_t type,
				subtilis_arm_br_instr_t *instr,
				subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0x5 << 25;
	if (instr->link) {
		word |= 1 << 24;
		subtilis_arm_link_add(ud->link, ud->words_written, err);
		word |= instr->target.label;
	} else {
		prv_add_back_patch(ud, instr->target.label, ud->words_written,
				   err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_ensure_code(ud, err);
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

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0xf << 24;
	word |= 0x0fffffff & instr->code;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->code[ud->words_written++] = word;
}

static void prv_encode_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_ldrc_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_stran_instr_t stran;
	subtilis_arm_encode_ud_t *ud = user_data;

	/*
	 * So this is a little tricky.  If we're just loading a normal
	 * compile time constant we want to ensure that a constant pool
	 * does not get flushed after we write the constant but before
	 * we write the LDR instruction.  Loading constants with lca
	 * involves two instructions, an LDR and an ADD, both of which
	 * use R15 as a source.  It's important that they are not split
	 * up by a constant pool.  So here we use a prv_check_pool_adj
	 * to force a pool flush if there's not enough space for the
	 * constant and all the instructions neeeded to load it.
	 */

	/*
	 * adj is the number of additional words that can be written
	 * without causing a pool flush. 1 word for the constant,
	 * one word for the ldr and optionally one word for the add
	 * if we're doing an lca.
	 */

	size_t adj = 2;

	if (instr->link_time)
		adj += 1;

	prv_check_pool_adj(ud, adj, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_const(ud, instr->label, ud->words_written,
		      SUBTILIS_ARM_ENCODE_BP_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (ud->ldrc_int == SIZE_MAX)
		ud->ldrc_int = ud->words_written;

	stran.ccode = instr->ccode;
	stran.dest = instr->dest;
	stran.base = 15;
	stran.offset.type = SUBTILIS_ARM_OP2_I32;
	stran.offset.op.integer = 0;
	stran.pre_indexed = true;
	stran.write_back = false;
	stran.subtract = false;
	stran.byte = false;
	prv_encode_stran_instr(user_data, op, SUBTILIS_ARM_INSTR_LDR, &stran,
			       err);
}

static void prv_encode_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_encode_data_instr(user_data, op, type, instr, err);
}

static void prv_encode_fpa_rounding(subtilis_fpa_rounding_t rounding,
				    uint32_t *word)
{
	switch (rounding) {
	case SUBTILIS_FPA_ROUNDING_NEAREST:
		break;
	case SUBTILIS_FPA_ROUNDING_PLUS_INFINITY:
		*word |= 1 << 5;
		break;
	case SUBTILIS_FPA_ROUNDING_MINUS_INFINITY:
		*word |= 1 << 6;
		break;
	case SUBTILIS_FPA_ROUNDING_ZERO:
		*word |= 3 << 5;
		break;
	}
}

static void prv_encode_fpa_data_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_data_instr_t *instr,
				      bool dyadic, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;
	uint32_t op_code = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0xE << 24;

	switch (type) {
	case SUBTILIS_FPA_INSTR_ADF:
	case SUBTILIS_FPA_INSTR_MVF:
		op_code = 0;
		break;
	case SUBTILIS_FPA_INSTR_MUF:
	case SUBTILIS_FPA_INSTR_MNF:
		op_code = 1;
		break;
	case SUBTILIS_FPA_INSTR_SUF:
	case SUBTILIS_FPA_INSTR_ABS:
		op_code = 2;
		break;
	case SUBTILIS_FPA_INSTR_RSF:
	case SUBTILIS_FPA_INSTR_RND:
		op_code = 3;
		break;
	case SUBTILIS_FPA_INSTR_DVF:
	case SUBTILIS_FPA_INSTR_SQT:
		op_code = 4;
		break;
	case SUBTILIS_FPA_INSTR_RDF:
	case SUBTILIS_FPA_INSTR_LOG:
		op_code = 5;
		break;
	case SUBTILIS_FPA_INSTR_POW:
	case SUBTILIS_FPA_INSTR_LGN:
		op_code = 6;
		break;
	case SUBTILIS_FPA_INSTR_RPW:
	case SUBTILIS_FPA_INSTR_EXP:
		op_code = 7;
		break;
	case SUBTILIS_FPA_INSTR_RMF:
	case SUBTILIS_FPA_INSTR_SIN:
		op_code = 8;
		break;
	case SUBTILIS_FPA_INSTR_FML:
	case SUBTILIS_FPA_INSTR_COS:
		op_code = 9;
		break;
	case SUBTILIS_FPA_INSTR_FDV:
	case SUBTILIS_FPA_INSTR_TAN:
		op_code = 10;
		break;
	case SUBTILIS_FPA_INSTR_FRD:
	case SUBTILIS_FPA_INSTR_ASN:
		op_code = 11;
		break;
	case SUBTILIS_FPA_INSTR_POL:
	case SUBTILIS_FPA_INSTR_ACS:
		op_code = 12;
		break;
	case SUBTILIS_FPA_INSTR_ATN:
		op_code = 13;
		break;
	case SUBTILIS_FPA_INSTR_URD:
		op_code = 14;
		break;
	case SUBTILIS_FPA_INSTR_NRM:
		op_code = 15;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= op_code << 20;

	if (instr->size == 8)
		word |= 1 << 7;
	else if (instr->size == 12)
		word |= 1 << 19;

	if (dyadic)
		word |= instr->op1 << 16;
	else
		word |= 1 << 15;

	word |= instr->dest << 12;
	word |= 1 << 8;
	prv_encode_fpa_rounding(instr->rounding, &word);
	if (instr->immediate) {
		word |= 1 << 3;
		word |= instr->op2.imm;
	} else {
		word |= instr->op2.reg;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ud->code[ud->words_written++] = word;
}

static void prv_encode_fpa_data_dyadic_instr(void *user_data,
					     subtilis_arm_op_t *op,
					     subtilis_arm_instr_type_t type,
					     subtilis_fpa_data_instr_t *instr,
					     subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_encode_fpa_data_instr(user_data, op, type, instr, true, err);
}

static void prv_encode_fpa_data_monadic_instr(void *user_data,
					      subtilis_arm_op_t *op,
					      subtilis_arm_instr_type_t type,
					      subtilis_fpa_data_instr_t *instr,
					      subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_encode_fpa_data_instr(user_data, op, type, instr, false, err);
}

static void prv_encode_fpa_stran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_fpa_stran_instr_t *instr,
				       subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= (0xC | (instr->pre_indexed ? 1 : 0)) << 24;
	if (!instr->subtract)
		word |= 1 << 23;

	if (instr->size == 8)
		word |= 1 << 15;
	else if (instr->size == 12)
		word |= 1 << 22;

	if (instr->write_back)
		word |= 1 << 21;

	if (type == SUBTILIS_FPA_INSTR_LDF)
		word |= 1 << 20;

	word |= instr->base << 16;
	word |= instr->dest << 12;
	word |= 1 << 8;
	word |= instr->offset;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_fpa_tran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_tran_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0xE << 24;

	if (type == SUBTILIS_FPA_INSTR_FLT) {
		if (instr->size == 8)
			word |= 1 << 7;
		else if (instr->size == 12)
			word |= 1 << 19;
		word |= instr->dest << 16;
		word |= instr->op2.reg << 12;
	} else if (type == SUBTILIS_FPA_INSTR_FIX) {
		if (instr->immediate) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		word |= instr->dest << 12;
		word |= instr->op2.reg;
		word |= 1 << 20;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= 1 << 8;
	prv_encode_fpa_rounding(instr->rounding, &word);
	word |= 1 << 4;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_fpa_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_fpa_cmp_instr_t *instr,
				     subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;
	uint32_t op_code = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0xE << 24;

	switch (type) {
	case SUBTILIS_FPA_INSTR_CMF:
		op_code = 4;
		break;
	case SUBTILIS_FPA_INSTR_CNF:
		op_code = 5;
		break;
	case SUBTILIS_FPA_INSTR_CMFE:
		op_code = 6;
		break;
	case SUBTILIS_FPA_INSTR_CNFE:
		op_code = 7;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	word |= op_code << 21;
	word |= 1 << 20;
	word |= instr->dest << 16;
	word |= 15 << 12;
	word |= 1 << 8;
	word |= 1 << 4;

	if (instr->immediate) {
		word |= 1 << 3;
		word |= instr->op2.imm;
	} else {
		word |= instr->op2.reg;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
}

static void prv_encode_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_ldrc_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_fpa_stran_instr_t stran;
	subtilis_arm_encode_ud_t *ud = user_data;

	/*
	 * adj is the number of additional words that can be written
	 * without causing a pool flush.  2 for the double and one
	 * for the ldr instruction.
	 */

	size_t adj = 3;

	prv_check_pool_adj(ud, adj, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (ud->ldrc_real == SIZE_MAX)
		ud->ldrc_real = ud->words_written;

	prv_add_const(ud, instr->label, ud->words_written,
		      SUBTILIS_ARM_ENCODE_BP_LDRF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran.ccode = instr->ccode;
	stran.size = instr->size;
	stran.dest = instr->dest;
	stran.base = 15;
	stran.offset = 0;
	stran.pre_indexed = true;
	stran.write_back = false;
	stran.subtract = false;
	prv_encode_fpa_stran_instr(user_data, op, SUBTILIS_FPA_INSTR_LDF,
				   &stran, err);
}

static void prv_encode_fpa_cptran_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_fpa_cptran_instr_t *instr,
					subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= 0xE << 24;
	if (type == SUBTILIS_FPA_INSTR_WFS)
		word |= 2 << 20;
	else
		word |= 3 << 20;
	word |= instr->dest << 12;
	word |= (1 << 8 | 1 << 4);

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->code[ud->words_written++] = word;
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
		if ((dist < -(1 << 23)) || (dist > ((1 << 23) - 1))) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		dist &= 0xffffff;
		ud->code[bp->code_index] |= dist;
	}
}

static void prv_arm_encode(subtilis_arm_section_t *arm_s,
			   subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	subtlis_arm_walker_t walker;

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
	walker.fpa_data_monadic_fn = prv_encode_fpa_data_monadic_instr;
	walker.fpa_data_dyadic_fn = prv_encode_fpa_data_dyadic_instr;
	walker.fpa_stran_fn = prv_encode_fpa_stran_instr;
	walker.fpa_tran_fn = prv_encode_fpa_tran_instr;
	walker.fpa_cmp_fn = prv_encode_fpa_cmp_instr;
	walker.fpa_ldrc_fn = prv_encode_fpa_ldrc_instr;
	walker.fpa_cptran_fn = prv_encode_fpa_cptran_instr;

	subtilis_arm_walk(arm_s, &walker, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_flush_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_apply_back_patches(ud, err);
}

static void prv_copy_constant_to_buf(subtilis_arm_prog_t *arm_p,
				     subtilis_arm_encode_ud_t *ud,
				     subtilis_constant_data_t *data)
{
	size_t i;
	uint32_t *word;
	size_t ptr;
	size_t null_bytes_needed;
	uint8_t *bptr;

	if (data->dbl && arm_p->reverse_fpa_consts) {
		ptr = ud->words_written;
		for (i = 0; i < data->data_size / sizeof(double); i++) {
			word = (uint32_t *)&data->data[i * sizeof(double)];
			ud->code[ptr++] = word[1];
			ud->code[ptr++] = word[0];
		}
	} else {
		memcpy(&ud->code[ud->words_written], data->data,
		       data->data_size);
		if (data->data_size & 3) {
			null_bytes_needed = 4 - (data->data_size & 3);
			bptr = ((uint8_t *)(&ud->code[ud->words_written])) +
			       data->data_size;
			for (i = 0; i < null_bytes_needed; i++)
				*bptr++ = 0;
		}
	}
}

static void prv_encode_prog(subtilis_arm_prog_t *arm_p,
			    subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	subtilis_arm_section_t *arm_s;
	size_t i;
	size_t size_in_bytes;
	size_t size_in_words;
	size_t *const_locations = NULL;

	for (i = 0; i < arm_p->num_sections; i++) {
		arm_s = arm_p->sections[i];
		subtilis_arm_link_section(ud->link, i, ud->words_written);
		prv_reset_encode_ud(ud, arm_s);
		prv_arm_encode(arm_s, ud, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/* Let's write the constants arrays and strings */

	if (arm_p->constant_pool->size > 0) {
		const_locations = malloc(arm_p->constant_pool->size *
					 sizeof(*const_locations));
		if (!const_locations) {
			subtilis_error_set_oom(err);
			return;
		}
		for (i = 0; i < arm_p->constant_pool->size; i++) {
			size_in_bytes = arm_p->constant_pool->data[i].data_size;
			size_in_words = size_in_bytes >> 2;
			if (size_in_bytes > size_in_words << 2)
				size_in_words++;
			prv_ensure_code_size(ud, size_in_words, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			const_locations[i] = ud->words_written;
			prv_copy_constant_to_buf(
			    arm_p, ud, &arm_p->constant_pool->data[i]);
			ud->words_written += size_in_words;
		}
	}

	subtilis_arm_link_link(ud->link, ud->code, ud->words_written,
			       const_locations, arm_p->constant_pool->size,
			       err);

cleanup:

	free(const_locations);
}

void subtilis_arm_encode(subtilis_arm_prog_t *arm_p, const char *fname,
			 subtilis_arm_encode_plat_t plat, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t ud;

	prv_init_encode_ud(&ud, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(arm_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (plat) {
		plat(ud.code, ud.words_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

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
