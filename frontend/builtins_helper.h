/*
 * Copyright (c) 2021 Mark Ryan
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

#ifndef __SUBTILIS_BUILTINS_HELPER_H__
#define __SUBTILIS_BUILTINS_HELPER_H__

#include "parser.h"

void subtilis_builtin_memset_i32(subtilis_parser_t *p, size_t base_reg,
				 size_t size_reg, size_t val_reg,
				 subtilis_error_t *err);
void subtilis_builtin_memset_i8(subtilis_parser_t *p, size_t base_reg,
				size_t size_reg, size_t val_reg,
				subtilis_error_t *err);
void subtilis_builtin_memset_i64(subtilis_parser_t *p, size_t base_reg,
				 size_t size_reg, size_t val_reg_low,
				 size_t val_reg_high, subtilis_error_t *err);
void subtilis_builtin_bzero(subtilis_parser_t *p, size_t base_reg, size_t loc,
			    size_t size, subtilis_error_t *err);
void subtilis_builtin_bzero_reg(subtilis_parser_t *p, size_t base_reg,
				size_t loc, size_t size_reg,
				subtilis_error_t *err);

#endif
