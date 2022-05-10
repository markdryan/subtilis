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

#ifndef SUBTILIS_REC_TYPE_H
#define SUBTILIS_REC_TYPE_H

#include "parser.h"

void subtilis_rec_type_zero(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t mem_reg, size_t loc, bool push,
			    subtilis_error_t *err);
void subtilis_rec_type_zero_body(subtilis_parser_t *p,
				 const subtilis_type_t *type, size_t mem_reg,
				 size_t loc, size_t zero_fill_size,
				 subtilis_error_t *err);
void subtilis_type_rec_copy_ref(subtilis_parser_t *p,
				const subtilis_type_t *type, size_t dest_reg,
				size_t loc, size_t src_reg,
				subtilis_error_t *err);
void subtilis_rec_type_copy(subtilis_parser_t *p, const subtilis_type_t *type,
			    size_t dest_reg, size_t loc, size_t src_reg,
			    subtilis_error_t *err);

#endif
