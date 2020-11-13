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
	{ "BL", SUBTILIS_ARM_INSTR_B, NULL, },
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
	{ "FDV", SUBTILIS_FPA_INSTR_FDV, NULL, },
	{ "FIX", SUBTILIS_FPA_INSTR_FIX, NULL, },
	{ "FLT", SUBTILIS_FPA_INSTR_FLT, NULL, },
	{ "FML", SUBTILIS_FPA_INSTR_FML, NULL, },
	{ "FRD", SUBTILIS_FPA_INSTR_FRD, NULL, },
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
	{ "SBC", SUBTILIS_ARM_INSTR_SBC, NULL, },
	{ "SIN", SUBTILIS_FPA_INSTR_SIN, NULL, },
	{ "SQT", SUBTILIS_FPA_INSTR_SQT, NULL, },
	{ "STF", SUBTILIS_FPA_INSTR_STF, NULL, },
	{ "STM", SUBTILIS_ARM_INSTR_STM, NULL, },
	{ "STR", SUBTILIS_ARM_INSTR_STR, NULL, },
	{ "SUB", SUBTILIS_ARM_INSTR_SUB, NULL, },
	{ "SUF", SUBTILIS_FPA_INSTR_SUF, NULL, },
	{ "SWI", SUBTILIS_ARM_INSTR_SWI, NULL, },
};

static const subtilis_arm_ass_mnemomic_t t_mnem[] = {
	{ "TAN", SUBTILIS_FPA_INSTR_TAN, NULL, },
	{ "TEQ", SUBTILIS_ARM_INSTR_TEQ, NULL, },
	{ "TST", SUBTILIS_ARM_INSTR_TST, NULL, },
};

static const subtilis_arm_ass_mnemomic_t u_mnem[] = {
	{ "URD", SUBTILIS_FPA_INSTR_URD, NULL, },
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
	size_t index;

	if (subtilis_string_pool_find(c->label_pool, name, &index)) {
		if (subtilis_bitset_isset(&c->pending_labels, index)) {
			subtilis_bitset_clear(&c->pending_labels, index);
		} else {
			subtilis_error_set_already_defined(
			    err, name, c->l->stream->name, c->l->line);
			return;
		}
	} else {
		index = subtilis_string_pool_register(c->label_pool, name, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	subtilis_arm_section_add_label(c->arm_s, index, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(c->l, c->t, err);
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
	bool link = (strlen(name) > 1) && (name[1] == 'l' || name[1] == 'L');

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

static void prv_parse_stran_pre(subtilis_arm_ass_context_t *c,
				subtilis_arm_instr_type_t itype,
				subtilis_arm_ccode_type_t ccode,
				subtilis_arm_reg_t dest,
				subtilis_arm_reg_t base, bool byte,
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

	prv_parse_stran_pre_off(c, itype, ccode, dest, base, byte, offset,
				subtract, err);

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran_post(subtilis_arm_ass_context_t *c,
				 subtilis_arm_instr_type_t itype,
				 subtilis_arm_ccode_type_t ccode,
				 subtilis_arm_reg_t dest,
				 subtilis_arm_reg_t base, bool byte,
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
		offset.type = SUBTILIS_ARM_OP2_I32;
		offset.op.integer = 0;
		prv_parse_stran_pre_off(c, itype, ccode, dest, base, byte,
					offset, false, err);
		return;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val = prv_get_offset(c, &subtract, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

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
	stran->byte = byte;
	stran->subtract = subtract;

cleanup:

	subtilis_arm_exp_val_free(val);
}

static void prv_parse_stran(subtilis_arm_ass_context_t *c,
			    subtilis_arm_instr_type_t itype,
			    subtilis_arm_ccode_type_t ccode, bool byte,
			    subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t base;
	const char *tbuf;

	dest = prv_get_reg(c, err);
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
		if (!strcmp(tbuf, "]")) {
			prv_parse_stran_post(c, itype, ccode, dest, base, byte,
					     err);
			return;
		}
		if (!strcmp(tbuf, ",")) {
			prv_parse_stran_pre(c, itype, ccode, dest, base, byte,
					    err);
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

static void prv_parse_instruction(subtilis_arm_ass_context_t *c,
				  const char *name,
				  subtilis_arm_instr_type_t itype,
				  subtilis_arm_ccode_type_t ccode, bool status,
				  subtilis_arm_mtran_type_t mtran_type,
				  bool byte, bool teqp, subtilis_error_t *err)
{
	switch (itype) {
	case SUBTILIS_ARM_INSTR_B:
		prv_parse_branch(c, name, itype, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_MOV:
	case SUBTILIS_ARM_INSTR_MVN:
		prv_parse_arm_2_arg(c, itype, ccode, status, err);
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
		prv_parse_arm_data(c, itype, ccode, status, err);
		break;
	case SUBTILIS_ARM_INSTR_MUL:
	case SUBTILIS_ARM_INSTR_MLA:
		prv_parse_arm_mul(c, itype, ccode, status, err);
		break;
	case SUBTILIS_ARM_INSTR_TST:
	case SUBTILIS_ARM_INSTR_TEQ:
	case SUBTILIS_ARM_INSTR_CMP:
	case SUBTILIS_ARM_INSTR_CMN:
		prv_parse_cmp(c, itype, ccode, true, teqp, err);
		break;
	case SUBTILIS_ARM_INSTR_SWI:
		prv_parse_swi(c, ccode, err);
		break;
	case SUBTILIS_ARM_INSTR_LDR:
	case SUBTILIS_ARM_INSTR_STR:
		prv_parse_stran(c, itype, ccode, byte, err);
		break;
	case SUBTILIS_ARM_INSTR_LDM:
	case SUBTILIS_ARM_INSTR_STM:
		prv_parse_mtran(c, itype, ccode, mtran_type, err);
		break;
	case SUBTILIS_ARM_INSTR_ADR:
		prv_parse_adr(c, ccode, err);
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
		if (tbuf[0] == ',') {
			prv_parse_fpa_stran_pre(c, itype, ccode, size, dest,
						base, err);
			return;
		}

		if (tbuf[0] == ']') {
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
	size_t ptr;
	size_t search_len;
	subtilis_arm_instr_type_t itype;
	subtilis_arm_iclass_t iclass;
	subtilis_arm_mtran_type_t mtran_type = SUBTILIS_ARM_MTRAN_IA;
	subtilis_arm_ccode_type_t ccode = SUBTILIS_ARM_CCODE_AL;
	bool status = false;
	bool byte = false;
	bool teqp = false;
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
	ptr = max_length;
	if (ptr == token_end) {
		if ((itype == SUBTILIS_ARM_INSTR_LDM) ||
		    (itype == SUBTILIS_ARM_INSTR_STM))
			prv_parse_label(c, tbuf, err);
		else
			prv_parse_instruction(c, tbuf, itype, ccode, status,
					      mtran_type, byte, false, err);
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
		status = tbuf[ptr] == 'S' && status_valid;
		byte = tbuf[ptr] == 'B' && byte_valid;
		teqp = tbuf[ptr] == 'P' && itype == SUBTILIS_ARM_INSTR_TEQ;
		if (!status && !byte && !teqp) {
			prv_parse_label(c, tbuf, err);
			return;
		}
	} else if (ptr != token_end) {
		prv_parse_label(c, tbuf, err);
		return;
	}

	prv_parse_instruction(c, tbuf, itype, ccode, status, mtran_type, byte,
			      teqp, err);
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
	if (c->t->type != SUBTILIS_TOKEN_OPERATOR || tbuf[0] != '=') {
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
	subtilis_backend_sys_trans sys_trans, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_ass_context_t context;
	size_t *pending_labels;
	size_t pending_labels_num;
	const char *label_name;
	subtilis_arm_section_t *arm_s = NULL;
	subtilis_string_pool_t *label_pool = NULL;

	label_pool = subtilis_string_pool_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	arm_s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, set, err);
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
