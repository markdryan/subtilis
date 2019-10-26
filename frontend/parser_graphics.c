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

#include "parser_graphics.h"
#include "parser_exp.h"

subtilis_exp_t *subtilis_parser_get_point(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err)
{
	return subtilis_parser_bracketed_2_int_args(
	    p, t, SUBTILIS_OP_INSTR_POINT, err);
}

subtilis_exp_t *subtilis_parser_get_tint(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	return subtilis_parser_bracketed_2_int_args(
	    p, t, SUBTILIS_OP_INSTR_TINT, err);
}
