/*
 * Copyright (c) 2022 Mark Ryan
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

#ifndef __SUBTILIS_PARSER_REC_H
#define __SUBTILIS_PARSER_REC_H

#include "expression.h"
#include "parser.h"

char *subtilis_parser_parse_rec_type(subtilis_parser_t *p, subtilis_token_t *t,
				     subtilis_type_t *type,
				     subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_rec_exp(subtilis_parser_t *p,
					subtilis_token_t *t,
					const subtilis_type_t *type,
					size_t mem_reg, size_t loc,
					subtilis_error_t *err);

void subtilis_parser_rec_init(subtilis_parser_t *p, subtilis_token_t *t,
			      const subtilis_type_t *type, size_t mem_reg,
			      size_t loc, bool push, subtilis_error_t *err);

/*
 * Allow an initialiser list to initialise an existing REC.
 */

void subtilis_parser_rec_reset(subtilis_parser_t *p, subtilis_token_t *t,
			       const subtilis_type_t *type, size_t mem_reg,
			       size_t loc, subtilis_error_t *err);

#endif
