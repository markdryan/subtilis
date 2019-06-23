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

#include "type.h"

/* clang-format off */
static const char *const prv_fixed_type_names[] = {
	"real",    /* SUBTILIS_TYPE_REAL */
	"integer", /* SUBTILIS_TYPE_INTEGER */
	"string",  /* SUBTILIS_TYPE_STRING */
	"void",    /* SUBTILIS_TYPE_VOID */
};

/* clang-format on */

subtilis_type_section_t *subtilis_type_section_new(subtilis_type_t rtype,
						   size_t num_parameters,
						   subtilis_type_t *parameters,
						   subtilis_error_t *err)
{
	size_t i;
	subtilis_type_section_t *stype = malloc(sizeof(*stype));

	if (!stype) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	stype->ref_count = 1;
	stype->return_type = rtype;
	stype->num_parameters = num_parameters;
	stype->parameters = parameters;
	stype->int_regs = 0;
	stype->fp_regs = 0;

	for (i = 0; i < num_parameters; i++) {
		if (parameters[i] == SUBTILIS_TYPE_INTEGER)
			stype->int_regs++;
		else
			stype->fp_regs++;
	}

	return stype;
}

void subtilis_type_section_delete(subtilis_type_section_t *stype)
{
	if (!stype)
		return;

	stype->ref_count--;

	if (stype->ref_count != 0)
		return;

	free(stype->parameters);
	free(stype);
}

const char *subtilis_type_name(subtilis_type_t typ)
{
	size_t index = (size_t)typ;

	if (index < SUBTILIS_TYPE_MAX)
		return prv_fixed_type_names[index];

	return "unknown";
}

subtilis_type_section_t *
subtilis_type_section_dup(subtilis_type_section_t *stype)
{
	stype->ref_count++;
	return stype;
}
