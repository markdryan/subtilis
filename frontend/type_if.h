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

#ifndef __SUBTILIS_TYPE_IF_H
#define __SUBTILIS_TYPE_IF_H

#include "expression.h"
#include "parser.h"

typedef subtilis_exp_t *(*subtilis_type_if_none_t)(subtilis_parser_t *p,
						   subtilis_error_t *err);
typedef void (*subtilis_type_if_reg_t)(subtilis_parser_t *p, size_t reg,
				       subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_unary_t)(subtilis_parser_t *p,
						    subtilis_exp_t *e,
						    subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_binary_t)(subtilis_parser_t *p,
						     subtilis_exp_t *a1,
						     subtilis_exp_t *a2,
						     subtilis_error_t *err);
typedef subtilis_exp_t *(*subtilis_type_if_binary_nc_t)(subtilis_parser_t *p,
							subtilis_exp_t *a1,
							subtilis_exp_t *a2,
							bool swapped,
							subtilis_error_t *err);
typedef void (*subtilis_type_if_dup_t)(subtilis_exp_t *e1, subtilis_exp_t *e2,
				       subtilis_error_t *err);

struct subtilis_type_if_ {
	bool is_const;
	subtilis_type_if_none_t zero;
	subtilis_type_if_reg_t zero_reg;
	subtilis_type_if_unary_t exp_to_var;
	subtilis_type_if_unary_t copy_var;
	subtilis_type_if_dup_t dup;
	subtilis_type_if_unary_t to_int32;
	subtilis_type_if_unary_t to_float64;
	subtilis_type_if_unary_t unary_minus;
	subtilis_type_if_binary_t add;
	subtilis_type_if_binary_t mul;
	subtilis_type_if_binary_t and;
	subtilis_type_if_binary_t or ;
	subtilis_type_if_unary_t not;
	subtilis_type_if_binary_t eor;
	subtilis_type_if_binary_t eq;
	subtilis_type_if_binary_t neq;
	subtilis_type_if_binary_nc_t sub;
	subtilis_type_if_binary_t div;
	subtilis_type_if_binary_t mod;
	subtilis_type_if_binary_nc_t divr;
	subtilis_type_if_binary_nc_t gt;
	subtilis_type_if_binary_nc_t lte;
	subtilis_type_if_binary_nc_t lt;
	subtilis_type_if_binary_nc_t gte;
	subtilis_type_if_binary_t lsl;
	subtilis_type_if_binary_t lsr;
	subtilis_type_if_binary_t asr;
};

typedef struct subtilis_type_if_ subtilis_type_if;

subtilis_exp_t *subtilis_type_if_zero(subtilis_parser_t *p,
				      subtilis_type_t type,
				      subtilis_error_t *err);
void subtilis_type_if_zero_reg(subtilis_parser_t *p, subtilis_type_t type,
			       size_t reg, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_exp_to_var(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_copy_var(subtilis_parser_t *p,
					  subtilis_exp_t *e,
					  subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_dup(subtilis_exp_t *e, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_to_int(subtilis_parser_t *p, subtilis_exp_t *e,
					subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_to_float64(subtilis_parser_t *p,
					    subtilis_exp_t *e,
					    subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_unary_minus(subtilis_parser_t *p,
					     subtilis_exp_t *e,
					     subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_add(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_mul(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_and(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_or(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_eor(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_not(subtilis_parser_t *p, subtilis_exp_t *e,
				     subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_eq(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_neq(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_sub(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_div(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_mod(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_divr(subtilis_parser_t *p, subtilis_exp_t *a1,
				      subtilis_exp_t *a2,
				      subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_gt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_lte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_lt(subtilis_parser_t *p, subtilis_exp_t *a1,
				    subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_gte(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_lsl(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_lsr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);
subtilis_exp_t *subtilis_type_if_asr(subtilis_parser_t *p, subtilis_exp_t *a1,
				     subtilis_exp_t *a2, subtilis_error_t *err);

#endif
