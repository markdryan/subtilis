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

#include <string.h>

#include "error.h"
#include "parser.h"

typedef void (*subtilis_keyword_fn)(subtilis_lexer_t *, subtilis_token_t *,
				    subtilis_error_t *);

const subtilis_keyword_fn keyword_fns[];

static void prv_expression(subtilis_lexer_t *l, subtilis_token_t *t,
			   subtilis_error_t *err);

static void prv_priority1(subtilis_lexer_t *l, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;

	tbuf = subtilis_token_get_text(t);
	switch (t->type) {
	case SUBTILIS_TOKEN_INTEGER:
		break;
	case SUBTILIS_TOKEN_OPERATOR:
		if (strcmp(tbuf, "(")) {
			subtilis_error_set_exp_expected(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
		prv_expression(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		tbuf = subtilis_token_get_text(t);
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
		break;
	case SUBTILIS_TOKEN_IDENTIFIER:
		/* TODO temporary check */
		if (t->tok.id_type != SUBTILIS_IDENTIFIER_INTEGER) {
			subtilis_error_set_integer_expected(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
	default:
		subtilis_error_set_exp_expected(err, tbuf, l->stream->name,
						l->line);
		break;
	}
	subtilis_lexer_get(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_priority2(subtilis_lexer_t *l, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;

	prv_priority1(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	tbuf = subtilis_token_get_text(t);
	while (!strcmp(tbuf, "*") || !strcmp(tbuf, "/")) {
		subtilis_lexer_get(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		prv_priority1(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_priority3(subtilis_lexer_t *l, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;

	prv_priority2(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	tbuf = subtilis_token_get_text(t);
	while (!strcmp(tbuf, "+") || !strcmp(tbuf, "-")) {
		subtilis_lexer_get(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		prv_priority2(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_expression(subtilis_lexer_t *l, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	subtilis_lexer_get(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	prv_priority3(l, t, err);
}

static void prv_assignment(subtilis_lexer_t *l, subtilis_token_t *t,
			   subtilis_error_t *err)
{
	const char *tbuf;
	/* TODO: Look up in the hash tables */

	subtilis_lexer_get(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, l->stream->name, l->line);
		return;
	}

	if (strcmp(tbuf, "=") && strcmp(tbuf, "+=") && strcmp(tbuf, "-=")) {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, l->stream->name, l->line);
		return;
	}
	prv_expression(l, t, err);
}

static void prv_let(subtilis_lexer_t *l, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_id_expected(err, tbuf, l->stream->name,
					       l->line);
		return;
	}

	prv_assignment(l, t, err);
}

static void prv_print(subtilis_lexer_t *l, subtilis_token_t *t,
		      subtilis_error_t *err)
{
	prv_expression(l, t, err);
}

void prv_root(subtilis_lexer_t *l, subtilis_token_t *t, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;

	subtilis_lexer_get(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_keyword_expected(
			    err, tbuf, l->stream->name, l->line);
			return;
		}

		fn = keyword_fns[t->tok.keyword.type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
		fn(l, t, err);
	}
}

void subtilis_parse(subtilis_lexer_t *l, subtilis_error_t *err)
{
	subtilis_token_t *t = NULL;

	t = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_root(l, t, err);

	subtilis_token_delete(t);
}

/* The ordering of this table is very important.  The functions it
 * contains must correspond to the enumerated types in
 * subtilis_keyword_type_t
 */

/* clang-format off */
const subtilis_keyword_fn keyword_fns[] = {
	NULL, /* SUBTILIS_KEYWORD_ABS */
	NULL, /* SUBTILIS_KEYWORD_ACS */
	NULL, /* SUBTILIS_KEYWORD_ADVAL */
	NULL, /* SUBTILIS_KEYWORD_AND */
	NULL, /* SUBTILIS_KEYWORD_APPEND */
	NULL, /* SUBTILIS_KEYWORD_ASC */
	NULL, /* SUBTILIS_KEYWORD_ASN */
	NULL, /* SUBTILIS_KEYWORD_ATN */
	NULL, /* SUBTILIS_KEYWORD_AUTO */
	NULL, /* SUBTILIS_KEYWORD_BEAT */
	NULL, /* SUBTILIS_KEYWORD_BEATS */
	NULL, /* SUBTILIS_KEYWORD_BGET_HASH */
	NULL, /* SUBTILIS_KEYWORD_BPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_BY */
	NULL, /* SUBTILIS_KEYWORD_CALL */
	NULL, /* SUBTILIS_KEYWORD_CASE */
	NULL, /* SUBTILIS_KEYWORD_CHAIN */
	NULL, /* SUBTILIS_KEYWORD_CHR_STR */
	NULL, /* SUBTILIS_KEYWORD_CIRCLE */
	NULL, /* SUBTILIS_KEYWORD_CLEAR */
	NULL, /* SUBTILIS_KEYWORD_CLG */
	NULL, /* SUBTILIS_KEYWORD_CLOSE_HASH */
	NULL, /* SUBTILIS_KEYWORD_CLS */
	NULL, /* SUBTILIS_KEYWORD_COLOR */
	NULL, /* SUBTILIS_KEYWORD_COLOUR */
	NULL, /* SUBTILIS_KEYWORD_COS */
	NULL, /* SUBTILIS_KEYWORD_COUNT */
	NULL, /* SUBTILIS_KEYWORD_CRUNCH */
	NULL, /* SUBTILIS_KEYWORD_DATA */
	NULL, /* SUBTILIS_KEYWORD_DEF */
	NULL, /* SUBTILIS_KEYWORD_DEG */
	NULL, /* SUBTILIS_KEYWORD_DELETE */
	NULL, /* SUBTILIS_KEYWORD_DIM */
	NULL, /* SUBTILIS_KEYWORD_DIV */
	NULL, /* SUBTILIS_KEYWORD_DRAW */
	NULL, /* SUBTILIS_KEYWORD_EDIT */
	NULL, /* SUBTILIS_KEYWORD_ELLIPSE */
	NULL, /* SUBTILIS_KEYWORD_ELSE */
	NULL, /* SUBTILIS_KEYWORD_END */
	NULL, /* SUBTILIS_KEYWORD_ENDCASE */
	NULL, /* SUBTILIS_KEYWORD_ENDIF */
	NULL, /* SUBTILIS_KEYWORD_ENDPROC */
	NULL, /* SUBTILIS_KEYWORD_ENDWHILE */
	NULL, /* SUBTILIS_KEYWORD_EOF_HASH */
	NULL, /* SUBTILIS_KEYWORD_EOR */
	NULL, /* SUBTILIS_KEYWORD_ERL */
	NULL, /* SUBTILIS_KEYWORD_ERR */
	NULL, /* SUBTILIS_KEYWORD_ERROR */
	NULL, /* SUBTILIS_KEYWORD_EVAL */
	NULL, /* SUBTILIS_KEYWORD_EXP */
	NULL, /* SUBTILIS_KEYWORD_EXT_HASH */
	NULL, /* SUBTILIS_KEYWORD_FALSE */
	NULL, /* SUBTILIS_KEYWORD_FILL */
	NULL, /* SUBTILIS_KEYWORD_FN */
	NULL, /* SUBTILIS_KEYWORD_FOR */
	NULL, /* SUBTILIS_KEYWORD_GCOL */
	NULL, /* SUBTILIS_KEYWORD_GET */
	NULL, /* SUBTILIS_KEYWORD_GET_STR */
	NULL, /* SUBTILIS_KEYWORD_GET_STR_HASH */
	NULL, /* SUBTILIS_KEYWORD_GOSUB */
	NULL, /* SUBTILIS_KEYWORD_GOTO */
	NULL, /* SUBTILIS_KEYWORD_HELP */
	NULL, /* SUBTILIS_KEYWORD_HIMEM */
	NULL, /* SUBTILIS_KEYWORD_IF */
	NULL, /* SUBTILIS_KEYWORD_INKEY */
	NULL, /* SUBTILIS_KEYWORD_INKEY_STR */
	NULL, /* SUBTILIS_KEYWORD_INPUT */
	NULL, /* SUBTILIS_KEYWORD_INPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_INSTALL */
	NULL, /* SUBTILIS_KEYWORD_INSTR */
	NULL, /* SUBTILIS_KEYWORD_INT */
	NULL, /* SUBTILIS_KEYWORD_LEFT_STR */
	NULL, /* SUBTILIS_KEYWORD_LEN */
	prv_let, /* SUBTILIS_KEYWORD_LET */
	NULL, /* SUBTILIS_KEYWORD_LIBRARY */
	NULL, /* SUBTILIS_KEYWORD_LINE */
	NULL, /* SUBTILIS_KEYWORD_LIST */
	NULL, /* SUBTILIS_KEYWORD_LISTO */
	NULL, /* SUBTILIS_KEYWORD_LN */
	NULL, /* SUBTILIS_KEYWORD_LOAD */
	NULL, /* SUBTILIS_KEYWORD_LOCAL */
	NULL, /* SUBTILIS_KEYWORD_LOG */
	NULL, /* SUBTILIS_KEYWORD_LOMEM */
	NULL, /* SUBTILIS_KEYWORD_LVAR */
	NULL, /* SUBTILIS_KEYWORD_MID_STR */
	NULL, /* SUBTILIS_KEYWORD_MOD */
	NULL, /* SUBTILIS_KEYWORD_MODE */
	NULL, /* SUBTILIS_KEYWORD_MOUSE */
	NULL, /* SUBTILIS_KEYWORD_MOVE */
	NULL, /* SUBTILIS_KEYWORD_NEW */
	NULL, /* SUBTILIS_KEYWORD_NEXT */
	NULL, /* SUBTILIS_KEYWORD_NOT */
	NULL, /* SUBTILIS_KEYWORD_OF */
	NULL, /* SUBTILIS_KEYWORD_OFF */
	NULL, /* SUBTILIS_KEYWORD_OLD */
	NULL, /* SUBTILIS_KEYWORD_ON */
	NULL, /* SUBTILIS_KEYWORD_OPENIN */
	NULL, /* SUBTILIS_KEYWORD_OPENOUT */
	NULL, /* SUBTILIS_KEYWORD_OPENUP */
	NULL, /* SUBTILIS_KEYWORD_OR */
	NULL, /* SUBTILIS_KEYWORD_ORIGIN */
	NULL, /* SUBTILIS_KEYWORD_OSCLI */
	NULL, /* SUBTILIS_KEYWORD_OTHERWISE */
	NULL, /* SUBTILIS_KEYWORD_OVERLAY */
	NULL, /* SUBTILIS_KEYWORD_PAGE */
	NULL, /* SUBTILIS_KEYWORD_PI */
	NULL, /* SUBTILIS_KEYWORD_PLOT */
	NULL, /* SUBTILIS_KEYWORD_POINT */
	NULL, /* SUBTILIS_KEYWORD_POS */
	prv_print, /* SUBTILIS_KEYWORD_PRINT */
	NULL, /* SUBTILIS_KEYWORD_PRINT_HASH */
	NULL, /* SUBTILIS_KEYWORD_PROC */
	NULL, /* SUBTILIS_KEYWORD_PTR_HASH */
	NULL, /* SUBTILIS_KEYWORD_QUIT */
	NULL, /* SUBTILIS_KEYWORD_RAD */
	NULL, /* SUBTILIS_KEYWORD_READ */
	NULL, /* SUBTILIS_KEYWORD_RECTANGLE */
	NULL, /* SUBTILIS_KEYWORD_REM */
	NULL, /* SUBTILIS_KEYWORD_RENUMBER */
	NULL, /* SUBTILIS_KEYWORD_REPEAT */
	NULL, /* SUBTILIS_KEYWORD_REPORT */
	NULL, /* SUBTILIS_KEYWORD_REPORT_STR */
	NULL, /* SUBTILIS_KEYWORD_RESTORE */
	NULL, /* SUBTILIS_KEYWORD_RETURN */
	NULL, /* SUBTILIS_KEYWORD_RIGHT_STR */
	NULL, /* SUBTILIS_KEYWORD_RND */
	NULL, /* SUBTILIS_KEYWORD_RUN */
	NULL, /* SUBTILIS_KEYWORD_SAVE */
	NULL, /* SUBTILIS_KEYWORD_SGN */
	NULL, /* SUBTILIS_KEYWORD_SIN */
	NULL, /* SUBTILIS_KEYWORD_SOUND */
	NULL, /* SUBTILIS_KEYWORD_SPC */
	NULL, /* SUBTILIS_KEYWORD_SQR */
	NULL, /* SUBTILIS_KEYWORD_STEP */
	NULL, /* SUBTILIS_KEYWORD_STEREO */
	NULL, /* SUBTILIS_KEYWORD_STOP */
	NULL, /* SUBTILIS_KEYWORD_STRING_STR */
	NULL, /* SUBTILIS_KEYWORD_STRS */
	NULL, /* SUBTILIS_KEYWORD_SUM */
	NULL, /* SUBTILIS_KEYWORD_SUMLEN */
	NULL, /* SUBTILIS_KEYWORD_SWAP */
	NULL, /* SUBTILIS_KEYWORD_SYS */
	NULL, /* SUBTILIS_KEYWORD_TAB */
	NULL, /* SUBTILIS_KEYWORD_TAN */
	NULL, /* SUBTILIS_KEYWORD_TEMPO */
	NULL, /* SUBTILIS_KEYWORD_TEXTLOAD */
	NULL, /* SUBTILIS_KEYWORD_TEXTSAVE */
	NULL, /* SUBTILIS_KEYWORD_THEN */
	NULL, /* SUBTILIS_KEYWORD_TIME */
	NULL, /* SUBTILIS_KEYWORD_TIME_STR */
	NULL, /* SUBTILIS_KEYWORD_TINT */
	NULL, /* SUBTILIS_KEYWORD_TO */
	NULL, /* SUBTILIS_KEYWORD_TOP */
	NULL, /* SUBTILIS_KEYWORD_TRACE */
	NULL, /* SUBTILIS_KEYWORD_TRUE */
	NULL, /* SUBTILIS_KEYWORD_TWIN */
	NULL, /* SUBTILIS_KEYWORD_UNTIL */
	NULL, /* SUBTILIS_KEYWORD_USR */
	NULL, /* SUBTILIS_KEYWORD_VAL */
	NULL, /* SUBTILIS_KEYWORD_VDU */
	NULL, /* SUBTILIS_KEYWORD_VOICES */
	NULL, /* SUBTILIS_KEYWORD_VPOS */
	NULL, /* SUBTILIS_KEYWORD_WAIT */
	NULL, /* SUBTILIS_KEYWORD_WHEN */
	NULL, /* SUBTILIS_KEYWORD_WHILE */
	NULL, /* SUBTILIS_KEYWORD_WIDTH */
};

/* clang-format on */
