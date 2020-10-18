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

#include <string.h>

#include "arm_core.h"
#include "arm_expression.h"
#include "arm_keywords.h"
#include "assembler.h"

typedef void (*subtilis_arm_ass_fn_t)(void *);

struct subtilis_arm_ass_mnemomic_t_ {
	const char *name;
	subtilis_arm_instr_type_t type;
	subtilis_arm_ass_fn_t asm_fn;
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

static const subtilis_arm_ass_mnemomic_t a_mnem[] = {
	{ "ABS", SUBTILIS_FPA_INSTR_ABS, NULL },
	{ "ACS", SUBTILIS_FPA_INSTR_ACS, NULL },
	{ "ADC", SUBTILIS_ARM_INSTR_ADC, NULL },
	{ "ADD", SUBTILIS_ARM_INSTR_ADD, NULL },
	{ "ADF", SUBTILIS_FPA_INSTR_ADF, NULL },
	{ "AND", SUBTILIS_ARM_INSTR_AND, NULL },
	{ "ASN", SUBTILIS_FPA_INSTR_ASN, NULL },
	{ "ATN", SUBTILIS_FPA_INSTR_ATN, NULL },
};

static const subtilis_arm_ass_mnemomic_t b_mnem[] = {
	{ "B", SUBTILIS_ARM_INSTR_B, NULL },
	{ "BIC", SUBTILIS_ARM_INSTR_BIC, NULL },
};

static const subtilis_arm_ass_mnemomic_t c_mnem[] = {
	{ "CMF", SUBTILIS_FPA_INSTR_CMF, NULL },
	{ "CMFE", SUBTILIS_FPA_INSTR_CMFE, NULL },
	{ "CMN", SUBTILIS_ARM_INSTR_CMN, NULL },
	{ "CMP", SUBTILIS_ARM_INSTR_CMP, NULL },
	{ "CNF", SUBTILIS_FPA_INSTR_CNF, NULL },
	{ "CNFE", SUBTILIS_FPA_INSTR_CNFE, NULL },
	{ "COS", SUBTILIS_FPA_INSTR_COS, NULL },
};

static const subtilis_arm_ass_mnemomic_t d_mnem[] = {
	{ "DVF", SUBTILIS_FPA_INSTR_DVF, NULL },
};

static const subtilis_arm_ass_mnemomic_t e_mnem[] = {
	{ "EOR", SUBTILIS_ARM_INSTR_EOR, NULL },
	{ "EXP", SUBTILIS_FPA_INSTR_EXP, NULL },
};

static const subtilis_arm_ass_mnemomic_t f_mnem[] = {
	{ "FDV", SUBTILIS_FPA_INSTR_FDV, NULL },
	{ "FIX", SUBTILIS_FPA_INSTR_FIX, NULL },
	{ "FLT", SUBTILIS_FPA_INSTR_FLT, NULL },
	{ "FML", SUBTILIS_FPA_INSTR_FML, NULL },
	{ "FRD", SUBTILIS_FPA_INSTR_FRD, NULL },
};

static const subtilis_arm_ass_mnemomic_t l_mnem[] = {
	{ "LDF", SUBTILIS_FPA_INSTR_LDF, NULL },
	{ "LDM", SUBTILIS_ARM_INSTR_LDM, NULL },
	{ "LDR", SUBTILIS_ARM_INSTR_LDR, NULL },
	{ "LGN", SUBTILIS_FPA_INSTR_LGN, NULL },
	{ "LOG", SUBTILIS_FPA_INSTR_LOG, NULL },
};

static const subtilis_arm_ass_mnemomic_t m_mnem[] = {
	{ "MLA", SUBTILIS_ARM_INSTR_MLA, NULL },
	{ "MNF", SUBTILIS_FPA_INSTR_MNF, NULL },
	{ "MOV", SUBTILIS_ARM_INSTR_MOV, NULL },
	{ "MUF", SUBTILIS_FPA_INSTR_MUF, NULL },
	{ "MUL", SUBTILIS_ARM_INSTR_MUL, NULL },
	{ "MVF", SUBTILIS_FPA_INSTR_MVF, NULL },
	{ "MVN", SUBTILIS_ARM_INSTR_MVN, NULL },
};

static const subtilis_arm_ass_mnemomic_t n_mnem[] = {
	{ "NRM", SUBTILIS_FPA_INSTR_NRM, NULL },
};

static const subtilis_arm_ass_mnemomic_t o_mnem[] = {
	{ "ORR", SUBTILIS_ARM_INSTR_ORR, NULL },
};

static const subtilis_arm_ass_mnemomic_t p_mnem[] = {
	{ "POL", SUBTILIS_FPA_INSTR_POL, NULL },
	{ "POW", SUBTILIS_FPA_INSTR_POW, NULL },
};

static const subtilis_arm_ass_mnemomic_t r_mnem[] = {
	{ "RDF", SUBTILIS_FPA_INSTR_RDF, NULL },
	{ "RFS", SUBTILIS_FPA_INSTR_RFS, NULL },
	{ "RMF", SUBTILIS_FPA_INSTR_RMF, NULL },
	{ "RND", SUBTILIS_FPA_INSTR_RND, NULL },
	{ "RPW", SUBTILIS_FPA_INSTR_RPW, NULL },
	{ "RSB", SUBTILIS_ARM_INSTR_RSB, NULL },
	{ "RSC", SUBTILIS_ARM_INSTR_RSC, NULL },
	{ "RSF", SUBTILIS_FPA_INSTR_RSF, NULL },
};

static const subtilis_arm_ass_mnemomic_t s_mnem[] = {
	{ "SBC", SUBTILIS_ARM_INSTR_SBC, NULL },
	{ "SIN", SUBTILIS_FPA_INSTR_SIN, NULL },
	{ "SQT", SUBTILIS_FPA_INSTR_SQT, NULL },
	{ "STF", SUBTILIS_FPA_INSTR_STF, NULL },
	{ "STM", SUBTILIS_ARM_INSTR_STM, NULL },
	{ "STR", SUBTILIS_ARM_INSTR_STR, NULL },
	{ "SUB", SUBTILIS_ARM_INSTR_SUB, NULL },
	{ "SUF", SUBTILIS_FPA_INSTR_SUF, NULL },
	{ "SWI", SUBTILIS_ARM_INSTR_SWI, NULL },
};

static const subtilis_arm_ass_mnemomic_t t_mnem[] = {
	{ "TAN", SUBTILIS_FPA_INSTR_TAN, NULL },
	{ "TEQ", SUBTILIS_ARM_INSTR_TEQ, NULL },
	{ "TST", SUBTILIS_ARM_INSTR_TST, NULL },
};

static const subtilis_arm_ass_mnemomic_t u_mnem[] = {
	{ "URD", SUBTILIS_FPA_INSTR_URD, NULL },
};

static const subtilis_arm_ass_mnemomic_t w_mnem[] = {
	{ "WFS", SUBTILIS_FPA_INSTR_WFS, NULL },
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
	{ f_mnem, sizeof(f_mnem) / sizeof(f_mnem[0]), 3},
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
	{ NULL, 0, 0 }, /* Q */
	{ r_mnem, sizeof(r_mnem) / sizeof(r_mnem[0]), 3},
	{ s_mnem, sizeof(s_mnem) / sizeof(s_mnem[0]), 3},
	{ t_mnem, sizeof(t_mnem) / sizeof(t_mnem[0]), 3},
	{ u_mnem, sizeof(u_mnem) / sizeof(u_mnem[0]), 3},
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

static void prv_parse_instruction(subtilis_arm_ass_context_t *c,
				  const char *name,
				  subtilis_arm_instr_type_t itype,
				  subtilis_arm_ccode_type_t ccode, bool status,
				  subtilis_error_t *err)
{
	switch (itype) {
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
		prv_parse_arm_data(c, itype, ccode, status, err);
		break;
	default:
		subtilis_error_set_not_supported(err, name, c->l->stream->name,
						 c->l->line);
		break;
	}
}

static void prv_parse_identifier(subtilis_arm_ass_context_t *c,
				 subtilis_error_t *err)
{
	const char *tbuf;
	size_t index;
	size_t i;
	size_t base_len;
	size_t token_end;
	bool status;
	size_t ptr;
	subtilis_arm_ccode_type_t ccode;
	subtilis_arm_instr_type_t itype;
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

	for (i = 0; i < keyword_map[index].count; i++)
		if (!strncmp(keyword_map[index].mnems[i].name, tbuf,
			     keyword_map[index].max_length)) {
			base_len = strlen(keyword_map[index].mnems[i].name);
			if (base_len > max_length) {
				max_length = base_len;
				max_index = i;
			}
		}

	if (max_index == SIZE_MAX) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	/*
	 * It might still be an identifier
	 */

	itype = keyword_map[index].mnems[max_index].type;

	ptr = max_length;
	token_end = strlen(tbuf);
	if (ptr == token_end) {
		status = false;
		ccode = SUBTILIS_ARM_CCODE_AL;
	} else {
		if (tbuf[ptr] == 'S') {
			status = true;
			ptr++;
		}
		if (ptr == token_end) {
			ccode = SUBTILIS_ARM_CCODE_AL;
		} else if (ptr + 2 != token_end) {
			prv_parse_label(c, tbuf, err);
			return;
		}
		for (i = 0; i < sizeof(condition_codes) / sizeof(const char *);
		     i++)
			if ((tbuf[ptr] == condition_codes[i][0]) &&
			    (tbuf[ptr + 1] == condition_codes[i][1]))
				break;

		if (i == sizeof(condition_codes) / sizeof(const char *)) {
			prv_parse_label(c, tbuf, err);
			return;
		}
		ccode = (subtilis_arm_ccode_type_t)i;
	}

	prv_parse_instruction(c, tbuf, itype, ccode, status, err);
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
}

/* clang-format off */
subtilis_arm_section_t *subtilis_arm_asm_parse(
	subtilis_lexer_t *l, subtilis_token_t *t, subtilis_arm_op_pool_t *pool,
	subtilis_type_section_t *stype, const subtilis_settings_t *set,
	subtilis_arm_swi_info_t *swi_info, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_section_t *arm_s;
	subtilis_arm_ass_context_t context;

	arm_s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, set, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	context.arm_s = arm_s;
	context.l = l;
	context.t = t;
	context.stype = stype;
	context.set = set;
	context.swi_info = swi_info;

	prv_parser_main_loop(&context, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return arm_s;

cleanup:

	subtilis_arm_section_delete(arm_s);

	return NULL;
}
