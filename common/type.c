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

const subtilis_type_t subtilis_type_const_real = {SUBTILIS_TYPE_CONST_REAL};

/* clang-format off */
const subtilis_type_t subtilis_type_const_integer = {
	SUBTILIS_TYPE_CONST_INTEGER
};

/* clang-format on */

const subtilis_type_t subtilis_type_const_string = {SUBTILIS_TYPE_CONST_STRING};
const subtilis_type_t subtilis_type_real = {SUBTILIS_TYPE_REAL};
const subtilis_type_t subtilis_type_integer = {SUBTILIS_TYPE_INTEGER};
const subtilis_type_t subtilis_type_string = {SUBTILIS_TYPE_STRING};
const subtilis_type_t subtilis_type_void = {SUBTILIS_TYPE_VOID};
const subtilis_type_t subtilis_type_local_buffer = {SUBTILIS_TYPE_LOCAL_BUFFER};

/* clang-format off */
static const char *const prv_fixed_type_names[] = {
	"const real",    /* SUBTILIS_TYPE_CONST_REAL */
	"const integer", /* SUBTILIS_TYPE_CONST_INTEGER */
	"const string",    /* SUBTILIS_TYPE_CONST_STRING */
	"real",    /* SUBTILIS_TYPE_REAL */
	"integer", /* SUBTILIS_TYPE_INTEGER */
	"string",  /* SUBTILIS_TYPE_STRING */
	"void",    /* SUBTILIS_TYPE_VOID */
	"array of reals", /* SUBTILIS_TYPE_ARRAY_REAL */
	"array of ints", /* SUBTILIS_TYPE_ARRAY_INTEGER */
	"array of strings", /* SUBTILIS_TYPE_ARRAY_STRING */
};

/* clang-format on */

static bool prv_array_type_match(const subtilis_type_t *t1,
				 const subtilis_type_t *t2)
{
	size_t i;

	if (t1->params.array.num_dims != t2->params.array.num_dims)
		return false;

	for (i = 0; i < t1->params.array.num_dims; i++) {
		if (t1->params.array.dims[i] == -1)
			continue;
		if (t1->params.array.dims[i] != t2->params.array.dims[i])
			return false;
	}

	return true;
}

bool subtilis_type_eq(const subtilis_type_t *a, const subtilis_type_t *b)
{
	if (a->type != b->type)
		return false;
	switch (a->type) {
	case SUBTILIS_TYPE_ARRAY_REAL:
	case SUBTILIS_TYPE_ARRAY_INTEGER:
		return prv_array_type_match(a, b);
	case SUBTILIS_TYPE_STRING:
		return b->type == b->type;
	default:
		return true;
	}
}

subtilis_type_section_t *subtilis_type_section_new(const subtilis_type_t *rtype,
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
	stype->return_type = *rtype;
	stype->num_parameters = num_parameters;
	stype->parameters = parameters;
	stype->int_regs = 0;
	stype->fp_regs = 0;

	for (i = 0; i < num_parameters; i++) {
		switch (parameters[i].type) {
		case SUBTILIS_TYPE_INTEGER:
		case SUBTILIS_TYPE_STRING:
		case SUBTILIS_TYPE_ARRAY_REAL:
		case SUBTILIS_TYPE_ARRAY_INTEGER:
			stype->int_regs++;
			break;
		case SUBTILIS_TYPE_REAL:
			stype->fp_regs++;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			break;
		}
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

const char *subtilis_type_name(const subtilis_type_t *typ)
{
	size_t index = (size_t)typ->type;

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
