/*
 * Copyright (c) 2019 Mark Ryan
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

#include "lexer.h"
#include "parser_exp.h"
#include "parser_math.h"
#include "type_if.h"

static subtilis_exp_t *prv_real_unary_fn(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_op_instr_type_t itype,
					 double (*const_fn)(double),
					 subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;

	e = subtilis_parser_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_real(const_fn(e->exp.ir_op.real), err);
	} else {
		reg = subtilis_ir_section_add_instr2(p->current, itype,
						     e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = subtilis_exp_new_real_var(reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_rad(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_t *e3 = NULL;
	const double radian = ((22 / 7.0) / 180);

	e = subtilis_parser_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_real(e->exp.ir_op.real * radian, err);
	} else {
		e3 = subtilis_exp_new_real(radian, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = subtilis_type_if_mul(p, e, e3, err);
		e = NULL;
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_pi(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_exp_new_real(22 / 7.0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

subtilis_exp_t *subtilis_parser_int(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e2 = NULL;

	e = subtilis_parser_real_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (e->type.type == SUBTILIS_TYPE_CONST_REAL) {
		e2 = subtilis_exp_new_int32(floor(e->exp.ir_op.real), err);
	} else {
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOV_FPRD_I32, e->exp.ir_op,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e2 = subtilis_exp_new_int32_var(reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_exp_delete(e);

	return e2;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_sin(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_SIN, sin, err);
}

subtilis_exp_t *subtilis_parser_cos(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_COS, cos, err);
}

subtilis_exp_t *subtilis_parser_tan(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_TAN, tan, err);
}

subtilis_exp_t *subtilis_parser_asn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ASN, asin, err);
}

subtilis_exp_t *subtilis_parser_acs(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ACS, acos, err);
}

subtilis_exp_t *subtilis_parser_atn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_ATN, atan, err);
}

subtilis_exp_t *subtilis_parser_log(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_op_instr_type_t itype,
				    double (*log_fn)(double),
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = prv_real_unary_fn(p, t, itype, log_fn, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}
	return e;
}

subtilis_exp_t *subtilis_parser_sqr(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_SQR, sqrt, err);
}

subtilis_exp_t *subtilis_parser_exp(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	return prv_real_unary_fn(p, t, SUBTILIS_OP_INSTR_EXPR, exp, err);
}

subtilis_exp_t *subtilis_parser_abs(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_abs(p, e, err);
}

subtilis_exp_t *subtilis_parser_sgn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_sgn(p, e, err);
}
