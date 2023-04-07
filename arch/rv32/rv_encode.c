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

#include "rv_encode.h"
#include "rv_link.h"
#include "rv_opcodes.h"
#include "rv_walker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The encoder is a bit weird.  It does 4 things.
 * 1. assembles the instructions into machine code.
 * 2. resolves local jumps
 * 3. resolves function calls
 * 4. creates constant pools and encodes the offsets to those
 *    constants in the instructions that reference them.
 */

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
	SUBTILIS_RV_ENCODE_BP_LDR,
	SUBTILIS_RV_ENCODE_BP_LDRP,
	SUBTILIS_RV_ENCODE_BP_LDRF,
} subtilis_rv_encode_const_type_t;

struct subtilis_rv_encode_const_t_ {
	size_t label;
	size_t section_label;
	size_t code_index;
	subtilis_rv_encode_const_type_t type;
};

typedef struct subtilis_rv_encode_const_t_ subtilis_rv_encode_const_t;

typedef enum {
	SUBTILIS_RV_ENCODE_BP_TYPE_BRANCH,
	SUBTILIS_RV_ENCODE_BP_TYPE_JAL,
	SUBTILIS_RV_ENCODE_BP_TYPE_ADR,
} subtilis_rv_encode_bp_type_t;

struct subtilis_rv_encode_bp_t_ {
	subtilis_rv_encode_bp_type_t type;
	size_t label;
	size_t code_index;
};

typedef struct subtilis_rv_encode_bp_t_ subtilis_rv_encode_bp_t;

struct subtilis_rv_encode_ud_t_ {
	subtilis_rv_section_t *rv_s;
	size_t *label_offsets;
	size_t max_label_offsets;
	size_t max_labels;
	uint8_t *code;
	size_t bytes_written;
	size_t max_bytes;
	subtilis_rv_encode_const_t *constants;
	size_t const_count;
	size_t max_const_count;
	size_t int_const_count;
	size_t real_const_count;
	subtilis_rv_encode_bp_t *back_patches;
	size_t back_patch_count;
	size_t max_patch_count;
	subtilis_rv_link_t *link;
	size_t ldrc_real;
	size_t ldrc_int;
	size_t globals;
};

typedef struct subtilis_rv_encode_ud_t_ subtilis_rv_encode_ud_t;

static void prv_free_encode_ud(subtilis_rv_encode_ud_t *ud)
{
	subtilis_rv_link_delete(ud->link);
	free(ud->back_patches);
	free(ud->constants);
	free(ud->code);
	free(ud->label_offsets);
}

static void prv_init_encode_ud(subtilis_rv_encode_ud_t *ud,
			       subtilis_rv_prog_t *rv_p,
			       size_t globals,
			       subtilis_error_t *err)
{
	size_t i;
	subtilis_rv_section_t *rv_s;
	size_t max_label_offsets = 0;
	size_t constants = 0;
	size_t real_constants = 0;

	memset(ud, 0, sizeof(*ud));

	for (i = 0; i < rv_p->num_sections; i++) {
		rv_s = rv_p->sections[i];
		if (rv_s->label_counter > max_label_offsets)
			max_label_offsets = rv_s->label_counter;
		constants += rv_s->constants.ui32_count;
		real_constants += rv_s->constants.real_count;
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

	ud->max_bytes = ((rv_p->op_pool->len + constants) * sizeof(uint32_t)) +
			(real_constants * 8);
	ud->code = malloc(ud->max_bytes);
	if (!ud->code) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	ud->link = subtilis_rv_link_new(rv_p->num_sections, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	ud->globals = globals;

	return;

on_error:

	prv_free_encode_ud(ud);
}

static void prv_reset_pool_state(subtilis_rv_encode_ud_t *ud)
{
	ud->const_count = 0;
	ud->int_const_count = 0;
	ud->real_const_count = 0;
	ud->ldrc_real = SIZE_MAX;
	ud->ldrc_int = SIZE_MAX;
}

static void prv_ensure_code_size(subtilis_rv_encode_ud_t *ud, size_t bytes,
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

static void prv_ensure_code(subtilis_rv_encode_ud_t *ud, subtilis_error_t *err)
{
	prv_ensure_code_size(ud, 4, err);
}

static void prv_encode_label(void *user_data, subtilis_rv_op_t *op,
			     size_t label, subtilis_error_t *err)
{
	subtilis_rv_encode_ud_t *ud = user_data;
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

static uint32_t *prv_get_word_ptr(subtilis_rv_encode_ud_t *ud, size_t index,
				  subtilis_error_t *err)
{
	if (index & 3) {
		subtilis_error_set_ass_bad_alignment(err);
		return NULL;
	}

	if (index + 4 > ud->bytes_written) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return (uint32_t *)&ud->code[index];
}

static void prv_add_word(subtilis_rv_encode_ud_t *ud, uint32_t word,
			 subtilis_error_t *err)
{
	if (ud->bytes_written & 3) {
		subtilis_error_set_ass_bad_alignment(err);
		return;
	}
	memcpy(&ud->code[ud->bytes_written], &word, sizeof(word));
	ud->bytes_written += sizeof(word);
}

static void prv_apply_constants(subtilis_rv_encode_ud_t *ud,
				subtilis_error_t *err)
{
#if 0
	subtilis_rv_encode_const_t *cnst;
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
#endif
}

static void prv_flush_constants(subtilis_rv_encode_ud_t *ud,
				subtilis_error_t *err)
{
#if 0
	size_t i;
	size_t j;
	subtilis_rv_encode_const_t *cnst;
	uint32_t *real_ptr;
	int32_t constant_index;
	subtilis_rv_section_t *arm_s = ud->arm_s;

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
#endif
}

static void prv_add_back_patch(subtilis_rv_encode_ud_t *ud,
			       subtilis_rv_encode_bp_type_t type, size_t label,
			       size_t code_index, subtilis_error_t *err)
{
	size_t new_max;
	subtilis_rv_encode_bp_t *new_bp;

	if (ud->back_patch_count == ud->max_patch_count) {
		new_max = ud->max_patch_count + SUBTILIS_ENCODER_BACKPATCH_GRAN;
		new_bp = realloc(ud->back_patches,
				 new_max * sizeof(subtilis_rv_encode_bp_t));
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

static int32_t prv_compute_dist(size_t first, size_t second, size_t limit,
				subtilis_error_t *err)
{
	ssize_t diff;

	diff = first - second;

	if (first < second) {
		/*
		 * if we're doing a backwards jump.
		 */

		if (diff < -limit) {
			subtilis_error_set_assertion_failed(err);
			return 0;
		}
	} else {
		if (diff >= limit) {
			subtilis_error_set_assertion_failed(err);
			return 0;
		}
	}

	return (int32_t )diff;
}

static void prv_reverse_branch(uint32_t *branch, subtilis_error_t *err)
{
	uint32_t new_funct3;
	uint32_t word = *branch & 0x01ff807f;
	uint32_t funct3 = (*branch >> 12) & 7;

	if (rv_opcodes[SUBTILIS_RV_BEQ].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BNE].funct3;
	} else if (rv_opcodes[SUBTILIS_RV_BNE].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BEQ].funct3;
	} else if (rv_opcodes[SUBTILIS_RV_BLT].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BGE].funct3;
	} else if (rv_opcodes[SUBTILIS_RV_BGE].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BLT].funct3;
	} else if (rv_opcodes[SUBTILIS_RV_BLTU].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BGEU].funct3;
	} else if (rv_opcodes[SUBTILIS_RV_BGEU].funct3 == funct3) {
		new_funct3 = rv_opcodes[SUBTILIS_RV_BLTU].funct3;
	} else {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	word |= new_funct3 << 12;

	/*
	 * skip one instruction.
	 */

	word |= 4 << 8;

	*branch = word;
}

static void prv_encode_long_branch(subtilis_rv_encode_ud_t *ud,
				   size_t code_index, int32_t dist,
				   subtilis_error_t *err)
{
	uint32_t *branch;
	uint32_t *jump;

	/*
	 * The dist is going to be -4 as we're encoding it into the instruction
	 * that follows the branch.
	 */

	dist -= 4;

	if ((dist < -1048576) || dist >= 1048576) {
		/*
		 * TODO: need a proper error for this.
		 */

		subtilis_error_set_ass_jump_top_far(err, dist);
		return;
	}

	/*
	 * We need to reverse our branch and jump over the next instruction
	 * which will be a jump.
	 */

	/*
	 * There must be at least 3 more instructions in the code stream.
	 * Our current branch, a nop which we'll turn into a jal and a
	 * an instruction to execute if the original branch condition is
	 * not taken.
	 */

	if (code_index + 12 > ud->bytes_written) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	branch = (uint32_t *)&ud->code[code_index];
	jump = (uint32_t *)&ud->code[code_index + 4];

	prv_reverse_branch(branch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/*
	 * Convert our NOP into a jump.
	 */

	*jump = rv_opcodes[SUBTILIS_RV_JAL].opcode;
	subtilis_rv_link_encode_jal(jump, dist);
}

static void prv_apply_back_patches(subtilis_rv_encode_ud_t *ud,
				   subtilis_error_t *err)
{

	subtilis_rv_encode_bp_t *bp;
	size_t i;
	int32_t dist;
	uint32_t *ptr;

	for (i = 0; i < ud->back_patch_count; i++) {
		bp = &ud->back_patches[i];
		ptr = prv_get_word_ptr(ud, bp->code_index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		switch (bp->type) {
		case SUBTILIS_RV_ENCODE_BP_TYPE_BRANCH:
			dist = ud->label_offsets[bp->label] - bp->code_index;
			if (dist < -4096 || dist > 4095) {
				prv_encode_long_branch(ud, bp->code_index, dist,
						       err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				continue;
			}
			dist = (dist / 2);
			dist &= 0xfff;
			*ptr |= (dist & 0xf) << 8;
			*ptr |= (dist  & 0x400) >> 3;
			*ptr |= (dist & 0x3f0) << 21;
			*ptr |= (dist & 0x800) << 20;
			break;
		case SUBTILIS_RV_ENCODE_BP_TYPE_JAL:
			dist = prv_compute_dist(ud->label_offsets[bp->label],
						bp->code_index, 4096, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_rv_link_encode_jal(ptr, dist);
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
#if 0
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
#endif
		}
	}
}

static void prv_check_pool_adj(subtilis_rv_encode_ud_t *ud, size_t adj,
			       subtilis_error_t *err)
{
#if 0
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
#endif
}

static void prv_check_pool(subtilis_rv_encode_ud_t *ud, subtilis_error_t *err)
{
	prv_check_pool_adj(ud, 0, err);
}

static void prv_reset_encode_ud(subtilis_rv_encode_ud_t *ud,
				subtilis_rv_section_t *rv_s)
{
	ud->rv_s = rv_s;
	ud->max_labels = rv_s->label_counter;
	ud->back_patch_count = 0;
	prv_reset_pool_state(ud);
}

static void prv_add_const(subtilis_rv_encode_ud_t *ud, size_t label,
			  size_t code_index,
			  subtilis_rv_encode_const_type_t type,
			  subtilis_error_t *err)
{
	size_t new_max;
	subtilis_rv_encode_const_t *new_const;

	if (ud->const_count == ud->max_const_count) {
		new_max = ud->max_const_count + SUBTILIS_ENCODER_BACKPATCH_GRAN;
		new_const =
		    realloc(ud->constants,
			    new_max * sizeof(subtilis_rv_encode_const_t));
		if (!new_const) {
			subtilis_error_set_oom(err);
			return;
		}
		ud->max_const_count = new_max;
		ud->constants = new_const;
	}
	if (type != SUBTILIS_RV_ENCODE_BP_LDRF)
		ud->int_const_count++;
	else
		ud->real_const_count++;
	new_const = &ud->constants[ud->const_count++];
	new_const->label = label;
	new_const->code_index = code_index;
	new_const->type = type;
}

static void prv_write_file(subtilis_rv_encode_ud_t *ud, const char *fname,
			   subtilis_rv_encode_plat_t plat_header,
			   subtilis_rv_encode_plat_t plat_tail,
			   subtilis_error_t *err)
{
	FILE *fp;

	fp = fopen(fname, "w");
	if (!fp) {
		subtilis_error_set_file_open(err, fname);
		return;
	}

	plat_header(fp, ud->code, ud->bytes_written, ud->globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

	if (fwrite(ud->code, 1, ud->bytes_written, fp) < ud->bytes_written) {
		subtilis_error_set_file_write(err);
		goto fail;
	}

	plat_tail(fp, ud->code, ud->bytes_written, ud->globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto fail;

	if (fclose(fp) != 0)
		subtilis_error_set_file_close(err);

	return;

fail:

	(void)fclose(fp);
}

static void prv_encode_r(void *user_data, subtilis_rv_op_t *op,
			 subtilis_rv_instr_type_t itype,
			 subtilis_rv_instr_encoding_t etype,
			 rv_rtype_t *r, subtilis_error_t *err)
{
	uint32_t word;
	const rv_opcode_t *op_code = &rv_opcodes[itype];
	subtilis_rv_encode_ud_t *ud = user_data;

	word = op_code->opcode | (op_code->funct3 << 12) |
		(op_code->funct7 << 25);
	word |= (r->rd & 0x1f) << 7;
	word |= (r->rs1 & 0x1f) << 15;
	word |= (r->rs2 & 0x1f) << 20;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_i(void *user_data, subtilis_rv_op_t *op,
			 subtilis_rv_instr_type_t itype,
			 subtilis_rv_instr_encoding_t etype,
			 rv_itype_t *i, subtilis_error_t *err)
{
	uint32_t word;
	const rv_opcode_t *op_code = &rv_opcodes[itype];
	subtilis_rv_encode_ud_t *ud = user_data;

	word = op_code->opcode | (op_code->funct3 << 12);
	word |= (i->rd & 0x1f) << 7;
	word |= (i->rs1 & 0x1f) << 15;
	word |= (i->imm & 0xfff) << 20;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_sb(void *user_data, subtilis_rv_op_t *op,
			  subtilis_rv_instr_type_t itype,
			  subtilis_rv_instr_encoding_t etype,
			  rv_sbtype_t *sb, subtilis_error_t *err)
{
	uint32_t word;
	const rv_opcode_t *op_code = &rv_opcodes[itype];
	subtilis_rv_encode_ud_t *ud = user_data;

	word = op_code->opcode | (op_code->funct3 << 12);

	if (etype == SUBTILIS_RV_S_TYPE) {
		word |= (sb->op.imm & 0x1f) << 7;
		word |= (sb->op.imm & 0xfe0) << 20;
	} else {
		/*
		 * If it's a branch we'll need to fill in the immediate field
		 * when we apply the backpatches. That's currently how this is
		 * local jumps.
		 */

		prv_add_back_patch(ud, SUBTILIS_RV_ENCODE_BP_TYPE_BRANCH,
				   sb->op.label, ud->bytes_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	word |= (sb->rs1 & 0x1f) << 15;
	word |= (sb->rs2 & 0x1f) << 20;

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_uj(void *user_data, subtilis_rv_op_t *op,
			  subtilis_rv_instr_type_t itype,
			  subtilis_rv_instr_encoding_t etype,
			  rv_ujtype_t *uj, subtilis_error_t *err)
{
	uint32_t word;
	const rv_opcode_t *op_code = &rv_opcodes[itype];
	subtilis_rv_encode_ud_t *ud = user_data;

	word = op_code->opcode;
	word |= (uj->rd & 0x1f) << 7;
	if (etype == SUBTILIS_RV_U_TYPE) {
		word |= uj->op.imm << 12;
	} else if (uj->rd != 0) {
		/*
		 * If it's a call we'll leave it 0 for now and fill in the
		 * address when we link.
		 */

		subtilis_rv_link_add(ud->link, ud->bytes_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		word |= (uj->op.label << 12);
	} else {
		/*
		 * Otherwise it's an unconditional jump.
		 */

		prv_add_back_patch(ud, SUBTILIS_RV_ENCODE_BP_TYPE_JAL,
				   uj->op.label, ud->bytes_written, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

	}

	prv_ensure_code(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_add_word(ud, word, err);
}

static void prv_encode_directive(void *user_data, subtilis_rv_op_t *op,
				 subtilis_error_t *err)
{
	size_t len;
	size_t i;
	subtilis_rv_encode_ud_t *ud = user_data;

	switch (op->type) {
	case SUBTILIS_RV_OP_ALIGN:
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
	case SUBTILIS_RV_OP_BYTE:
		prv_ensure_code_size(ud, 1, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		ud->code[ud->bytes_written++] = op->op.byte;
		break;
	case SUBTILIS_RV_OP_TWO_BYTE:
		prv_ensure_code_size(ud, 2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(&ud->code[ud->bytes_written], &op->op.two_bytes, 2);
		ud->bytes_written += 2;
		break;
	case SUBTILIS_RV_OP_FOUR_BYTE:
		prv_ensure_code_size(ud, 4, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(&ud->code[ud->bytes_written], &op->op.four_bytes, 4);
		ud->bytes_written += 4;
		break;
	case SUBTILIS_RV_OP_DOUBLE:
		prv_ensure_code_size(ud, 8, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(&ud->code[ud->bytes_written], &op->op.dbl, 8);
		ud->bytes_written += 8;
		break;
	case SUBTILIS_RV_OP_FLOAT:
		prv_ensure_code_size(ud, 4, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		memcpy(&ud->code[ud->bytes_written], &op->op.flt, 4);
		ud->bytes_written += 4;
		break;
	case SUBTILIS_RV_OP_STRING:
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

static void prv_rv_encode(subtilis_rv_section_t *rv_s,
			  subtilis_rv_encode_ud_t *ud, subtilis_error_t *err)
{
	subtilis_rv_walker_t walker;
	subtilis_rv_op_t align_op;

	walker.user_data = ud;
	walker.label_fn = prv_encode_label;
	walker.directive_fn = prv_encode_directive;
	walker.r_fn = prv_encode_r;
	walker.i_fn = prv_encode_i;
	walker.sb_fn = prv_encode_sb;
	walker.uj_fn = prv_encode_uj;

	subtilis_rv_walk(rv_s, &walker, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_flush_constants(ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_apply_back_patches(ud, err);

	/*
	 * Forcibly align the end of each section to a 4 byte boundary.
	 */

	align_op.type = SUBTILIS_RV_OP_ALIGN;
	align_op.op.alignment = 4;
	prv_encode_directive(ud, &align_op, err);
}

static void prv_copy_constant_to_buf(subtilis_rv_prog_t *arm_p,
				     subtilis_rv_encode_ud_t *ud,
				     subtilis_constant_data_t *data,
				     subtilis_error_t *err)
{
	size_t i;
	uint32_t *word;
	uint32_t *ptr;
	size_t null_bytes_needed;
	uint8_t *bptr;

	memcpy(&ud->code[ud->bytes_written], data->data,
	       data->data_size);
	if (data->data_size & 3) {
		null_bytes_needed = 4 - (data->data_size & 3);
		bptr = &ud->code[ud->bytes_written] + data->data_size;
		for (i = 0; i < null_bytes_needed; i++)
			*bptr++ = 0;
	}
}

static void prv_encode_prog(subtilis_rv_prog_t *rv_p,
			    subtilis_rv_encode_ud_t *ud, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s;
	size_t i;
	size_t size_in_bytes;
	size_t size_in_words;
	size_t *const_locations = NULL;

	for (i = 0; i < rv_p->num_sections; i++) {
		rv_s = rv_p->sections[i];
		subtilis_rv_link_section(ud->link, i, ud->bytes_written);
		prv_reset_encode_ud(ud, rv_s);
		prv_rv_encode(rv_s, ud, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/* Let's write the constants arrays and strings */

	if (rv_p->constant_pool->size > 0) {
		const_locations = malloc(rv_p->constant_pool->size *
					 sizeof(*const_locations));
		if (!const_locations) {
			subtilis_error_set_oom(err);
			return;
		}
		for (i = 0; i < rv_p->constant_pool->size; i++) {
			size_in_bytes = rv_p->constant_pool->data[i].data_size;
			size_in_words = size_in_bytes >> 2;
			if (size_in_bytes > size_in_words << 2)
				size_in_bytes = (size_in_words + 1) << 2;
			prv_ensure_code_size(ud, size_in_bytes, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			const_locations[i] = ud->bytes_written;
			prv_copy_constant_to_buf(
			    rv_p, ud, &rv_p->constant_pool->data[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			ud->bytes_written += size_in_bytes;
		}
	}

	subtilis_rv_link_link(ud->link, ud->code, ud->bytes_written,
			      const_locations, rv_p->constant_pool->size,
			      err);

cleanup:

	free(const_locations);
}

void subtilis_rv_encode(subtilis_rv_prog_t *rv_p, const char *fname,
			size_t globals,
			subtilis_rv_encode_plat_t plat_header,
			subtilis_rv_encode_plat_t plat_tail,
			subtilis_error_t *err)
{
	subtilis_rv_encode_ud_t ud;

	prv_init_encode_ud(&ud, rv_p, globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(rv_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_write_file(&ud, fname, plat_header, plat_tail, err);

cleanup:

	prv_free_encode_ud(&ud);
}

uint8_t *subtilis_rv_encode_buf(subtilis_rv_prog_t *rv_p,
				size_t *bytes_written,
				size_t globals,
				subtilis_error_t *err)
{
	subtilis_rv_encode_ud_t ud;
	uint8_t *retval = NULL;

	prv_init_encode_ud(&ud, rv_p, globals, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_encode_prog(rv_p, &ud, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	*bytes_written = ud.bytes_written;
	retval = ud.code;
	ud.code = NULL;

cleanup:

	prv_free_encode_ud(&ud);

	return retval;
}
