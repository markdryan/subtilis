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

#include <stdlib.h>

#include "call.h"

subtilis_parser_call_t *
subtilis_parser_call_new(subtilis_ir_section_t *s, size_t index,
			 bool in_error_handler, char *name,
			 subtilis_type_section_t *ct, size_t line,
			 subtilis_builtin_type_t ft, subtilis_error_t *err)
{
	subtilis_parser_call_t *call;

	call = malloc(sizeof(*call));
	if (!call) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	call->s = s;
	call->index = index;
	call->in_error_handler = in_error_handler;
	call->name = name;
	call->call_type = ct;
	call->line = line;
	call->ftype = ft;

	return call;

on_error:

	return NULL;
}

void subtilis_parser_call_delete(subtilis_parser_call_t *call)
{
	if (!call)
		return;

	subtilis_type_section_delete(call->call_type);
	free(call->name);
	free(call);
}

subtilis_parser_call_addr_t *
subtilis_parser_call_addr_new(subtilis_ir_section_t *s, size_t index, bool eh,
			      char *name, const subtilis_type_t *type,
			      size_t line, subtilis_error_t *err)
{
	subtilis_parser_call_addr_t *call_addr;

	call_addr = malloc(sizeof(*call_addr));
	if (!call_addr) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	call_addr->call_type.type = SUBTILIS_TYPE_VOID;

	call_addr->s = s;
	call_addr->index = index;
	call_addr->in_error_handler = eh;
	subtilis_type_init_copy(&call_addr->call_type, type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	call_addr->name = name;
	call_addr->line = line;

	return call_addr;

on_error:
	free(name);
	subtilis_parser_call_addr_delete(call_addr);

	return NULL;
}

void subtilis_parser_call_addr_delete(subtilis_parser_call_addr_t *call_addr)
{
	if (!call_addr)
		return;

	subtilis_type_free(&call_addr->call_type);
	free(call_addr->name);
	free(call_addr);
}
