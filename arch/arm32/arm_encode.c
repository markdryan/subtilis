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
#define SUBTILIS_ENCODER_CODE_GRAN 4096

typedef enum {
	SUBTILIS_ARM_ENCODE_BP_LDR,
	SUBTILIS_ARM_ENCODE_BP_LDRP,
	SUBTILIS_ARM_ENCODE_BP_LDRF,
} subtilis_arm_encode_const_type_t;

struct subtilis_arm_encode_const_t_ {
	size_t label;
	size_t section_label;
	size_t code_index;
	subtilis_arm_encode_const_type_t type;
};

typedef struct subtilis_arm_encode_const_t_ subtilis_arm_encode_const_t;

typedef enum {
	SUBTILIS_ARM_ENCODE_BP_TYPE_BRANCH,
	SUBTILIS_ARM_ENCODE_BP_TYPE_ADR,
} subtilis_arm_encode_bp_type_t;

struct subtilis_arm_encode_bp_t_ {
	subtilis_arm_encode_bp_type_t type;
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
	uint8_t *code;
	size_t bytes_written;
	size_t max_bytes;
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

static void prv_encode_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_data_instr_t *instr,
				 subtilis_error_t *err);

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

	ud->max_bytes = ((arm_p->op_pool->len + constants) * sizeof(uint32_t)) +
			(real_constants * 8);
	ud->code = malloc(ud->max_bytes);
	if (!ud->code) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	ud->link = subtilis_arm_link_new(arm_p->num_sections, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	ud->reverse_fpa_consts =
	    arm_p->fp_if && arm_p->fp_if->reverse_fpa_consts;

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

static void prv_ensure_code_size(subtilis_arm_encode_ud_t *ud, size_t bytes,
				 subtilis_error_t *err)
{
	size_t new_max_bytes;
	uint8_t *new_code;

	if (ud->bytes_written + bytes < ud->max_bytes)
		return;

	if (bytes < SUBTILIS_ENCODER_CODE_GRAN)
		new_max_bytes = SUBTILIS_ENCODER_CODE_GRAN + ud->max_bytes;
	else
		new_max_bytes = bytes + ud->max_bytes;
	new_code = realloc(ud->code, new_max_bytes);
	if (!new_code) {
		subtilis_error_set_oom(err);
		return;
	}

	ud->max_bytes = new_max_bytes;
	ud->code = new_code;
}

static void prv_ensure_code(subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	prv_ensure_code_size(ud, 4, err);
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

	ud->label_offsets[label] = ud->bytes_written;
}

static uint32_t *prv_get_word_ptr(subtilis_arm_encode_ud_t *ud, size_t index,
				  subtilis_error_t *err)
{
	if (index & 3) {
		subtilis_error_set_ass_bad_alignment(err);
		return NULL;
	}

	return (uint32_t *)&ud->code[index];
}

static void prv_add_word(subtilis_arm_encode_ud_t *ud, uint32_t word,
			 subtilis_error_t *err)
{
	uint32_t *code_ptr;

	if (ud->bytes_written & 3) {
		subtilis_error_set_ass_bad_alignment(err);
		return;
	}

	code_ptr = (uint32_t *)&ud->code[ud->bytes_written];
	*code_ptr = word;
	ud->bytes_written += 4;
}

static void prv_apply_constants(subtilis_arm_encode_ud_t *ud,
				subtilis_error_t *err)
{
	subtilis_arm_encode_const_t *cnst;
	size_t i;
	int32_t dist;
	uint32_t *code_ptr;

	for (i = 0; i < ud->const_count; i++) {
		cnst = &ud->constants[i];
		dist = (ud->label_offsets[cnst->label] - cnst->code_index) - 8;
		switch (cnst->type) {
		case SUBTILIS_ARM_ENCODE_BP_LDR:
		case SUBTILIS_ARM_ENCODE_BP_LDRP:
			if (dist < 0)
				dist = -dist;
			if (dist > 4096) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			break;
		case SUBTILIS_ARM_ENCODE_BP_LDRF:
			if (dist < 0)
				dist = -dist;
			if (dist > 1024) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			dist /= 4;
			break;
		}
		code_ptr = prv_get_word_ptr(ud, cnst->code_index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*code_ptr |= dist;
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
		ud->label_offsets[cnst->label] = ud->bytes_written;
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
				    ud->bytes_written, constant_index, err);
				prv_add_word(ud, 0xffff, err);
			} else {
				prv_add_word(
				    ud, arm_s->constants.ui32[j].integer, err);
			}
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			break;
		case SUBTILIS_ARM_ENCODE_BP_LDRP:
			prv_ensure_code(ud, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_arm_link_extref_add(ud->link, cnst->code_index,
						     ud->bytes_written,
						     cnst->section_label, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_add_word(ud, 0, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
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
				prv_add_word(ud, real_ptr[1], err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				prv_ensure_code(ud, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				prv_add_word(ud, real_ptr[0], err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
			} else {
				prv_add_word(ud, real_ptr[0], err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				prv_ensure_code(ud, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				prv_add_word(ud, real_ptr[1], err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
			}
			break;
		}
	}

	prv_apply_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_reset_pool_state(ud);
}

static void prv_add_back_patch(subtilis_arm_encode_ud_t *ud,
			       subtilis_arm_encode_bp_type_t type, size_t label,
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
	new_bp->type = type;
	new_bp->label = label;
	new_bp->code_index = code_index;
}

static void prv_check_pool_adj(subtilis_arm_encode_ud_t *ud, size_t adj,
			       subtilis_error_t *err)
{
	uint32_t word = 0;
	bool pool_needed = false;
	size_t pool_end = (ud->int_const_count * 4) +
			  (ud->real_const_count * 8) + ud->bytes_written + adj;

	/* We need to leave one word for the branch statement. */

	pool_needed =
	    (ud->ldrc_real != SIZE_MAX) && ((pool_end - ud->ldrc_real) >= 1020);
	if (!pool_needed)
		pool_needed = (ud->ldrc_int != SIZE_MAX) &&
			      ((pool_end - ud->ldrc_int) >= 4092);

	if (!pool_needed)
		return;

	/* We add a B instruction to jump over the pool */

	word |= SUBTILIS_ARM_CCODE_AL << 28;
	word |= 0x5 << 25;
	prv_add_back_patch(ud, SUBTILIS_ARM_ENCODE_BP_TYPE_BRANCH,
			   ud->max_labels, ud->bytes_written, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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
	if (type != SUBTILIS_ARM_ENCODE_BP_LDRF)
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

	if (fwrite(ud->code, 1, ud->bytes_written, fp) < ud->bytes_written) {
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

	prv_add_word(ud, word, err);
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
		word |= 1 << 21;
		word |= instr->rn << 12;
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

	prv_add_word(ud, word, err);
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
	prv_add_word(ud, word, err);
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

	if (instr->write_back)
		word |= 1 << 21;

	if (instr->status)
		word |= 1 << 22;

	if (type == SUBTILIS_ARM_INSTR_LDM)
		word |= 1 << 20;

	word |= instr->op0 << 16;
	word |= instr->reg_list & 0xffff;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
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
	if (instr->indirect) {
		/*
		 * We're going to do a mov pc, reg.  There's no need to do
		 * any linking stuff as the address is computed at runtime.
		 */

		word |= SUBTILIS_ARM_INSTR_MOV << 21;
		word |= 15 << 12;
		word |= instr->target.reg & 0xf;
	} else {
		word |= 0x5 << 25;
		if (instr->link)
			word |= 1 << 24;
		if (instr->link && !instr->local) {
			subtilis_arm_link_add(ud->link, ud->bytes_written, err);
			word |= instr->target.label;
		} else {
			prv_add_back_patch(
			    ud, SUBTILIS_ARM_ENCODE_BP_TYPE_BRANCH,
			    instr->target.label, ud->bytes_written, err);
		}
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
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
	prv_add_word(ud, word, err);
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
	 * adj is the number of additional bytes that can be written
	 * without causing a pool flush. 4 bytes for the constant,
	 * 4 bytes for the ldr and optionally four bytes for the add
	 * if we're doing an lca.
	 */

	size_t adj = 8;

	if (instr->link_time)
		adj += 4;

	prv_check_pool_adj(ud, adj, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_const(ud, instr->label, ud->bytes_written,
		      SUBTILIS_ARM_ENCODE_BP_LDR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (ud->ldrc_int == SIZE_MAX)
		ud->ldrc_int = ud->bytes_written;

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

static void prv_encode_adr_instr(void *user_data, subtilis_arm_op_t *op,
				 subtilis_arm_instr_type_t type,
				 subtilis_arm_adr_instr_t *instr,
				 subtilis_error_t *err)
{
	subtilis_arm_data_instr_t data;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_add_back_patch(ud, SUBTILIS_ARM_ENCODE_BP_TYPE_ADR, instr->label,
			   ud->bytes_written, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	data.ccode = instr->ccode;
	data.status = false;
	data.dest = instr->dest;
	data.op1 = 15;
	data.op2.type = SUBTILIS_ARM_OP2_I32;
	data.op2.op.integer = 0;

	prv_encode_data_instr(user_data, op, SUBTILIS_ARM_INSTR_ADD, &data,
			      err);
}

static void prv_encode_ldrp_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_ldrp_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	subtilis_arm_stran_instr_t stran;

	/*
	 * We need to make sure we have enough room for one PC relative load and
	 * an add and the constant.
	 */

	prv_check_pool_adj(ud, 12, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_const(ud, instr->constant_label, ud->bytes_written,
		      SUBTILIS_ARM_ENCODE_BP_LDRP, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	ud->constants[ud->const_count - 1].section_label = instr->section_label;

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

static void prv_encode_cmov_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_cmov_instr_t *instr,
				  subtilis_error_t *err)
{
	subtilis_arm_data_instr_t data;

	if (!instr->fused) {
		memset(&data, 0, sizeof(data));

		data.ccode = SUBTILIS_ARM_CCODE_AL;
		data.status = true;
		data.op1 = instr->op1;
		data.op2.type = SUBTILIS_ARM_OP2_I32;
		data.op2.op.integer = 0;

		prv_encode_cmp_instr(user_data, NULL, SUBTILIS_ARM_INSTR_CMP,
				     &data, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	memset(&data, 0, sizeof(data));

	data.ccode = instr->fused ? instr->true_cond : SUBTILIS_ARM_CCODE_EQ;
	data.status = false;
	data.dest = instr->dest;
	data.op2.type = SUBTILIS_ARM_OP2_REG;
	data.op2.op.reg = instr->op3;

	prv_encode_mov_instr(user_data, NULL, SUBTILIS_ARM_INSTR_MOV, &data,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	memset(&data, 0, sizeof(data));

	data.ccode = instr->fused ? instr->false_cond : SUBTILIS_ARM_CCODE_NE;
	data.status = false;
	data.dest = instr->dest;
	data.op2.type = SUBTILIS_ARM_OP2_REG;
	data.op2.op.reg = instr->op2;

	prv_encode_mov_instr(user_data, NULL, SUBTILIS_ARM_INSTR_MOV, &data,
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

static void prv_encode_flags_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_arm_flags_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_ARM_INSTR_MRS) {
		word = 0x01000000;
		word |= (instr->op.reg & 0xf) << 12;
	} else {
		word = 0x01200000;
		if (instr->op2_reg) {
			word |= instr->op.reg & 0xf;
		} else {
			word |= 1 << 25;
			word |= (instr->op.integer & 0xfff);
		}
		word |= instr->fields;
	}

	if (instr->flag_reg == SUBTILIS_ARM_FLAGS_SPSR)
		word |= 1 << 22;
	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
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
	prv_add_word(ud, word, err);
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

	prv_add_word(ud, word, err);
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
			word |= 1 << 3;
			word |= instr->op2.imm;
		} else {
			word |= instr->op2.reg;
		}
		word |= instr->dest << 12;
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

	prv_add_word(ud, word, err);
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

	prv_add_word(ud, word, err);
}

static void prv_encode_fpa_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_fpa_ldrc_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_fpa_stran_instr_t stran;
	subtilis_arm_encode_ud_t *ud = user_data;

	/*
	 * adj is the number of additional bytes that can be written
	 * without causing a pool flush.  8 for the double and 4
	 * for the ldr instruction.
	 */

	size_t adj = 12;

	prv_check_pool_adj(ud, adj, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (ud->ldrc_real == SIZE_MAX)
		ud->ldrc_real = ud->bytes_written;

	prv_add_const(ud, instr->label, ud->bytes_written,
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

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_stran_instr(void *user_data, subtilis_arm_op_t *op,
				       subtilis_arm_instr_type_t type,
				       subtilis_vfp_stran_instr_t *instr,
				       subtilis_error_t *err)
{
	uint32_t copro = 0xb;
	uint32_t dest = instr->dest & 0x1f;
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 6 << 25;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word |= instr->ccode << 28;
	word |= (0xC | (instr->pre_indexed ? 1 : 0)) << 24;
	if (!instr->subtract)
		word |= 1 << 23;

	if ((type == SUBTILIS_VFP_INSTR_FSTS) ||
	    (type == SUBTILIS_VFP_INSTR_FLDS)) {
		word |= (dest & 1) << 22;
		dest = dest >> 1;
		copro--;
	}

	if (instr->write_back)
		word |= 1 << 21;

	if (type == SUBTILIS_VFP_INSTR_FLDS || type == SUBTILIS_VFP_INSTR_FLDD)
		word |= 1 << 20;

	word |= instr->base << 16;
	word |= dest << 12;
	word |= copro << 8;
	word |= instr->offset;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static uint32_t prv_encode_src_dest(subtilis_arm_reg_t src,
				    subtilis_arm_reg_t dest, bool float32_src,
				    bool float32_dest)
{
	uint32_t word = 0;

	if (float32_src) {
		word |= (src >> 1) & 0xf;
		word |= (src & 1) << 5;
	} else {
		word |= src & 0xf;
	}
	if (float32_dest) {
		word |= ((dest >> 1) & 0xf) << 12;
		word |= (dest & 1) << 22;
	} else {
		word |= (dest & 0xf) << 12;
	}

	return word;
}

static void prv_encode_vfp_copy_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_copy_instr_t *instr,
				      subtilis_error_t *err)
{
	uint32_t word;
	bool float32 = false;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCPYS:
		word = 0xeb00a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FCPYD:
		word = 0xeb00b40;
		break;
	case SUBTILIS_VFP_INSTR_FNEGS:
		word = 0xeb10a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FNEGD:
		word = 0xeb10b40;
		break;
	case SUBTILIS_VFP_INSTR_FABSS:
		word = 0xeb00ac0;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FABSD:
		word = 0xeb00bc0;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= prv_encode_src_dest(instr->src, instr->dest, float32, float32);

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_ldrc_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_ldrc_instr_t *instr,
				      subtilis_error_t *err)
{
	subtilis_vfp_stran_instr_t stran;
	subtilis_arm_encode_ud_t *ud = user_data;

	/*
	 * adj is the number of additional bytes that can be written
	 * without causing a pool flush.  8 for the double and 4
	 * for the ldr instruction.
	 */

	size_t adj = 12;

	prv_check_pool_adj(ud, adj, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (ud->ldrc_real == SIZE_MAX)
		ud->ldrc_real = ud->bytes_written;

	prv_add_const(ud, instr->label, ud->bytes_written,
		      SUBTILIS_ARM_ENCODE_BP_LDRF, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran.ccode = instr->ccode;
	stran.dest = instr->dest;
	stran.base = 15;
	stran.offset = 0;
	stran.pre_indexed = true;
	stran.write_back = false;
	stran.subtract = false;
	prv_encode_vfp_stran_instr(user_data, op, SUBTILIS_VFP_INSTR_FLDD,
				   &stran, err);
}

static void prv_encode_vfp_tran_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_tran_instr_t *instr,
				      subtilis_error_t *err)
{
	uint32_t word;
	bool float32_src = true;
	bool float32_dest = true;
	subtilis_arm_reg_t src = instr->src;
	subtilis_arm_reg_t dest = instr->dest;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FTOSIS:
		word = 0xebd0a40;
		break;
	case SUBTILIS_VFP_INSTR_FTOUIS:
		word = 0xebc0a40;
		break;
	case SUBTILIS_VFP_INSTR_FTOUIZS:
		word = 0xebc0ac0;
		break;
	case SUBTILIS_VFP_INSTR_FTOSIZS:
		word = 0xebd0ac0;
		break;
	case SUBTILIS_VFP_INSTR_FSITOS:
		word = 0xeb80ac0;
		break;
	case SUBTILIS_VFP_INSTR_FUITOS:
		word = 0xeb80a40;
		break;
	case SUBTILIS_VFP_INSTR_FSITOD:
		word = 0xeb80bc0;
		float32_dest = false;
		if (instr->use_dregs)
			src *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FTOSID:
		word = 0xebd0b40;
		float32_src = false;
		if (instr->use_dregs)
			dest *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FTOUID:
		word = 0xebc0b40;
		float32_src = false;
		if (instr->use_dregs)
			dest *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FTOSIZD:
		word = 0xebd0bc0;
		float32_src = false;
		if (instr->use_dregs)
			dest *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FTOUIZD:
		word = 0xebc0bc0;
		float32_src = false;
		if (instr->use_dregs)
			dest *= 2;
		break;
	case SUBTILIS_VFP_INSTR_FUITOD:
		word = 0xebc0bc0;
		float32_dest = false;
		if (instr->use_dregs)
			src *= 2;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	word |= prv_encode_src_dest(src, dest, float32_src, float32_dest);

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_tran_dbl_instr(void *user_data,
					  subtilis_arm_op_t *op,
					  subtilis_arm_instr_type_t type,
					  subtilis_vfp_tran_dbl_instr_t *instr,
					  subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMDRR:
		word = 0xc400b10;
		word |= instr->dest1 & 0xf;
		word |= (instr->src1 & 0xf) << 12;
		word |= (instr->src2 & 0xf) << 16;
		break;
	case SUBTILIS_VFP_INSTR_FMRRD:
		word = 0xc500b10;
		word |= instr->src1 & 0xf;
		word |= (instr->dest1 & 0xf) << 12;
		word |= (instr->dest2 & 0xf) << 16;
		break;
	case SUBTILIS_VFP_INSTR_FMSRR:
		if (instr->dest1 + 1 != instr->dest2) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		word = 0xc400a10;
		word |= (instr->dest1 >> 1) & 0xf;
		word |= (instr->dest1 & 1) << 5;
		word |= (instr->src1 & 0xf) << 12;
		word |= (instr->src2 & 0xf) << 16;
		break;
	case SUBTILIS_VFP_INSTR_FMRRS:
		if (instr->src1 + 1 != instr->src2) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		word = 0xc500a10;
		word |= (instr->src1 >> 1) & 0xf;
		word |= (instr->src1 & 1) << 5;
		word |= (instr->dest1 & 0xf) << 12;
		word |= (instr->dest2 & 0xf) << 16;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_cptran_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_vfp_cptran_instr_t *instr,
					subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_reg_t src = instr->src;
	subtilis_arm_reg_t dest = instr->dest;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_VFP_INSTR_FMSR) {
		if (instr->use_dregs)
			dest *= 2;
		word = 0xe000a10;
		word |= (src & 0xf) << 12;
		word |= ((dest >> 1) & 0xf) << 16;
		word |= (dest & 1) << 7;
	} else {
		if (instr->use_dregs)
			src *= 2;
		word = 0xe100a10;
		word |= (dest & 0xf) << 12;
		word |= ((src >> 1) & 0xf) << 16;
		word |= (src & 1) << 7;
	}

	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_data_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_data_instr_t *instr,
				      subtilis_error_t *err)
{
	uint32_t word = 0;
	bool float32 = false;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FMACS:
		word |= 0xe000a00;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FMACD:
		word |= 0xe000b00;
		break;
	case SUBTILIS_VFP_INSTR_FNMACS:
		word |= 0xe000a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FNMACD:
		word |= 0xe000b40;
		break;
	case SUBTILIS_VFP_INSTR_FMSCS:
		word |= 0xe100a00;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FMSCD:
		word |= 0xe100b00;
		break;
	case SUBTILIS_VFP_INSTR_FNMSCS:
		word |= 0xe100a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FNMSCD:
		word |= 0xe100b40;
		break;
	case SUBTILIS_VFP_INSTR_FMULS:
		word |= 0xe200a00;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FMULD:
		word |= 0xe200b00;
		break;
	case SUBTILIS_VFP_INSTR_FNMULS:
		word |= 0xe200a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FNMULD:
		word |= 0xe200b40;
		break;
	case SUBTILIS_VFP_INSTR_FADDS:
		word |= 0xe300a00;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FADDD:
		word |= 0xe300b00;
		break;
	case SUBTILIS_VFP_INSTR_FSUBS:
		word |= 0xe300a40;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FSUBD:
		word |= 0xe300b40;
		break;
	case SUBTILIS_VFP_INSTR_FDIVS:
		word |= 0xe800a00;
		float32 = true;
		break;
	case SUBTILIS_VFP_INSTR_FDIVD:
		word |= 0xe800b00;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;

	if (float32) {
		word |= ((instr->op1 >> 1) & 0xf) << 16;
		word |= (instr->op1 & 1) << 7;

		word |= (instr->op2 >> 1) & 0xf;
		word |= (instr->op2 & 1) << 5;

		word |= ((instr->dest >> 1) & 0xf) << 12;
		word |= (instr->dest & 1) << 22;
	} else {
		word |= (instr->op1 & 0xf) << 16;
		word |= instr->op2 & 0xf;
		word |= (instr->dest & 0xf) << 12;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_cmp_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_cmp_instr_t *instr,
				     subtilis_error_t *err)
{
	uint32_t word = 0;
	bool float32 = false;
	bool two_ops = false;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_VFP_INSTR_FCMPS:
		two_ops = true;
		float32 = true;
		word |= 0xeb40a40;
		break;
	case SUBTILIS_VFP_INSTR_FCMPD:
		two_ops = true;
		word |= 0xeb40b40;
		break;
	case SUBTILIS_VFP_INSTR_FCMPES:
		two_ops = true;
		float32 = true;
		word |= 0xeb40ac0;
		break;
	case SUBTILIS_VFP_INSTR_FCMPED:
		two_ops = true;
		word |= 0xeb40bc0;
		break;
	case SUBTILIS_VFP_INSTR_FCMPZS:
		float32 = true;
		word |= 0xeb50a40;
		break;
	case SUBTILIS_VFP_INSTR_FCMPZD:
		word |= 0xeb50b40;
		break;
	case SUBTILIS_VFP_INSTR_FCMPEZS:
		two_ops = true;
		word |= 0xeb50ac0;
		break;
	case SUBTILIS_VFP_INSTR_FCMPEZD:
		word |= 0xeb50bc0;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;

	if (float32) {
		if (two_ops) {
			word |= (instr->op2 >> 1) & 0xf;
			word |= (instr->op2 & 1) << 5;
		}

		word |= ((instr->op1 >> 1) & 0xf) << 12;
		word |= (instr->op1 & 1) << 22;
	} else {
		if (two_ops)
			word |= instr->op2 & 0xf;
		word |= (instr->op1 & 0xf) << 12;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_sqrt_instr(void *user_data, subtilis_arm_op_t *op,
				      subtilis_arm_instr_type_t type,
				      subtilis_vfp_sqrt_instr_t *instr,
				      subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_VFP_INSTR_FSQRTD) {
		word = 0xeb10bc0;
		word |= instr->op1 & 0xf;
		word |= (instr->dest & 0xf) << 12;
	} else {
		word = 0xeb10ac0;
		word |= (instr->op1 >> 1) & 0xf;
		word |= (instr->op1 & 1) << 5;
		word |= ((instr->dest >> 1) & 0xf) << 12;
		word |= (instr->dest & 1) << 22;
	}

	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_sysreg_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_vfp_sysreg_instr_t *instr,
					subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_VFP_INSTR_FMRX)
		word = 0xef00a10;
	else
		word = 0xee00a10;

	word |= (instr->arm_reg & 0xf) << 12;
	switch (instr->sysreg) {
	case SUBTILIS_VFP_SYSREG_FPSID:
		break;
	case SUBTILIS_VFP_SYSREG_FPSCR:
		word |= 1 << 16;
		break;
	case SUBTILIS_VFP_SYSREG_FPEXC:
		word |= 8 << 16;
		break;
	}

	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_vfp_cvt_instr(void *user_data, subtilis_arm_op_t *op,
				     subtilis_arm_instr_type_t type,
				     subtilis_vfp_cvt_instr_t *instr,
				     subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_encode_ud_t *ud = user_data;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (type == SUBTILIS_VFP_INSTR_FCVTDS) {
		word = 0xeb70ac0;
		word |= (instr->dest << 12);
		word |= (instr->op1 >> 1) | ((instr->op1 & 1) << 5);
	} else {
		word = 0xeb70bc0;
		word |= instr->op1 & 0xf;
		word |=
		    ((instr->dest & 0x1e) << 11) | ((instr->dest & 1) << 22);
	}
	word |= instr->ccode << 28;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_stran_misc_instr(void *user_data, subtilis_arm_op_t *op,
					subtilis_arm_instr_type_t type,
					subtilis_arm_stran_misc_instr_t *instr,
					subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word = 0x90;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((instr->base > 15) || (instr->dest > 15)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= instr->ccode << 28;
	if (instr->pre_indexed)
		word |= 1 << 24;

	if (instr->write_back)
		word |= 1 << 21;
	if (!instr->subtract)
		word |= 1 << 23;
	word |= instr->base << 16;
	word |= instr->dest << 12;

	switch (instr->type) {
	case SUBTILIS_ARM_STRAN_MISC_SB:
		word |= 1 << 6;
		if (type == SUBTILIS_ARM_STRAN_MISC_LDR)
			word |= 1 << 20;
		break;
	case SUBTILIS_ARM_STRAN_MISC_SH:
		word |= 1 << 6;
		word |= 1 << 5;
		if (type == SUBTILIS_ARM_STRAN_MISC_LDR)
			word |= 1 << 20;
		break;
	case SUBTILIS_ARM_STRAN_MISC_H:
		if (type == SUBTILIS_ARM_STRAN_MISC_LDR)
			word |= 1 << 20;
		word |= 1 << 5;
		break;
	case SUBTILIS_ARM_STRAN_MISC_D:
		word |= 1 << 6;
		if (type == SUBTILIS_ARM_STRAN_MISC_STR)
			word |= 1 << 5;
		break;
	}

	if (instr->reg_offset) {
		word |= instr->offset.reg;
	} else {
		word |= 1 << 22;
		word |= instr->offset.imm & 0xf;
		word |= (instr->offset.imm & 0xf0) << 8;
	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
}

static void prv_encode_simd_instr(void *user_data, subtilis_arm_op_t *op,
				  subtilis_arm_instr_type_t type,
				  subtilis_arm_reg_only_instr_t *instr,
				  subtilis_error_t *err)
{
	uint32_t word;
	subtilis_arm_encode_ud_t *ud = user_data;
	const uint32_t opcodes[] = {
	    0x06200010, // SUBTILIS_ARM_SIMD_QADD16
	    0x06200090, // SUBTILIS_ARM_SIMD_QADD8
	    0x06200030, // SUBTILIS_ARM_SIMD_QADDSUBX
	    0x06200070, // SUBTILIS_ARM_SIMD_QSUB16
	    0x062000f0, // SUBTILIS_ARM_SIMD_QSUB8
	    0x06200050, // SUBTILIS_ARM_SIMD_QSUBADDX,
	    0x06100010, // SUBTILIS_ARM_SIMD_SADD16
	    0x06100090, // SUBTILIS_ARM_SIMD_SADD8
	    0x06100030, // SUBTILIS_ARM_SIMD_SADDSUBX,
	    0x06100070, // SUBTILIS_ARM_SIMD_SSUB16
	    0x061000f0, // SUBTILIS_ARM_SIMD_SSUB8,
	    0x06100050, // SUBTILIS_ARM_SIMD_SSUBADDX
	    0x06300010, // SUBTILIS_ARM_SIMD_SHADD16
	    0x06300090, // SUBTILIS_ARM_SIMD_SHADD8
	    0x06300030, // SUBTILIS_ARM_SIMD_SHADDSUBX,
	    0x06300070, // SUBTILIS_ARM_SIMD_SHSUB16,
	    0x063000f0, // SUBTILIS_ARM_SIMD_SHSUB8,
	    0x06300050, // SUBTILIS_ARM_SIMD_SHSUBADDX,
	    0x06500010, // SUBTILIS_ARM_SIMD_UADD16,
	    0x06500090, // SUBTILIS_ARM_SIMD_UADD8,
	    0x06500030, // SUBTILIS_ARM_SIMD_UADDSUBX,
	    0x06500070, // SUBTILIS_ARM_SIMD_USUB16,
	    0x065000f0, // SUBTILIS_ARM_SIMD_USUB8,
	    0x06500050, // SUBTILIS_ARM_SIMD_USUBADDX,
	    0x06700010, // SUBTILIS_ARM_SIMD_UHADD16,
	    0x06700090, // SUBTILIS_ARM_SIMD_UHADD8,
	    0x06700030, // SUBTILIS_ARM_SIMD_UHADDSUBX,
	    0x06700070, // SUBTILIS_ARM_SIMD_UHSUB16,
	    0x067000f0, // SUBTILIS_ARM_SIMD_UHSUB8,
	    0x06700050, // SUBTILIS_ARM_SIMD_UHSUBADDX,
	    0x06600010, // SUBTILIS_ARM_SIMD_UQADD16,
	    0x06600090, // SUBTILIS_ARM_SIMD_UQADD8,
	    0x06600030, // SUBTILIS_ARM_SIMD_UQADDSUBX,
	    0x06600070, // SUBTILIS_ARM_SIMD_UQSUB16,
	    0x066000f0, // SUBTILIS_ARM_SIMD_UQSUB8,
	    0x06600050, // SUBTILIS_ARM_SIMD_UQSUBADDX,
	};
	size_t index = type - SUBTILIS_ARM_SIMD_QADD16;

	if (type > SUBTILIS_ARM_SIMD_UQSUBADDX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	word = opcodes[index];
	word |= instr->ccode << 28;
	word |= (instr->dest & 0xf) << 12;
	word |= (instr->op1 & 0xf) << 16;
	word |= (instr->op2 & 0xf);

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
}

static void prv_encode_signx_instr(void *user_data, subtilis_arm_op_t *op,
				   subtilis_arm_instr_type_t type,
				   subtilis_arm_signx_instr_t *instr,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t *ud = user_data;
	uint32_t word;
	uint32_t ror;

	prv_check_pool(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	switch (type) {
	case SUBTILIS_ARM_INSTR_SXTB:
		word = 0x6af0070;
		break;
	case SUBTILIS_ARM_INSTR_SXTB16:
		word = 0x68f0070;
		break;
	case SUBTILIS_ARM_INSTR_SXTH:
		word = 0x6bf0070;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	switch (instr->rotate) {
	case SUBTILIS_ARM_SIGNX_ROR_NONE:
		ror = 0;
		break;
	case SUBTILIS_ARM_SIGNX_ROR_8:
		ror = 1;
		break;
	case SUBTILIS_ARM_SIGNX_ROR_16:
		ror = 2;
		break;
	case SUBTILIS_ARM_SIGNX_ROR_24:
		ror = 3;
		break;
	}

	word |= (ror << 10);
	word |= instr->ccode << 28;
	word |= (instr->dest & 0xf) << 12;
	word |= (instr->op1 & 0xf);

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_add_word(ud, word, err);
}

static int32_t prv_compute_dist(size_t first, size_t second, size_t limit,
				subtilis_error_t *err)
{
	size_t tmp;
	int32_t mul = 1;
	size_t diff;

	if (first < second) {
		tmp = first;
		first = second;
		second = tmp;
		mul = -1;
	}

	diff = first - second;
	if (diff > limit) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	return (int32_t)(diff * mul);
}

static void prv_apply_back_patches(subtilis_arm_encode_ud_t *ud,
				   subtilis_error_t *err)
{
	subtilis_arm_encode_bp_t *bp;
	size_t i;
	int32_t dist;
	uint32_t *ptr;
	uint32_t encoded;

	for (i = 0; i < ud->back_patch_count; i++) {
		bp = &ud->back_patches[i];
		ptr = prv_get_word_ptr(ud, bp->code_index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		switch (bp->type) {
		case SUBTILIS_ARM_ENCODE_BP_TYPE_BRANCH:
			dist = prv_compute_dist(ud->label_offsets[bp->label],
						bp->code_index, (1 << 23), err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			dist = (dist / 4) - 2;
			dist &= 0xffffff;
			*ptr |= dist;
			break;
		case SUBTILIS_ARM_ENCODE_BP_TYPE_ADR:
			dist =
			    prv_compute_dist(ud->label_offsets[bp->label],
					     bp->code_index, 0xffffffff, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;

			dist -= 8;
			if (dist < 0) {
				/*
				 * Here we switch the ADD for a SUB.
				 */

				*ptr &= ~(1 << 23);
				*ptr |= 1 << 22;
				dist = -dist;
			}

			if (!subtilis_arm_encode_imm(dist, &encoded)) {
				/* TODO: Need a proper error for this */
				subtilis_error_set_ass_bad_adr(err);
				return;
			}

			*ptr |= encoded;
			break;
		}
	}
}

static void prv_encode_directive(void *user_data, subtilis_arm_op_t *op,
				 subtilis_error_t *err)
{
	uint32_t *dbl_ptr;
	size_t len;
	size_t i;
	subtilis_arm_encode_ud_t *ud = user_data;

	switch (op->type) {
	case SUBTILIS_ARM_OP_ALIGN:
		len = ud->bytes_written & (op->op.alignment - 1);
		if (len == 0)
			return;
		len = op->op.alignment - len;
		prv_ensure_code_size(ud, len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		for (i = 0; i < len; i++)
			ud->code[ud->bytes_written++] = 0;
		break;
	case SUBTILIS_ARM_OP_BYTE:
		prv_ensure_code_size(ud, 1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ud->code[ud->bytes_written++] = op->op.byte;
		break;
	case SUBTILIS_ARM_OP_TWO_BYTE:
		prv_ensure_code_size(ud, 2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*((uint16_t *)&ud->code[ud->bytes_written]) = op->op.two_bytes;
		ud->bytes_written += 2;
		break;
	case SUBTILIS_ARM_OP_FOUR_BYTE:
		prv_ensure_code_size(ud, 4, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*((uint32_t *)&ud->code[ud->bytes_written]) = op->op.four_bytes;
		ud->bytes_written += 4;
		break;
	case SUBTILIS_ARM_OP_DOUBLE:
		prv_ensure_code_size(ud, 8, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		*((double *)&ud->code[ud->bytes_written]) = op->op.dbl;
		ud->bytes_written += 8;
		break;
	case SUBTILIS_ARM_OP_DOUBLER:
		prv_ensure_code_size(ud, 8, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dbl_ptr = (uint32_t *)&op->op.dbl;
		*((uint32_t *)&ud->code[ud->bytes_written]) = dbl_ptr[1];
		*((uint32_t *)&ud->code[ud->bytes_written + 4]) = dbl_ptr[0];
		ud->bytes_written += 8;
		break;
	case SUBTILIS_ARM_OP_FLOAT:
		prv_ensure_code_size(ud, 4, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		dbl_ptr = (uint32_t *)&op->op.flt;
		*((uint32_t *)&ud->code[ud->bytes_written]) = *dbl_ptr;
		ud->bytes_written += 4;
		break;
	case SUBTILIS_ARM_OP_STRING:
		len = strlen(op->op.str) + 1;
		prv_ensure_code_size(ud, len, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		strcpy((char *)&ud->code[ud->bytes_written], op->op.str);
		ud->bytes_written += len;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
}

static void prv_arm_encode(subtilis_arm_section_t *arm_s,
			   subtilis_arm_encode_ud_t *ud, subtilis_error_t *err)
{
	subtlis_arm_walker_t walker;
	subtilis_arm_op_t align_op;

	walker.user_data = ud;
	walker.label_fn = prv_encode_label;
	walker.directive_fn = prv_encode_directive;
	walker.data_fn = prv_encode_data_instr;
	walker.mul_fn = prv_encode_mul_instr;
	walker.cmp_fn = prv_encode_cmp_instr;
	walker.mov_fn = prv_encode_mov_instr;
	walker.stran_fn = prv_encode_stran_instr;
	walker.mtran_fn = prv_encode_mtran_instr;
	walker.br_fn = prv_encode_br_instr;
	walker.swi_fn = prv_encode_swi_instr;
	walker.ldrc_fn = prv_encode_ldrc_instr;
	walker.ldrp_fn = prv_encode_ldrp_instr;
	walker.adr_fn = prv_encode_adr_instr;
	walker.cmov_fn = prv_encode_cmov_instr;
	walker.flags_fn = prv_encode_flags_instr;
	walker.fpa_data_monadic_fn = prv_encode_fpa_data_monadic_instr;
	walker.fpa_data_dyadic_fn = prv_encode_fpa_data_dyadic_instr;
	walker.fpa_stran_fn = prv_encode_fpa_stran_instr;
	walker.fpa_tran_fn = prv_encode_fpa_tran_instr;
	walker.fpa_cmp_fn = prv_encode_fpa_cmp_instr;
	walker.fpa_ldrc_fn = prv_encode_fpa_ldrc_instr;
	walker.fpa_cptran_fn = prv_encode_fpa_cptran_instr;
	walker.vfp_stran_fn = prv_encode_vfp_stran_instr;
	walker.vfp_copy_fn = prv_encode_vfp_copy_instr;
	walker.vfp_ldrc_fn = prv_encode_vfp_ldrc_instr;
	walker.vfp_tran_fn = prv_encode_vfp_tran_instr;
	walker.vfp_tran_dbl_fn = prv_encode_vfp_tran_dbl_instr;
	walker.vfp_cptran_fn = prv_encode_vfp_cptran_instr;
	walker.vfp_data_fn = prv_encode_vfp_data_instr;
	walker.vfp_cmp_fn = prv_encode_vfp_cmp_instr;
	walker.vfp_sqrt_fn = prv_encode_vfp_sqrt_instr;
	walker.vfp_sysreg_fn = prv_encode_vfp_sysreg_instr;
	walker.vfp_cvt_fn = prv_encode_vfp_cvt_instr;
	walker.stran_misc_fn = prv_encode_stran_misc_instr;
	walker.simd_fn = prv_encode_simd_instr;
	walker.signx_fn = prv_encode_signx_instr;

	subtilis_arm_walk(arm_s, &walker, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_flush_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_apply_back_patches(ud, err);

	/*
	 * Forcibly align the end of each section to a 4 byte boundary.
	 */

	align_op.type = SUBTILIS_ARM_OP_ALIGN;
	align_op.op.alignment = 4;
	prv_encode_directive(ud, &align_op, err);
}

static void prv_copy_constant_to_buf(subtilis_arm_prog_t *arm_p,
				     subtilis_arm_encode_ud_t *ud,
				     subtilis_constant_data_t *data,
				     subtilis_error_t *err)
{
	size_t i;
	uint32_t *word;
	uint32_t *ptr;
	size_t null_bytes_needed;
	uint8_t *bptr;

	if (data->dbl && arm_p->fp_if && arm_p->fp_if->reverse_fpa_consts) {
		ptr = prv_get_word_ptr(ud, ud->bytes_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		for (i = 0; i < data->data_size / sizeof(double); i++) {
			word = (uint32_t *)&data->data[i * sizeof(double)];
			*ptr++ = word[1];
			*ptr++ = word[0];
		}
	} else {
		memcpy(&ud->code[ud->bytes_written], data->data,
		       data->data_size);
		if (data->data_size & 3) {
			null_bytes_needed = 4 - (data->data_size & 3);
			bptr = &ud->code[ud->bytes_written] + data->data_size;
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
		subtilis_arm_link_section(ud->link, i, ud->bytes_written);
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
				size_in_bytes = (size_in_words + 1) << 2;
			prv_ensure_code_size(ud, size_in_bytes, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			const_locations[i] = ud->bytes_written;
			prv_copy_constant_to_buf(
			    arm_p, ud, &arm_p->constant_pool->data[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			ud->bytes_written += size_in_bytes;
		}
	}

	subtilis_arm_link_link(ud->link, ud->code, ud->bytes_written,
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
		plat(ud.code, ud.bytes_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	prv_write_file(&ud, fname, err);

cleanup:

	prv_free_encode_ud(&ud);
}

uint8_t *subtilis_arm_encode_buf(subtilis_arm_prog_t *arm_p,
				 size_t *bytes_written, subtilis_error_t *err)
{
	subtilis_arm_encode_ud_t ud;
	uint8_t *retval = NULL;

	prv_init_encode_ud(&ud, arm_p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(arm_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	*bytes_written = ud.bytes_written;
	retval = ud.code;
	ud.code = NULL;

cleanup:

	prv_free_encode_ud(&ud);

	return retval;
}
