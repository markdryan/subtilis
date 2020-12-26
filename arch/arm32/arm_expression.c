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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "arm_expression.h"
#include "assembler.h"

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
typedef int32_t (*subtilis_ass_log_str_fn_t)(const char *, const char *);

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

subtilis_arm_exp_val_t *subtilis_arm_exp_new_str_str(const char *str,
						     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = SUBTILIS_ARM_EXP_TYPE_STRING;
	subtilis_buffer_init(&e->val.buf, strlen(str) + 1);
	subtilis_buffer_append_string(&e->val.buf, str, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&e->val.buf, err);

	return e;

cleanup:
	subtilis_arm_exp_val_free(e);

	return NULL;
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
	subtilis_buffer_zero_terminate(&e->val.buf, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_arm_exp_val_free(e);
		return NULL;
	}

	return e;
}

static subtilis_arm_exp_val_t *prv_new_reg(subtilis_arm_reg_t reg,
					   subtilis_arm_exp_type_t type,
					   subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e = malloc(sizeof(*e));

	if (!e) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	e->type = type;
	e->val.reg = reg;

	return e;
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_reg(subtilis_arm_reg_t reg,
						 subtilis_error_t *err)
{
	return prv_new_reg(reg, SUBTILIS_ARM_EXP_TYPE_REG, err);
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_freg(subtilis_arm_reg_t reg,
						  subtilis_error_t *err)
{
	return prv_new_reg(reg, SUBTILIS_ARM_EXP_TYPE_FREG, err);
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_dreg(subtilis_arm_reg_t reg,
						  subtilis_error_t *err)
{
	return prv_new_reg(reg, SUBTILIS_ARM_EXP_TYPE_DREG, err);
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_sreg(subtilis_arm_reg_t reg,
						  subtilis_error_t *err)
{
	return prv_new_reg(reg, SUBTILIS_ARM_EXP_TYPE_SREG, err);
}

subtilis_arm_exp_val_t *subtilis_arm_exp_new_sysreg(subtilis_arm_reg_t reg,
						    subtilis_error_t *err)
{
	return prv_new_reg(reg, SUBTILIS_ARM_EXP_TYPE_SYSREG, err);
}

subtilis_arm_exp_val_t *subtilis_arm_exp_dup(subtilis_arm_exp_val_t *val,
					     subtilis_error_t *err)
{
	const char *id;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_FREG:
		return subtilis_arm_exp_new_freg(val->val.reg, err);
	case SUBTILIS_ARM_EXP_TYPE_REG:
		return subtilis_arm_exp_new_reg(val->val.reg, err);
	case SUBTILIS_ARM_EXP_TYPE_SREG:
		return subtilis_arm_exp_new_sreg(val->val.reg, err);
	case SUBTILIS_ARM_EXP_TYPE_DREG:
		return subtilis_arm_exp_new_dreg(val->val.reg, err);
	case SUBTILIS_ARM_EXP_TYPE_SYSREG:
		return subtilis_arm_exp_new_sysreg(val->val.reg, err);
	case SUBTILIS_ARM_EXP_TYPE_INT:
		return subtilis_arm_exp_new_int32(val->val.integer, err);
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		return subtilis_arm_exp_new_real(val->val.real, err);
	case SUBTILIS_ARM_EXP_TYPE_STRING:
		return subtilis_arm_exp_new_str(&val->val.buf, err);
	case SUBTILIS_ARM_EXP_TYPE_ID:
		id = subtilis_buffer_get_string(&val->val.buf);
		return subtilis_arm_exp_new_id(id, err);
	}

	subtilis_error_set_assertion_failed(err);

	return NULL;
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

static subtilis_arm_exp_val_t *prv_logical_str(subtilis_arm_ass_context_t *c,
					       subtilis_arm_exp_val_t *e1,
					       subtilis_arm_exp_val_t *e2,
					       subtilis_ass_log_str_fn_t fn,
					       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e;
	const char *str1 = subtilis_buffer_get_string(&e1->val.buf);
	const char *str2 = subtilis_buffer_get_string(&e2->val.buf);

	e = subtilis_arm_exp_new_int32(fn(str1, str2), err);
	subtilis_arm_exp_val_free(e1);
	subtilis_arm_exp_val_free(e2);
	return e;
}

static size_t prv_parse_reg_num(const char *id, size_t name_len)
{
	size_t reg;

	if (id[1] < '0' || id[1] > '9')
		return SIZE_MAX;
	reg = id[1] - '0';
	if (name_len == 2) {
		if (id[2] < '0' || id[2] > '9')
			return SIZE_MAX;
		reg *= 10;
		reg += id[2] - '0';
	}

	return reg;
}

subtilis_arm_reg_t subtilis_arm_exp_parse_reg(subtilis_arm_ass_context_t *c,
					      const char *id,
					      subtilis_error_t *err)
{
	size_t reg;
	size_t name_len;

	name_len = strlen(&id[1]);
	if (name_len != 1 && name_len != 2)
		return SIZE_MAX;

	if ((id[0] == 'P') && (id[1] == 'C'))
		return (subtilis_arm_reg_t)15;

	if ((id[0] != 'R') && (id[0] != 'r'))
		return SIZE_MAX;

	reg = prv_parse_reg_num(id, name_len);

	if (reg > 15) {
		subtilis_error_set_ass_bad_reg(err, id, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}

	return (subtilis_arm_reg_t)reg;
}

subtilis_arm_reg_t subtilis_arm_exp_parse_freg(subtilis_arm_ass_context_t *c,
					       const char *id,
					       subtilis_error_t *err)
{
	size_t reg;
	size_t name_len;

	name_len = strlen(&id[1]);
	if (name_len != 1)
		return SIZE_MAX;

	if ((id[0] != 'F') && (id[0] != 'f'))
		return SIZE_MAX;

	if (id[1] < '0' || id[1] > '7') {
		subtilis_error_set_ass_bad_reg(err, id, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}
	reg = id[1] - '0';

	return (subtilis_arm_reg_t)reg;
}

subtilis_arm_reg_t subtilis_arm_exp_parse_dreg(subtilis_arm_ass_context_t *c,
					       const char *id,
					       subtilis_error_t *err)
{
	size_t reg;
	size_t name_len;

	name_len = strlen(&id[1]);
	if (name_len != 1)
		return SIZE_MAX;

	reg = prv_parse_reg_num(id, name_len);
	if (reg > 15) {
		subtilis_error_set_ass_bad_reg(err, id, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}

	return (subtilis_arm_reg_t)reg;
}

subtilis_arm_reg_t subtilis_arm_exp_parse_sreg(subtilis_arm_ass_context_t *c,
					       const char *id,
					       subtilis_error_t *err)
{
	size_t reg;
	size_t name_len;

	name_len = strlen(&id[1]);
	if (name_len != 1)
		return SIZE_MAX;

	if ((id[0] != 'S') && (id[0] != 's'))
		return SIZE_MAX;

	reg = prv_parse_reg_num(id, name_len);
	if (reg > 31) {
		subtilis_error_set_ass_bad_reg(err, id, c->l->stream->name,
					       c->l->line);
		return SIZE_MAX;
	}

	return (subtilis_arm_reg_t)reg;
}

subtilis_arm_reg_t subtilis_arm_exp_parse_sysreg(subtilis_arm_ass_context_t *c,
						 const char *id)
{
	size_t reg;

	if (!strcmp(id, "FPSID") || !strcmp(id, "fpsid"))
		reg = SUBTILIS_VFP_SYSREG_FPSID;
	else if (!strcmp(id, "FPSCR") || !strcmp(id, "fpscr"))
		reg = SUBTILIS_VFP_SYSREG_FPSCR;
	else if (!strcmp(id, "FPEXC") || !strcmp(id, "fpexc"))
		reg = SUBTILIS_VFP_SYSREG_FPEXC;
	else
		reg = SIZE_MAX;

	return (subtilis_arm_reg_t)reg;
}

/* clang-format off */
subtilis_arm_exp_val_t *subtilis_arm_exp_process_id(
	subtilis_arm_ass_context_t *c, const char *id, subtilis_error_t *err)
/* clang-format on */

{
	subtilis_arm_reg_t reg;
	subtilis_arm_exp_val_t *val;

	reg = subtilis_arm_exp_parse_reg(c, id, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (reg != SIZE_MAX)
		return subtilis_arm_exp_new_reg(reg, err);

	reg = subtilis_arm_exp_parse_freg(c, id, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (reg != SIZE_MAX)
		return subtilis_arm_exp_new_freg(reg, err);

	reg = subtilis_arm_exp_parse_sreg(c, id, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (reg != SIZE_MAX)
		return subtilis_arm_exp_new_sreg(reg, err);

	reg = subtilis_arm_exp_parse_dreg(c, id, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (reg != SIZE_MAX)
		return subtilis_arm_exp_new_dreg(reg, err);

	reg = subtilis_arm_exp_parse_sysreg(c, id);
	if (reg != SIZE_MAX)
		return subtilis_arm_exp_new_sysreg(reg, err);

	val = subtilis_arm_asm_find_def(c, id);
	if (val)
		return subtilis_arm_exp_dup(val, err);

	return subtilis_arm_exp_new_id(id, err);
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

static subtilis_arm_exp_val_t *prv_bracketed_exp(subtilis_arm_ass_context_t *c,
						 subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e = NULL;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(c->t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, c->l->stream->name,
					    c->l->line);
		return NULL;
	}

	e = prv_process_bktd_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_arm_exp_val_free(e);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_real_bkt_exp(subtilis_arm_ass_context_t *c,
						subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		val->type = SUBTILIS_ARM_EXP_TYPE_REAL;
		val->val.real = (double)val->val.integer;
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_int_bkt_exp(subtilis_arm_ass_context_t *c,
					       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		val->type = SUBTILIS_ARM_EXP_TYPE_INT;
		val->val.integer = (int32_t)val->val.real;
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_int(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		val->type = SUBTILIS_ARM_EXP_TYPE_INT;
		val->val.integer = (int32_t)val->val.real;
		break;
	case SUBTILIS_ARM_EXP_TYPE_FREG:
	case SUBTILIS_ARM_EXP_TYPE_REG:
		val->type = SUBTILIS_ARM_EXP_TYPE_INT;
		val->val.integer = (int32_t)val->val.reg;
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_dbl_unary(subtilis_arm_ass_context_t *c,
					     double (*fn)(double),
					     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_real_bkt_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	val->val.real = fn(val->val.real);

	return val;
}

static subtilis_arm_exp_val_t *prv_rad(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	const double radian = ((22 / 7.0) / 180);

	val = prv_real_bkt_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	val->val.real = val->val.real * radian;

	return val;
}

static subtilis_arm_exp_val_t *prv_abs(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		if (val->val.integer < 0)
			val->val.integer = -val->val.integer;
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		if (val->val.real < 0)
			val->val.real = -val->val.real;
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_chr_str(subtilis_arm_ass_context_t *c,
					   subtilis_error_t *err)
{
	char c_str[2];
	subtilis_arm_exp_val_t *val;
	subtilis_arm_exp_val_t *val2 = NULL;

	val = prv_int_bkt_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	c_str[0] = val->val.integer & 255;
	c_str[1] = 0;

	val2 = subtilis_arm_exp_new_str_str(c_str, err);

cleanup:

	subtilis_arm_exp_val_free(val);

	return val2;
}

static subtilis_arm_exp_val_t *prv_asc(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_exp_val_t *val2 = NULL;
	int32_t v;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_const_string_expected(
		    err, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	if (subtilis_buffer_get_size(&val->val.buf) <= 1)
		v = -1;
	else
		v = subtilis_buffer_get_string(&val->val.buf)[0];

	val2 = subtilis_arm_exp_new_int32(v, err);

cleanup:

	subtilis_arm_exp_val_free(val);

	return val2;
}

static subtilis_arm_exp_val_t *prv_len(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	subtilis_arm_exp_val_t *val2 = NULL;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_const_string_expected(
		    err, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	val2 = subtilis_arm_exp_new_int32(
	    subtilis_buffer_get_size(&val->val.buf) - 1, err);

cleanup:

	subtilis_arm_exp_val_free(val);

	return val2;
}

static subtilis_arm_exp_val_t *prv_sgn(subtilis_arm_ass_context_t *c,
				       subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		if (val->val.integer < 0)
			val->val.integer = -1;
		else if (val->val.integer > 0)
			val->val.integer = 1;
		else
			val->val.integer = 0;
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		val->type = SUBTILIS_ARM_EXP_TYPE_INT;
		if (val->val.real < 0)
			val->val.integer = -1;
		else if (val->val.real > 0)
			val->val.integer = 1;
		else
			val->val.integer = 0;
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_left_right(subtilis_arm_ass_context_t *c,
					      int32_t *len,
					      subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *val = NULL;
	subtilis_arm_exp_val_t *val2 = NULL;
	int32_t length = 1;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(c->t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, c->l->stream->name,
					    c->l->line);
		return NULL;
	}

	val = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (val->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_const_string_expected(
		    err, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
	    (strcmp(tbuf, ",") && strcmp(tbuf, ")"))) {
		subtilis_error_set_expected(err, ", or ) ", tbuf,
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, ",")) {
		val2 = subtilis_arm_exp_val_get(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		length = prv_exp_to_int(c, val2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    strcmp(tbuf, ")")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, c->l->stream->name, c->l->line);
			goto cleanup;
		}
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_exp_val_free(val2);

	*len = length;
	return val;

cleanup:

	subtilis_arm_exp_val_free(val2);
	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_left_str(subtilis_arm_ass_context_t *c,
					    subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	int32_t start;
	int32_t len;

	val = prv_left_right(c, &start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	len = subtilis_buffer_get_size(&val->val.buf) - 1;
	if (len == 0 || start < 0 || start >= len)
		return val;

	subtilis_buffer_delete(&val->val.buf, start, len - start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_right_str(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	int32_t start;
	int32_t len;

	val = prv_left_right(c, &start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	len = subtilis_buffer_get_size(&val->val.buf) - 1;
	if (len == 0 || start < 0 || start >= len)
		return val;

	subtilis_buffer_delete(&val->val.buf, 0, len - start, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	subtilis_buffer_zero_terminate(&val->val.buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return val;

cleanup:

	subtilis_arm_exp_val_free(val);

	return NULL;
}

static subtilis_arm_exp_val_t *prv_mid_str(subtilis_arm_ass_context_t *c,
					   subtilis_error_t *err)
{
	const char *tbuf;
	int32_t string_len;
	const char *seed_string;
	int32_t start;
	subtilis_arm_exp_val_t *ret_val = NULL;
	subtilis_arm_exp_val_t *str = NULL;
	subtilis_arm_exp_val_t *count = NULL;
	int32_t length = -1;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(c->t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, c->l->stream->name,
					    c->l->line);
		return NULL;
	}

	str = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (str->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_const_string_expected(
		    err, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		goto cleanup;
	}

	count = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	start = prv_exp_to_int(c, count, err);
	subtilis_arm_exp_val_free(count);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
	    (strcmp(tbuf, ")") && strcmp(tbuf, ","))) {
		subtilis_error_set_expected(err, ", or )", tbuf,
					    c->l->stream->name, c->l->line);
		goto cleanup;
	}

	if (!strcmp(tbuf, ",")) {
		count = subtilis_arm_exp_val_get(c, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		length = prv_exp_to_int(c, count, err);
		subtilis_arm_exp_val_free(count);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		tbuf = subtilis_token_get_text(c->t);
		if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) ||
		    strcmp(tbuf, ")")) {
			subtilis_error_set_right_bkt_expected(
			    err, tbuf, c->l->stream->name, c->l->line);
			goto cleanup;
		}
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	string_len = subtilis_buffer_get_size(&str->val.buf) - 1;
	if ((string_len == 0) || (start < 0) || (length == 0)) {
		ret_val = subtilis_arm_exp_new_str_str("", err);
	} else {
		seed_string = subtilis_buffer_get_string(&str->val.buf);
		if (length < 0 || length >= (string_len - start)) {
			ret_val = subtilis_arm_exp_new_str_str(
			    &seed_string[start], err);
		} else {
			subtilis_buffer_delete(&str->val.buf, 0, start, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			if (start + length < string_len)
				subtilis_buffer_delete(
				    &str->val.buf, length,
				    string_len - (start + length), err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			subtilis_buffer_zero_terminate(&str->val.buf, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;

			ret_val = subtilis_arm_exp_new_str(&str->val.buf, err);
		}
	}

cleanup:

	subtilis_arm_exp_val_free(str);

	return ret_val;
}

static subtilis_arm_exp_val_t *prv_string_str(subtilis_arm_ass_context_t *c,
					      subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_buffer_t buf;
	size_t i;
	size_t string_len;
	int32_t repeat;
	const char *seed_string;
	subtilis_arm_exp_val_t *retval = NULL;
	subtilis_arm_exp_val_t *count = NULL;
	subtilis_arm_exp_val_t *string = NULL;

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(c->t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, c->l->stream->name,
					    c->l->line);
		return NULL;
	}

	count = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	repeat = prv_exp_to_int(c, count, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, c->l->stream->name,
					    c->l->line);
		goto cleanup;
	}

	string = subtilis_arm_exp_val_get(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (string->type != SUBTILIS_ARM_EXP_TYPE_STRING) {
		subtilis_error_set_const_string_expected(
		    err, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	tbuf = subtilis_token_get_text(c->t);
	if ((c->t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, c->l->stream->name, c->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(c->l, c->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	string_len = subtilis_buffer_get_size(&string->val.buf) - 1;
	seed_string = subtilis_buffer_get_string(&string->val.buf);
	subtilis_buffer_init(&buf, (count->val.integer * string_len) + 1);

	for (i = 0; i < repeat; i++) {
		subtilis_buffer_append(&buf, seed_string, string_len, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_buffer_free(&buf);
			goto cleanup;
		}
	}
	subtilis_buffer_zero_terminate(&buf, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_buffer_free(&buf);
		goto cleanup;
	}

	retval = subtilis_arm_exp_new_str(&buf, err);
	subtilis_buffer_free(&buf);

cleanup:

	subtilis_arm_exp_val_free(count);
	subtilis_arm_exp_val_free(string);

	return retval;
}

static subtilis_arm_exp_val_t *prv_str_str(subtilis_arm_ass_context_t *c,
					   subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *val;
	char buf[64];
	subtilis_arm_exp_val_t *val2 = NULL;

	val = prv_bracketed_exp(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_INT:
		sprintf(buf, "%d", val->val.integer);
		break;
	case SUBTILIS_ARM_EXP_TYPE_REAL:
		sprintf(buf, "%f", val->val.real);
		break;
	default:
		subtilis_error_set_numeric_exp_expected(err, c->l->stream->name,
							c->l->line);
		goto cleanup;
	}

	val2 = subtilis_arm_exp_new_str_str(buf, err);

cleanup:

	subtilis_arm_exp_val_free(val);

	return val2;
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
		e = subtilis_arm_exp_process_id(c, tbuf, err);
		break;

	case SUBTILIS_TOKEN_KEYWORD:
		switch (c->t->tok.keyword.type) {
		case SUBTILIS_ARM_KEYWORD_TRUE:
			e = subtilis_arm_exp_new_int32(-1, err);
			break;
		case SUBTILIS_ARM_KEYWORD_FALSE:
			e = subtilis_arm_exp_new_int32(0, err);
			break;
		case SUBTILIS_ARM_KEYWORD_NOT:
			e = prv_not(c, err);
			return e;
		case SUBTILIS_ARM_KEYWORD_INT:
			return prv_int(c, err);
		case SUBTILIS_ARM_KEYWORD_SIN:
			return prv_dbl_unary(c, sin, err);
		case SUBTILIS_ARM_KEYWORD_COS:
			return prv_dbl_unary(c, cos, err);
		case SUBTILIS_ARM_KEYWORD_TAN:
			return prv_dbl_unary(c, tan, err);
		case SUBTILIS_ARM_KEYWORD_ACS:
			return prv_dbl_unary(c, acos, err);
		case SUBTILIS_ARM_KEYWORD_ASN:
			return prv_dbl_unary(c, asin, err);
		case SUBTILIS_ARM_KEYWORD_ATN:
			return prv_dbl_unary(c, atan, err);
		case SUBTILIS_ARM_KEYWORD_SQR:
			return prv_dbl_unary(c, sqrt, err);
		case SUBTILIS_ARM_KEYWORD_EXP:
			return prv_dbl_unary(c, exp, err);
		case SUBTILIS_ARM_KEYWORD_LOG:
			return prv_dbl_unary(c, log10, err);
		case SUBTILIS_ARM_KEYWORD_LN:
			return prv_dbl_unary(c, log, err);
		case SUBTILIS_ARM_KEYWORD_RAD:
			return prv_rad(c, err);
		case SUBTILIS_ARM_KEYWORD_ABS:
			return prv_abs(c, err);
		case SUBTILIS_ARM_KEYWORD_SGN:
			return prv_sgn(c, err);
		case SUBTILIS_ARM_KEYWORD_PI:
			e = subtilis_arm_exp_new_real(22 / 7.0, err);
			break;
		case SUBTILIS_ARM_KEYWORD_CHR_STR:
			return prv_chr_str(c, err);
		case SUBTILIS_ARM_KEYWORD_ASC:
			return prv_asc(c, err);
		case SUBTILIS_ARM_KEYWORD_LEN:
			return prv_len(c, err);
		case SUBTILIS_ARM_KEYWORD_LEFT_STR:
			return prv_left_str(c, err);
		case SUBTILIS_ARM_KEYWORD_RIGHT_STR:
			return prv_right_str(c, err);
		case SUBTILIS_ARM_KEYWORD_MID_STR:
			return prv_mid_str(c, err);
		case SUBTILIS_ARM_KEYWORD_STRING_STR:
			return prv_string_str(c, err);
		case SUBTILIS_ARM_KEYWORD_STR_STR:
			return prv_str_str(c, err);
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

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

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
	char op[4];
	int keyword = SUBTILIS_ARM_KEYWORD_MAX;

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

static subtilis_arm_exp_val_t *prv_strcat(subtilis_arm_exp_val_t *e1,
					  subtilis_arm_exp_val_t *e2,
					  subtilis_error_t *err)

{
	subtilis_buffer_remove_terminator(&e1->val.buf);
	subtilis_buffer_append_buffer(&e1->val.buf, &e2->val.buf, err);
	subtilis_arm_exp_val_free(e2);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_arm_exp_val_free(e1);
		return NULL;
	}

	return e1;
}

static subtilis_arm_exp_val_t *prv_priority4(subtilis_arm_ass_context_t *c,
					     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;
	subtilis_ass_int_fn_t int_fn;
	subtilis_ass_real_fn_t real_fn;
	bool plus;

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
		plus = false;
		tbuf = subtilis_token_get_text(c->t);

		if (!strcmp(tbuf, "+")) {
			int_fn = prv_add_int32_fn;
			real_fn = prv_add_real_fn;
			plus = true;
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

		if (plus && e1->type == SUBTILIS_ARM_EXP_TYPE_STRING &&
		    e2->type == SUBTILIS_ARM_EXP_TYPE_STRING)
			e1 = prv_strcat(e1, e2, err);
		else
			e1 =
			    prv_binary_numeric(c, e1, e2, int_fn, real_fn, err);
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

static int32_t prv_equal_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) ? 0 : -1;
}

static int32_t prv_not_equal_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) ? -1 : 0;
}

static int32_t prv_gt_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) > 0 ? -1 : 0;
}

static int32_t prv_lt_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) < 0 ? -1 : 0;
}

static int32_t prv_gte_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) >= 0 ? -1 : 0;
}

static int32_t prv_lte_str_fn(const char *a, const char *b)
{
	return strcmp(a, b) <= 0 ? -1 : 0;
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
	subtilis_ass_log_str_fn_t log_str_fn = NULL;
	subtilis_ass_int_fn_t int_fn = NULL;

	e1 = prv_priority4(c, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (c->t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(c->t);

		if (!strcmp(tbuf, "=")) {
			log_int_fn = prv_equal_int32_fn;
			log_real_fn = prv_equal_real_fn;
			log_str_fn = prv_equal_str_fn;
		} else if (!strcmp(tbuf, "<>")) {
			log_int_fn = prv_not_equal_int32_fn;
			log_real_fn = prv_not_equal_real_fn;
			log_str_fn = prv_not_equal_str_fn;
		} else if (!strcmp(tbuf, ">")) {
			log_int_fn = prv_gt_int32_fn;
			log_real_fn = prv_gt_real_fn;
			log_str_fn = prv_gt_str_fn;
		} else if (!strcmp(tbuf, "<=")) {
			log_int_fn = prv_lte_int32_fn;
			log_real_fn = prv_lte_real_fn;
			log_str_fn = prv_lte_str_fn;
		} else if (!strcmp(tbuf, "<")) {
			log_int_fn = prv_lt_int32_fn;
			log_real_fn = prv_lt_real_fn;
			log_str_fn = prv_lt_str_fn;
		} else if (!strcmp(tbuf, ">=")) {
			log_int_fn = prv_gte_int32_fn;
			log_real_fn = prv_gte_real_fn;
			log_str_fn = prv_gte_str_fn;
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

		if (int_fn) {
			e1 = prv_binary_int(c, e1, e2, int_fn, err);
		} else {
			if ((e1->type == SUBTILIS_ARM_EXP_TYPE_STRING) &&
			    (e1->type == e2->type) && log_str_fn)
				e1 =
				    prv_logical_str(c, e1, e2, log_str_fn, err);
			else
				e1 = prv_logical_numeric(c, e1, e2, log_int_fn,
							 log_real_fn, err);
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

subtilis_arm_exp_val_t *subtilis_arm_exp_pri7(subtilis_arm_ass_context_t *c,
					      subtilis_error_t *err)
{
	subtilis_arm_exp_val_t *e1 = NULL;
	subtilis_arm_exp_val_t *e2 = NULL;

	/* clang-format off */
	subtilis_arm_exp_val_t *(*fn)(
	    subtilis_arm_ass_context_t *c, subtilis_arm_exp_val_t *e1,
	    subtilis_arm_exp_val_t *e2, subtilis_error_t *err);
	/* clang-format on */

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
	return subtilis_arm_exp_pri7(c, err);
}

const char *subtilis_arm_exp_type_name(subtilis_arm_exp_val_t *val)
{
	switch (val->type) {
	case SUBTILIS_ARM_EXP_TYPE_FREG:
		return "floating point register";
	case SUBTILIS_ARM_EXP_TYPE_REG:
		return "register";
	case SUBTILIS_ARM_EXP_TYPE_SREG:
		return "vfp S register";
	case SUBTILIS_ARM_EXP_TYPE_DREG:
		return "vfp D register";
	case SUBTILIS_ARM_EXP_TYPE_SYSREG:
		return "vfp SYSREG register";
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
