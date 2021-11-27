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

#include "parser_exp.h"
#include "parser_mem.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_mem_heap_free(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e = NULL;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_HEAP_FREE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	e = subtilis_exp_new_int32_var(reg, err);
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

static void prv_mem_statement(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_exp_t **objs, subtilis_error_t *err)
{
	const char *tbuf;
	size_t args;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, "(")) {
		subtilis_error_set_expected(err, "( ", tbuf, p->l->stream->name,
					    p->l->line);
		return;
	}

	args = subtilis_var_bracketed_args_have_b(p, t, &objs[0], 2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (args != 2) {
		subtilis_error_set_exp_expected(err, ")", p->l->stream->name,
						p->l->line);
		goto cleanup;
	}

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return;

cleanup:

	subtilis_exp_delete(objs[1]);
	subtilis_exp_delete(objs[0]);
}

void subtilis_parser_copy(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	subtilis_exp_t *objs[2] = {NULL, NULL};

	prv_mem_statement(p, t, &objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_type_if_memcpy(p, objs[0], objs[1], err);
}

subtilis_exp_t *subtilis_parser_copy_exp(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	subtilis_exp_t *objs[2] = {NULL, NULL};
	subtilis_exp_t *retval = NULL;

	prv_mem_statement(p, t, &objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	retval = subtilis_type_if_dup(objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_memcpy(p, objs[0], objs[1], err);
	objs[0] = NULL;
	objs[1] = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return retval;

cleanup:
	subtilis_exp_delete(retval);
	subtilis_exp_delete(objs[1]);
	subtilis_exp_delete(objs[0]);
	return NULL;
}

void subtilis_parser_append(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	subtilis_exp_t *objs[2] = {NULL, NULL};

	prv_mem_statement(p, t, &objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (objs[0]->temporary) {
		subtilis_error_set_temporary_not_allowed(
		    err, "first argument to append", p->l->stream->name,
		    p->l->line);
		goto cleanup;
	}

	subtilis_type_if_append(p, objs[0], objs[1], err);
	return;

cleanup:
	subtilis_exp_delete(objs[1]);
	subtilis_exp_delete(objs[0]);
}

subtilis_exp_t *subtilis_parser_append_exp(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err)
{
	subtilis_exp_t *objs[2] = {NULL, NULL};
	subtilis_exp_t *retval = NULL;

	prv_mem_statement(p, t, &objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	retval = subtilis_type_if_dup(objs[0], err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_append(p, objs[0], objs[1], err);
	objs[0] = NULL;
	objs[1] = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return retval;

cleanup:
	subtilis_exp_delete(retval);
	subtilis_exp_delete(objs[1]);
	subtilis_exp_delete(objs[0]);
	return NULL;
}
