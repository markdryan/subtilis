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

#include "arm_core.h"
#include "arm_expression.h"
#include "arm_keywords.h"
#include "assembler.h"

typedef void (*subtilis_arm_ass_fn_t)(void *);

struct subtilis_arm_ass_modifiers_t_ {
	bool status;
	bool byte;
	bool teqp;
	bool hword;
	bool dword;
};

typedef struct subtilis_arm_ass_modifiers_t_ subtilis_arm_ass_modifiers_t;

struct subtilis_arm_ass_mnemomic_t_ {
	const char *name;
	subtilis_arm_instr_type_t type;
	subtilis_arm_ass_fn_t asm_fn;
	bool status_valid;
};

typedef struct subtilis_arm_ass_mnemomic_t_ subtilis_arm_ass_mnemomic_t;

/*
 * Must match order of codes in subtilis_arm_ccode_type_t
 */

/* clang-format off */
static const char *const condition_codes[] = {
	"EQ",
	"NE",
	"CS",
	"CC",
	"MI",
	"PL",
	"VS",
	"VC",
	"HI",
	"LS",
	"GE",
	"LT",
	"GT",
	"LE",
	"AL",
	"NV",
};

/*
 * Must match order of codes in subtilis_arm_mtran_type_t
 */

static const char *const mtran_types[] = {
	"IA",
	"IB",
	"DA",
	"DB",
	"FA",
	"FD",
	"EA",
	"ED",
};

static const subtilis_arm_ass_mnemomic_t a_mnem[] = {
	{ "ABS", SUBTILIS_FPA_INSTR_ABS, NULL, },
	{ "ACS", SUBTILIS_FPA_INSTR_ACS, NULL, },
	{ "ADC", SUBTILIS_ARM_INSTR_ADC, NULL, },
	{ "ADD", SUBTILIS_ARM_INSTR_ADD, NULL, },
	{ "ADF", SUBTILIS_FPA_INSTR_ADF, NULL, },
	{ "ADR", SUBTILIS_ARM_INSTR_ADR, NULL, },
	{ "AND", SUBTILIS_ARM_INSTR_AND, NULL, },
	{ "ASN", SUBTILIS_FPA_INSTR_ASN, NULL, },
	{ "ATN", SUBTILIS_FPA_INSTR_ATN, NULL, },
};

static const subtilis_arm_ass_mnemomic_t b_mnem[] = {
	{ "B", SUBTILIS_ARM_INSTR_B, NULL, },
	{ "BIC", SUBTILIS_ARM_INSTR_BIC, NULL, },
};

static const subtilis_arm_ass_mnemomic_t c_mnem[] = {
	{ "CMF", SUBTILIS_FPA_INSTR_CMF, NULL, },
	{ "CMFE", SUBTILIS_FPA_INSTR_CMFE, NULL, },
	{ "CMN", SUBTILIS_ARM_INSTR_CMN, NULL, },
	{ "CMP", SUBTILIS_ARM_INSTR_CMP, NULL, },
	{ "CNF", SUBTILIS_FPA_INSTR_CNF, NULL, },
	{ "CNFE", SUBTILIS_FPA_INSTR_CNFE, NULL, },
	{ "COS", SUBTILIS_FPA_INSTR_COS, NULL, },
};

static const subtilis_arm_ass_mnemomic_t d_mnem[] = {
	{ "DVF", SUBTILIS_FPA_INSTR_DVF, NULL, },
};

static const subtilis_arm_ass_mnemomic_t e_mnem[] = {
	{ "EOR", SUBTILIS_ARM_INSTR_EOR, NULL, },
	{ "EXP", SUBTILIS_FPA_INSTR_EXP, NULL, },
};

static const subtilis_arm_ass_mnemomic_t f_mnem[] = {
	{ "FABSD", SUBTILIS_VFP_INSTR_FABSD, NULL, },
	{ "FABSS", SUBTILIS_VFP_INSTR_FABSS, NULL, },

	{ "FADDD", SUBTILIS_VFP_INSTR_FADDD, NULL, },
	{ "FADDS", SUBTILIS_VFP_INSTR_FADDS, NULL, },

	{ "FCMPD", SUBTILIS_VFP_INSTR_FCMPD, NULL, },
	{ "FCMPED", SUBTILIS_VFP_INSTR_FCMPED, NULL, },
	{ "FCMPES", SUBTILIS_VFP_INSTR_FCMPES, NULL, },
	{ "FCMPEZD", SUBTILIS_VFP_INSTR_FCMPEZD, NULL, },
	{ "FCMPEZS", SUBTILIS_VFP_INSTR_FCMPEZS, NULL, },
	{ "FCMPS", SUBTILIS_VFP_INSTR_FCMPS, NULL, },
	{ "FCMPZD", SUBTILIS_VFP_INSTR_FCMPZD, NULL, },
	{ "FCMPZS", SUBTILIS_VFP_INSTR_FCMPZS, NULL, },

	{ "FCPYD", SUBTILIS_VFP_INSTR_FCPYD, NULL, },
	{ "FCPYS", SUBTILIS_VFP_INSTR_FCPYS, NULL, },
	{ "FCVTSD", SUBTILIS_VFP_INSTR_FCVTSD, NULL, },
	{ "FCVTDS", SUBTILIS_VFP_INSTR_FCVTDS, NULL, },
	{ "FDIVD", SUBTILIS_VFP_INSTR_FDIVD, NULL, },
	{ "FDIVS", SUBTILIS_VFP_INSTR_FDIVS, NULL },

	{ "FDV", SUBTILIS_FPA_INSTR_FDV, NULL, },
	{ "FIX", SUBTILIS_FPA_INSTR_FIX, NULL, },
	{ "FLDD", SUBTILIS_VFP_INSTR_FLDD, NULL, },
	{ "FLDS", SUBTILIS_VFP_INSTR_FLDS, NULL, },
	{ "FLT", SUBTILIS_FPA_INSTR_FLT, NULL, },
	{ "FMACD", SUBTILIS_VFP_INSTR_FMACD, NULL, },
	{ "FMACS", SUBTILIS_VFP_INSTR_FMACS, NULL, },
	{ "FMDRR", SUBTILIS_VFP_INSTR_FMDRR, NULL, },
	{ "FML", SUBTILIS_FPA_INSTR_FML, NULL, },
	{ "FMRRD", SUBTILIS_VFP_INSTR_FMRRD, NULL, },
	{ "FMRRS", SUBTILIS_VFP_INSTR_FMRRS, NULL, },
	{ "FMRS", SUBTILIS_VFP_INSTR_FMRS, NULL, },
	{ "FMRX", SUBTILIS_VFP_INSTR_FMRX, NULL },
	{ "FMSCD", SUBTILIS_VFP_INSTR_FMSCD, NULL, },
	{ "FMSCS", SUBTILIS_VFP_INSTR_FMSCS, NULL, },
	{ "FMSR", SUBTILIS_VFP_INSTR_FMSR, NULL, },
	{ "FMSRR", SUBTILIS_VFP_INSTR_FMSRR, NULL, },
	{ "FMULD", SUBTILIS_VFP_INSTR_FMULD, NULL, },
	{ "FMULS", SUBTILIS_VFP_INSTR_FMULS, NULL, },
	{ "FMXR", SUBTILIS_VFP_INSTR_FMXR, NULL, },
	{ "FMSTAT", SUBTILIS_VFP_INSTR_FMRX, NULL, },
	{ "FNEGD", SUBTILIS_VFP_INSTR_FNEGD, NULL, },
	{ "FNEGS", SUBTILIS_VFP_INSTR_FNEGS, NULL, },
	{ "FNMACD", SUBTILIS_VFP_INSTR_FNMACD, NULL, },
	{ "FNMACS", SUBTILIS_VFP_INSTR_FNMACS, NULL, },
	{ "FNMSCD", SUBTILIS_VFP_INSTR_FNMSCD, NULL, },
	{ "FNMSCS", SUBTILIS_VFP_INSTR_FNMSCS, NULL, },
	{ "FNMULD", SUBTILIS_VFP_INSTR_FNMULD, NULL, },
	{ "FNMULS", SUBTILIS_VFP_INSTR_FNMULS, NULL, },
	{ "FRD", SUBTILIS_FPA_INSTR_FRD, NULL, },
	{ "FSITOD", SUBTILIS_VFP_INSTR_FSITOD, NULL, },
	{ "FSITOS", SUBTILIS_VFP_INSTR_FSITOS, NULL, },
	{ "FSQRTD", SUBTILIS_VFP_INSTR_FSQRTD, NULL, },
	{ "FSQRTS", SUBTILIS_VFP_INSTR_FSQRTS, NULL, },
	{ "FSTD", SUBTILIS_VFP_INSTR_FSTD, NULL, },
	{ "FSTS", SUBTILIS_VFP_INSTR_FSTS, NULL, },
	{ "FSUBD", SUBTILIS_VFP_INSTR_FSUBD, NULL, },
	{ "FSUBS", SUBTILIS_VFP_INSTR_FSUBS, NULL, },
	{ "FTOSID", SUBTILIS_VFP_INSTR_FTOSID, NULL },
	{ "FTOSIS", SUBTILIS_VFP_INSTR_FTOSIS, NULL },
	{ "FTOSIZD", SUBTILIS_VFP_INSTR_FTOSIZD, NULL },
	{ "FTOSIZS", SUBTILIS_VFP_INSTR_FTOSIZS, NULL },
	{ "FTOUID", SUBTILIS_VFP_INSTR_FTOUID, NULL, },
	{ "FTOUIS", SUBTILIS_VFP_INSTR_FTOUIS, NULL, },
	{ "FTOUIZD", SUBTILIS_VFP_INSTR_FTOUIZD, NULL, },
	{ "FTOUIZS", SUBTILIS_VFP_INSTR_FTOUIZS, NULL, },
	{ "FUITOD", SUBTILIS_VFP_INSTR_FUITOD, NULL, },
	{ "FUITOS", SUBTILIS_VFP_INSTR_FUITOS, NULL, },
};

static const subtilis_arm_ass_mnemomic_t l_mnem[] = {
	{ "LDF", SUBTILIS_FPA_INSTR_LDF, NULL, },
	{ "LDM", SUBTILIS_ARM_INSTR_LDM, NULL, },
	{ "LDR", SUBTILIS_ARM_INSTR_LDR, NULL, },
	{ "LGN", SUBTILIS_FPA_INSTR_LGN, NULL, },
	{ "LOG", SUBTILIS_FPA_INSTR_LOG, NULL, },
};

static const subtilis_arm_ass_mnemomic_t m_mnem[] = {
	{ "MLA", SUBTILIS_ARM_INSTR_MLA, NULL, },
	{ "MNF", SUBTILIS_FPA_INSTR_MNF, NULL, },
	{ "MOV", SUBTILIS_ARM_INSTR_MOV, NULL, },
	{ "MRS", SUBTILIS_ARM_INSTR_MRS, NULL, },
	{ "MSR", SUBTILIS_ARM_INSTR_MSR, NULL, },
	{ "MUF", SUBTILIS_FPA_INSTR_MUF, NULL, },
	{ "MUL", SUBTILIS_ARM_INSTR_MUL, NULL, },
	{ "MVF", SUBTILIS_FPA_INSTR_MVF, NULL, },
	{ "MVN", SUBTILIS_ARM_INSTR_MVN, NULL, },
};

static const subtilis_arm_ass_mnemomic_t n_mnem[] = {
	{ "NRM", SUBTILIS_FPA_INSTR_NRM, NULL, },
};

static const subtilis_arm_ass_mnemomic_t o_mnem[] = {
	{ "ORR", SUBTILIS_ARM_INSTR_ORR, NULL, },
};

static const subtilis_arm_ass_mnemomic_t p_mnem[] = {
	{ "POL", SUBTILIS_FPA_INSTR_POL, NULL, },
	{ "POW", SUBTILIS_FPA_INSTR_POW, NULL, },
};

static const subtilis_arm_ass_mnemomic_t q_mnem[] = {
	{ "QADD16", SUBTILIS_ARM_SIMD_QADD16, NULL, },
	{ "QADD8", SUBTILIS_ARM_SIMD_QADD8, NULL, },
	{ "QADDSUBX", SUBTILIS_ARM_SIMD_QADDSUBX, NULL, },
	{ "QSUB16", SUBTILIS_ARM_SIMD_QSUB16, NULL, },
	{ "QSUB8", SUBTILIS_ARM_SIMD_QSUB8, NULL, },
	{ "QSUBADDX", SUBTILIS_ARM_SIMD_QSUBADDX, NULL, },
};

static const subtilis_arm_ass_mnemomic_t r_mnem[] = {
	{ "RDF", SUBTILIS_FPA_INSTR_RDF, NULL, },
	{ "RFS", SUBTILIS_FPA_INSTR_RFS, NULL, },
	{ "RMF", SUBTILIS_FPA_INSTR_RMF, NULL, },
	{ "RND", SUBTILIS_FPA_INSTR_RND, NULL, },
	{ "RPW", SUBTILIS_FPA_INSTR_RPW, NULL, },
	{ "RSB", SUBTILIS_ARM_INSTR_RSB, NULL, },
	{ "RSC", SUBTILIS_ARM_INSTR_RSC, NULL, },
	{ "RSF", SUBTILIS_FPA_INSTR_RSF, NULL, },
};

static const subtilis_arm_ass_mnemomic_t s_mnem[] = {
	{ "SADD16", SUBTILIS_ARM_SIMD_SADD16, NULL, },
	{ "SADD8", SUBTILIS_ARM_SIMD_SADD8, NULL, },
	{ "SADDSUBX", SUBTILIS_ARM_SIMD_SADDSUBX, NULL, },
	{ "SBC", SUBTILIS_ARM_INSTR_SBC, NULL, },
	{ "SHADD16", SUBTILIS_ARM_SIMD_SHADD16, NULL, },
	{ "SHADD8", SUBTILIS_ARM_SIMD_SHADD8, NULL, },
	{ "SHADDSUBX", SUBTILIS_ARM_SIMD_SHADDSUBX, NULL, },
	{ "SHSUB16", SUBTILIS_ARM_SIMD_SHSUB16, NULL, },
	{ "SHSUB8", SUBTILIS_ARM_SIMD_SHSUB8, NULL, },
	{ "SHSUBADDX", SUBTILIS_ARM_SIMD_SHSUBADDX, NULL, },
	{ "SIN", SUBTILIS_FPA_INSTR_SIN, NULL, },
	{ "SQT", SUBTILIS_FPA_INSTR_SQT, NULL, },
	{ "SSUB16", SUBTILIS_ARM_SIMD_SSUB16, NULL, },
	{ "SSUB8", SUBTILIS_ARM_SIMD_SSUB8, NULL, },
	{ "SSUBADDX", SUBTILIS_ARM_SIMD_SSUBADDX, NULL, },
	{ "STF", SUBTILIS_FPA_INSTR_STF, NULL, },
	{ "STM", SUBTILIS_ARM_INSTR_STM, NULL, },
	{ "STR", SUBTILIS_ARM_INSTR_STR, NULL, },
	{ "SUB", SUBTILIS_ARM_INSTR_SUB, NULL, },
	{ "SUF", SUBTILIS_FPA_INSTR_SUF, NULL, },
	{ "SWI", SUBTILIS_ARM_INSTR_SWI, NULL, },
	{ "SXTB", SUBTILIS_ARM_INSTR_SXTB, NULL, },
	{ "SXTB16", SUBTILIS_ARM_INSTR_SXTB16, NULL, },
	{ "SXTH", SUBTILIS_ARM_INSTR_SXTH, NULL, },
};

static const subtilis_arm_ass_mnemomic_t t_mnem[] = {
	{ "TAN", SUBTILIS_FPA_INSTR_TAN, NULL, },
	{ "TEQ", SUBTILIS_ARM_INSTR_TEQ, NULL, },
	{ "TST", SUBTILIS_ARM_INSTR_TST, NULL, },
};

static const subtilis_arm_ass_mnemomic_t u_mnem[] = {
	{ "UADD16", SUBTILIS_ARM_SIMD_UADD16, NULL, },
	{ "UADD8", SUBTILIS_ARM_SIMD_UADD8, NULL, },
	{ "UADDSUBX", SUBTILIS_ARM_SIMD_UADDSUBX, NULL, },
	{ "UHADD16", SUBTILIS_ARM_SIMD_UHADD16, NULL, },
	{ "UHADD8", SUBTILIS_ARM_SIMD_UHADD8, NULL, },
	{ "UHADDSUBX", SUBTILIS_ARM_SIMD_UHADDSUBX, NULL, },
	{ "UHSUB16", SUBTILIS_ARM_SIMD_UHSUB16, NULL, },
	{ "UHSUB8", SUBTILIS_ARM_SIMD_UHSUB8, NULL, },
	{ "UHSUBADDX", SUBTILIS_ARM_SIMD_UHSUBADDX, NULL, },
	{ "UQADD16", SUBTILIS_ARM_SIMD_UQADD16, NULL, },
	{ "UQADD8", SUBTILIS_ARM_SIMD_UQADD8, NULL, },
	{ "UQADDSUBX", SUBTILIS_ARM_SIMD_UQADDSUBX, NULL, },
	{ "UQSUB16", SUBTILIS_ARM_SIMD_UQSUB16, NULL, },
	{ "UQSUB8", SUBTILIS_ARM_SIMD_UQSUB8, NULL, },
	{ "UQSUBADDX", SUBTILIS_ARM_SIMD_UQSUBADDX, NULL, },
	{ "URD", SUBTILIS_FPA_INSTR_URD, NULL, },
	{ "USUB16", SUBTILIS_ARM_SIMD_USUB16, NULL, },
	{ "USUB8", SUBTILIS_ARM_SIMD_USUB8, NULL, },
	{ "USUBADDX", SUBTILIS_ARM_SIMD_USUBADDX, NULL, },
};

static const subtilis_arm_ass_mnemomic_t w_mnem[] = {
	{ "WFS", SUBTILIS_FPA_INSTR_WFS, NULL, },
};

/* clang-format on */

struct subtilis_arm_ass_mnem_key_t_ {
	const subtilis_arm_ass_mnemomic_t *mnems;
	size_t count;
	size_t max_length;
};

typedef struct subtilis_arm_ass_mnem_key_t_ subtilis_arm_ass_mnem_key_t;

/* clang-format off */

static const subtilis_arm_ass_mnem_key_t keyword_map[] = {
	{ a_mnem, sizeof(a_mnem) / sizeof(a_mnem[0]), 3},
	{ b_mnem, sizeof(b_mnem) / sizeof(b_mnem[0]), 3},
	{ c_mnem, sizeof(c_mnem) / sizeof(c_mnem[0]), 4},
	{ d_mnem, sizeof(d_mnem) / sizeof(d_mnem[0]), 3},
	{ e_mnem, sizeof(e_mnem) / sizeof(e_mnem[0]), 3},
	{ f_mnem, sizeof(f_mnem) / sizeof(f_mnem[0]), 7},
	{ NULL, 0, 0 }, /* G */
	{ NULL, 0, 0 }, /* H */
	{ NULL, 0, 0 }, /* I */
	{ NULL, 0, 0 }, /* J */
	{ NULL, 0, 0 }, /* K */
	{ l_mnem, sizeof(l_mnem) / sizeof(l_mnem[0]), 3},
	{ m_mnem, sizeof(m_mnem) / sizeof(m_mnem[0]), 3},
	{ n_mnem, sizeof(n_mnem) / sizeof(n_mnem[0]), 3},
	{ o_mnem, sizeof(o_mnem) / sizeof(o_mnem[0]), 3},
	{ p_mnem, sizeof(p_mnem) / sizeof(p_mnem[0]), 3},
	{ q_mnem, sizeof(q_mnem) / sizeof(q_mnem[0]), 8},
	{ r_mnem, sizeof(r_mnem) / sizeof(r_mnem[0]), 3},
	{ s_mnem, sizeof(s_mnem) / sizeof(s_mnem[0]), 9},
	{ t_mnem, sizeof(t_mnem) / sizeof(t_mnem[0]), 3},
	{ u_mnem, sizeof(u_mnem) / sizeof(u_mnem[0]), 9},
	{ NULL, 0, 0 }, /* V */
	{ w_mnem, sizeof(w_mnem) / sizeof(w_mnem[0]), 3},
	{ NULL, 0, 0 }, /* X */
	{ NULL, 0, 0 }, /* Y */
	{ NULL, 0, 0 }, /* Z */
};

/* clang-format on */

static void prv_parse_label(subtilis_arm_ass_context_t *c, const char *name,
			    subtilis_error_t *err)
{
	size_t index;
	const char *tbuf;
	char *name_cpy;

	name_cpy = malloc(strlen(name) + 1);
	if (!name_cpy) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name_cpy, name);

	if (subtilis_string_pool_find(c->label_pool, name, &index)) {
		if (subtilis_bitset_isset(&c->pending_labels, index)) {
			subtilis_bitset_clear(&c->pending_labels, index);
		} else {
			subtilis_error_set_already_defined(
			    err, name, c->l->stream->name, c->l->line);
			goto cleanup;
		}
	} else {
		index = subtilis_string_pool_register(c->label_pool, name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_arm_section_add_label(c->arm_s, index, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ":")) {
		subtilis_error_set_label_missing_colon(
		    err, name_cpy, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);

cleanup:

	free(name_cpy);
}

static void prv_get_op2(subtilis_arm_ass_context_t *c, subtilis_arm_op2_t *op2,
			subtilis_error_t *err)
{
	uint32_t encoded;
	int32_t num;
	subtilis_arm_exp_val_t *val;
	const char *tbuf;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if ((val->type == SUBTILIS_ARM_EXP_TYPE_INT) ||
	    (val->type == SUBTILIS_ARM_EXP_TYPE_REAL)) {
		if (val->type == SUBTILIS_ARM_EXP_TYPE_REAL)
			num = (int32_t)val->val.real;
		else
			num = val->val.integer;
		if (!subtilis_arm_encode_imm(num, &encoded)) {
			subtilis_error_set_ass_integer_encode(
			    err, num, c->l->stream->name, c->l->line);
			goto cleanup;
		}
		op2->type = SUBTILIS_ARM_OP2_I32;
		op2->op.integer = encoded;
		goto cleanup;
	}

	if (val->type != SUBTILIS_ARM_EXP_TYPE_REG) {
		subtilis_error_set_expected(err, "register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	op2->op.reg = val->val.reg;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type != SUBTILIS_TOKEN_OPERATOR) {
		op2->type = SUBTILIS_ARM_OP2_REG;
		goto cleanup;
	}

	op2->type = SUBTILIS_ARM_OP2_SHIFTED;

	tbuf = subtilis_token_get_text(c->t);
	if (strcmp(tbuf, ",")) {
		op2->type = SUBTILIS_ARM_OP2_REG;
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_keyword_expected(
		    err, tbuf, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	switch (c->t->tok.keyword.type) {
	case SUBTILIS_ARM_KEYWORD_ASR:
		op2->op.shift.type = SUBTILIS_ARM_SHIFT_ASR;
		break;
	case SUBTILIS_ARM_KEYWORD_LSL:
		op2->op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
		break;
	case SUBTILIS_ARM_KEYWORD_LSR:
		op2->op.shift.type = SUBTILIS_ARM_SHIFT_LSR;
		break;
	case SUBTILIS_ARM_KEYWORD_ROR:
		op2->op.shift.type = SUBTILIS_ARM_SHIFT_ROR;
		break;
	case SUBTILIS_ARM_KEYWORD_RRX:
		op2->op.shift.type = SUBTILIS_ARM_SHIFT_RRX;
		break;
	default:
		subtilis_error_set_expected(
		    err, "ASR or LSL or LSR or ROR or RRX", tbuf,
		    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_arm_exp_val_free(val);
	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		op2->op.shift.shift.integer = val->val.integer;
		op2->op.shift.shift_reg = false;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		op2->op.shift.shift.reg = val->val.reg;
		op2->op.shift.shift_reg = true;
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	subtilis_arm_exp_val_free(val);
}

static subtilis_arm_reg_t prv_get_reg(subtilis_arm_ass_context_t *c,
				      subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_REG) {
		subtilis_error_set_expected(err, "register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_fpa_reg(subtilis_arm_ass_context_t *c,
					  subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_FREG) {
		subtilis_error_set_expected(err, "floating point register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_vfp_sreg(subtilis_arm_ass_context_t *c,
					   subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_SREG) {
		subtilis_error_set_expected(err, "vfp S register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_vfp_dreg(subtilis_arm_ass_context_t *c,
					   subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_DREG) {
		subtilis_error_set_expected(err, "vfp D register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_vfp_sysreg(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_SYSREG) {
		subtilis_error_set_expected(err, "FPSCR or FPEXC or FPSID",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_dest_flagsreg(subtilis_arm_ass_context_t *c,
						subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_FLAGS_REG ||
	    val->val.reg <= SUBTILIS_ARM_EXP_SPSR_REG) {
		subtilis_error_set_expected(err, "CPSR_[cxfs] or SPSR_[cxfs]",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_get_src_flagsreg(subtilis_arm_ass_context_t *c,
					       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t reg;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_FLAGS_REG ||
	    val->val.reg > SUBTILIS_ARM_EXP_SPSR_REG) {
		subtilis_error_set_expected(err, "CPSR or SPSR",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		reg = SIZE_MAX;
	} else {
		reg = val->val.reg;
	}

	subtilis_arm_exp_val_free(val);

	return reg;
}

static subtilis_arm_reg_t prv_parse_reg(subtilis_arm_ass_context_t *c,
					subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_reg_t reg;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	tbuf = subtilis_token_get_text(c->t);
	reg = subtilis_arm_exp_parse_reg(c, tbuf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (reg == SIZE_MAX) {
		subtilis_error_set_ass_bad_reg(err, tbuf, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	return reg;
}

static void prv_parse_branch(subtilis_arm_ass_context_t *c, const char *name,
			     subtilis_arm_instr_type_t itype,
			     subtilis_arm_ccode_type_t ccode,
			     subtilis_error_t *err)
{
	subtilis_arm_instr_t *instr;
	subtilis_arm_br_instr_t *br;
	size_t index;
	subtilis_arm_exp_val_t *val;
	size_t name_len = strlen(name);
	bool link = ((name_len == 2) || (name_len == 4)) &&
		    (name[1] == 'l' || name[1] == 'L');

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_ID) {
		subtilis_error_set_expected(err, "label", name,
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	name = subtilis_buffer_get_string(&val->val.buf);
	if (!subtilis_string_pool_find(c->label_pool, name, &index)) {
		index = subtilis_string_pool_register(c->label_pool, name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_bitset_set(&c->pending_labels, index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr =
	    subtilis_arm_section_add_instr(c->arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	br = &instr->operands.br;
	br->ccode = ccode;
	br->link = link;
	br->local = true;
	br->link_type = SUBTILIS_ARM_BR_LINK_VOID;
	br->target.label = index;

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_arm_data(subtilis_arm_ass_context_t *c,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode, bool status,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_op2_t op2;
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	prv_get_op2(c, &op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = op1;
	datai->op2 = op2;
	datai->status = status;
}

static void prv_parse_arm_mul(subtilis_arm_ass_context_t *c,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode, bool status,
			      subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_mul_instr_t *mul;
	subtilis_arm_reg_t op3 = 0;
	char buffer[32];

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (dest == 15) {
		sprintf(buffer, "R%zu", dest);
		subtilis_error_set_ass_bad_reg(err, buffer, c->l->stream->name,
					       c->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (dest == op1) {
		sprintf(buffer, "R%zu", dest);
		subtilis_error_set_ass_bad_reg(err, buffer, c->l->stream->name,
					       c->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op2 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (itype == SUBTILIS_ARM_INSTR_MLA) {
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) &&
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		op3 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	mul = &instr->operands.mul;
	mul->ccode = ccode;
	mul->dest = dest;
	mul->rm = op1;
	mul->rs = op2;
	mul->rn = op3;
	mul->status = status;
}

static void prv_parse_arm_2_arg(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode, bool status,
				subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_op2_t op2;
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	prv_get_op2(c, &op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	datai->dest = dest;
	datai->op1 = dest;
	datai->op2 = op2;
	datai->status = status;
}

static void prv_parse_cmp(subtilis_arm_ass_context_t *c,
			  subtilis_arm_instr_type_t itype,
			  subtilis_arm_ccode_type_t ccode, bool status,
			  bool teqp, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_op2_t op2;
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	prv_get_op2(c, &op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	datai = &instr->operands.data;
	datai->ccode = ccode;
	if (teqp)
		datai->dest = 15;
	else
		datai->dest = 0;
	datai->op1 = dest;
	datai->op2 = op2;
	datai->status = status;
}

static void prv_parse_swi(subtilis_arm_ass_context_t *c,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_error_t *err)
{
	const char *swi_name;
	subtilis_arm_exp_val_t *val;
	size_t code;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		code = (size_t)val->val.integer;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_STRING) {
		swi_name = subtilis_buffer_get_string(&val->val.buf);
		code = c->sys_trans(swi_name);
		if (code == SIZE_MAX) {
			subtilis_error_set_sys_call_unknown(
			    err, swi_name, c->l->stream->name, c->l->line);
			goto cleanup;
		}
	} else {
		subtilis_error_set_expected(err, "string or integer expected",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_arm_add_swi(c->arm_s, ccode, code, 0, 0, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static subtilis_arm_shift_type_t prv_get_st_shift(subtilis_arm_ass_context_t *c,
						  subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_shift_type_t st = SUBTILIS_ARM_SHIFT_ASR;

	tbuf = subtilis_token_get_text(c->t);
	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return st;
	if (c->t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_keyword_expected(
		    err, tbuf, c->l->stream->name, c->l->line);
		return st;
	}

	switch (c->t->tok.keyword.type) {
	case SUBTILIS_ARM_KEYWORD_ASR:
		st = SUBTILIS_ARM_SHIFT_ASR;
		break;
	case SUBTILIS_ARM_KEYWORD_LSL:
		st = SUBTILIS_ARM_SHIFT_LSL;
		break;
	case SUBTILIS_ARM_KEYWORD_LSR:
		st = SUBTILIS_ARM_SHIFT_LSR;
		break;
	case SUBTILIS_ARM_KEYWORD_ROR:
		st = SUBTILIS_ARM_SHIFT_ROR;
		break;
	case SUBTILIS_ARM_KEYWORD_RRX:
		st = SUBTILIS_ARM_SHIFT_RRX;
		break;
	default:
		subtilis_error_set_expected(err, "shift keyword", tbuf,
					    c->l->stream->name, c->l->line);
	}

	return st;
}

static subtilis_arm_exp_val_t *prv_get_offset(subtilis_arm_ass_context_t *c,
					      bool *subtract,
					      subtilis_error_t *err)
{
	const char *tbuf;

	if (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(c->t);
		if (strcmp(tbuf, "-")) {
			subtilis_error_set_expected(
			    err, "-, register or integer", tbuf,
			    c->l->stream->name, c->l->line);
			return NULL;
		}
		*subtract = true;
		return subtilis_arm_exp_val_get(c, err);
	}
	return subtilis_arm_exp_pri7(c, err);
}

static void prv_parse_int_offset(subtilis_arm_ass_context_t *c,
				 subtilis_arm_exp_val_t *val,
				 subtilis_arm_op2_t *offset, bool *subtract,
				 subtilis_error_t *err)
{
	offset->type = SUBTILIS_ARM_OP2_I32;
	if (val->val.integer < 0) {
		*subtract = true;
		offset->op.integer = -val->val.integer;
	} else {
		offset->op.integer = val->val.integer;
	}
	if (offset->op.integer > 4095)
		subtilis_error_set_ass_bad_offset(
		    err, val->val.integer, c->l->stream->name, c->l->line);
}

static void prv_parse_shift_offset(subtilis_arm_ass_context_t *c,
				   subtilis_arm_exp_val_t *val,
				   subtilis_arm_op2_t *offset,
				   subtilis_error_t *err)
{
	offset->op.shift.reg = val->val.reg;
	offset->type = SUBTILIS_ARM_OP2_SHIFTED;
	offset->op.shift.type = prv_get_st_shift(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		offset->op.shift.shift_reg = false;
		offset->op.shift.shift.integer = val->val.integer;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		offset->op.shift.shift_reg = true;
		offset->op.shift.shift.reg = val->val.reg;
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
	}

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran_pre_off(subtilis_arm_ass_context_t *c,
				    subtilis_arm_instr_type_t itype,
				    subtilis_arm_ccode_type_t ccode,
				    subtilis_arm_reg_t dest,
				    subtilis_arm_reg_t base, bool byte,
				    subtilis_arm_op2_t offset, bool subtract,
				    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	bool write_back = false;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, "!"))) {
		write_back = true;

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = true;
	stran->write_back = write_back;
	stran->byte = byte;
	stran->subtract = subtract;
}

static void prv_parse_stran_misc_int_offset(subtilis_arm_ass_context_t *c,
					    subtilis_arm_exp_val_t *val,
					    int8_t *offset, bool *subtract,
					    subtilis_error_t *err)
{
	int32_t off;

	if (val->val.integer < 0) {
		*subtract = true;
		off = -val->val.integer;
	} else {
		off = val->val.integer;
	}
	if (off > 255)
		subtilis_error_set_ass_bad_offset(
		    err, val->val.integer, c->l->stream->name, c->l->line);
	*offset = off;
}

static void
prv_stran_misc_set_type(subtilis_arm_stran_misc_instr_t *stran_misc,
			const subtilis_arm_ass_modifiers_t *modifiers,
			subtilis_error_t *err)
{
	if (modifiers->byte && modifiers->status) {
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_SB;
	} else if (modifiers->hword) {
		if (modifiers->status)
			stran_misc->type = SUBTILIS_ARM_STRAN_MISC_SH;
		else
			stran_misc->type = SUBTILIS_ARM_STRAN_MISC_H;
	} else if (modifiers->dword) {
		stran_misc->type = SUBTILIS_ARM_STRAN_MISC_D;
	} else {
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_stran_misc_zero(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				const subtilis_arm_ass_modifiers_t *modifiers,
				subtilis_arm_reg_t base, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_misc_instr_t *stran_misc;
	bool write_back = false;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, "!"))) {
		write_back = true;

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran_misc = &instr->operands.stran_misc;

	prv_stran_misc_set_type(stran_misc, modifiers, err);
	if (err->type)
		return;

	stran_misc->ccode = ccode;
	stran_misc->dest = dest;
	stran_misc->base = base;
	stran_misc->reg_offset = false;
	stran_misc->offset.imm = 0;
	stran_misc->pre_indexed = true;
	stran_misc->write_back = write_back;
	stran_misc->subtract = false;
}

static void prv_stran_misc_pre(subtilis_arm_ass_context_t *c,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			       const subtilis_arm_ass_modifiers_t *modifiers,
			       subtilis_arm_exp_val_t *val,
			       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_misc_instr_t *stran_misc;
	bool reg_offset;
	int8_t int_offset = 0;
	subtilis_arm_reg_t reg = SIZE_MAX;
	bool write_back = false;
	bool subtract = false;

	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		prv_parse_stran_misc_int_offset(c, val, &int_offset, &subtract,
						err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		reg_offset = false;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		reg_offset = true;
		reg = val->val.reg;
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "]"))) {
		subtilis_error_set_expected(err, "]", tbuf, c->l->stream->name,
					    c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && (!strcmp(tbuf, "!"))) {
		write_back = true;

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	stran_misc = &instr->operands.stran_misc;

	prv_stran_misc_set_type(stran_misc, modifiers, err);
	if (err->type)
		goto cleanup;

	stran_misc->ccode = ccode;
	stran_misc->dest = dest;
	stran_misc->base = base;
	stran_misc->reg_offset = reg_offset;
	if (reg_offset)
		stran_misc->offset.reg = reg;
	else
		stran_misc->offset.imm = int_offset;
	stran_misc->pre_indexed = true;
	stran_misc->write_back = write_back;
	stran_misc->subtract = subtract;

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran_pre(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				subtilis_arm_reg_t base,
				const subtilis_arm_ass_modifiers_t *modifiers,
				subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_op2_t offset;
	const char *tbuf;
	bool subtract = false;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = prv_get_offset(c, &subtract, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (itype == SUBTILIS_ARM_STRAN_MISC_LDR ||
	    itype == SUBTILIS_ARM_STRAN_MISC_STR) {
		prv_stran_misc_pre(c, itype, ccode, dest, base, modifiers, val,
				   err);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		prv_parse_int_offset(c, val, &offset, &subtract, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, "]"))) {
			subtilis_error_set_expected(
			    err, "]", tbuf, c->l->stream->name, c->l->line);
			goto cleanup;
		}
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		if (c->t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(err, ", or ]", tbuf,
						    c->l->stream->name,
						    c->l->line);
			goto cleanup;
		}
		if (!strcmp(tbuf, "]")) {
			offset.op.reg = val->val.reg;
			offset.type = SUBTILIS_ARM_OP2_REG;
		} else if (!strcmp(tbuf, ",")) {
			prv_parse_shift_offset(c, val, &offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
			    (strcmp(tbuf, "]"))) {
				subtilis_error_set_expected(err, "]", tbuf,
							    c->l->stream->name,
							    c->l->line);
				goto cleanup;
			}
		} else {
			subtilis_error_set_expected(err, ", or ]", tbuf,
						    c->l->stream->name,
						    c->l->line);
			goto cleanup;
		}
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_parse_stran_pre_off(c, itype, ccode, dest, base, modifiers->byte,
				offset, subtract, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

/* clang-format off */
static void prv_stran_misc_post(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				subtilis_arm_reg_t base,
				const subtilis_arm_ass_modifiers_t *modifiers,
				subtilis_arm_exp_val_t *val,
				subtilis_error_t *err)
/* clang-format on */
{
	int8_t int_offset;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_misc_instr_t *stran_misc;
	subtilis_arm_reg_t reg;
	bool reg_offset;
	bool subtract = false;

	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		prv_parse_stran_misc_int_offset(c, val, &int_offset, &subtract,
						err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		reg_offset = false;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		reg_offset = true;
		reg = val->val.reg;
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	stran_misc = &instr->operands.stran_misc;

	prv_stran_misc_set_type(stran_misc, modifiers, err);
	if (err->type)
		goto cleanup;

	stran_misc->ccode = ccode;
	stran_misc->dest = dest;
	stran_misc->base = base;
	stran_misc->reg_offset = reg_offset;
	if (reg_offset)
		stran_misc->offset.reg = reg;
	else
		stran_misc->offset.imm = int_offset;
	stran_misc->pre_indexed = false;
	stran_misc->write_back = true;
	stran_misc->subtract = subtract;

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran_post(subtilis_arm_ass_context_t *c,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t base,
				 const subtilis_arm_ass_modifiers_t *modifiers,
				 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *val;
	subtilis_arm_op2_t offset;
	subtilis_arm_instr_t *instr;
	subtilis_arm_stran_instr_t *stran;
	bool subtract = false;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		if (itype == SUBTILIS_ARM_STRAN_MISC_LDR ||
		    itype == SUBTILIS_ARM_STRAN_MISC_STR) {
			prv_stran_misc_zero(c, itype, ccode, dest, modifiers,
					    base, err);
		} else {
			offset.type = SUBTILIS_ARM_OP2_I32;
			offset.op.integer = 0;
			prv_parse_stran_pre_off(c, itype, ccode, dest, base,
						modifiers->byte, offset, false,
						err);
		}
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = prv_get_offset(c, &subtract, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (itype == SUBTILIS_ARM_STRAN_MISC_LDR ||
	    itype == SUBTILIS_ARM_STRAN_MISC_STR) {
		prv_stran_misc_post(c, itype, ccode, dest, base, modifiers, val,
				    err);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		prv_parse_int_offset(c, val, &offset, &subtract, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    !strcmp(tbuf, ",")) {
			prv_parse_shift_offset(c, val, &offset, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			offset.op.reg = val->val.reg;
			offset.type = SUBTILIS_ARM_OP2_REG;
		}
	} else {
		subtilis_error_set_expected(err, "integer or register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	stran = &instr->operands.stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = false;
	stran->write_back = true;
	stran->byte = modifiers->byte;
	stran->subtract = subtract;

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran(subtilis_arm_ass_context_t *c,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    const subtilis_arm_ass_modifiers_t *modifiers,
			    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	const char *tbuf;
	char buffer[32];

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (modifiers->dword && (dest & 1)) {
		sprintf(buffer, "R%zu", dest);
		subtilis_error_set_ass_bad_reg(err, buffer, c->l->stream->name,
					       c->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "["))) {
		subtilis_error_set_expected(err, "[", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	base = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		if (!strcmp(tbuf, "]")) {
			prv_parse_stran_post(c, itype, ccode, dest, base,
					     modifiers, err);
			return;
		}
		if (!strcmp(tbuf, ",")) {
			prv_parse_stran_pre(c, itype, ccode, dest, base,
					    modifiers, err);
			return;
		}
	}

	subtilis_error_set_expected(err, ", or ]", tbuf, c->l->stream->name,
				    c->l->line);
}

static void prv_parse_mtran_range(subtilis_arm_ass_context_t *c, size_t reg1,
				  size_t *reg_list, subtilis_error_t *err)
{
	subtilis_arm_reg_t reg2;
	size_t i;

	reg2 = prv_parse_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (reg1 >= reg2) {
		subtilis_error_set_ass_bad_range(
		    err, reg1, reg2, c->l->stream->name, c->l->line);
		return;
	}

	for (i = reg1 + 1; i <= reg2; i++)
		*reg_list |= 1 << i;
}

static void prv_parse_mtran(subtilis_arm_ass_context_t *c,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_arm_mtran_type_t mtran_type,
			    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	const char *tbuf;
	subtilis_arm_reg_t reg1;
	size_t reg_list = 0;
	bool write_back = false;
	bool status = false;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "!")) {
		write_back = true;
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
	}

	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "{")) {
		subtilis_error_set_expected(err, "{", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	do {
		reg1 = prv_parse_reg(c, err);
		reg_list |= 1 << reg1;
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
		if (c->t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(err, "-, } or ,", tbuf,
						    c->l->stream->name,
						    c->l->line);
			return;
		}

		if (!strcmp(tbuf, ","))
			continue;
		if (!strcmp(tbuf, "-")) {
			prv_parse_mtran_range(c, reg1, &reg_list, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			tbuf = subtilis_token_get_text(c->t);
			if (c->t->type != SUBTILIS_TOKEN_OPERATOR) {
				subtilis_error_set_expected(err, "} or ,", tbuf,
							    c->l->stream->name,
							    c->l->line);
				return;
			}
			if (!strcmp(tbuf, ","))
				continue;
			if (!strcmp(tbuf, "}"))
				break;

			subtilis_error_set_expected(err, "} or ,", tbuf,
						    c->l->stream->name,
						    c->l->line);
			return;
		} else if (!strcmp(tbuf, "}")) {
			break;
		}

		subtilis_error_set_expected(err, "} or ,", tbuf,
					    c->l->stream->name, c->l->line);
		return;
	} while (true);

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "^")) {
		status = true;
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
	subtilis_arm_add_mtran(c->arm_s, itype, ccode, dest, reg_list,
			       mtran_type, write_back, status, err);
}

static void prv_parse_adr(subtilis_arm_ass_context_t *c,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	const char *str;
	size_t index;
	subtilis_arm_reg_t dest;
	const char *tbuf;
	const char *bad_reg = "R15";

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_REG) {
		subtilis_error_set_expected(err, "register",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	dest = val->val.reg;
	if (dest == 15) {
		subtilis_error_set_ass_bad_reg(err, bad_reg, c->l->stream->name,
					       c->l->line);
		goto cleanup;
	}

	subtilis_arm_exp_val_free(val);
	val = NULL;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		goto cleanup;
	}

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_ID) {
		subtilis_error_set_expected(err, "label",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	str = subtilis_buffer_get_string(&val->val.buf);
	if (!subtilis_string_pool_find(c->label_pool, str, &index)) {
		index = subtilis_string_pool_register(c->label_pool, str, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_bitset_set(&c->pending_labels, index, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	subtilis_add_adr(c->arm_s, ccode, dest, index, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_msr(subtilis_arm_ass_context_t *c,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_reg_t dest;
	const char *tbuf;
	uint32_t encoded;
	int32_t num;
	uint32_t flags;

	dest = prv_get_dest_flagsreg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	flags = dest & 0xfffffffe;

	if ((val->type == SUBTILIS_ARM_EXP_TYPE_INT) ||
	    (val->type == SUBTILIS_ARM_EXP_TYPE_REAL)) {
		if (val->type == SUBTILIS_ARM_EXP_TYPE_REAL)
			num = (int32_t)val->val.real;
		else
			num = val->val.integer;
		if (!subtilis_arm_encode_imm(num, &encoded)) {
			subtilis_error_set_ass_integer_encode(
			    err, num, c->l->stream->name, c->l->line);
			goto cleanup;
		}
		subtilis_arm_add_flags_imm(c->arm_s, SUBTILIS_ARM_INSTR_MSR,
					   ccode, dest & 1, flags, num, err);
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REG) {
		subtilis_arm_add_flags(c->arm_s, SUBTILIS_ARM_INSTR_MSR, ccode,
				       dest & 1, flags, val->val.reg, err);
	}

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_mrs(subtilis_arm_ass_context_t *c,
			  subtilis_arm_ccode_type_t ccode,
			  subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	const char *tbuf;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_src_flagsreg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_flags(c->arm_s, SUBTILIS_ARM_INSTR_MRS, ccode, op1, 0,
			       dest, err);
}

static void prv_parse_reg_only(subtilis_arm_ass_context_t *c,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	const char *tbuf;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op2 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_add_reg_only(c->arm_s, itype, ccode, dest, op1, op2, err);
}

static void prv_parse_signx(subtilis_arm_ass_context_t *c,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode,
			    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	const char *tbuf;
	subtilis_arm_exp_val_t *val = NULL;
	subtilis_arm_signx_rotate_t rotate = SUBTILIS_ARM_SIGNX_ROR_NONE;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
		if (c->t->type != SUBTILIS_TOKEN_KEYWORD ||
		    (c->t->tok.keyword.type != SUBTILIS_ARM_KEYWORD_ROR)) {
			subtilis_error_set_expected(
			    err, "ROR", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		val = subtilis_arm_exp_val_get(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (val->type != SUBTILIS_ARM_EXP_TYPE_INT) {
			subtilis_error_set_expected(err, "0, 8, 16 or 24", tbuf,
						    c->l->stream->name,
						    c->l->line);
			goto cleanup;
		}

		switch (val->val.integer) {
		case 0:
			break;
		case 8:
			rotate = SUBTILIS_ARM_SIGNX_ROR_8;
			break;
		case 16:
			rotate = SUBTILIS_ARM_SIGNX_ROR_16;
			break;
		case 24:
			rotate = SUBTILIS_ARM_SIGNX_ROR_24;
			break;
		default:
			subtilis_error_set_expected(err, "0, 8, 16 or 24", tbuf,
						    c->l->stream->name,
						    c->l->line);
			goto cleanup;
		}
	}

	subtilis_arm_add_signx(c->arm_s, itype, ccode, dest, op1, rotate, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_instruction(subtilis_arm_ass_context_t *c,
				  const char *name,
				  subtilis_arm_instr_type_t itype,
				  subtilis_arm_ccode_type_t ccode,
				  const subtilis_arm_ass_modifiers_t *modifiers,
				  subtilis_arm_mtran_type_t mtran_type,
				  subtilis_error_t *err)
{
	switch (itype) {
	case SUBTILIS_ARM_INSTR_B:
		prv_parse_branch(c, name, itype, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_MOV:
	case SUBTILIS_ARM_INSTR_MVN:
		prv_parse_arm_2_arg(c, itype, ccode, modifiers->status, err);
		break;
	case SUBTILIS_ARM_INSTR_AND:
	case SUBTILIS_ARM_INSTR_EOR:
	case SUBTILIS_ARM_INSTR_SUB:
	case SUBTILIS_ARM_INSTR_RSB:
	case SUBTILIS_ARM_INSTR_ADD:
	case SUBTILIS_ARM_INSTR_ADC:
	case SUBTILIS_ARM_INSTR_SBC:
	case SUBTILIS_ARM_INSTR_RSC:
	case SUBTILIS_ARM_INSTR_ORR:
	case SUBTILIS_ARM_INSTR_BIC:
		prv_parse_arm_data(c, itype, ccode, modifiers->status, err);
		break;
	case SUBTILIS_ARM_INSTR_MUL:
	case SUBTILIS_ARM_INSTR_MLA:
		prv_parse_arm_mul(c, itype, ccode, modifiers->status, err);
		break;
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
	case SUBTILIS_ARM_INSTR_CMP:
	case SUBTILIS_ARM_INSTR_CMN:
		prv_parse_cmp(c, itype, ccode, true, modifiers->teqp, err);
		break;
	case SUBTILIS_ARM_INSTR_SWI:
		prv_parse_swi(c, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_LDR:
	case SUBTILIS_ARM_INSTR_STR:
		if (modifiers->status || modifiers->dword || modifiers->hword) {
			if (itype == SUBTILIS_ARM_INSTR_LDR)
				itype = SUBTILIS_ARM_STRAN_MISC_LDR;
			else
				itype = SUBTILIS_ARM_STRAN_MISC_STR;
		}
		prv_parse_stran(c, itype, ccode, modifiers, err);
		break;
	case SUBTILIS_ARM_INSTR_LDM:
	case SUBTILIS_ARM_INSTR_STM:
		prv_parse_mtran(c, itype, ccode, mtran_type, err);
		break;
	case SUBTILIS_ARM_INSTR_ADR:
		prv_parse_adr(c, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_MRS:
		prv_parse_mrs(c, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_MSR:
		prv_parse_msr(c, ccode, err);
		break;
	case SUBTILIS_ARM_SIMD_QADD16:
	case SUBTILIS_ARM_SIMD_QADD8:
	case SUBTILIS_ARM_SIMD_QADDSUBX:
	case SUBTILIS_ARM_SIMD_QSUB16:
	case SUBTILIS_ARM_SIMD_QSUB8:
	case SUBTILIS_ARM_SIMD_QSUBADDX:
	case SUBTILIS_ARM_SIMD_SADD16:
	case SUBTILIS_ARM_SIMD_SADD8:
	case SUBTILIS_ARM_SIMD_SADDSUBX:
	case SUBTILIS_ARM_SIMD_SSUB16:
	case SUBTILIS_ARM_SIMD_SSUB8:
	case SUBTILIS_ARM_SIMD_SSUBADDX:
	case SUBTILIS_ARM_SIMD_SHADD16:
	case SUBTILIS_ARM_SIMD_SHADD8:
	case SUBTILIS_ARM_SIMD_SHADDSUBX:
	case SUBTILIS_ARM_SIMD_SHSUB16:
	case SUBTILIS_ARM_SIMD_SHSUB8:
	case SUBTILIS_ARM_SIMD_SHSUBADDX:
	case SUBTILIS_ARM_SIMD_UADD16:
	case SUBTILIS_ARM_SIMD_UADD8:
	case SUBTILIS_ARM_SIMD_UADDSUBX:
	case SUBTILIS_ARM_SIMD_USUB16:
	case SUBTILIS_ARM_SIMD_USUB8:
	case SUBTILIS_ARM_SIMD_USUBADDX:
	case SUBTILIS_ARM_SIMD_UHADD16:
	case SUBTILIS_ARM_SIMD_UHADD8:
	case SUBTILIS_ARM_SIMD_UHADDSUBX:
	case SUBTILIS_ARM_SIMD_UHSUB16:
	case SUBTILIS_ARM_SIMD_UHSUB8:
	case SUBTILIS_ARM_SIMD_UHSUBADDX:
	case SUBTILIS_ARM_SIMD_UQADD16:
	case SUBTILIS_ARM_SIMD_UQADD8:
	case SUBTILIS_ARM_SIMD_UQADDSUBX:
	case SUBTILIS_ARM_SIMD_UQSUB16:
	case SUBTILIS_ARM_SIMD_UQSUB8:
	case SUBTILIS_ARM_SIMD_UQSUBADDX:
		prv_parse_reg_only(c, itype, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_SXTB:
	case SUBTILIS_ARM_INSTR_SXTB16:
	case SUBTILIS_ARM_INSTR_SXTH:
		prv_parse_signx(c, itype, ccode, err);
		break;
	default:
		subtilis_error_set_not_supported(err, name, c->l->stream->name,
						 c->l->line);
		break;
	}
}

static bool prv_get_fpa_op2(subtilis_arm_ass_context_t *c,
			    subtilis_fpa_op2_t *op2, subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	bool immediate = false;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_FREG:
		op2->reg = val->val.reg;
		break;
	case SUBTILIS_ARM_EXP_TYPE_INT:
		if (!subtilis_fpa_encode_real((double)val->val.integer,
					      &op2->imm)) {
			subtilis_error_set_ass_bad_real_imm(
			    err, (double)val->val.integer, c->l->stream->name,
			    c->l->line);
			goto cleanup;
		}
		immediate = true;
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		if (!subtilis_fpa_encode_real(val->val.real, &op2->imm)) {
			subtilis_error_set_ass_bad_real_imm(
			    err, val->val.real, c->l->stream->name, c->l->line);
			goto cleanup;
		}
		immediate = true;
		break;
	default:
		subtilis_error_set_expected(
		    err, "floating point register or immediate",
		    subtilis_arm_exp_type_name(val), c->l->stream->name,
		    c->l->line);
		goto cleanup;
	}

cleanup:

	subtilis_arm_exp_val_free(val);

	return immediate;
}

static void prv_parse_fpa_data(subtilis_arm_ass_context_t *c,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_fpa_rounding_t rnd, size_t size,
			       bool dyadic, subtilis_error_t *err)
{
	subtilis_fpa_data_instr_t *datai;
	subtilis_arm_instr_t *instr;
	const char *tbuf;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai = &instr->operands.fpa_data;

	datai->dest = prv_get_fpa_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	if (dyadic) {
		datai->op1 = prv_get_fpa_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}
	}

	datai->immediate = prv_get_fpa_op2(c, &datai->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	datai->ccode = ccode;
	datai->rounding = rnd;
	datai->size = size;
}

static void prv_parse_fpa_cmp(subtilis_arm_ass_context_t *c,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_error_t *err)
{
	subtilis_fpa_cmp_instr_t *cmpi;
	subtilis_arm_instr_t *instr;
	const char *tbuf;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	cmpi = &instr->operands.fpa_cmp;

	cmpi->dest = prv_get_fpa_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	cmpi->immediate = prv_get_fpa_op2(c, &cmpi->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	cmpi->ccode = ccode;
}

static void prv_parse_fpa_flt(subtilis_arm_ass_context_t *c,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_fpa_rounding_t rnd, size_t size,
			      subtilis_error_t *err)
{
	subtilis_fpa_tran_instr_t *tran;
	subtilis_arm_instr_t *instr;
	const char *tbuf;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	tran = &instr->operands.fpa_tran;

	tran->dest = prv_get_fpa_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	tran->op2.reg = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran->immediate = false;
	tran->ccode = ccode;
	tran->size = size;
	tran->rounding = rnd;
}

static void prv_parse_fpa_fix(subtilis_arm_ass_context_t *c,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_fpa_rounding_t rnd,
			      subtilis_error_t *err)
{
	subtilis_fpa_tran_instr_t *tran;
	subtilis_arm_instr_t *instr;
	const char *tbuf;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	tran = &instr->operands.fpa_tran;

	tran->dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	tran->immediate = prv_get_fpa_op2(c, &tran->op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tran->ccode = ccode;
	tran->size = 0;
	tran->rounding = rnd;
}

static uint8_t prv_get_fpa_stran_offset(subtilis_arm_ass_context_t *c,
					bool *subtract, subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val = NULL;
	uint32_t absv = 0;
	bool sub = false;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_INT) {
		subtilis_error_set_expected(err, "integer",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	if (val->val.integer < 0) {
		sub = true;
		absv = -val->val.integer;
	} else {
		absv = val->val.integer;
	}

	if (absv > 1020 || ((absv & 3) != 0)) {
		subtilis_error_set_ass_bad_offset(
		    err, val->val.integer, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	*subtract = sub;
	absv >>= 2;

cleanup:

	subtilis_arm_exp_val_free(val);

	return absv;
}

static void prv_parse_fpa_stran_pre(subtilis_arm_ass_context_t *c,
				    subtilis_arm_instr_type_t itype,
				    subtilis_arm_ccode_type_t ccode,
				    size_t size, subtilis_arm_reg_t dest,
				    subtilis_arm_reg_t base,
				    subtilis_error_t *err)
{
	const char *tbuf;
	uint8_t offset;
	bool sub = false;
	subtilis_fpa_stran_instr_t *stran;
	subtilis_arm_instr_t *instr;
	bool write_back = false;

	offset = prv_get_fpa_stran_offset(c, &sub, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "]")) {
		subtilis_error_set_expected(err, "]", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "!")) {
		if (base == 15) {
			subtilis_error_set_ass_bad_reg(
			    err, "R15", c->l->stream->name, c->l->line);
			return;
		}
		write_back = true;
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = size;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = true;
	stran->write_back = write_back;
	stran->subtract = sub;
}

static void prv_parse_fpa_stran_post(subtilis_arm_ass_context_t *c,
				     subtilis_arm_instr_type_t itype,
				     subtilis_arm_ccode_type_t ccode,
				     size_t size, subtilis_arm_reg_t dest,
				     subtilis_arm_reg_t base,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_fpa_stran_instr_t *stran;
	subtilis_arm_instr_t *instr;
	uint8_t offset = 0;
	bool sub = false;
	bool pre_index = true;
	bool write_back = false;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		offset = prv_get_fpa_stran_offset(c, &sub, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		write_back = true;
		pre_index = false;

		if (base == 15) {
			subtilis_error_set_ass_bad_reg(
			    err, "R15", c->l->stream->name, c->l->line);
			return;
		}
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.fpa_stran;
	stran->ccode = ccode;
	stran->size = size;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = pre_index;
	stran->write_back = write_back;
	stran->subtract = sub;
}

static void prv_parse_fpa_stran(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode, size_t size,
				subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	const char *tbuf;

	dest = prv_get_fpa_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "["))) {
		subtilis_error_set_expected(err, "[", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	base = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		if (!strcmp(tbuf, ",")) {
			prv_parse_fpa_stran_pre(c, itype, ccode, size, dest,
						base, err);
			return;
		}

		if (!strcmp(tbuf, "]")) {
			prv_parse_fpa_stran_post(c, itype, ccode, size, dest,
						 base, err);
			return;
		}
	}

	subtilis_error_set_expected(err, "] or ,", tbuf, c->l->stream->name,
				    c->l->line);
}

static void prv_parse_fpa_cptran(subtilis_arm_ass_context_t *c,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_fpa_cptran_instr_t *cptran;
	subtilis_arm_instr_t *instr;

	dest = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	cptran = &instr->operands.fpa_cptran;
	cptran->ccode = ccode;
	cptran->dest = dest;
}

static void prv_parse_fpa_instruction(subtilis_arm_ass_context_t *c,
				      const char *name,
				      subtilis_arm_instr_type_t itype,
				      subtilis_arm_ccode_type_t ccode,
				      subtilis_fpa_rounding_t rnd, size_t size,
				      subtilis_error_t *err)
{
	switch (itype) {
	case SUBTILIS_FPA_INSTR_MVF:
	case SUBTILIS_FPA_INSTR_MNF:
	case SUBTILIS_FPA_INSTR_POL:
	case SUBTILIS_FPA_INSTR_ABS:
	case SUBTILIS_FPA_INSTR_RND:
	case SUBTILIS_FPA_INSTR_LOG:
	case SUBTILIS_FPA_INSTR_LGN:
	case SUBTILIS_FPA_INSTR_EXP:
	case SUBTILIS_FPA_INSTR_SIN:
	case SUBTILIS_FPA_INSTR_COS:
	case SUBTILIS_FPA_INSTR_TAN:
	case SUBTILIS_FPA_INSTR_ASN:
	case SUBTILIS_FPA_INSTR_ACS:
	case SUBTILIS_FPA_INSTR_ATN:
	case SUBTILIS_FPA_INSTR_SQT:
	case SUBTILIS_FPA_INSTR_URD:
	case SUBTILIS_FPA_INSTR_NRM:
		prv_parse_fpa_data(c, itype, ccode, rnd, size, false, err);
		break;
	case SUBTILIS_FPA_INSTR_ADF:
	case SUBTILIS_FPA_INSTR_MUF:
	case SUBTILIS_FPA_INSTR_SUF:
	case SUBTILIS_FPA_INSTR_RSF:
	case SUBTILIS_FPA_INSTR_DVF:
	case SUBTILIS_FPA_INSTR_RDF:
	case SUBTILIS_FPA_INSTR_RMF:
	case SUBTILIS_FPA_INSTR_FML:
	case SUBTILIS_FPA_INSTR_FDV:
	case SUBTILIS_FPA_INSTR_FRD:
	case SUBTILIS_FPA_INSTR_RPW:
	case SUBTILIS_FPA_INSTR_POW:
		prv_parse_fpa_data(c, itype, ccode, rnd, size, true, err);
		break;
	case SUBTILIS_FPA_INSTR_LDF:
	case SUBTILIS_FPA_INSTR_STF:
		prv_parse_fpa_stran(c, itype, ccode, size, err);
		break;
	case SUBTILIS_FPA_INSTR_FLT:
		prv_parse_fpa_flt(c, itype, ccode, rnd, size, err);
		break;
	case SUBTILIS_FPA_INSTR_FIX:
		prv_parse_fpa_fix(c, itype, ccode, rnd, err);
		break;
	case SUBTILIS_FPA_INSTR_CMF:
	case SUBTILIS_FPA_INSTR_CNF:
	case SUBTILIS_FPA_INSTR_CMFE:
	case SUBTILIS_FPA_INSTR_CNFE:
		prv_parse_fpa_cmp(c, itype, ccode, err);
		break;
	case SUBTILIS_FPA_INSTR_WFS:
	case SUBTILIS_FPA_INSTR_RFS:
		prv_parse_fpa_cptran(c, itype, ccode, err);
		break;
	default:
		subtilis_error_set_not_supported(err, name, c->l->stream->name,
						 c->l->line);
		break;
	}
}

static void prv_parse_possible_fpa(subtilis_arm_ass_context_t *c,
				   subtilis_arm_instr_type_t itype,
				   const char *tbuf, const char *flags,
				   size_t flags_len, subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_ccode_type_t ccode = SUBTILIS_ARM_CCODE_AL;
	subtilis_fpa_rounding_t rnd = SUBTILIS_FPA_ROUNDING_NEAREST;
	size_t size = 0;
	size_t ptr = 0;

	/*
	 * TODO: Need to do LFM/SFM/WFC/RFC.
	 */

	if (flags_len >= 2) {
		for (i = 0; i < sizeof(condition_codes) / sizeof(const char *);
		     i++)
			if ((flags[ptr] == condition_codes[i][0]) &&
			    (flags[ptr + 1] == condition_codes[i][1]))
				break;

		if (i < sizeof(condition_codes) / sizeof(const char *)) {
			ptr += 2;
			ccode = (subtilis_arm_ccode_type_t)i;
		}
	}

	if (itype != SUBTILIS_FPA_INSTR_FIX &&
	    itype != SUBTILIS_FPA_INSTR_RFS &&
	    itype != SUBTILIS_FPA_INSTR_WFS &&
	    itype != SUBTILIS_FPA_INSTR_CMF &&
	    itype != SUBTILIS_FPA_INSTR_CNF &&
	    itype != SUBTILIS_FPA_INSTR_CMFE &&
	    itype != SUBTILIS_FPA_INSTR_CNFE) {
		if (ptr == flags_len) {
			prv_parse_label(c, tbuf, err);
			return;
		}

		switch (flags[ptr]) {
		case 'S':
			size = 4;
			break;
		case 'D':
			size = 8;
			break;
		case 'E':
			size = 12;
			break;
		default:
			prv_parse_label(c, tbuf, err);
			return;
		}

		/*
		 * The fast versions of multiply and divide only work
		 * on floats.
		 */

		if (((itype == SUBTILIS_FPA_INSTR_FML) ||
		     (itype == SUBTILIS_FPA_INSTR_FDV) ||
		     (itype == SUBTILIS_FPA_INSTR_FRD)) &&
		    size != 4) {
			prv_parse_label(c, tbuf, err);
			return;
		}

		ptr++;
	}

	if (ptr == flags_len) {
		prv_parse_fpa_instruction(c, tbuf, itype, ccode, rnd, size,
					  err);
		return;
	}

	if (itype == SUBTILIS_FPA_INSTR_LDF ||
	    itype == SUBTILIS_FPA_INSTR_STF) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	if (ptr + 1 > flags_len) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	switch (flags[ptr]) {
	case 'P':
		rnd = SUBTILIS_FPA_ROUNDING_PLUS_INFINITY;
		break;
	case 'M':
		rnd = SUBTILIS_FPA_ROUNDING_MINUS_INFINITY;
		break;
	case 'Z':
		rnd = SUBTILIS_FPA_ROUNDING_ZERO;
		break;
	default:
		prv_parse_label(c, tbuf, err);
		return;
	}

	prv_parse_fpa_instruction(c, tbuf, itype, ccode, rnd, size, err);
}

static void prv_parse_vfp_copy(subtilis_arm_ass_context_t *c, const char *name,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	const char *tbuf;

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		dest = prv_get_vfp_sreg(c, err);
		break;
	default:
		dest = prv_get_vfp_dreg(c, err);
		break;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}
	switch (itype) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FABSS:
		src = prv_get_vfp_sreg(c, err);
		break;
	default:
		src = prv_get_vfp_dreg(c, err);
		break;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_copy(c->arm_s, ccode, itype, dest, src, err);
}

static void prv_parse_vfp_tran(subtilis_arm_ass_context_t *c, const char *name,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;
	const char *tbuf;

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FSITOD:
	case SUBTILIS_VFP_INSTR_FUITOD:
		dest = prv_get_vfp_dreg(c, err);
		break;
	default:
		dest = prv_get_vfp_sreg(c, err);
		break;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FTOUID:
	case SUBTILIS_VFP_INSTR_FTOUIZD:
	case SUBTILIS_VFP_INSTR_FTOSID:
	case SUBTILIS_VFP_INSTR_FTOSIZD:
		src = prv_get_vfp_dreg(c, err);
		break;
	default:
		src = prv_get_vfp_sreg(c, err);
		break;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_tran(c->arm_s, itype, ccode, false, dest, src, err);
}

static subtilis_arm_reg_t prv_get_data_reg(subtilis_arm_ass_context_t *c,
					   subtilis_arm_instr_type_t itype,
					   subtilis_error_t *err)
{
	switch (itype) {
	case SUBTILIS_VFP_INSTR_FMACS:
	case SUBTILIS_VFP_INSTR_FNMACS:
	case SUBTILIS_VFP_INSTR_FMSCS:
	case SUBTILIS_VFP_INSTR_FNMSCS:
	case SUBTILIS_VFP_INSTR_FMULS:
	case SUBTILIS_VFP_INSTR_FNMULS:
	case SUBTILIS_VFP_INSTR_FADDS:
	case SUBTILIS_VFP_INSTR_FSUBS:
	case SUBTILIS_VFP_INSTR_FDIVS:
		return prv_get_vfp_sreg(c, err);
	default:
		return prv_get_vfp_dreg(c, err);
	}
}

static void prv_parse_vfp_data(subtilis_arm_ass_context_t *c, const char *name,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	const char *tbuf;

	dest = prv_get_data_reg(c, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op1 = prv_get_data_reg(c, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	op2 = prv_get_data_reg(c, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_data(c->arm_s, itype, ccode, dest, op1, op2, err);
}

static void prv_parse_vfp_cptran(subtilis_arm_ass_context_t *c,
				 const char *name,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t src;

	if (itype == SUBTILIS_VFP_INSTR_FMSR) {
		dest = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		src = prv_get_reg(c, err);
	} else {
		dest = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		src = prv_get_vfp_sreg(c, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_cptran(c->arm_s, itype, ccode, false, dest, src, err);
}

static void prv_parse_vfp_cmp(subtilis_arm_ass_context_t *c, const char *name,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_error_t *err)
{
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	const char *tbuf;

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FCMPD:
	case SUBTILIS_VFP_INSTR_FCMPED:
		op1 = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}
		op2 = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmp(c->arm_s, itype, ccode, op1, op2, err);
		break;
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPES:
		op1 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}
		op2 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmp(c->arm_s, itype, ccode, op1, op2, err);
		break;
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
		op1 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmpz(c->arm_s, itype, ccode, op1, err);
		break;
	case SUBTILIS_VFP_INSTR_FCMPZD:
	case SUBTILIS_VFP_INSTR_FCMPEZD:
		op1 = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_vfp_add_cmpz(c->arm_s, itype, ccode, op1, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}
}

static void prv_parse_vfp_cvt(subtilis_arm_ass_context_t *c, const char *name,
			      subtilis_arm_instr_type_t itype,
			      subtilis_arm_ccode_type_t ccode,
			      subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	const char *tbuf;

	if (itype == SUBTILIS_VFP_INSTR_FCVTDS) {
		dest = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		op1 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		dest = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		op1 = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_vfp_add_cvt(c->arm_s, itype, ccode, dest, op1, err);
}

static void prv_parse_vfp_sqrt(subtilis_arm_ass_context_t *c, const char *name,
			       subtilis_arm_instr_type_t itype,
			       subtilis_arm_ccode_type_t ccode,
			       subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	const char *tbuf;

	if (itype == SUBTILIS_VFP_INSTR_FSQRTD)
		dest = prv_get_vfp_dreg(c, err);
	else
		dest = prv_get_vfp_sreg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	if (itype == SUBTILIS_VFP_INSTR_FSQRTD)
		op1 = prv_get_vfp_dreg(c, err);
	else
		op1 = prv_get_vfp_sreg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sqrt(c->arm_s, itype, ccode, dest, op1, err);
}

static void prv_parse_vfp_sysreg(subtilis_arm_ass_context_t *c,
				 const char *name,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_error_t *err)
{
	subtilis_arm_reg_t sysreg = SIZE_MAX;
	subtilis_arm_reg_t reg = SIZE_MAX;
	const char *tbuf;

	if (!strcmp(name, "FMSTAT")) {
		subtilis_vfp_add_sysreg(c->arm_s, itype, ccode,
					SUBTILIS_VFP_SYSREG_FPSCR, 15, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		subtilis_lexer_get(c->l, c->t, err);
		return;
	}

	if (itype == SUBTILIS_VFP_INSTR_FMXR)
		sysreg = prv_get_vfp_sysreg(c, err);
	else
		reg = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	if (itype == SUBTILIS_VFP_INSTR_FMXR)
		reg = prv_get_reg(c, err);
	else
		sysreg = prv_get_vfp_sysreg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_sysreg(c->arm_s, itype, ccode, sysreg, reg, err);
}

static void prv_parse_vfp_tran_dbl(subtilis_arm_ass_context_t *c,
				   const char *name,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_error_t *err)
{
	subtilis_arm_reg_t dest1 = 0;
	subtilis_arm_reg_t dest2 = 0;
	subtilis_arm_reg_t src1 = 0;
	subtilis_arm_reg_t src2 = 0;
	const char *tbuf;

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FMDRR:
		dest1 = prv_get_vfp_dreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src1 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src2 = prv_get_reg(c, err);
		break;
	case SUBTILIS_VFP_INSTR_FMRRD:
		dest1 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		dest2 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src1 = prv_get_vfp_dreg(c, err);
		break;
	case SUBTILIS_VFP_INSTR_FMSRR:
		dest1 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		dest2 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src1 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}
		src2 = prv_get_reg(c, err);
		break;
	case SUBTILIS_VFP_INSTR_FMRRS:
		dest1 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		dest2 = prv_get_reg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src1 = prv_get_vfp_sreg(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    (strcmp(tbuf, ","))) {
			subtilis_error_set_expected(
			    err, ",", tbuf, c->l->stream->name, c->l->line);
			return;
		}

		src2 = prv_get_vfp_sreg(c, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_vfp_add_tran_dbl(c->arm_s, itype, ccode, dest1, dest2, src1,
				  src2, err);
}

static void prv_parse_vfp_stran_pre(subtilis_arm_ass_context_t *c,
				    subtilis_arm_instr_type_t itype,
				    subtilis_arm_ccode_type_t ccode,
				    subtilis_arm_reg_t dest,
				    subtilis_arm_reg_t base,
				    subtilis_error_t *err)
{
	const char *tbuf;
	uint8_t offset;
	bool sub = false;
	subtilis_vfp_stran_instr_t *stran;
	subtilis_arm_instr_t *instr;
	bool write_back = false;

	offset = prv_get_fpa_stran_offset(c, &sub, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "]")) {
		subtilis_error_set_expected(err, "]", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "!")) {
		if (base == 15) {
			subtilis_error_set_ass_bad_reg(
			    err, "R15", c->l->stream->name, c->l->line);
			return;
		}
		write_back = true;
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.vfp_stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = true;
	stran->write_back = write_back;
	stran->subtract = sub;
}

static void prv_parse_vfp_stran_post(subtilis_arm_ass_context_t *c,
				     subtilis_arm_instr_type_t itype,
				     subtilis_arm_ccode_type_t ccode,
				     subtilis_arm_reg_t dest,
				     subtilis_arm_reg_t base,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_vfp_stran_instr_t *stran;
	subtilis_arm_instr_t *instr;
	uint8_t offset = 0;
	bool sub = false;
	bool pre_index = true;
	bool write_back = false;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ",")) {
		offset = prv_get_fpa_stran_offset(c, &sub, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		write_back = true;
		pre_index = false;

		if (base == 15) {
			subtilis_error_set_ass_bad_reg(
			    err, "R15", c->l->stream->name, c->l->line);
			return;
		}
	}

	instr = subtilis_arm_section_add_instr(c->arm_s, itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.vfp_stran;
	stran->ccode = ccode;
	stran->dest = dest;
	stran->base = base;
	stran->offset = offset;
	stran->pre_indexed = pre_index;
	stran->write_back = write_back;
	stran->subtract = sub;
}

static void prv_parse_vfp_stran(subtilis_arm_ass_context_t *c, const char *name,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	const char *tbuf;

	switch (itype) {
	case SUBTILIS_VFP_INSTR_FSTS:
	case SUBTILIS_VFP_INSTR_FLDS:
		dest = prv_get_vfp_sreg(c, err);
		break;
	case SUBTILIS_VFP_INSTR_FSTD:
	case SUBTILIS_VFP_INSTR_FLDD:
		dest = prv_get_vfp_dreg(c, err);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(tbuf, "["))) {
		subtilis_error_set_expected(err, "[", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	base = prv_get_reg(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		if (!strcmp(tbuf, ",")) {
			prv_parse_vfp_stran_pre(c, itype, ccode, dest, base,
						err);
			return;
		}

		if (!strcmp(tbuf, "]")) {
			prv_parse_vfp_stran_post(c, itype, ccode, dest, base,
						 err);
			return;
		}
	}

	subtilis_error_set_expected(err, "] or ,", tbuf, c->l->stream->name,
				    c->l->line);
}

static void prv_parse_vfp_instruction(subtilis_arm_ass_context_t *c,
				      const char *name,
				      subtilis_arm_instr_type_t itype,
				      subtilis_arm_ccode_type_t ccode,
				      subtilis_error_t *err)
{
	switch (itype) {
	case SUBTILIS_VFP_INSTR_FCPYS:
	case SUBTILIS_VFP_INSTR_FCPYD:
	case SUBTILIS_VFP_INSTR_FNEGS:
	case SUBTILIS_VFP_INSTR_FNEGD:
	case SUBTILIS_VFP_INSTR_FABSS:
	case SUBTILIS_VFP_INSTR_FABSD:
		prv_parse_vfp_copy(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FSITOS:
	case SUBTILIS_VFP_INSTR_FSITOD:
	case SUBTILIS_VFP_INSTR_FTOSIS:
	case SUBTILIS_VFP_INSTR_FTOSID:
	case SUBTILIS_VFP_INSTR_FTOUIS:
	case SUBTILIS_VFP_INSTR_FTOUID:
	case SUBTILIS_VFP_INSTR_FTOSIZS:
	case SUBTILIS_VFP_INSTR_FTOSIZD:
	case SUBTILIS_VFP_INSTR_FTOUIZS:
	case SUBTILIS_VFP_INSTR_FTOUIZD:
	case SUBTILIS_VFP_INSTR_FUITOD:
	case SUBTILIS_VFP_INSTR_FUITOS:
		prv_parse_vfp_tran(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FMSR:
	case SUBTILIS_VFP_INSTR_FMRS:
		prv_parse_vfp_cptran(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FMACS:
	case SUBTILIS_VFP_INSTR_FMACD:
	case SUBTILIS_VFP_INSTR_FNMACS:
	case SUBTILIS_VFP_INSTR_FNMACD:
	case SUBTILIS_VFP_INSTR_FMSCS:
	case SUBTILIS_VFP_INSTR_FMSCD:
	case SUBTILIS_VFP_INSTR_FNMSCS:
	case SUBTILIS_VFP_INSTR_FNMSCD:
	case SUBTILIS_VFP_INSTR_FMULS:
	case SUBTILIS_VFP_INSTR_FMULD:
	case SUBTILIS_VFP_INSTR_FNMULS:
	case SUBTILIS_VFP_INSTR_FNMULD:
	case SUBTILIS_VFP_INSTR_FADDS:
	case SUBTILIS_VFP_INSTR_FADDD:
	case SUBTILIS_VFP_INSTR_FSUBS:
	case SUBTILIS_VFP_INSTR_FSUBD:
	case SUBTILIS_VFP_INSTR_FDIVS:
	case SUBTILIS_VFP_INSTR_FDIVD:
		prv_parse_vfp_data(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FCMPS:
	case SUBTILIS_VFP_INSTR_FCMPD:
	case SUBTILIS_VFP_INSTR_FCMPES:
	case SUBTILIS_VFP_INSTR_FCMPED:
	case SUBTILIS_VFP_INSTR_FCMPZS:
	case SUBTILIS_VFP_INSTR_FCMPZD:
	case SUBTILIS_VFP_INSTR_FCMPEZS:
	case SUBTILIS_VFP_INSTR_FCMPEZD:
		prv_parse_vfp_cmp(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FCVTDS:
	case SUBTILIS_VFP_INSTR_FCVTSD:
		prv_parse_vfp_cvt(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FSQRTD:
	case SUBTILIS_VFP_INSTR_FSQRTS:
		prv_parse_vfp_sqrt(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FMXR:
	case SUBTILIS_VFP_INSTR_FMRX:
		prv_parse_vfp_sysreg(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FMDRR:
	case SUBTILIS_VFP_INSTR_FMRRD:
	case SUBTILIS_VFP_INSTR_FMSRR:
	case SUBTILIS_VFP_INSTR_FMRRS:
		prv_parse_vfp_tran_dbl(c, name, itype, ccode, err);
		return;
	case SUBTILIS_VFP_INSTR_FSTS:
	case SUBTILIS_VFP_INSTR_FLDS:
	case SUBTILIS_VFP_INSTR_FSTD:
	case SUBTILIS_VFP_INSTR_FLDD:
		prv_parse_vfp_stran(c, name, itype, ccode, err);
		return;
	default:
		subtilis_error_set_assertion_failed(err);
	}
}

static void prv_parse_possible_vfp(subtilis_arm_ass_context_t *c,
				   subtilis_arm_instr_type_t itype,
				   const char *tbuf, const char *flags,
				   size_t flags_len, subtilis_error_t *err)
{
	size_t i;
	subtilis_arm_ccode_type_t ccode = SUBTILIS_ARM_CCODE_AL;
	size_t ptr = 0;

	/*
	 * TODO: Need to do store multiple
	 */

	if (flags_len >= 2) {
		for (i = 0; i < sizeof(condition_codes) / sizeof(const char *);
		     i++)
			if ((flags[ptr] == condition_codes[i][0]) &&
			    (flags[ptr + 1] == condition_codes[i][1]))
				break;

		if (i < sizeof(condition_codes) / sizeof(const char *)) {
			ptr += 2;
			ccode = (subtilis_arm_ccode_type_t)i;
		}
	}

	if (ptr == flags_len) {
		prv_parse_vfp_instruction(c, tbuf, itype, ccode, err);
		return;
	}

	prv_parse_label(c, tbuf, err);
}

static void prv_parse_identifier(subtilis_arm_ass_context_t *c,
				 subtilis_error_t *err)
{
	const char *tbuf;
	size_t index;
	size_t i;
	size_t base_len;
	size_t token_end;
	bool status_valid;
	bool byte_valid;
	bool hword_valid;
	bool dword_valid;
	size_t ptr;
	size_t search_len;
	subtilis_arm_instr_type_t itype;
	subtilis_arm_iclass_t iclass;
	subtilis_arm_ass_modifiers_t modifiers = {false};
	subtilis_arm_mtran_type_t mtran_type = SUBTILIS_ARM_MTRAN_IA;
	subtilis_arm_ccode_type_t ccode = SUBTILIS_ARM_CCODE_AL;
	size_t max_length = 0;
	size_t max_index = SIZE_MAX;

	tbuf = subtilis_token_get_text(c->t);
	if (tbuf[0] < 'A' || tbuf[0] > 'Z') {
		prv_parse_label(c, tbuf, err);
		return;
	}

	index = tbuf[0] - 'A';
	if (!keyword_map[index].mnems) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	for (i = 0; i < keyword_map[index].count; i++) {
		search_len = strlen(keyword_map[index].mnems[i].name);
		if (search_len > keyword_map[index].max_length)
			search_len = keyword_map[index].max_length;
		if (!strncmp(keyword_map[index].mnems[i].name, tbuf,
			     search_len)) {
			base_len = strlen(keyword_map[index].mnems[i].name);
			if (base_len > max_length) {
				max_length = base_len;
				max_index = i;
			}
		}
	}

	if (max_index == SIZE_MAX) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	/*
	 * It might still be an identifier
	 */

	token_end = strlen(tbuf);
	itype = keyword_map[index].mnems[max_index].type;
	iclass = subtilis_arm_get_iclass(itype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (iclass == SUBTILIS_ARM_ICLASS_FPA) {
		prv_parse_possible_fpa(c, itype, tbuf, &tbuf[max_length],
				       token_end - max_length, err);
		return;
	}

	if (iclass == SUBTILIS_ARM_ICLASS_VFP) {
		prv_parse_possible_vfp(c, itype, tbuf, &tbuf[max_length],
				       token_end - max_length, err);
		return;
	}

	/* Special case for BL */

	if ((itype == SUBTILIS_ARM_INSTR_B) && (token_end > 1) &&
	    (token_end != 3) && (tbuf[1] == 'L'))
		max_length++;

	status_valid = itype != SUBTILIS_ARM_INSTR_LDR &&
		       itype != SUBTILIS_ARM_INSTR_STR &&
		       itype != SUBTILIS_ARM_INSTR_CMP &&
		       itype != SUBTILIS_ARM_INSTR_CMN &&
		       itype != SUBTILIS_ARM_INSTR_LDM &&
		       itype != SUBTILIS_ARM_INSTR_LDM &&
		       itype != SUBTILIS_ARM_INSTR_TST &&
		       itype != SUBTILIS_ARM_INSTR_TEQ &&
		       itype != SUBTILIS_ARM_INSTR_B;
	byte_valid =
	    itype == SUBTILIS_ARM_INSTR_LDR || itype == SUBTILIS_ARM_INSTR_STR;
	hword_valid = byte_valid;
	dword_valid = hword_valid;

	ptr = max_length;
	if (ptr == token_end) {
		if ((itype == SUBTILIS_ARM_INSTR_LDM) ||
		    (itype == SUBTILIS_ARM_INSTR_STM))
			prv_parse_label(c, tbuf, err);
		else
			prv_parse_instruction(c, tbuf, itype, ccode, &modifiers,
					      mtran_type, err);
		return;
	}

	if (ptr + 2 <= token_end) {
		for (i = 0; i < sizeof(condition_codes) / sizeof(const char *);
		     i++)
			if ((tbuf[ptr] == condition_codes[i][0]) &&
			    (tbuf[ptr + 1] == condition_codes[i][1]))
				break;

		if (i < sizeof(condition_codes) / sizeof(const char *)) {
			ptr += 2;
			ccode = (subtilis_arm_ccode_type_t)i;
		}
	}

	if ((ptr + 2 == token_end) && ((itype == SUBTILIS_ARM_INSTR_LDM) ||
				       (itype == SUBTILIS_ARM_INSTR_STM))) {
		for (i = 0; i < sizeof(mtran_types) / sizeof(mtran_types[0]);
		     i++)
			if (!strcmp(&tbuf[ptr], mtran_types[i]))
				break;

		if (i < sizeof(mtran_types) / sizeof(mtran_types[0])) {
			mtran_type = (subtilis_arm_mtran_type_t)i;
			ptr += 2;
		}
	}

	if (ptr + 1 == token_end) {
		modifiers.status = tbuf[ptr] == 'S' && status_valid;
		modifiers.byte = tbuf[ptr] == 'B' && byte_valid;
		modifiers.hword = tbuf[ptr] == 'H' && hword_valid;
		modifiers.dword = tbuf[ptr] == 'D' && dword_valid;
		modifiers.teqp =
		    tbuf[ptr] == 'P' && itype == SUBTILIS_ARM_INSTR_TEQ;
		if (!modifiers.status && !modifiers.byte && !modifiers.teqp &&
		    !modifiers.hword && !modifiers.dword) {
			prv_parse_label(c, tbuf, err);
			return;
		}
	} else if ((ptr + 2 == token_end) &&
		   (status_valid || itype == SUBTILIS_ARM_INSTR_LDR) &&
		   (byte_valid || hword_valid)) {
		if (tbuf[ptr] != 'S') {
			prv_parse_label(c, tbuf, err);
			return;
		}
		modifiers.status = true;
		modifiers.byte = tbuf[ptr + 1] == 'B';
		modifiers.hword = tbuf[ptr + 1] == 'H';
		if (!modifiers.byte && !modifiers.hword) {
			prv_parse_label(c, tbuf, err);
			return;
		}
	} else if (ptr != token_end) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	prv_parse_instruction(c, tbuf, itype, ccode, &modifiers, mtran_type,
			      err);
}

typedef bool (*subtilis_arm_ass_valid_int_fn_t)(int32_t);

static int32_t *prv_get_uint32_list(subtilis_arm_ass_context_t *c,
				    size_t *count,
				    subtilis_arm_ass_valid_int_fn_t fn,
				    subtilis_error_t *err)
{
	const char *tbuf;
	int32_t *new_nums;
	int32_t num;
	int32_t *nums = NULL;
	size_t max_nums = 0;
	size_t num_count = 0;
	subtilis_arm_exp_val_t *val = NULL;

	do {
		val = subtilis_arm_exp_val_get(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
			num = val->val.integer;
		} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REAL) {
			num = (int32_t)val->val.real;
		} else {
			subtilis_error_set_integer_expected(
			    err, subtilis_arm_exp_type_name(val),
			    c->l->stream->name, c->l->line);
			goto cleanup;
		}

		subtilis_arm_exp_val_free(val);
		val = NULL;

		if (fn && !fn(num)) {
			subtilis_error_set_ass_integer_too_big(
			    err, c->t->tok.integer, c->l->stream->name,
			    c->l->line);
			goto cleanup;
		}

		if (num_count == max_nums) {
			max_nums += 32;
			new_nums = realloc(nums, max_nums * sizeof(*nums));
			if (!new_nums)
				goto cleanup;
			nums = new_nums;
		}

		nums[num_count++] = num;
		tbuf = subtilis_token_get_text(c->t);
	} while ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ","));

	*count = num_count;

	return nums;

cleanup:

	subtilis_arm_exp_val_free(val);
	free(nums);

	return NULL;
}

static bool prv_check_equb(int32_t i) { return !(i < -128 || i > 255); }

static void prv_parse_equb(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	int32_t *nums;
	size_t i;
	size_t num_count = 0;

	nums = prv_get_uint32_list(c, &num_count, prv_check_equb, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_byte(c->arm_s, (uint8_t)nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static bool prv_check_equw(int32_t i) { return !(i < -32768 || i > 0xffff); }

static void prv_parse_equw(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	int32_t *nums;
	size_t i;
	size_t num_count = 0;

	nums = prv_get_uint32_list(c, &num_count, prv_check_equw, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_two_bytes(c->arm_s, (uint16_t)nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static void prv_parse_equd(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	int32_t *nums;
	size_t i;
	size_t num_count = 0;

	nums = prv_get_uint32_list(c, &num_count, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_four_bytes(c->arm_s, nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static double *prv_get_double_list(subtilis_arm_ass_context_t *c, size_t *count,
				   subtilis_error_t *err)
{
	const char *tbuf;
	double *new_nums;
	double num;
	double *nums = NULL;
	size_t max_nums = 0;
	size_t num_count = 0;
	subtilis_arm_exp_val_t *val = NULL;

	do {
		val = subtilis_arm_exp_val_get(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
			num = (double)val->val.integer;
		} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REAL) {
			num = val->val.real;
		} else {
			subtilis_error_set_numeric_expected(
			    err, subtilis_arm_exp_type_name(val),
			    c->l->stream->name, c->l->line);
			goto cleanup;
		}

		subtilis_arm_exp_val_free(val);
		val = NULL;

		if (num_count == max_nums) {
			max_nums += 32;
			new_nums = realloc(nums, max_nums * sizeof(*nums));
			if (!new_nums)
				goto cleanup;
			nums = new_nums;
		}

		nums[num_count++] = num;
		tbuf = subtilis_token_get_text(c->t);
	} while ((c->t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ","));

	*count = num_count;

	return nums;

cleanup:

	subtilis_arm_exp_val_free(val);
	free(nums);

	return NULL;
}

static void prv_parse_equdbl(subtilis_arm_ass_context_t *c,
			     subtilis_error_t *err)
{
	double *nums;
	size_t num_count;
	size_t i;

	nums = prv_get_double_list(c, &num_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_double(c->arm_s, nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static void prv_parse_equdblr(subtilis_arm_ass_context_t *c,
			      subtilis_error_t *err)
{
	double *nums;
	size_t num_count;
	size_t i;

	nums = prv_get_double_list(c, &num_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_doubler(c->arm_s, nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static void prv_parse_equf(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	double *nums;
	size_t num_count;
	size_t i;

	nums = prv_get_double_list(c, &num_count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < num_count; i++) {
		subtilis_arm_add_float(c->arm_s, (float)nums[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	free(nums);
}

static void prv_parse_equs(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	const char *str;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_expected(err, "string",
					    subtilis_arm_exp_type_name(val),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	str = subtilis_buffer_get_string(&val->val.buf);
	subtilis_arm_add_string(c->arm_s, str, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_align(subtilis_arm_ass_context_t *c,
			    subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	int32_t align;

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val->type == SUBTILIS_ARM_EXP_TYPE_INT) {
		align = val->val.integer;
	} else if (val->type == SUBTILIS_ARM_EXP_TYPE_REAL) {
		align = (int32_t)val->val.real;
	} else {
		subtilis_error_set_numeric_expected(
		    err, subtilis_arm_exp_type_name(val), c->l->stream->name,
		    c->l->line);
		goto cleanup;
	}

	if (align <= 0 || ((align - 1) & align) != 0 || align > 1024) {
		subtilis_error_set_ass_bad_align(err, align, c->l->stream->name,
						 c->l->line);
		goto cleanup;
	}

	subtilis_arm_add_align(c->arm_s, (uint32_t)align, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_def(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_ass_def_t *new_defs;
	size_t new_max_defs;
	const char *id_name;
	subtilis_arm_exp_val_t *val1 = NULL;
	subtilis_arm_exp_val_t *val2 = NULL;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_error_set_expected(err, "identifier", tbuf,
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(c->t);
	val1 = subtilis_arm_exp_process_id(c, tbuf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (val1->type != SUBTILIS_ARM_EXP_TYPE_ID) {
		subtilis_error_set_expected(err, "identifier",
					    subtilis_arm_exp_type_name(val1),
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	if (c->t->type != SUBTILIS_TOKEN_OPERATOR || strcmp(tbuf, "=")) {
		subtilis_error_set_expected(err, "=", tbuf, c->l->stream->name,
					    c->l->line);
		goto cleanup;
	}

	val2 = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (c->def_count == c->max_defs) {
		new_max_defs = c->max_defs + SUBTILIS_CONFIG_CONSTANT_POOL_GRAN;
		new_defs = realloc(c->defs, sizeof(*new_defs) * new_max_defs);
		if (!new_defs) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		c->defs = new_defs;
		c->max_defs = new_max_defs;
	}

	id_name = subtilis_buffer_get_string(&val1->val.buf);
	c->defs[c->def_count].name =
	    malloc(subtilis_buffer_get_size(&val1->val.buf));
	if (!c->defs[c->def_count].name) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(c->defs[c->def_count].name, id_name);
	c->defs[c->def_count++].val = val2;
	val2 = NULL;

cleanup:

	subtilis_arm_exp_val_free(val2);
	subtilis_arm_exp_val_free(val1);
}

static void prv_parse_for(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	size_t var;
	const char *tbuf;
	subtilis_arm_exp_val_t *index_var;
	subtilis_arm_exp_val_t *upto = NULL;

	prv_parse_def(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	var = c->def_count - 1;
	index_var = c->defs[var].val;

	if ((index_var->type != SUBTILIS_ARM_EXP_TYPE_INT) &&
	    (index_var->type != SUBTILIS_ARM_EXP_TYPE_REAL) &&
	    (index_var->type != SUBTILIS_ARM_EXP_TYPE_FREG) &&
	    (index_var->type != SUBTILIS_ARM_EXP_TYPE_REG)) {
		subtilis_error_set_expected(
		    err, "numeric", subtilis_arm_exp_type_name(index_var),
		    c->l->stream->name, c->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    (c->t->tok.keyword.type != SUBTILIS_ARM_KEYWORD_TO)) {
		subtilis_error_set_expected(err, "TO", tbuf, c->l->stream->name,
					    c->l->line);
		return;
	}

	upto = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (index_var->type != upto->type) {
		switch (index_var->type) {
		case SUBTILIS_ARM_EXP_TYPE_INT:
			if (upto->type != SUBTILIS_ARM_EXP_TYPE_REAL) {
				subtilis_error_set_expected(
				    err, "numeric",
				    subtilis_arm_exp_type_name(upto),
				    c->l->stream->name, c->l->line);
				goto cleanup;
			}
			index_var->type = SUBTILIS_ARM_EXP_TYPE_REAL;
			index_var->val.real = (double)index_var->val.integer;
			break;
		case SUBTILIS_ARM_EXP_TYPE_REAL:
			if (upto->type != SUBTILIS_ARM_EXP_TYPE_INT) {
				subtilis_error_set_expected(
				    err, "numeric",
				    subtilis_arm_exp_type_name(upto),
				    c->l->stream->name, c->l->line);
				goto cleanup;
			}
			upto->type = SUBTILIS_ARM_EXP_TYPE_REAL;
			upto->val.real = (double)upto->val.integer;
			break;
		default:
			subtilis_error_set_bad_conversion(
			    err, subtilis_arm_exp_type_name(index_var),
			    subtilis_arm_exp_type_name(upto),
			    c->l->stream->name, c->l->line);
			goto cleanup;
		}
	}

	if (c->num_blocks == c->max_blocks) {
		if (c->blocks) {
			subtilis_error_set_too_many_blocks(
			    err, c->l->stream->name, c->l->line);
			goto cleanup;
		}
		c->blocks = malloc(sizeof(*c->blocks) * 32);
		if (!c->blocks) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		c->max_blocks = 32;
	}
	c->blocks[c->num_blocks].def_start = var;
	c->blocks[c->num_blocks].upto = upto;
	c->num_blocks++;

	subtilis_lexer_push_block(c->l, c->t, err);
	return;

cleanup:

	subtilis_arm_exp_val_free(upto);
}

static void prv_parse_next(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	subtilis_ass_block_t *block;
	subtilis_arm_exp_val_t *index_var;
	size_t var;
	bool finished;
	size_t i;

	if (c->num_blocks == 0) {
		subtilis_error_set_keyword_unexpected(
		    err, "NEXT", c->l->stream->name, c->l->line);
		return;
	}

	block = &c->blocks[c->num_blocks - 1];
	var = block->def_start;
	index_var = c->defs[var].val;

	switch (block->upto->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		index_var->val.integer++;
		finished = index_var->val.integer > block->upto->val.integer;
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		index_var->val.real++;
		finished = index_var->val.real > block->upto->val.real;
		break;
	case SUBTILIS_ARM_EXP_TYPE_FREG:
	case SUBTILIS_ARM_EXP_TYPE_REG:
		index_var->val.reg++;
		finished = index_var->val.reg > block->upto->val.reg;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		return;
	}

	for (i = var + 1; i < c->def_count; i++) {
		free(c->defs[i].name);
		subtilis_arm_exp_val_free(c->defs[i].val);
	}
	c->def_count = var + 1;
	if (finished) {
		c->num_blocks--;
		subtilis_arm_exp_val_free(c->blocks[c->num_blocks].upto);
		subtilis_lexer_pop_block(c->l, err);
	} else {
		subtilis_lexer_set_block_start(c->l, err);
	}

	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_lexer_get(c->l, c->t, err);
}

static void prv_free_defs(subtilis_arm_ass_context_t *c)
{
	size_t i;

	for (i = 0; i < c->def_count; i++) {
		free(c->defs[i].name);
		subtilis_arm_exp_val_free(c->defs[i].val);
	}
	free(c->defs);
}

static void prv_parse_keyword(subtilis_arm_ass_context_t *c, const char *name,
			      const subtilis_token_keyword_t *keyword,
			      subtilis_error_t *err)
{
	switch (keyword->type) {
	/*
	 * We can't call functions or procedures inside a block of
	 * assembler. So we'll just treat these as identifiers.
	 */
	case SUBTILIS_KEYWORD_FN:
	case SUBTILIS_KEYWORD_PROC:
		prv_parse_identifier(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_ALIGN:
		prv_parse_align(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_DEF:
		prv_parse_def(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUB:
		prv_parse_equb(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUD:
		prv_parse_equd(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUDBL:
		prv_parse_equdbl(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUF:
		prv_parse_equf(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUDBLR:
		prv_parse_equdblr(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUS:
		prv_parse_equs(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_EQUW:
		prv_parse_equw(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_FOR:
		prv_parse_for(c, err);
		break;
	case SUBTILIS_ARM_KEYWORD_NEXT:
		prv_parse_next(c, err);
		break;
	default:
		subtilis_error_set_ass_keyword_bad_use(
		    err, name, c->l->stream->name, c->l->line);
		break;
	}
}

static void prv_parser_main_loop(subtilis_arm_ass_context_t *c,
				 subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(c->t);
	while ((c->t->type != SUBTILIS_TOKEN_OPERATOR) && (strcmp(tbuf, "]"))) {
		switch (c->t->type) {
		case SUBTILIS_TOKEN_KEYWORD:
			prv_parse_keyword(c, tbuf, &c->t->tok.keyword, err);
			break;
		case SUBTILIS_TOKEN_IDENTIFIER:
			prv_parse_identifier(c, err);
			break;
		default:
			subtilis_error_set_expected(
			    err, "keyword or identifier", tbuf,
			    c->l->stream->name, c->l->line);
			return;
		}

		if (err->type != SUBTILIS_ERROR_OK)
			return;

		tbuf = subtilis_token_get_text(c->t);
	}

	if (!(c->t->type == SUBTILIS_TOKEN_OPERATOR && !strcmp(tbuf, "]"))) {
		subtilis_error_set_expected(err, "label or keyword", tbuf,
					    c->l->stream->name, c->l->line);
		return;
	}

	if (c->num_blocks != 0)
		subtilis_error_set_compund_not_term(err, c->l->stream->name,
						    c->l->line);
}

static void prv_context_free(subtilis_arm_ass_context_t *c)
{
	size_t i;

	prv_free_defs(c);
	subtilis_bitset_free(&c->pending_labels);
	subtilis_string_pool_delete(c->label_pool);

	for (i = 0; i < c->num_blocks; i++)
		subtilis_arm_exp_val_free(c->blocks[i].upto);
	free(c->blocks);
}

/* clang-format off */
subtilis_arm_section_t *subtilis_arm_asm_parse(
	subtilis_lexer_t *l, subtilis_token_t *t, subtilis_arm_op_pool_t *pool,
	subtilis_type_section_t *stype, const subtilis_settings_t *set,
	subtilis_backend_sys_trans sys_trans, int32_t start_address,
	subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_ass_context_t context;
	size_t pending_labels_num;
	const char *label_name;
	size_t *pending_labels = NULL;
	subtilis_arm_section_t *arm_s = NULL;
	subtilis_string_pool_t *label_pool = NULL;

	label_pool = subtilis_string_pool_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	arm_s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, set, NULL,
					 start_address, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	context.arm_s = arm_s;
	context.l = l;
	context.t = t;
	context.stype = stype;
	context.set = set;
	context.sys_trans = sys_trans;
	context.label_pool = label_pool;
	context.def_count = 0;
	context.max_defs = 0;
	context.defs = NULL;
	context.blocks = NULL;
	context.num_blocks = 0;
	context.max_blocks = 0;
	subtilis_bitset_init(&context.pending_labels);

	prv_parser_main_loop(&context, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	pending_labels = subtilis_bitset_values(&context.pending_labels,
						&pending_labels_num, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (pending_labels_num > 0) {
		if (pending_labels[0] > label_pool->length) {
			subtilis_error_set_assertion_failed(err);
			goto cleanup;
		}
		label_name = label_pool->strings[pending_labels[0]];
		subtilis_error_set_ass_unknown_label(err, label_name,
						     l->stream->name, l->line);
		goto cleanup;
	}

	prv_context_free(&context);

	return arm_s;

cleanup:
	free(pending_labels);
	prv_context_free(&context);
	subtilis_arm_section_delete(arm_s);

	return NULL;
}

subtilis_arm_exp_val_t *subtilis_arm_asm_find_def(subtilis_arm_ass_context_t *c,
						  const char *name)
{
	size_t i;

	for (i = 0; i < c->def_count; i++)
		if (!strcmp(c->defs[i].name, name))
			return c->defs[i].val;

	return NULL;
}
