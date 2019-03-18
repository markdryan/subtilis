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

#include "expression.h"
#include "ir.h"
#include "parser.h"

#ifndef __SUBTILIS_VARIABLE_H__
#define __SUBTILIS_VARIABLE_H__

void subtilis_var_assign_to_reg(subtilis_parser_t *p, subtilis_token_t *t,
				const char *tbuf, size_t loc, subtilis_exp_t *e,
				subtilis_error_t *err);
void subtilis_var_assign_to_mem(subtilis_parser_t *p, const char *tbuf,
				subtilis_ir_operand_t op1, size_t loc,
				subtilis_exp_t *e, subtilis_error_t *err);
void subtilis_var_assign_hidden(subtilis_parser_t *p, const char *var_name,
				subtilis_type_t id_type, subtilis_exp_t *e,
				subtilis_error_t *err);
subtilis_exp_t *subtilis_var_lookup_var(subtilis_parser_t *p, const char *tbuf,
					subtilis_error_t *err);

#endif
