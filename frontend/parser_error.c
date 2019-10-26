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

#include "globals.h"
#include "parser_error.h"
#include "variable.h"

subtilis_exp_t *subtilis_parser_get_err(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err)
{
	subtilis_exp_t *e;

	e = subtilis_var_lookup_var(p, subtilis_err_hidden_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_exp_delete(e);
		return NULL;
	}

	return e;
}

void subtilis_parser_handle_escape(subtilis_parser_t *p, subtilis_error_t *err)
{
	/* Let's not test for escape in an error handler. */

	if (!p->handle_escapes || p->current->in_error_handler)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current,
					     SUBTILIS_OP_INSTR_TESTESC, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_exp_handle_errors(p, err);
}
