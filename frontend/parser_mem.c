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

void subtilis_parser_copy(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *obj1 = NULL;
	subtilis_exp_t *obj2 = NULL;

	obj1 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	obj2 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_memcpy(p, obj1, obj2, err);
	return;

cleanup:
	subtilis_exp_delete(obj1);
}

void subtilis_parser_append(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_exp_t *obj1 = NULL;
	subtilis_exp_t *obj2 = NULL;

	obj1 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	tbuf = subtilis_token_get_text(t);
	if ((t->type != SUBTILIS_TOKEN_OPERATOR) || strcmp(tbuf, ",")) {
		subtilis_error_set_expected(err, ",", tbuf, p->l->stream->name,
					    p->l->line);
		goto cleanup;
	}

	obj2 = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_type_if_append(p, obj1, obj2, err);
	return;

cleanup:

	subtilis_exp_delete(obj1);
}
