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

#include "math.h"
#include <stdlib.h>
#include <string.h>

#include "arm_expression.h"

#include "../../common/buffer.h"
#include "arm_keywords.h"

/*
 * TODO: This is almost entirely generic and needs to be moved
 * to a common area so it can be reused by future backends.
 */

typedef int32_t (*subtilis_ass_int_fn_t)(subtilis_arm_ass_context_t *, int32_t,
					 int32_t, subtilis_error_t *);
typedef double (*subtilis_ass_real_fn_t)(subtilis_arm_ass_context_t *, double,
					 double, subtilis_error_t *);
typedef int32_t (*subtilis_ass_log_int_fn_t)(int32_t, int32_t);
typedef int32_t (*subtilis_ass_log_real_fn_t)(double, double);

static subtilis_arm_exp_val_t *prv_priority1(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err);

subtilis_arm_exp_val_t *subtilis_arm_exp_new_int32(int32_t val,
						   subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	e->type = SUBTILIS_ARM_EXP_TYPE_INT;
	e->val.integer = val;

	return e;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_real(double val,
						  subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = SUBTILIS_ARM_EXP_TYPE_REAL;
	e->val.real = val;

	return e;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_str(subtilis_buffer_t *buf,
						 subtilis_error_t *err)
{
	size_t len;
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = SUBTILIS_ARM_EXP_TYPE_STRING;
	len = subtilis_buffer_get_size(buf);
	subtilis_buffer_init(&e->val.buf, len);
	subtilis_buffer_append(&e->val.buf, buf->buffer->data, len, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_arm_exp_val_free(e);
		return NULL;
	}

	return e;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_id(const char *id,
						subtilis_error_t *err)
{
	size_t len;
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = SUBTILIS_ARM_EXP_TYPE_ID;
	len = strlen(id);
	subtilis_buffer_init(&e->val.buf, len);
	subtilis_buffer_append(&e->val.buf, id, len, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_arm_exp_val_free(e);
		return NULL;
	}

	return e;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_reg(subtilis_arm_reg_t reg,
						 subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = SUBTILIS_ARM_EXP_TYPE_REG;
	e->val.reg = reg;

	return e;
}

static int32_t prv_exp_to_int(subtilis_arm_ass_context_t *c,
			      subtilis_arm_exp_val_t *e1, subtilis_error_t *err)
{
	switch (e1->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		return e1->val.integer;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		return (int32_t)e1->val.real;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		return 0;
	}
}

static double prv_exp_to_real(subtilis_arm_ass_context_t *c,
			      subtilis_arm_exp_val_t *e1, subtilis_error_t *err)
{
	switch (e1->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		return (double)e1->val.integer;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		return e1->val.real;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		return 0;
	}
}

static subtilis_arm_exp_val_t *prv_binary_int(subtilis_arm_ass_context_t *c,
					      subtilis_arm_exp_val_t *e1,
					      subtilis_arm_exp_val_t *e2,
					      subtilis_ass_int_fn_t fn,
					      subtilis_error_t *err)
{
	int32_t val1;
	int32_t val2;

	val1 = prv_exp_to_int(c, e1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val2 = prv_exp_to_int(c, e2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e1->type = SUBTILIS_ARM_EXP_TYPE_INT;
	e1->val.integer = fn(c, val1, val2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_exp_val_free(e2);
	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_binary_real(subtilis_arm_ass_context_t *c,
					       subtilis_arm_exp_val_t *e1,
					       subtilis_arm_exp_val_t *e2,
					       subtilis_ass_real_fn_t fn,
					       subtilis_error_t *err)
{
	double val1;
	double val2;

	val1 = prv_exp_to_real(c, e1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val2 = prv_exp_to_real(c, e2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e1->type = SUBTILIS_ARM_EXP_TYPE_REAL;
	e1->val.real = fn(c, val1, val2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_exp_val_free(e2);
	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_logical_int(subtilis_arm_ass_context_t *c,
					       subtilis_arm_exp_val_t *e1,
					       subtilis_arm_exp_val_t *e2,
					       subtilis_ass_log_int_fn_t fn,
					       subtilis_error_t *err)
{
	int32_t val1;
	int32_t val2;

	val1 = prv_exp_to_int(c, e1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val2 = prv_exp_to_int(c, e2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e1->type = SUBTILIS_ARM_EXP_TYPE_INT;
	e1->val.integer = fn(val1, val2);

	subtilis_arm_exp_val_free(e2);
	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_logical_real(subtilis_arm_ass_context_t *c,
						subtilis_arm_exp_val_t *e1,
						subtilis_arm_exp_val_t *e2,
						subtilis_ass_log_real_fn_t fn,
						subtilis_error_t *err)
{
	double val1;
	double val2;

	val1 = prv_exp_to_real(c, e1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val2 = prv_exp_to_real(c, e2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e1->type = SUBTILIS_ARM_EXP_TYPE_INT;
	e1->val.integer = fn(val1, val2);
	subtilis_arm_exp_val_free(e2);
	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);

	return NULL;
}

static subtilis_arm_exp_val_t *
prv_binary_numeric(subtilis_arm_ass_context_t *c, subtilis_arm_exp_val_t *e1,
		   subtilis_arm_exp_val_t *e2, subtilis_ass_int_fn_t int_fn,
		   subtilis_ass_real_fn_t real_fn, subtilis_error_t *err)
{
	if ((e1->type == SUBTILIS_ARM_EXP_TYPE_INT) &&
	    (e2->type == SUBTILIS_ARM_EXP_TYPE_INT)) {
		return prv_binary_int(c, e1, e2, int_fn, err);
	}

	return prv_binary_real(c, e1, e2, real_fn, err);
}

static subtilis_arm_exp_val_t *
prv_logical_numeric(subtilis_arm_ass_context_t *c, subtilis_arm_exp_val_t *e1,
		    subtilis_arm_exp_val_t *e2,
		    subtilis_ass_log_int_fn_t int_fn,
		    subtilis_ass_log_real_fn_t rfn, subtilis_error_t *err)
{
	if ((e1->type == SUBTILIS_ARM_EXP_TYPE_INT) &&
	    (e2->type == SUBTILIS_ARM_EXP_TYPE_INT)) {
		return prv_logical_int(c, e1, e2, int_fn, err);
	}

	return prv_logical_real(c, e1, e2, rfn, err);
}

static subtilis_arm_reg_t parse_reg(subtilis_arm_ass_context_t *c,
				    const char *id, subtilis_error_t *err)
{
	size_t reg;
	size_t name_len;

	if ((id[0] != 'R') && (id[0] != 'r'))
		return SIZE_MAX;

	name_len = strlen(&id[1]);
	if (name_len != 1 && name_len != 2)
		return SIZE_MAX;

	if (id[1] < '0' || id[1] > '9')
		return SIZE_MAX;
	reg = id[1] - '0';
	if (name_len == 2) {
		if (id[2] < '0' || id[2] > '9')
			return SIZE_MAX;
		reg *= 10;
		reg += id[2] - '0';
	}

	if (reg > 15) {
		subtilis_error_set_ass_bad_reg(err, id, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}

	return (subtilis_arm_reg_t)reg;
}

static subtilis_arm_exp_val_t *prv_process_id(subtilis_arm_ass_context_t *c,
					      const char *id,
					      subtilis_error_t *err)
{
	subtilis_arm_reg_t reg;

	reg = parse_reg(c, id, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (reg == SIZE_MAX)
		return subtilis_arm_exp_new_id(id, err);

	return subtilis_arm_exp_new_reg(reg, err);
}

static subtilis_arm_exp_val_t *
prv_process_bktd_exp(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e;

	e = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_arm_exp_val_free(e);
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, c->l->stream->name, c->l->line);
		return NULL;
	}

	return e;
}

static subtilis_arm_exp_val_t *
prv_unary_minus_exp(subtilis_arm_ass_context_t *c, subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type == SUBTILIS_ARM_EXP_TYPE_REAL)
		e->val.real = -e->val.real;
	else if (e->type == SUBTILIS_ARM_EXP_TYPE_INT)
		e->val.integer = -e->val.integer;
	else
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);

	return e;
}

static subtilis_arm_exp_val_t *prv_not(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e->type = SUBTILIS_ARM_EXP_TYPE_INT;
	e->val.integer = ~prv_exp_to_int(c, e, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_arm_exp_val_free(e);
		return NULL;
	}

	return e;
}

static subtilis_arm_exp_val_t *prv_priority1(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e = NULL;

	tbuf = subtilis_token_get_text(c->t);
	switch (c->t->type) {
	case SUBTILIS_TOKEN_STRING:
		e = subtilis_arm_exp_new_str(&c->t->buf, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_INTEGER:
		e = subtilis_arm_exp_new_int32(c->t->tok.integer, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_REAL:
		e = subtilis_arm_exp_new_real(c->t->tok.real, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_OPERATOR:
		if (!strcmp(tbuf, "(")) {
			e = prv_process_bktd_exp(c, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			if (!strcmp(tbuf, "-")) {
				e = prv_unary_minus_exp(c, err);
				if (err->type != SUBTILIS_ERROR_OK)
					goto cleanup;

				/* we don't want to read another token here.It's
				 * already been read by the recursive call to
				 * prv_priority1.
				 */

				return e;
			}
			subtilis_error_set_exp_expected(
			    err, "( or - ", c->l->stream->name, c->l->line);
			goto cleanup;
		}
		break;
	case SUBTILIS_TOKEN_IDENTIFIER:
		e = prv_process_id(c, tbuf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		break;

	case SUBTILIS_TOKEN_KEYWORD:
		switch (c->t->tok.keyword.type) {
		case SUBTILIS_ARM_KEYWORD_TRUE:
			e = subtilis_arm_exp_new_int32(-1, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_ARM_KEYWORD_FALSE:
			e = subtilis_arm_exp_new_int32(0, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_ARM_KEYWORD_NOT:
			e = prv_not(c, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			return e;
		/*
	case SUBTILIS_KEYWORD_INT:
		return subtilis_parser_int(p, t, err);
	case SUBTILIS_KEYWORD_SIN:
		return subtilis_parser_sin(p, t, err);
	case SUBTILIS_KEYWORD_COS:
		return subtilis_parser_cos(p, t, err);
	case SUBTILIS_KEYWORD_TAN:
		return subtilis_parser_tan(p, t, err);
	case SUBTILIS_KEYWORD_ACS:
		return subtilis_parser_acs(p, t, err);
	case SUBTILIS_KEYWORD_ASN:
		return subtilis_parser_asn(p, t, err);
	case SUBTILIS_KEYWORD_ATN:
		return subtilis_parser_atn(p, t, err);
	case SUBTILIS_KEYWORD_SQR:
		return subtilis_parser_sqr(p, t, err);
	case SUBTILIS_KEYWORD_EXP:
		return subtilis_parser_exp(p, t, err);
	case SUBTILIS_KEYWORD_LOG:
		return subtilis_parser_log(p, t, SUBTILIS_OP_INSTR_LOG,
					   log10, err);
	case SUBTILIS_KEYWORD_LN:
		return subtilis_parser_log(p, t, SUBTILIS_OP_INSTR_LN,
					   log, err);
	case SUBTILIS_KEYWORD_RAD:
		return subtilis_parser_rad(p, t, err);
	case SUBTILIS_KEYWORD_ABS:
		return subtilis_parser_abs(p, t, err);
	case SUBTILIS_KEYWORD_SGN:
		return subtilis_parser_sgn(p, t, err);
	case SUBTILIS_KEYWORD_PI:
		return subtilis_parser_pi(p, t, err);
	case SUBTILIS_KEYWORD_CHR_STR:
		return subtilis_parser_chrstr(p, t, err);
	case SUBTILIS_KEYWORD_ASC:
		return subtilis_parser_asc(p, t, err);
	case SUBTILIS_KEYWORD_LEN:
		return subtilis_parser_len(p, t, err);
	case SUBTILIS_KEYWORD_LEFT_STR:
		return subtilis_parser_left_str_exp(p, t, err);
	case SUBTILIS_KEYWORD_RIGHT_STR:
		return subtilis_parser_right_str_exp(p, t, err);
	case SUBTILIS_KEYWORD_MID_STR:
		return subtilis_parser_mid_str_exp(p, t, err);
	case SUBTILIS_KEYWORD_STRING_STR:
		return subtilis_parser_string_str(p, t, err);
	case SUBTILIS_KEYWORD_STR_STR:
		return subtilis_parser_str_str(p, t, err);
		*/
		default:
			subtilis_error_set_exp_expected(
			    err, "Unexpected keyword in expression",
			    c->l->stream->name, c->l->line);
			goto cleanup;
		}
		break;
	default:
		subtilis_error_set_exp_expected(err, tbuf, c->l->stream->name,
						c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_arm_exp_val_free(e);
	return NULL;
}

static double prv_pow_real_fn(subtilis_arm_ass_context_t *c, double a, double b,
			      subtilis_error_t *err)
{
	if (b == 0.0)
		return 1.0;

	return pow(a, b);
}

static int32_t prv_pow_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	int32_t accum;
	size_t i;

	if (b == 0)
		return 1;

	accum = a;
	for (i = 1; i < b; i++)
		accum *= a;

	return accum;
}

static subtilis_arm_exp_val_t *prv_priority2(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;

	e1 = prv_priority1(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(c->t);
		if (strcmp(tbuf, "^"))
			break;

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority1(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if ((e2->type == SUBTILIS_ARM_EXP_TYPE_INT) &&
		    (e2->val.integer < 0)) {
			e2->val.real = (double)e2->val.integer;
			e2->type = SUBTILIS_ARM_EXP_TYPE_REAL;
		}

		e1 = prv_binary_numeric(c, e1, e2, prv_pow_int32_fn,
					prv_pow_real_fn, err);

		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

static double prv_mul_real_fn(subtilis_arm_ass_context_t *c, double a, double b,
			      subtilis_error_t *err)
{
	return a * b;
}

static double prv_div_real_fn(subtilis_arm_ass_context_t *c, double a, double b,
			      subtilis_error_t *err)
{
	if (b == 0.0) {
		subtilis_error_set_divide_by_zero(err, c->l->stream->name,
						  c->l->line);
		return 0;
	}

	return a / b;
}

static int32_t prv_mul_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return a * b;
}

static int32_t prv_div_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	if (b == 0) {
		subtilis_error_set_divide_by_zero(err, c->l->stream->name,
						  c->l->line);
		return 0;
	}

	return a / b;
}

static int32_t prv_mod_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	if (b == 0) {
		subtilis_error_set_divide_by_zero(err, c->l->stream->name,
						  c->l->line);
		return 0;
	}

	return a % b;
}

static subtilis_arm_exp_val_t *prv_priority3(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;
	subtilis_token_type_t tt;
	int keyword;
	char op[4];

	e1 = prv_priority2(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while ((c->t->type == SUBTILIS_TOKEN_OPERATOR) ||
	       (c->t->type == SUBTILIS_TOKEN_KEYWORD)) {
		tbuf = subtilis_token_get_text(c->t);
		tt = c->t->type;
		if (tt == SUBTILIS_TOKEN_OPERATOR) {
			if (strcmp(tbuf, "*") && strcmp(tbuf, "/"))
				break;
			strcpy(op, tbuf);
		} else {
			keyword = c->t->tok.keyword.type;
			if ((keyword != SUBTILIS_ARM_KEYWORD_DIV) &&
			    (keyword != SUBTILIS_ARM_KEYWORD_MOD))
				break;
		}

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority2(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (tt == SUBTILIS_TOKEN_KEYWORD) {
			if (keyword == SUBTILIS_ARM_KEYWORD_DIV)
				e1 = prv_binary_int(c, e1, e2, prv_div_int32_fn,
						    err);
			else if (keyword == SUBTILIS_ARM_KEYWORD_MOD)
				e1 = prv_binary_int(c, e1, e2, prv_mod_int32_fn,
						    err);
			else
				break;
		} else if (!strcmp(op, "*")) {
			e1 = prv_binary_numeric(c, e1, e2, prv_mul_int32_fn,
						prv_mul_real_fn, err);
		} else {
			e1 = prv_binary_real(c, e1, e2, prv_div_real_fn, err);
		}

		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

static double prv_add_real_fn(subtilis_arm_ass_context_t *c, double a, double b,
			      subtilis_error_t *err)
{
	return a + b;
}

static double prv_sub_real_fn(subtilis_arm_ass_context_t *c, double a, double b,
			      subtilis_error_t *err)
{
	return a - b;
}

static int32_t prv_add_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return a + b;
}

static int32_t prv_sub_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return a - b;
}

static subtilis_arm_exp_val_t *prv_priority4(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;
	subtilis_ass_int_fn_t int_fn;
	subtilis_ass_real_fn_t real_fn;

	e1 = prv_priority3(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	tbuf = subtilis_token_get_text(c->t);

	/*
	 * Special case for number too big.
	 */

	if ((c->t->type == SUBTILIS_TOKEN_INTEGER) &&
	    (c->t->tok.integer == -2147483648)) {
		subtilis_error_set_number_too_long(
		    err, tbuf, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	while (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(c->t);

		if (!strcmp(tbuf, "+")) {
			int_fn = prv_add_int32_fn;
			real_fn = prv_add_real_fn;
		} else if (!strcmp(tbuf, "-")) {
			int_fn = prv_sub_int32_fn;
			real_fn = prv_sub_real_fn;
		} else {
			break;
		}

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = prv_priority3(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = prv_binary_numeric(c, e1, e2, int_fn, real_fn, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

static int32_t prv_equal_real_fn(double a, double b) { return a == b ? -1 : 0; }

static int32_t prv_not_equal_real_fn(double a, double b)
{
	return a != b ? -1 : 0;
}

static int32_t prv_gt_real_fn(double a, double b) { return a > b ? -1 : 0; }

static int32_t prv_lt_real_fn(double a, double b) { return a < b ? -1 : 0; }

static int32_t prv_gte_real_fn(double a, double b) { return a >= b ? -1 : 0; }

static int32_t prv_lte_real_fn(double a, double b) { return a <= b ? -1 : 0; }

static int32_t prv_equal_int32_fn(int32_t a, int32_t b)
{
	return a == b ? -1 : 0;
}

static int32_t prv_not_equal_int32_fn(int32_t a, int32_t b)
{
	return a != b ? -1 : 0;
}

static int32_t prv_gt_int32_fn(int32_t a, int32_t b) { return a > b ? -1 : 0; }

static int32_t prv_lt_int32_fn(int32_t a, int32_t b) { return a < b ? -1 : 0; }

static int32_t prv_gte_int32_fn(int32_t a, int32_t b)
{
	return a >= b ? -1 : 0;
}

static int32_t prv_lte_int32_fn(int32_t a, int32_t b)
{
	return a <= b ? -1 : 0;
}

static int32_t prv_lsl_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return a << (b & 63);
}

static int32_t prv_lsr_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return (int32_t)((uint32_t)a >> (uint32_t)(b & 63));
}

static int32_t prv_asr_int32_fn(subtilis_arm_ass_context_t *c, int32_t a,
				int32_t b, subtilis_error_t *err)
{
	return a >> (b & 63);
}

static subtilis_arm_exp_val_t *prv_priority5(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;
	subtilis_ass_log_int_fn_t log_int_fn = NULL;
	subtilis_ass_log_real_fn_t log_real_fn = NULL;
	subtilis_ass_int_fn_t int_fn = NULL;

	e1 = prv_priority4(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(c->t);

		if (!strcmp(tbuf, "=")) {
			log_int_fn = prv_equal_int32_fn;
			log_real_fn = prv_equal_real_fn;
		} else if (!strcmp(tbuf, "<>")) {
			log_int_fn = prv_not_equal_int32_fn;
			log_real_fn = prv_not_equal_real_fn;
		} else if (!strcmp(tbuf, ">")) {
			log_int_fn = prv_gt_int32_fn;
			log_real_fn = prv_gt_real_fn;
		} else if (!strcmp(tbuf, "<=")) {
			log_int_fn = prv_lte_int32_fn;
			log_real_fn = prv_lte_real_fn;
		} else if (!strcmp(tbuf, "<")) {
			log_int_fn = prv_lt_int32_fn;
			log_real_fn = prv_lt_real_fn;
		} else if (!strcmp(tbuf, ">=")) {
			log_int_fn = prv_gte_int32_fn;
			log_real_fn = prv_gte_real_fn;
		} else if (!strcmp(tbuf, "<<")) {
			int_fn = prv_lsl_int32_fn;
		} else if (!strcmp(tbuf, ">>")) {
			int_fn = prv_lsr_int32_fn;
		} else if (!strcmp(tbuf, ">>>")) {
			int_fn = prv_asr_int32_fn;
		} else {
			break;
		}

		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority4(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		if (int_fn)
			e1 = prv_binary_int(c, e1, e2, int_fn, err);
		else
			e1 = prv_logical_numeric(c, e1, e2, log_int_fn,
						 log_real_fn, err);

		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

static int32_t prv_and_fn(subtilis_arm_ass_context_t *c, int32_t e1, int32_t e2,
			  subtilis_error_t *err)
{
	return e1 & e2;
}

static subtilis_arm_exp_val_t *prv_priority6(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;

	e1 = prv_priority5(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (c->t->type == SUBTILIS_TOKEN_KEYWORD &&
	       c->t->tok.keyword.type == SUBTILIS_ARM_KEYWORD_AND) {
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority5(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = prv_binary_int(c, e1, e2, prv_and_fn, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

static int32_t prv_or_fn(subtilis_arm_ass_context_t *c, int32_t e1, int32_t e2,
			 subtilis_error_t *err)
{
	return e1 | e2;
}

static int32_t prv_eor_fn(subtilis_arm_ass_context_t *c, int32_t e1, int32_t e2,
			  subtilis_error_t *err)
{
	return e1 ^ e2;
}

static subtilis_arm_exp_val_t *prv_eor(subtilis_arm_ass_context_t *c,
				       subtilis_arm_exp_val_t *e1,
				       subtilis_arm_exp_val_t *e2,
				       subtilis_error_t *err)
{
	return prv_binary_int(c, e1, e2, prv_eor_fn, err);
}

static subtilis_arm_exp_val_t *prv_or(subtilis_arm_ass_context_t *c,
				      subtilis_arm_exp_val_t *e1,
				      subtilis_arm_exp_val_t *e2,
				      subtilis_error_t *err)
{
	return prv_binary_int(c, e1, e2, prv_or_fn, err);
}

static subtilis_arm_exp_val_t *prv_priority7(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;
	subtilis_arm_exp_val_t *(*fn)(
	    subtilis_arm_ass_context_t * c, subtilis_arm_exp_val_t * e1,
	    subtilis_arm_exp_val_t * e2, subtilis_error_t * err);

	e1 = prv_priority6(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (c->t->type == SUBTILIS_TOKEN_KEYWORD) {
		if (c->t->tok.keyword.type == SUBTILIS_ARM_KEYWORD_EOR)
			fn = prv_eor;
		else if (c->t->tok.keyword.type == SUBTILIS_ARM_KEYWORD_OR)
			fn = prv_or;
		else
			break;
		subtilis_lexer_get(c->l, c->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority6(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = fn(c, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_arm_exp_val_free(e2);
	subtilis_arm_exp_val_free(e1);
	return NULL;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_val_get(subtilis_arm_ass_context_t *c,
						 subtilis_error_t *err)
{
	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return prv_priority7(c, err);
}

const char *subtilis_arm_exp_type_name(subtilis_arm_exp_val_t *val)
{
	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_REG:
		return "register";
	case SUBTILIS_ARM_EXP_TYPE_INT:
		return "integer";
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		return "real";
	case SUBTILIS_ARM_EXP_TYPE_STRING:
		return "string";
	case SUBTILIS_ARM_EXP_TYPE_ID:
		return "identifier";
	default:
		return "unknown";
	}
}

void subtilis_arm_exp_val_free(subtilis_arm_exp_val_t *val)
{
	if (!val)
		return;

	if ((val->type == SUBTILIS_ARM_EXP_TYPE_STRING) ||
	    (val->type == SUBTILIS_ARM_EXP_TYPE_ID))
		subtilis_buffer_free(&val->val.buf);

	free(val);
}
