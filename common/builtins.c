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

#include "builtins.h"

/* clang-format off */
const subtilis_builtin_t subtilis_builtin_list[] = {
	{"_idvi", SUBTILIS_BUILTINS_IDIV, { SUBTILIS_TYPE_INTEGER }, 2,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER} }, true },
	{"_memseti32", SUBTILIS_BUILTINS_MEMSETI32, { SUBTILIS_TYPE_VOID }, 3,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER} }, false },
	{"_memcpy", SUBTILIS_BUILTINS_MEMCPY, { SUBTILIS_TYPE_VOID }, 3,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER} }, false },
	{"_memcmp", SUBTILIS_BUILTINS_MEMCMP, { SUBTILIS_TYPE_INTEGER }, 3,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER}, }, false },
	{"_compare", SUBTILIS_BUILTINS_COMPARE, { SUBTILIS_TYPE_INTEGER }, 4,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER} }, false },
	{"_memseti8", SUBTILIS_BUILTINS_MEMSETI8, { SUBTILIS_TYPE_VOID }, 3,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER} }, false },
	{"_alloc", SUBTILIS_BUILTINS_ALLOC, { SUBTILIS_TYPE_INTEGER }, 1,
	 { {SUBTILIS_TYPE_INTEGER} }, true },
	{"_deref", SUBTILIS_BUILTINS_DEREF, { SUBTILIS_TYPE_VOID }, 1,
	 { {SUBTILIS_TYPE_INTEGER} }, false },
	{"_memseti64", SUBTILIS_BUILTINS_MEMSETI64, { SUBTILIS_TYPE_VOID }, 4,
	 { {SUBTILIS_TYPE_INTEGER}, {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER},
	   {SUBTILIS_TYPE_INTEGER} }, false },
};

/* clang-format on */

subtilis_type_section_t *subtilis_builtin_ts(subtilis_builtin_type_t type,
					     subtilis_error_t *err)
{
	const subtilis_builtin_t *f;
	subtilis_type_t *params;
	size_t i;
	subtilis_type_section_t *ts = NULL;

	f = &subtilis_builtin_list[type];
	params = malloc(sizeof(subtilis_type_t) * f->num_parameters);
	if (!params) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	for (i = 0; i < f->num_parameters; i++)
		params[i].type = SUBTILIS_TYPE_VOID;
	for (i = 0; i < f->num_parameters; i++) {
		subtilis_type_copy(&params[i], &f->arg_types[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	ts = subtilis_type_section_new(&f->ret_type, f->num_parameters, params,
				       err);

cleanup:
	for (i = 0; i < f->num_parameters; i++)
		subtilis_type_free(&params[i]);
	free(params);

	return ts;
}
