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

#include <stdlib.h>
#include <string.h>

#include "array_type.h"
#include "parser_array.h"
#include "parser_assignment.h"
#include "parser_call.h"
#include "parser_exp.h"
#include "string_type.h"
#include "type_if.h"
#include "variable.h"

typedef enum {
	SUBTILIS_ASSIGN_TYPE_EQUAL,
	SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL,
	SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL,
	SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL,
} subtilis_assign_type_t;

static subtilis_exp_t *prv_assignment_operator(subtilis_parser_t *p,
					       const char *var_name,
					       subtilis_exp_t *e,
					       subtilis_assign_type_t at,
					       subtilis_error_t *err)
{
	subtilis_exp_t *var;

	if (at == SUBTILIS_ASSIGN_TYPE_EQUAL)
		return e;

	var = subtilis_var_lookup_var(p, var_name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (at == SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL)
		e = subtilis_exp_add(p, var, e, err);
	else
		e = subtilis_type_if_sub(p, var, e, err);

	var = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return e;

cleanup:

	subtilis_exp_delete(var);
	subtilis_exp_delete(e);

	return NULL;
}

void subtilis_parser_create_array_ref(subtilis_parser_t *p,
				      const char *var_name,
				      const subtilis_type_t *array_type,
				      subtilis_exp_t *e, bool local,
				      subtilis_error_t *err)
{
	subtilis_type_t type;
	const subtilis_symbol_t *s;

	if (array_type->type != e->type.type) {
		subtilis_error_set_array_type_mismatch(err, p->l->stream->name,
						       p->l->line);
		goto cleanup;
	}
	subtilis_type_init_copy(&type, &e->type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * If the expression is a temporary variable and we're assigning to
	 * a local variable, we can simply just promote the temporary to a
	 * full local variable and avoid copying the data.
	 */

	if (local && e->temporary) {
		(void)subtilis_symbol_table_promote_tmp(
		    p->local_st, &e->type, e->temporary, var_name, err);
		subtilis_type_free(&type);
		subtilis_exp_delete(e);
		return;
	}

	s = subtilis_symbol_table_insert(local ? p->local_st : p->st, var_name,
					 &type, err);
	subtilis_type_free(&type);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_array_type_create_ref(
	    p, var_name, s,
	    local ? SUBTILIS_IR_REG_LOCAL : SUBTILIS_IR_REG_GLOBAL, e, err);
	return;

cleanup:

	subtilis_exp_delete(e);
}

static void prv_check_bad_def(subtilis_parser_t *p, const char *var_name,
			      subtilis_error_t *err)
{
	if (!strncmp(var_name, "DEFPROC", 7) ||
	    !strncmp(var_name, "defPROC", 7))
		subtilis_error_set_defproc_should_be_def_proc(
		    err, p->l->stream->name, p->l->line);
	else if (!strncmp(var_name, "DEFFN", 5) ||
		 !strncmp(var_name, "defFN", 5))
		subtilis_error_set_deffn_should_be_def_fn(
		    err, p->l->stream->name, p->l->line);
}

static void prv_missing_operator_error(subtilis_parser_t *p, const char *tbuf,
				       const char *var_name,
				       subtilis_error_t *err)
{
	subtilis_error_set_assignment_op_expected(err, tbuf, p->l->stream->name,
						  p->l->line);
	prv_check_bad_def(p, var_name, err);
}

static void prv_assign_array_or_vector(subtilis_parser_t *p,
				       subtilis_token_t *t, size_t dims,
				       subtilis_exp_t **indices,
				       const char *var_name,
				       const subtilis_type_t *array_type,
				       subtilis_error_t *err)
{
	const char *tbuf;
	size_t i;
	subtilis_assign_type_t at;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_exp_t *e = NULL;
	bool new_global = false;
	bool local;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		prv_missing_operator_error(p, tbuf, var_name, err);
		goto cleanup;
	}

	if (!strcmp(tbuf, "=")) {
		at = SUBTILIS_ASSIGN_TYPE_EQUAL;
	} else if (!strcmp(tbuf, "+=") && (dims > 0)) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=") && (dims > 0)) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else if (!strcmp(tbuf, ":=") && (dims == 0)) {
		at = SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL;
	} else {
		prv_missing_operator_error(p, tbuf, var_name, err);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type == SUBTILIS_ERROR_UNKNOWN_VARIABLE ||
	    err->type == SUBTILIS_ERROR_VARIABLE_BAD_LEVEL) {
		if (at != SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL)
			goto cleanup;
		subtilis_error_init(err);
		subtilis_parser_create_array_ref(p, var_name, array_type, e,
						 true, err);
		return;
	} else if (err->type != SUBTILIS_ERROR_OK) {
		goto cleanup;
	} else if (new_global) {
		if (dims != 0) {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
			goto cleanup;
		}
		local = (at == SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL);
		subtilis_parser_create_array_ref(p, var_name, array_type, e,
						 local, err);
		return;
	}

	switch (at) {
	case SUBTILIS_ASSIGN_TYPE_EQUAL:
		if (dims == 0)
			/* We're assigning an array reference */
			subtilis_parser_array_assign_reference(p, t, op1.reg, s,
							       e, err);
		else
			subtilis_type_if_indexed_write(p, var_name, &s->t,
						       op1.reg, s->loc, e,
						       indices, dims, err);
		e = NULL;
		break;
	case SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL:
		subtilis_type_if_indexed_add(p, var_name, &s->t, op1.reg,
					     s->loc, e, indices, dims, err);
		e = NULL;
		break;
	case SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL:
		subtilis_type_if_indexed_sub(p, var_name, &s->t, op1.reg,
					     s->loc, e, indices, dims, err);
		e = NULL;
		break;
	case SUBTILIS_ASSIGN_TYPE_CREATE_EQUAL:
		subtilis_error_set_already_defined(
		    err, var_name, p->l->stream->name, p->l->line);
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		break;
	}

cleanup:

	subtilis_exp_delete(e);
	for (i = 0; i < dims; i++)
		subtilis_exp_delete(indices[i]);
}

static void prv_assign_array(subtilis_parser_t *p, subtilis_token_t *t,
			     const char *var_name,
			     const subtilis_type_t *id_type,
			     subtilis_error_t *err)
{
	subtilis_type_t array_type;
	subtilis_exp_t *indices[SUBTILIS_MAX_DIMENSIONS];
	size_t dims = 0;

	array_type.type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_array_of(p, id_type, &array_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	dims = subtilis_var_bracketed_int_args_have_b(
	    p, t, indices, SUBTILIS_MAX_DIMENSIONS, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		prv_check_bad_def(p, var_name, err);
		goto cleanup;
	}

	prv_assign_array_or_vector(p, t, dims, indices, var_name, &array_type,
				   err);

cleanup:

	subtilis_type_free(&array_type);
}

static void prv_assign_vector(subtilis_parser_t *p, subtilis_token_t *t,
			      const char *var_name,
			      const subtilis_type_t *id_type,
			      subtilis_error_t *err)
{
	subtilis_type_t vector_type;
	subtilis_exp_t *indices[1];
	size_t dims = 0;

	vector_type.type = SUBTILIS_TYPE_VOID;
	subtilis_type_if_vector_of(p, id_type, &vector_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	indices[0] = subtilis_curly_bracketed_arg_have_b(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	if (indices[0])
		dims = 1;

	prv_assign_array_or_vector(p, t, dims, indices, var_name, &vector_type,
				   err);

cleanup:
	subtilis_type_free(&vector_type);
}

subtilis_exp_t *subtilis_parser_assign_local_num(subtilis_parser_t *p,
						 subtilis_token_t *t,
						 const char *var_name,
						 const subtilis_type_t *type,
						 subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	if (!subtilis_type_if_is_numeric(&e->type)) {
		subtilis_error_set_expected(err, "Numeric expression",
					    subtilis_type_name(&e->type),
					    p->l->stream->name, p->l->line);
		subtilis_exp_delete(e);
		return NULL;
	}
	e = subtilis_type_if_copy_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_type_if_exp_to_var(p, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	e = subtilis_type_if_coerce_type(p, e, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return e;
}

void subtilis_parser_zero_local_fn(subtilis_parser_t *p, subtilis_token_t *t,
				   const char *var_name,
				   const subtilis_type_t *id_type,
				   subtilis_error_t *err)
{
	subtilis_type_t type;
	subtilis_exp_t *e = NULL;

	subtilis_type_init_copy(&type, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_complete_custom_type(p, var_name, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)

		goto cleanup;
	e = subtilis_type_if_zero(p, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	(void)subtilis_symbol_table_insert_reg(p->local_st, var_name, &type,
					       e->exp.ir_op.reg, err);

cleanup:
	subtilis_exp_delete(e);
	subtilis_type_free(&type);
}

void subtilis_parser_assign_local_fn(subtilis_parser_t *p, subtilis_token_t *t,
				     const char *var_name,
				     const subtilis_type_t *id_type,
				     subtilis_error_t *err)
{
	subtilis_type_t type;
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (e->type.type != SUBTILIS_TYPE_FN) {
		subtilis_error_set_expected(err, subtilis_type_name(id_type),
					    subtilis_type_name(&e->type),
					    p->l->stream->name, p->l->line);
		goto cleanup;
	}

	subtilis_type_init_copy(&type, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_complete_custom_type(p, var_name, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (e->partial_name)
		subtilis_parser_call_add_addr(p, &type, e, err);
	else
		e = subtilis_type_if_coerce_type(p, e, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	(void)subtilis_symbol_table_insert_reg(p->local_st, var_name, &type,
					       e->exp.ir_op.reg, err);

cleanup:
	subtilis_exp_delete(e);
	subtilis_type_free(&type);
}

static void prv_assignment_local(subtilis_parser_t *p, subtilis_token_t *t,
				 const char *var_name,
				 const subtilis_type_t *id_type,
				 subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_exp_t *e;

	if ((p->current == p->main) && (p->level == 0)) {
		s = subtilis_symbol_table_lookup(p->st, var_name);
		if (s) {
			subtilis_error_set_local_obscures_global(
			    err, var_name, p->l->stream->name, p->l->line);
			return;
		}
	}

	if (subtilis_symbol_table_lookup(p->local_st, var_name)) {
		subtilis_error_set_already_defined(
		    err, var_name, p->l->stream->name, p->l->line);
		return;
	}

	if (subtilis_type_if_is_numeric(id_type)) {
		e = subtilis_parser_assign_local_num(p, t, var_name, id_type,
						     err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		(void)subtilis_symbol_table_insert_reg(
		    p->local_st, var_name, id_type, e->exp.ir_op.reg, err);
		subtilis_exp_delete(e);
		return;
	} else if (id_type->type == SUBTILIS_TYPE_FN) {
		subtilis_parser_assign_local_fn(p, t, var_name, id_type, err);
		return;
	} else if (!subtilis_type_if_is_reference(id_type)) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	/*
	 * We've got a reference type, e.g., a string.
	 */

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (e->temporary) {
		(void)subtilis_symbol_table_promote_tmp(
		    p->local_st, &e->type, e->temporary, var_name, err);

		subtilis_exp_delete(e);
		return;
	}

	s = subtilis_symbol_table_insert(p->local_st, var_name, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_new_ref(p, id_type, SUBTILIS_IR_REG_LOCAL, s->loc, e,
				 err);
}

char *subtilis_parser_get_assignment_var(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_type_t *id_type,
					 subtilis_error_t *err)
{
	const char *tbuf;
	char *var_name = NULL;

	tbuf = subtilis_token_get_text(t);
	var_name = malloc(strlen(tbuf) + 1);
	if (!var_name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(var_name, tbuf);
	subtilis_type_copy(id_type, &t->tok.id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return var_name;

cleanup:
	free(var_name);

	return NULL;
}

/* clang-format off */
void subtilis_parser_lookup_assignment_var(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   const char *var_name,
					   const subtilis_symbol_t **s,
					   size_t *mem_reg, bool *new_global,
					   subtilis_error_t *err)
/* clang-format on */

{
	*s = subtilis_symbol_table_lookup(p->local_st, var_name);
	if (*s) {
		*mem_reg = SUBTILIS_IR_REG_LOCAL;
	} else {
		*s = subtilis_symbol_table_lookup(p->st, var_name);
		if (*s) {
			*mem_reg = SUBTILIS_IR_REG_GLOBAL;
		} else if (p->current == p->main) {
			/*
			 * We explicitly disable statements like
			 *
			 * X% = X% + 1  or
			 * X% += 1
			 *
			 * for the cases where X% has not been defined.
			 */
			*new_global = true;
			*mem_reg = SUBTILIS_IR_REG_GLOBAL;
			if (p->level != 0) {
				subtilis_error_variable_bad_level(
				    err, var_name, p->l->stream->name,
				    p->l->line);
			}
		} else {
			subtilis_error_set_unknown_variable(
			    err, var_name, p->l->stream->name, p->l->line);
		}
	}
}

void subtilis_parser_assignment(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_ir_operand_t op1;
	const subtilis_symbol_t *s;
	subtilis_assign_type_t at;
	subtilis_type_t type;
	size_t mem_reg = SUBTILIS_IR_REG_LOCAL;
	bool new_global = false;
	char *var_name = NULL;
	subtilis_exp_t *e = NULL;

	type.type = SUBTILIS_TYPE_VOID;

	var_name = subtilis_parser_get_assignment_var(p, t, &type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR) {
		prv_missing_operator_error(p, tbuf, var_name, err);
		goto cleanup;
	}

	if (!strcmp(tbuf, "(")) {
		s = subtilis_symbol_table_lookup(p->local_st, var_name);
		if (!s) {
			s = subtilis_symbol_table_lookup(p->st, var_name);
			mem_reg = SUBTILIS_IR_REG_GLOBAL;
		}
		if (s && s->t.type == SUBTILIS_TYPE_FN)
			subtilis_parser_call_ptr(p, t, s, var_name, mem_reg,
						 err);
		else
			prv_assign_array(p, t, var_name, &type, err);
		goto cleanup;
	}

	if (!strcmp(tbuf, "{")) {
		prv_assign_vector(p, t, var_name, &type, err);
		goto cleanup;
	}

	if (!strcmp(tbuf, ":=")) {
		prv_assignment_local(p, t, var_name, &type, err);
		goto cleanup;
	}

	subtilis_parser_lookup_assignment_var(p, t, var_name, &s, &op1.reg,
					      &new_global, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!strcmp(tbuf, "=") || !strcmp(tbuf, ":=")) {
		at = SUBTILIS_ASSIGN_TYPE_EQUAL;
	} else if (new_global) {
		prv_missing_operator_error(p, tbuf, var_name, err);
		goto cleanup;
	} else if (!strcmp(tbuf, "+=")) {
		at = SUBTILIS_ASSIGN_TYPE_PLUS_EQUAL;
	} else if (!strcmp(tbuf, "-=") && subtilis_type_if_is_numeric(&type)) {
		at = SUBTILIS_ASSIGN_TYPE_MINUS_EQUAL;
	} else {
		subtilis_error_set_assignment_op_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		goto cleanup;
	}

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* We need special handling here.  If it's a new global we need
	 * to get the full type from the symbol table.  The type we get
	 * from the lexer is incomplete.  It knows that we have a
	 * function pointer but it doesn't know about the return type or
	 * the parameters.
	 */

	if (type.type == SUBTILIS_TYPE_FN) {
		subtilis_complete_custom_type(p, var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	if (new_global) {
		s = subtilis_symbol_table_insert(p->st, var_name, &type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	/* Ownership of e is passed to the following functions. */

	if (subtilis_type_if_is_reference(&s->t)) {
		if (at == SUBTILIS_ASSIGN_TYPE_EQUAL) {
			if (new_global)
				subtilis_type_if_new_ref(p, &s->t, op1.reg,
							 s->loc, e, err);
			else
				subtilis_type_if_assign_ref(p, &s->t, op1.reg,
							    s->loc, e, err);
		} else if (s->t.type == SUBTILIS_TYPE_STRING) {
			subtilis_string_type_add_eq(p, op1.reg, s->loc, e, err);
		} else {
			subtilis_error_set_assertion_failed(err);
		}
		goto cleanup;
	}

	if ((type.type == SUBTILIS_TYPE_FN) && (e->partial_name)) {
		if (e->type.type != SUBTILIS_TYPE_FN) {
			subtilis_error_set_expected(
			    err, subtilis_type_name(&type),
			    subtilis_type_name(&e->type), p->l->stream->name,
			    p->l->line);
			goto cleanup;
		}
		subtilis_parser_call_add_addr(p, &type, e, err);
	} else {
		e = subtilis_type_if_coerce_type(p, e, &type, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	e = prv_assignment_operator(p, var_name, e, at, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (s->is_reg)
		subtilis_type_if_assign_to_reg(p, s->loc, e, err);
	else
		subtilis_type_if_assign_to_mem(p, op1.reg, s->loc, e, err);

cleanup:

	subtilis_type_free(&type);
	free(var_name);
}

void subtilis_parser_let(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err)
{
	const char *tbuf;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (t->type != SUBTILIS_TOKEN_IDENTIFIER) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_id_expected(err, tbuf, p->l->stream->name,
					       p->l->line);
		return;
	}

	subtilis_parser_assignment(p, t, err);
}

void subtilis_parser_swap(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;
	bool lreg;
	bool rreg;
	subtilis_ir_operand_t lop;
	subtilis_ir_operand_t rop;
	subtilis_type_t ltype;
	subtilis_type_t rtype;

	ltype.type = SUBTILIS_TYPE_VOID;
	rtype.type = SUBTILIS_TYPE_VOID;

	lreg = subtilis_exp_get_lvalue(p, t, &lop, &ltype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	tbuf = subtilis_token_get_text(t);
	if (t->type != SUBTILIS_TOKEN_OPERATOR || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	rreg = subtilis_exp_get_lvalue(p, t, &rop, &rtype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (!subtilis_type_eq(&ltype, &rtype)) {
		subtilis_error_set_swap_type_mismatch(err, p->l->stream->name,
						      p->l->line);
		goto cleanup;
	}

	if (lreg) {
		if (rreg)
			subtilis_type_if_swap_reg_reg(p, &ltype, lop.reg,
						      rop.reg, err);
		else
			subtilis_type_if_swap_reg_mem(p, &ltype, lop.reg,
						      rop.reg, err);
	} else if (rreg) {
		subtilis_type_if_swap_reg_mem(p, &ltype, rop.reg, lop.reg, err);
	} else {
		subtilis_type_if_swap_mem_mem(p, &ltype, lop.reg, rop.reg, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_lexer_get(p->l, t, err);

cleanup:

	subtilis_type_free(&rtype);
	subtilis_type_free(&ltype);
}
