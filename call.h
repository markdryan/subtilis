/*
 * Copyright (c) 2018 Mark Ryan
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

#ifndef __SUBTILIS_PARSER_CALL_H
#define __SUBTILIS_PARSER_CALL_H

#include "ir.h"

struct subtilis_parser_call_t_ {
	subtilis_ir_section_t *s;
	size_t index;
	bool in_error_handler;
	char *name;
	subtilis_type_section_t *call_type;
	subtilis_builtin_type_t ftype;
	size_t line;
};

typedef struct subtilis_parser_call_t_ subtilis_parser_call_t;

subtilis_parser_call_t *
subtilis_parser_call_new(subtilis_ir_section_t *s, size_t index,
			 bool in_error_handler, char *name,
			 subtilis_type_section_t *ct, size_t line,
			 subtilis_builtin_type_t ft, subtilis_error_t *err);
void subtilis_parser_call_delete(subtilis_parser_call_t *call);

#endif
