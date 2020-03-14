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

#include "parser_string.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"

subtilis_exp_t *subtilis_parser_chrstr(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err)
{
	subtilis_exp_t *e;
	subtilis_exp_t *retval;
	subtilis_type_t type;
	size_t reg;
	const subtilis_symbol_t *s;
	subtilis_buffer_t buf;
	char c_str[2];

	subtilis_buffer_init(&buf, 2);

	e = subtilis_parser_integer_bracketed_exp(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	if (subtilis_type_if_is_const(&e->type)) {
		c_str[0] = e->exp.ir_op.integer & 255;
		c_str[1] = 0;
		subtilis_buffer_append(&buf, c_str, 2, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		retval = subtilis_exp_new_str(&buf, err);
		subtilis_buffer_free(&buf);
		subtilis_exp_delete(e);
		return retval;
	}

	type.type = SUBTILIS_TYPE_STRING;
	s = subtilis_symbol_table_insert_tmp(p->local_st, &type, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_string_type_new_ref_from_char(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					       e, err);
	e = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	reg = subtilis_reference_get_pointer(p, SUBTILIS_IR_REG_LOCAL, s->loc,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_exp_new_var(&s->t, reg, err);

cleanup:
	subtilis_buffer_free(&buf);
	subtilis_exp_delete(e);

	return NULL;
}

subtilis_exp_t *subtilis_parser_asc(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_asc(p, e, err);
}

subtilis_exp_t *subtilis_parser_len(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_parser_expression(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return subtilis_string_type_len(p, e, err);
}
