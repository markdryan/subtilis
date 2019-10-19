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

#include "variable.h"
#include "expression.h"
#include "globals.h"
#include "type_if.h"

void subtilis_var_assign_hidden(subtilis_parser_t *p, const char *var_name,
				const subtilis_type_t *id_type,
				subtilis_exp_t *e, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t op1;

	s = subtilis_symbol_table_lookup(p->st, var_name);
	if (!s) {
		s = subtilis_symbol_table_insert(p->st, var_name, id_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = SUBTILIS_IR_REG_GLOBAL;

	e = subtilis_exp_coerce_type(p, e, &s->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->is_reg)
		subtilis_type_if_assign_to_reg(p, s->loc, e, err);
	else
		subtilis_type_if_assign_to_mem(p, op1.reg, s->loc, e, err);

	return;

cleanup:

	subtilis_exp_delete(e);
}

subtilis_exp_t *subtilis_var_lookup_var(subtilis_parser_t *p, const char *tbuf,
					subtilis_error_t *err)
{
	size_t reg;
	const subtilis_symbol_t *s;

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

	if (!s->is_reg) {
		return subtilis_type_if_load_from_mem(p, &s->t, reg, s->loc,
						      err);
	} else {
		return subtilis_exp_new_var(&s->t, s->loc, err);
	}
}

void subtilis_var_set_eflag(subtilis_parser_t *p, bool value,
			    subtilis_error_t *err)
{
	subtilis_exp_t *ecode;

	ecode = subtilis_exp_new_int32(value ? -1 : 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_eflag_hidden_var,
				   &subtilis_type_integer, ecode, err);
}
