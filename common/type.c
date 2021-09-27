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
#include <string.h>

#include "type.h"

static void prv_fn_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err);

const subtilis_type_t subtilis_type_const_real = {SUBTILIS_TYPE_CONST_REAL};

/* clang-format off */
const subtilis_type_t subtilis_type_const_integer = {
	SUBTILIS_TYPE_CONST_INTEGER
};

/* clang-format on */

const subtilis_type_t subtilis_type_const_string = {SUBTILIS_TYPE_CONST_STRING};
const subtilis_type_t subtilis_type_real = {SUBTILIS_TYPE_REAL};
const subtilis_type_t subtilis_type_integer = {SUBTILIS_TYPE_INTEGER};
const subtilis_type_t subtilis_type_byte = {SUBTILIS_TYPE_BYTE};
const subtilis_type_t subtilis_type_string = {SUBTILIS_TYPE_STRING};
const subtilis_type_t subtilis_type_void = {SUBTILIS_TYPE_VOID};
const subtilis_type_t subtilis_type_local_buffer = {SUBTILIS_TYPE_LOCAL_BUFFER};
const subtilis_type_t subtilis_type_typedef = {SUBTILIS_TYPE_TYPEDEF};

/* clang-format off */
static const char *const prv_fixed_type_names[] = {
	"const real",    /* SUBTILIS_TYPE_CONST_REAL */
	"const integer", /* SUBTILIS_TYPE_CONST_INTEGER */
	"const byte",    /* SUBTILIS_TYPE_CONST_BYTE */
	"const string",    /* SUBTILIS_TYPE_CONST_STRING */
	"real",    /* SUBTILIS_TYPE_REAL */
	"integer", /* SUBTILIS_TYPE_INTEGER */
	"byte",    /* SUBTILIS_TYPE_BYTE */
	"string",  /* SUBTILIS_TYPE_STRING */
	"void",    /* SUBTILIS_TYPE_VOID */
	"function pointer",  /* SUBTILIS_TYPE_FN */
	"array of reals", /* SUBTILIS_TYPE_ARRAY_REAL */
	"array of ints", /* SUBTILIS_TYPE_ARRAY_INTEGER */
	"array of bytes", /* SUBTILIS_TYPE_ARRAY_BYTE */
	"array of strings", /* SUBTILIS_TYPE_ARRAY_STRING */
	"array of functions", /* SUBTILIS_TYPE_ARRAY_FN */
	"vector of reals", /* SUBTILIS_TYPE_VECTOR_REAL */
	"vector of ints", /* SUBTILIS_TYPE_VECTOR_INTEGER */
	"vector of bytes", /* SUBTILIS_TYPE_VECTOR_BYTE */
	"vector of strings", /* SUBTILIS_TYPE_VECTOR_STRING */
	"vector of functions", /* SUBTILIS_TYPE_VECTOR_STRING */
	"local buffer",  /* SUBTILIS_TYPE_LOCAL_BUFFER */
	"type",  /* SUBTILIS_TYPE_TYPEDEF */
};

/* clang-format on */

static bool prv_fn_type_match(const subtilis_type_fn_t *t1,
			      const subtilis_type_fn_t *t2)
{
	size_t i;

	if (!subtilis_type_eq(t1->ret_val, t2->ret_val))
		return false;

	if (t1->num_params != t2->num_params)
		return false;

	for (i = 0; i < t1->num_params; i++)
		if (!subtilis_type_eq(t1->params[i], t2->params[i]))
			return false;

	return true;
}

static void prv_fn_type_free(const subtilis_type_fn_t *typ)
{
	size_t i;

	free(typ->ret_val);
	for (i = 0; i < typ->num_params; i++)
		subtilis_type_free(typ->params[i]);
}

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

	if ((t1->type == SUBTILIS_TYPE_ARRAY_FN) ||
	    (t1->type == SUBTILIS_TYPE_VECTOR_FN))
		return prv_fn_type_match(&t1->params.array.params.fn,
					 &t2->params.array.params.fn);

	return true;
}

bool subtilis_type_eq(const subtilis_type_t *a, const subtilis_type_t *b)
{
	if (a->type != b->type)
		return false;
	switch (a->type) {
	case SUBTILIS_TYPE_ARRAY_REAL:
	case SUBTILIS_TYPE_ARRAY_INTEGER:
	case SUBTILIS_TYPE_ARRAY_STRING:
	case SUBTILIS_TYPE_ARRAY_BYTE:
	case SUBTILIS_TYPE_ARRAY_FN:
		return prv_array_type_match(a, b);
	case SUBTILIS_TYPE_FN:
		return prv_fn_type_match(&a->params.fn, &b->params.fn);
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
	subtilis_type_init_copy(&stype->return_type, rtype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	stype->num_parameters = num_parameters;
	stype->parameters = parameters;
	stype->int_regs = 0;
	stype->fp_regs = 0;

	for (i = 0; i < num_parameters; i++) {
		switch (parameters[i].type) {
		case SUBTILIS_TYPE_INTEGER:
		case SUBTILIS_TYPE_BYTE:
		case SUBTILIS_TYPE_STRING:
		case SUBTILIS_TYPE_ARRAY_REAL:
		case SUBTILIS_TYPE_ARRAY_INTEGER:
		case SUBTILIS_TYPE_ARRAY_STRING:
		case SUBTILIS_TYPE_ARRAY_BYTE:
		case SUBTILIS_TYPE_ARRAY_FN:
		case SUBTILIS_TYPE_VECTOR_REAL:
		case SUBTILIS_TYPE_VECTOR_INTEGER:
		case SUBTILIS_TYPE_VECTOR_STRING:
		case SUBTILIS_TYPE_VECTOR_BYTE:
		case SUBTILIS_TYPE_VECTOR_FN:
			stype->int_regs++;
			break;
		case SUBTILIS_TYPE_REAL:
			stype->fp_regs++;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			goto on_error;
		}
	}

	return stype;

on_error:

	free(stype);
	return NULL;
}

void subtilis_type_section_delete(subtilis_type_section_t *stype)
{
	if (!stype)
		return;

	subtilis_type_free(&stype->return_type);
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

static void prv_full_type_name(const subtilis_type_t *typ,
			       subtilis_buffer_t *buf, subtilis_error_t *err)
{
	if (typ->type == SUBTILIS_TYPE_FN)
		return prv_fn_type_name(typ, buf, err);

	subtilis_buffer_append_string(buf, subtilis_type_name(typ), err);
}

static void prv_fn_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err)
{
	size_t i;
	const char *fnorproc;
	const subtilis_type_fn_t *fn_type = &typ->params.fn;

	fnorproc = fn_type->ret_val ? "FN(" : "PROC(";
	subtilis_buffer_append_string(buf, fnorproc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	for (i = 0; i < fn_type->num_params; i++) {
		prv_full_type_name(fn_type->params[i], buf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (i == fn_type->num_params - 1)
			break;
		subtilis_buffer_append_string(buf, ",", err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
	subtilis_buffer_append_string(buf, ")", err);
}

void subtilis_full_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err)
{
	prv_full_type_name(typ, buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_buffer_zero_terminate(buf, err);
}

void subtilis_type_free(subtilis_type_t *typ)
{
	if (typ->type == SUBTILIS_TYPE_FN)
		prv_fn_type_free(&typ->params.fn);
	else if (typ->type == SUBTILIS_TYPE_ARRAY_FN)
		prv_fn_type_free(&typ->params.array.params.fn);
}

void subtilis_type_copy(subtilis_type_t *dst, const subtilis_type_t *src,
			subtilis_error_t *err)
{
	subtilis_type_free(dst);
	dst->type = SUBTILIS_TYPE_VOID;
	subtilis_type_init_copy(dst, src, err);
}

static void prv_init_copy_fn(subtilis_type_fn_t *dst,
			     const subtilis_type_fn_t *src,
			     subtilis_error_t *err)
{
	size_t i;

	dst->ret_val = calloc(1, sizeof(*dst->ret_val));
	if (!dst->ret_val) {
		subtilis_error_set_oom(err);
		return;
	}
	subtilis_type_init_copy(dst->ret_val, src->ret_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	dst->num_params = src->num_params;
	for (i = 0; i < dst->num_params; i++) {
		dst->params[i] = calloc(1, sizeof(*dst->params[i]));
		if (!dst->params[i]) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		subtilis_type_init_copy(dst->params[i], src->params[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	return;

cleanup:
	prv_fn_type_free(dst);
}

void subtilis_type_init_copy(subtilis_type_t *dst, const subtilis_type_t *src,
			     subtilis_error_t *err)
{
	if (src->type == SUBTILIS_TYPE_FN) {
		dst->type = src->type;
		prv_init_copy_fn(&dst->params.fn, &src->params.fn, err);
	} else if ((src->type == SUBTILIS_TYPE_ARRAY_FN) ||
		   (src->type == SUBTILIS_TYPE_VECTOR_FN)) {
		dst->type = src->type;
		dst->params.array.num_dims = src->params.array.num_dims;
		memcpy(dst->params.array.dims, src->params.array.dims,
		       sizeof(src->params.array.dims));
		prv_init_copy_fn(&dst->params.array.params.fn,
				 &src->params.array.params.fn, err);
	} else {
		*dst = *src;
	}
}

void subtilis_type_init_copy_from_fn(subtilis_type_t *dst,
				     const subtilis_type_fn_t *src,
				     subtilis_error_t *err)
{
	dst->type = SUBTILIS_TYPE_FN;
	prv_init_copy_fn(&dst->params.fn, src, err);
}

void subtilis_type_copy_from_fn(subtilis_type_t *dst,
				const subtilis_type_fn_t *src,
				subtilis_error_t *err)
{
	subtilis_type_free(dst);
	subtilis_type_init_copy_from_fn(dst, src, err);
}

void subtilis_type_copy_to_fn(subtilis_type_fn_t *dst,
			      const subtilis_type_t *src, subtilis_error_t *err)
{
	prv_fn_type_free(&src->params.fn);
	prv_init_copy_fn(dst, &src->params.fn, err);
}

void subtilis_type_init_to_from_fn(subtilis_type_fn_t *dst,
				   const subtilis_type_t *src,
				   subtilis_error_t *err)
{
	prv_init_copy_fn(dst, &src->params.fn, err);
}

subtilis_type_section_t *
subtilis_type_section_dup(subtilis_type_section_t *stype)
{
	stype->ref_count++;
	return stype;
}
