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

#ifndef __SUBTILIS_PARSER_STRING_H
#define __SUBTILIS_PARSER_STRING_H

#include "expression.h"

subtilis_exp_t *subtilis_parser_chrstr(subtilis_parser_t *p,
				       subtilis_token_t *t,
				       subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_asc(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_len(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_left_str_exp(subtilis_parser_t *p,
					     subtilis_token_t *t,
					     subtilis_error_t *err);
void subtilis_parser_left_str(subtilis_parser_t *p, subtilis_token_t *t,
			      subtilis_error_t *err);
void subtilis_parser_right_str(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err);
void subtilis_parser_mid_str(subtilis_parser_t *p, subtilis_token_t *t,
			     subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_right_str_exp(subtilis_parser_t *p,
					      subtilis_token_t *t,
					      subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_mid_str_exp(subtilis_parser_t *p,
					    subtilis_token_t *t,
					    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_string_str(subtilis_parser_t *p,
					   subtilis_token_t *t,
					   subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_str_str(subtilis_parser_t *p,
					subtilis_token_t *t,
					subtilis_error_t *err);

#endif
