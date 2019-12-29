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

#ifndef __SUBTILIS_PARSER_MATH_H
#define __SUBTILIS_PARSER_MATH_H

#include "expression.h"
#include "parser.h"

subtilis_exp_t *subtilis_parser_rad(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_pi(subtilis_parser_t *p, subtilis_token_t *t,
				   subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_int(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_sin(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_cos(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_tan(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_asn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_acs(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_atn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_log(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_op_instr_type_t itype,
				    double (*log_fn)(double n),
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_sqr(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_abs(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_sgn(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err);

#endif
