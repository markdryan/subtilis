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

#ifndef __SUBTILIS_PARSER_GRAPHICS_H
#define __SUBTILIS_PARSER_GRAPHICS_H

#include "expression.h"
#include "parser.h"

subtilis_exp_t *subtilis_parser_get_point(subtilis_parser_t *p,
					  subtilis_token_t *t,
					  subtilis_error_t *err);
subtilis_exp_t *subtilis_parser_get_tint(subtilis_parser_t *p,
					 subtilis_token_t *t,
					 subtilis_error_t *err);
void subtilis_parser_mode(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_plot(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_wait(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_move(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_fill(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_line(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_circle(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err);
void subtilis_parser_rectangle(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err);
void subtilis_parser_draw(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_point(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err);
void subtilis_parser_point(subtilis_parser_t *p, subtilis_token_t *t,
			   subtilis_error_t *err);
void subtilis_parser_gcol(subtilis_parser_t *p, subtilis_token_t *t,
			  subtilis_error_t *err);
void subtilis_parser_colour(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err);
void subtilis_parser_origin(subtilis_parser_t *p, subtilis_token_t *t,
			    subtilis_error_t *err);
void subtilis_parser_cls(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err);
void subtilis_parser_clg(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err);
void subtilis_parser_on(subtilis_parser_t *p, subtilis_token_t *t,
			subtilis_error_t *err);
void subtilis_parser_off(subtilis_parser_t *p, subtilis_token_t *t,
			 subtilis_error_t *err);

#endif
