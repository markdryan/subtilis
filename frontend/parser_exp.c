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
#include <string.h>

#include "../common/lexer.h"
#include "parser_array.h"
#include "parser_call.h"
#include "parser_error.h"
#include "parser_exp.h"
#include "parser_file.h"
#include "parser_graphics.h"
#include "parser_input.h"
#include "parser_math.h"
#include "parser_mem.h"
#include "parser_output.h"
#include "parser_rec.h"
#include "parser_rnd.h"
#include "parser_string.h"
#include "reference_type.h"
#include "type_if.h"
#include "variable.h"

static subtilis_exp_t *prv_priority1(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err);

/* We don't currently have enough time code to justify its own file. */

static subtilis_exp_t *prv_gettime(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_GETTIME, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_exp_handle_errors(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return e;

on_error:
	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_unary_minus_exp(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_type_if_unary_minus(p, e, err);
}

static subtilis_exp_t *prv_call_addr_exp(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	const char *tbuf;
	size_t call_reg;
	size_t call_site;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_KEYWORD) ||
	    ((t->tok.keyword.type != SUBTILIS_KEYWORD_PROC) &&
	     (t->tok.keyword.type != SUBTILIS_KEYWORD_FN))) {
		subtilis_error_set_procedure_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return NULL;
	}

	if (t->tok.keyword.type == SUBTILIS_KEYWORD_PROC)
		tbuf += 4;
	else if (t->tok.keyword.type == SUBTILIS_KEYWORD_FN)
		tbuf += 2;

	call_reg = subtilis_ir_section_add_get_partial_addr(p->current,
							    &call_site, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_partial_fn(call_reg, tbuf, call_site, err);
}

static subtilis_exp_t *prv_not_exp(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = prv_priority1(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_type_if_not(p, e, err);
}

bool subtilis_exp_get_lvalue(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_ir_operand_t *op, subtilis_type_t *type,
			     subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	const char *tbuf;
	size_t mem_reg = SUBTILIS_IR_REG_LOCAL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_IDENTIFIER)) {
		subtilis_error_set_lvalue_expected(err, p->l->stream->name,
						   p->l->line);
		return false;
	}

	s = subtilis_parser_get_symbol(p, tbuf, &mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;

	if (subtilis_type_if_is_numeric(&s->t) ||
	    (s->t.type == SUBTILIS_TYPE_FN) ||
	    (s->t.type == SUBTILIS_TYPE_STRING)) {
		subtilis_type_copy(type, &s->t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
		if (s->is_reg) {
			op->reg = s->loc;
			return true;
		}
		op->reg =
		    subtilis_reference_get_pointer(p, mem_reg, s->loc, err);
		return false;
	}

	/*
	 * Need special handling for arrays and vectors
	 */

	if (subtilis_type_if_is_array(&s->t)) {
		op->reg =
		    subtils_parser_array_lvalue(p, t, s, mem_reg, type, err);
		return false;
	}

	if (subtilis_type_if_is_vector(&s->t)) {
		op->reg =
		    subtils_parser_vector_lvalue(p, t, s, mem_reg, type, err);
		return false;
	}

	subtilis_error_set_assertion_failed(err);
	return false;
}

static subtilis_exp_t *prv_lookup_var(subtilis_parser_t *p, subtilis_token_t *t,
				      const char *tbuf, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;
	subtilis_exp_t *e1 = NULL;
	char *var_name = NULL;
	size_t mem_reg = SUBTILIS_IR_REG_LOCAL;

	tbuf = subtilis_token_get_text(t);
	var_name = malloc(strlen(tbuf) + 1);
	if (!var_name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(var_name, tbuf);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s = subtilis_parser_get_symbol(p, var_name, &mem_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if ((s->t.type == SUBTILIS_TYPE_FN) &&
	    (t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		if (s->t.params.fn.ret_val->type == SUBTILIS_TYPE_VOID) {
			subtilis_error_set_expected(
			    err, "function", "procedure", p->l->stream->name,
			    p->l->line);
			goto cleanup;
		}
		e = subtilis_parser_call_ptr(p, t, s, var_name, mem_reg, err);
	} else if ((s->t.type == SUBTILIS_TYPE_REC) &&
		   (t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ".")) {
		e = subtilis_parser_rec_exp(p, t, &s->t, mem_reg, s->loc, err);
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "(")) {
		e = subtils_parser_read_array(p, t, var_name, err);
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "{")) {
		e = subtils_parser_read_vector(p, t, var_name, err);
	} else {
		e = subtilis_var_lookup_var(p, var_name, err);
	}

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * If our expression evaluates to a function pointer, which
	 * can be called, or a container type like an array or a
	 * record, we need to check to see whether we're performing
	 * a chained operation on the expression.
	 */

	while ((e->type.type == SUBTILIS_TYPE_FN) ||
	       (e->type.type == SUBTILIS_TYPE_REC) ||
	       subtilis_type_if_is_array(&e->type) ||
	       subtilis_type_if_is_vector(&e->type)) {
		tbuf = subtilis_token_get_text(t);
		if ((e->type.type == SUBTILIS_TYPE_FN) &&
		    (t->type == SUBTILIS_TOKEN_OPERATOR) &&
		    !strcmp(tbuf, "(")) {
			e1 = subtilis_parser_call_known_ptr(
			    p, t, &e->type, e->exp.ir_op.reg, err);
			if ((err->type == SUBTILIS_ERROR_OK) && !e1) {
				subtilis_error_set_expected(
					err, "function", "procedure",
					p->l->stream->name, p->l->line);
				goto cleanup;
			}
		} else if ((e->type.type == SUBTILIS_TYPE_REC) &&
			   (t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, ".")) {
			e1 = subtilis_parser_rec_exp(p, t, &e->type,
						     e->exp.ir_op.reg, 0, err);
		} else if (subtilis_type_if_is_array(&e->type) &&
			   (t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, "(")) {
			e1 = subtils_parser_read_array_with_type(
			    p, t, &e->type, e->exp.ir_op.reg, 0, "temporary",
			    err);
		} else if (subtilis_type_if_is_vector(&e->type) &&
			   (t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, "{")) {
			e1 = subtils_parser_read_vector_with_type(
			    p, t, &e->type, e->exp.ir_op.reg, 0, "temporary",
			    err);
		} else {
			break;
		}

		subtilis_exp_delete(e);
		if (err->type != SUBTILIS_ERROR_OK) {
			e = NULL;
			goto cleanup;
		}
		e = e1;
	}

cleanup:

	free(var_name);

	return e;
}

static subtilis_exp_t *prv_priority1(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	tbuf = subtilis_token_get_text(t);
	switch (t->type) {
	case SUBTILIS_TOKEN_STRING:
		e = subtilis_exp_new_str(&t->buf, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_INTEGER:
		e = subtilis_exp_new_int32(t->tok.integer, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_REAL:
		e = subtilis_exp_new_real(t->tok.real, err);
		if (!e)
			goto cleanup;
		break;
	case SUBTILIS_TOKEN_OPERATOR:
		if (!strcmp(tbuf, "(")) {
			e = subtilis_parser_bracketed_exp_internal(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else if (!strcmp(tbuf, "-")) {
			return prv_unary_minus_exp(p, t, err);

			/* we don't want to read another token
			 * here.It's already been read by the
			 * recursive call to prv_priority1.
			 */
		} else if (!strcmp(tbuf, "!")) {
			e = prv_call_addr_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
		} else {
			subtilis_error_set_exp_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		break;
	case SUBTILIS_TOKEN_IDENTIFIER:
		e = prv_lookup_var(p, t, tbuf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		return e;
	case SUBTILIS_TOKEN_KEYWORD:
		switch (t->tok.keyword.type) {
		case SUBTILIS_KEYWORD_TRUE:
			e = subtilis_exp_new_int32(-1, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_KEYWORD_FALSE:
			e = subtilis_exp_new_int32(0, err);
			if (!e)
				goto cleanup;
			break;
		case SUBTILIS_KEYWORD_NOT:
			e = prv_not_exp(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto cleanup;
			return e;
		case SUBTILIS_KEYWORD_FN:
			return subtilis_parser_call(p, t, err);
		case SUBTILIS_KEYWORD_INT:
			return subtilis_parser_int(p, t, err);
		case SUBTILIS_KEYWORD_INTZ:
			return subtilis_parser_intz(p, t, err);
		case SUBTILIS_KEYWORD_TIME:
			return prv_gettime(p, t, err);
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
		case SUBTILIS_KEYWORD_RND:
			return subtilis_parser_rnd(p, t, err);
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
		case SUBTILIS_KEYWORD_GET:
			return subtilis_parser_get(p, t, err);
		case SUBTILIS_KEYWORD_GET_HASH:
			return subtilis_parser_get_hash(p, t, err);
		case SUBTILIS_KEYWORD_GET_STR:
			return subtilis_parser_get_str(p, t, err);
		case SUBTILIS_KEYWORD_INKEY:
			return subtilis_parser_inkey(p, t, err);
		case SUBTILIS_KEYWORD_POINT:
			return subtilis_parser_get_point(p, t, err);
		case SUBTILIS_KEYWORD_TINT:
			return subtilis_parser_get_tint(p, t, err);
		case SUBTILIS_KEYWORD_ERR:
			return subtilis_parser_get_err(p, t, err);
		case SUBTILIS_KEYWORD_DIM:
			return subtilis_parser_get_dim(p, t, err);
		case SUBTILIS_KEYWORD_CHR_STR:
			return subtilis_parser_chrstr(p, t, err);
		case SUBTILIS_KEYWORD_ASC:
			return subtilis_parser_asc(p, t, err);
		case SUBTILIS_KEYWORD_LEN:
			return subtilis_parser_len(p, t, err);
		case SUBTILIS_KEYWORD_LEFT_STR:
			return subtilis_parser_left_str_exp(p, t, err);
		case SUBTILIS_KEYWORD_HEAP_FREE:
			return subtilis_parser_mem_heap_free(p, t, err);
		case SUBTILIS_KEYWORD_RIGHT_STR:
			return subtilis_parser_right_str_exp(p, t, err);
		case SUBTILIS_KEYWORD_MID_STR:
			return subtilis_parser_mid_str_exp(p, t, err);
		case SUBTILIS_KEYWORD_POS:
			return subtilis_parser_pos(p, t, err);
		case SUBTILIS_KEYWORD_VPOS:
			return subtilis_parser_vpos(p, t, err);
		case SUBTILIS_KEYWORD_STRING_STR:
			return subtilis_parser_string_str(p, t, err);
		case SUBTILIS_KEYWORD_STR_STR:
			return subtilis_parser_str_str(p, t, err);
		case SUBTILIS_KEYWORD_VAL:
			return subtilis_parser_val(p, t, err);
		case SUBTILIS_KEYWORD_TRY:
			e = subtilis_parser_try_exp(p, t, err);
			break;
		case SUBTILIS_KEYWORD_TRY_ONE:
			return subtilis_parser_try_one_exp(p, t, err);
		case SUBTILIS_KEYWORD_OPENIN:
			return subtilis_parser_openin(p, t, err);
		case SUBTILIS_KEYWORD_OPENOUT:
			return subtilis_parser_openout(p, t, err);
		case SUBTILIS_KEYWORD_OPENUP:
			return subtilis_parser_openup(p, t, err);
		case SUBTILIS_KEYWORD_BGET_HASH:
			return subtilis_parser_bget(p, t, err);
		case SUBTILIS_KEYWORD_EOF_HASH:
			return subtilis_parser_eof(p, t, err);
		case SUBTILIS_KEYWORD_EXT_HASH:
			return subtilis_parser_ext(p, t, err);
		case SUBTILIS_KEYWORD_PTR_HASH:
			return subtilis_parser_get_ptr(p, t, err);
		case SUBTILIS_KEYWORD_APPEND:
			return subtilis_parser_append_exp(p, t, err);
		case SUBTILIS_KEYWORD_COPY:
			return subtilis_parser_copy_exp(p, t, err);
		case SUBTILIS_KEYWORD_DEF:
			return subtilis_parser_lambda(p, t, err);
		default:
			subtilis_error_set_exp_expected(
			    err, "Unexpected keyword in expression",
			    p->l->stream->name, p->l->line);
			goto cleanup;
		}
		break;
	default:
		subtilis_error_set_exp_expected(err, tbuf, p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

static subtilis_exp_t *prv_priority2(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;

	e1 = prv_priority1(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (strcmp(tbuf, "^"))
			break;

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority1(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = subtilis_type_if_pow(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority3(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority2(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while ((t->type == SUBTILIS_TOKEN_OPERATOR) ||
	       (t->type == SUBTILIS_TOKEN_KEYWORD)) {
		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_KEYWORD) {
			if (t->tok.keyword.type == SUBTILIS_KEYWORD_DIV)
				exp_fn = subtilis_type_if_div;
			else if (t->tok.keyword.type == SUBTILIS_KEYWORD_MOD)
				exp_fn = subtilis_type_if_mod;
			else
				break;
		} else if (!strcmp(tbuf, "*")) {
			exp_fn = subtilis_type_if_mul;
		} else if (!strcmp(tbuf, "/")) {
			exp_fn = subtilis_type_if_divr;
		} else {
			break;
		}
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority2(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority4(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority3(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	tbuf = subtilis_token_get_text(t);

	/*
	 * Special case for number too big.
	 */

	if ((t->type == SUBTILIS_TOKEN_INTEGER) &&
	    (t->tok.integer == -2147483648)) {
		subtilis_error_set_number_too_long(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "+"))
			exp_fn = subtilis_exp_add;
		else if (!strcmp(tbuf, "-"))
			exp_fn = subtilis_type_if_sub;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority3(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority5(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority4(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	while (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, "="))
			exp_fn = subtilis_type_if_eq;
		else if (!strcmp(tbuf, "<>"))
			exp_fn = subtilis_type_if_neq;
		else if (!strcmp(tbuf, ">"))
			exp_fn = subtilis_exp_gt;
		else if (!strcmp(tbuf, "<="))
			exp_fn = subtilis_exp_lte;
		else if (!strcmp(tbuf, "<"))
			exp_fn = subtilis_exp_lt;
		else if (!strcmp(tbuf, ">="))
			exp_fn = subtilis_exp_gte;
		else if (!strcmp(tbuf, "<<"))
			exp_fn = subtilis_type_if_lsl;
		else if (!strcmp(tbuf, ">>"))
			exp_fn = subtilis_type_if_lsr;
		else if (!strcmp(tbuf, ">>>"))
			exp_fn = subtilis_type_if_asr;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority4(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;

		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

static subtilis_exp_t *prv_priority6(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;

	e1 = prv_priority5(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (t->type == SUBTILIS_TOKEN_KEYWORD &&
	       t->tok.keyword.type == SUBTILIS_KEYWORD_AND) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority5(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = subtilis_type_if_and(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

subtilis_exp_t *subtilis_parser_priority7(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err)
{
	subtilis_exp_t *e1 = NULL;
	subtilis_exp_t *e2 = NULL;
	subtilis_exp_fn_t exp_fn;

	e1 = prv_priority6(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	while (t->type == SUBTILIS_TOKEN_KEYWORD) {
		if (t->tok.keyword.type == SUBTILIS_KEYWORD_EOR)
			exp_fn = subtilis_type_if_eor;
		else if (t->tok.keyword.type == SUBTILIS_KEYWORD_OR)
			exp_fn = subtilis_type_if_or;
		else
			break;
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e2 = prv_priority6(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		e1 = exp_fn(p, e1, e2, err);
		e2 = NULL;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	return e1;

cleanup:

	subtilis_exp_delete(e2);
	subtilis_exp_delete(e1);
	return NULL;
}

/* clang-format off */
subtilis_exp_t *subtilis_parser_call_1_arg_fn(subtilis_parser_t *p,
					      const char *name, size_t reg,
					      subtilis_builtin_type_t ftype,
					      subtilis_ir_reg_type_t ptype,
					      const subtilis_type_t *rtype,
					      bool check_errors,
					      subtilis_error_t *err)
/* clang-format on */
{
	subtilis_ir_arg_t *args = NULL;
	char *name_dup = NULL;

	args = malloc(sizeof(*args) * 1);
	if (!args) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	name_dup = malloc(strlen(name) + 1);
	if (!name_dup) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(name_dup, name);

	args[0].type = ptype;
	args[0].reg = reg;

	return subtilis_exp_add_call(p, name_dup, ftype, NULL, args, rtype, 1,
				     check_errors, err);

cleanup:

	free(args);

	return NULL;
}

/* clang-format off */
subtilis_exp_t *subtilis_parser_call_2_arg_fn(subtilis_parser_t *p,
					      const char *name, size_t arg1,
					      size_t arg2,
					      subtilis_ir_reg_type_t ptype1,
					      subtilis_ir_reg_type_t ptype2,
					      const subtilis_type_t *rtype,
					      bool check_errors,
					      subtilis_error_t *err)
/* clang-format on */

{
	subtilis_ir_arg_t *args = NULL;
	char *name_dup = NULL;

	args = malloc(sizeof(*args) * 2);
	if (!args) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	name_dup = malloc(strlen(name) + 1);
	if (!name_dup) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	strcpy(name_dup, name);

	args[0].type = ptype1;
	args[0].reg = arg1;
	args[1].type = ptype2;
	args[1].reg = arg2;

	return subtilis_exp_add_call(p, name_dup, SUBTILIS_BUILTINS_MAX, NULL,
				     args, rtype, 2, check_errors, err);

cleanup:

	free(args);

	return NULL;
}

subtilis_exp_t *subtilis_parser_expression(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err)
{
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_parser_priority7(p, t, err);
}

subtilis_exp_t *subtilis_parser_int_var_expression(subtilis_parser_t *p,
						   subtilis_token_t *t,
						   subtilis_error_t *err)
{
	subtilis_exp_t *e = NULL;

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

void subtilis_parser_statement_int_args(subtilis_parser_t *p,
					subtilis_token_t *t, subtilis_exp_t **e,
					size_t expected, subtilis_error_t *err)
{
	size_t i;
	const char *tbuf;

	e[0] = subtilis_parser_int_var_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 1; i < expected; i++) {
		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return;
		}

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			return;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		e[i] = subtilis_parser_int_var_expression(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
}

static void prv_bracketed_int_args(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_exp_t **e, size_t expected,
				   subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_statement_int_args(p, t, e, expected, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, ")")) {
		subtilis_error_set_exp_expected(err, ") ", p->l->stream->name,
						p->l->line);
		return;
	}

	subtilis_lexer_get(p->l, t, err);
}

subtilis_exp_t *
subtilis_parser_bracketed_2_int_args(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_op_instr_type_t itype,
				     subtilis_error_t *err)
{
	subtilis_exp_t *e[2];
	size_t reg;
	size_t i;
	subtilis_exp_t *retval = NULL;

	memset(&e, 0, sizeof(e));

	prv_bracketed_int_args(p, t, e, 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	reg = subtilis_ir_section_add_instr(p->current, itype, e[0]->exp.ir_op,
					    e[1]->exp.ir_op, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	retval = subtilis_exp_new_int32_var(reg, err);

cleanup:

	for (i = 0; i < 2; i++)
		subtilis_exp_delete(e[i]);

	return retval;
}

subtilis_exp_t *subtilis_parser_bracketed_exp_internal(subtilis_parser_t *p,
						       subtilis_token_t *t,
						       subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ")")) {
		subtilis_exp_delete(e);
		subtilis_error_set_right_bkt_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return NULL;
	}

	return e;
}

subtilis_exp_t *subtilis_parser_bracketed_exp(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "(")) {
		subtilis_error_set_exp_expected(err, "( ", p->l->stream->name,
						p->l->line);
		return NULL;
	}

	e = subtilis_parser_bracketed_exp_internal(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(e);
	return NULL;
}

subtilis_exp_t *subtilis_parser_integer_bracketed_exp(subtilis_parser_t *p,
						      subtilis_token_t *t,
						      subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_to_int(p, e, err);
}

subtilis_exp_t *subtilis_parser_real_bracketed_exp(subtilis_parser_t *p,
						   subtilis_token_t *t,
						   subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_type_if_to_float64(p, e, err);
}

static size_t prv_bracketed_args_have_b(subtilis_parser_t *p,
					subtilis_token_t *t, subtilis_exp_t **e,
					size_t max,
					subtilis_type_if_unary_t conv,
					subtilis_error_t *err)
{
	size_t i;
	const char *tbuf;

	memset(e, 0, sizeof(*e) * max);

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	if (t->type == SUBTILIS_TOKEN_OPERATOR) {
		tbuf = subtilis_token_get_text(t);
		if (!strcmp(tbuf, ")"))
			return 0;
	}

	e[0] = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	if (conv) {
		e[0] = conv(p, e[0], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}

	for (i = 1; i < max; i++) {
		tbuf = subtilis_token_get_text(t);
		if (t->type != SUBTILIS_TOKEN_OPERATOR) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		if (!strcmp(tbuf, ")"))
			return i;

		if (strcmp(tbuf, ",")) {
			subtilis_error_set_expected(
			    err, ",", tbuf, p->l->stream->name, p->l->line);
			goto on_error;
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		e[i] = subtilis_parser_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;

		if (conv) {
			e[i] = conv(p, e[i], err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto on_error;
		}
	}

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ")"))
		return i;

	/*
	 * Too many integers in the list
	 */

	subtilis_error_set_right_bkt_expected(err, tbuf, p->l->stream->name,
					      p->l->line);

on_error:

	for (i = 0; i < max && e[i]; i++)
		subtilis_exp_delete(e[i]);

	return 0;
}

size_t subtilis_var_bracketed_int_args_have_b(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_exp_t **e, size_t max,
					      subtilis_error_t *err)
{
	return prv_bracketed_args_have_b(p, t, e, max, subtilis_type_if_to_int,
					 err);
}

size_t subtilis_var_bracketed_args_have_b(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_exp_t **e, size_t max,
					  subtilis_error_t *err)
{
	return prv_bracketed_args_have_b(p, t, e, max, NULL, err);
}

static size_t prv_slice_have_b(subtilis_parser_t *p, subtilis_token_t *t,
			       const char *b, subtilis_exp_t **ee,
			       subtilis_error_t *err)
{
	const char *tbuf;
	char expected[3];

	ee[0] = NULL;
	ee[1] = NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, b))
		return 0;

	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, ":")) {
		ee[0] = subtilis_exp_new_int32(0, err);
	} else {
		ee[0] = subtilis_parser_priority7(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return 0;

		ee[0] = subtilis_type_if_to_int(p, ee[0], err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, b))
		return 1;

	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ":")) {
		subtilis_error_set_expected(err, ":", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, b)) {
		/*
		 * If ee[0] is constant 0, either because the programmer
		 * specified 0 or omitted the first slice parameter, there's no
		 * point in slicing the vector.  We want to return 0 to indicate
		 * that we should take a vector reference instead.
		 */
		if ((ee[0]->type.type == SUBTILIS_TYPE_CONST_INTEGER) &&
		    (ee[0]->exp.ir_op.integer == 0))
			return 0;
		ee[1] = NULL;
		return 2;
	}

	ee[1] = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	ee[1] = subtilis_type_if_to_int(p, ee[1], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, b)) {
		expected[0] = b[0];
		expected[1] = ' ';
		expected[2] = 0;
		subtilis_error_set_expected(err, expected, tbuf,
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	return 2;

cleanup:
	subtilis_exp_delete(ee[1]);
	subtilis_exp_delete(ee[0]);

	return 0;
}

size_t subtilis_curly_bracketed_slice_have_b(subtilis_parser_t *p,
					     subtilis_token_t *t,
					     subtilis_exp_t **ee,
					     subtilis_error_t *err)
{
	return prv_slice_have_b(p, t, "}", ee, err);
}

size_t subtilis_round_bracketed_slice_have_b(subtilis_parser_t *p,
					     subtilis_token_t *t,
					     subtilis_exp_t **ee,
					     subtilis_error_t *err)
{
	return prv_slice_have_b(p, t, ")", ee, err);
}

subtilis_exp_t *subtilis_curly_bracketed_arg_have_b(subtilis_parser_t *p,
						    subtilis_token_t *t,
						    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *e;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if ((t->type == SUBTILIS_TOKEN_OPERATOR) && !strcmp(tbuf, "}"))
		return NULL;

	e = subtilis_parser_priority7(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_type_if_to_int(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	tbuf = subtilis_token_get_text(t);
	if (strcmp(tbuf, "}")) {
		subtilis_exp_delete(e);
		subtilis_error_set_expected(err, "} ", tbuf, p->l->stream->name,
					    p->l->line);
		return NULL;
	}

	return e;
}

/*
 * Read the next expression which must be a variable or an
 * array access and return a reference to that variable.  If the
 * variable is a scalar local variable, its register is simply
 * returned.  If it's a global variable or an array expression
 * a pointer to the variable is returned.  This is used to
 * implement SYS and also hopefully one day return parameters.
 */

subtilis_exp_t *subtilis_var_lookup_ref(subtilis_parser_t *p,
					subtilis_token_t *t, bool *local,
					subtilis_error_t *err)
{
	const char *tbuf;
	size_t reg;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op1;

	tbuf = subtilis_token_get_text(t);

	reg = SUBTILIS_IR_REG_LOCAL;
	s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (!s) {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (!s) {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			return NULL;
		}
		reg = SUBTILIS_IR_REG_GLOBAL;
	}

	if (s->t.type == SUBTILIS_TYPE_INTEGER) {
		if (!s->is_reg) {
			*local = false;
			if (s->loc != 0) {
				op0.reg = reg;
				op1.integer = s->loc;
				reg = subtilis_ir_section_add_instr(
				    p->current, SUBTILIS_OP_INSTR_ADDI_I32, op0,
				    op1, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return NULL;
			}
			e = subtilis_exp_new_var(&s->t, reg, err);
		} else {
			*local = true;
			e = subtilis_exp_new_var(&s->t, s->loc, err);
		}

		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK) {
			subtilis_exp_delete(e);
			return NULL;
		}

		return e;
	} else if ((s->t.type == SUBTILIS_TYPE_ARRAY_INTEGER) ||
		   (s->t.type == SUBTILIS_TYPE_ARRAY_BYTE)) {
		subtilis_lexer_get(p->l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;

		tbuf = subtilis_token_get_text(t);
		if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
			subtilis_error_set_exp_expected(
			    err, "(", p->l->stream->name, p->l->line);

			return NULL;
		}

		*local = false;
		return subtils_parser_element_address(p, t, s, reg, tbuf, err);
	}

	subtilis_error_set_integer_variable_expected(
	    err, subtilis_type_name(&s->t), p->l->stream->name, p->l->line);
	return NULL;
}

void subtilis_complete_custom_type(subtilis_parser_t *p, const char *var_name,
				   subtilis_type_t *type, subtilis_error_t *err)
{
	const subtilis_symbol_t *f_s;
	const char *type_name;

	type_name = strchr(var_name, '@');
	if (!type_name) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	type_name++;

	f_s = subtilis_symbol_table_lookup(p->st, type_name);
	if (!f_s) {
		subtilis_error_set_unknown_type(err, type_name,
						p->l->stream->name, p->l->line);
		return;
	}

	if (f_s->t.type != SUBTILIS_TYPE_TYPEDEF) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_type_copy(type, &f_s->sub_t, err);
}

const subtilis_symbol_t *subtilis_parser_get_symbol(subtilis_parser_t *p,
						    const char *var_name,
						    size_t *mem_reg,
						    subtilis_error_t *err)
{
	const subtilis_symbol_t *s;

	s = subtilis_symbol_table_lookup(p->local_st, var_name);
	if (s) {
		*mem_reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		s = subtilis_symbol_table_lookup(p->st, var_name);
		if (!s) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			return NULL;
		}
		*mem_reg = SUBTILIS_IR_REG_GLOBAL;
	}

	return s;
}

void subtilis_exp_swap_int32_mem(subtilis_parser_t *p, size_t dest_reg,
				 size_t source_reg, int32_t off,
				 subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;
	subtilis_ir_operand_t tmp1;
	subtilis_ir_operand_t tmp2;
	subtilis_ir_operand_t d;
	subtilis_ir_operand_t s;

	d.reg = dest_reg;
	s.reg = source_reg;

	op2.integer = off;
	tmp1.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, s, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tmp2.reg = subtilis_ir_section_add_instr(
	    p->current, SUBTILIS_OP_INSTR_LOADO_I32, d, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, tmp1, d, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, tmp2, s, op2, err);
}
