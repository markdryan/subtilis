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

#define SUBTILIS_FIELD_GRANULARITY 16

static void prv_fn_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err);
static void prv_rec_type_name(const subtilis_type_t *typ,
			      subtilis_buffer_t *buf, subtilis_error_t *err);
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
	"rec",  /* SUBTILIS_TYPE_REC */
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

static void prv_fn_type_free(subtilis_type_fn_t *typ)
{
	size_t i;

	subtilis_type_free(typ->ret_val);
	free(typ->ret_val);
	for (i = 0; i < typ->num_params; i++) {
		subtilis_type_free(typ->params[i]);
		free(typ->params[i]);
	}
}

static bool prv_rec_type_match(const subtilis_type_rec_t *t1,
			       const subtilis_type_rec_t *t2)
{
	size_t i;

	if (strcmp(t1->name, t2->name))
		return false;

	if (t1->num_fields != t2->num_fields)
		return false;

	for (i = 0; i < t1->num_fields; i++) {
		if (strcmp(t1->fields[i].name, t2->fields[i].name))
			return false;
		if (!subtilis_type_eq(&t1->field_types[i], &t2->field_types[i]))
			return false;
	}

	return true;
}

static void prv_rec_type_free(subtilis_type_rec_t *typ)
{
	size_t i;

	free(typ->name);
	for (i = 0; i < typ->num_fields; i++) {
		free(typ->fields[i].name);
		subtilis_type_free(&typ->field_types[i]);
	}
	free(typ->fields);
	free(typ->field_types);
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
	case SUBTILIS_TYPE_REC:
		return prv_rec_type_match(&a->params.rec, &b->params.rec);
	default:
		return true;
	}
}

subtilis_type_section_t *
subtilis_type_section_new(const subtilis_type_t *rtype, size_t num_parameters,
			  const subtilis_type_t *parameters,
			  subtilis_check_args_t *check_args,
			  subtilis_error_t *err)

{
	size_t i;
	subtilis_type_t *ftype;
	subtilis_type_section_t *stype = malloc(sizeof(*stype));

	if (!stype) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	stype->ref_count = 1;

	ftype = &stype->type;
	ftype->type = SUBTILIS_TYPE_FN;
	ftype->params.fn.ret_val = calloc(1, sizeof(*ftype->params.fn.ret_val));
	if (!ftype->params.fn.ret_val) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	ftype->params.fn.ret_val->type = SUBTILIS_TYPE_VOID;
	subtilis_type_init_copy(ftype->params.fn.ret_val, rtype, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	ftype->params.fn.num_params = num_parameters;
	for (i = 0; i < num_parameters; i++) {
		ftype->params.fn.params[i] =
		    calloc(1, sizeof(*ftype->params.fn.params[i]));
		if (!ftype->params.fn.params[i]) {
			subtilis_error_set_oom(err);
			goto on_error;
		}
		ftype->params.fn.params[i]->type = SUBTILIS_TYPE_VOID;

		subtilis_type_init_copy(ftype->params.fn.params[i],
					&parameters[i], err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto on_error;
	}
	stype->check_args = check_args;
	stype->int_regs = 0;
	stype->fp_regs = 0;

	for (i = 0; i < ftype->params.fn.num_params; i++) {
		switch (ftype->params.fn.params[i]->type) {
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
		case SUBTILIS_TYPE_FN:
		case SUBTILIS_TYPE_REC:
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
	size_t i;

	if (!stype)
		return;

	stype->ref_count--;
	if (stype->ref_count != 0)
		return;
	subtilis_type_free(&stype->type);
	if (stype->check_args) {
		for (i = 0; i < stype->type.params.fn.num_params; i++)
			free(stype->check_args[i].arg_name);
		free(stype->check_args);
	}
	free(stype);
}

const char *subtilis_type_name(const subtilis_type_t *typ)
{
	size_t index;

	if (typ->type == SUBTILIS_TYPE_REC)
		return typ->params.rec.name;

	index = (size_t)typ->type;

	if (index < SUBTILIS_TYPE_MAX)
		return prv_fixed_type_names[index];

	return "unknown";
}

static void prv_full_type_name(const subtilis_type_t *typ,
			       subtilis_buffer_t *buf, subtilis_error_t *err)
{
	if (typ->type == SUBTILIS_TYPE_FN) {
		prv_fn_type_name(typ, buf, err);
		return;
	} else if (typ->type == SUBTILIS_TYPE_REC) {
		prv_rec_type_name(typ, buf, err);
		return;
	}

	subtilis_buffer_append_string(buf, subtilis_type_name(typ), err);
}

static void prv_fn_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err)
{
	size_t i;
	const subtilis_type_fn_t *fn_type = &typ->params.fn;

	if (fn_type->ret_val) {
		subtilis_buffer_append_string(buf, "FN", err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		prv_full_type_name(fn_type->ret_val, buf, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_buffer_append_string(buf, "(", err);
	} else {
		subtilis_buffer_append_string(buf, "PROC(", err);
	}
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

static void prv_rec_type_name(const subtilis_type_t *typ,
			      subtilis_buffer_t *buf, subtilis_error_t *err)
{
	size_t i;
	const subtilis_type_rec_t *rec_type = &typ->params.rec;

	subtilis_buffer_append_string(buf, rec_type->name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_buffer_append_string(buf, "(", err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	i = 0;
	if (rec_type->num_fields > 0) {
		for (; i < rec_type->num_fields - 1; i++) {
			subtilis_buffer_append_string(
			    buf, subtilis_type_name(&rec_type->field_types[i]),
			    err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			subtilis_buffer_append_string(buf, ",", err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
	subtilis_buffer_append_string(
	    buf, subtilis_type_name(&rec_type->field_types[i]), err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
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
	else if (typ->type == SUBTILIS_TYPE_REC)
		prv_rec_type_free(&typ->params.rec);
	else if ((typ->type == SUBTILIS_TYPE_ARRAY_FN) ||
		 (typ->type == SUBTILIS_TYPE_VECTOR_FN))
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
	dst->num_params = 0;
	subtilis_type_init_copy(dst->ret_val, src->ret_val, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	for (i = 0; i < src->num_params; i++) {
		dst->params[i] = calloc(1, sizeof(*dst->params[i]));
		if (!dst->params[i]) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		subtilis_type_init_copy(dst->params[i], src->params[i], err);
		dst->num_params++;
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}
	return;

cleanup:
	prv_fn_type_free(dst);
}

static void prv_init_copy_rec(subtilis_type_rec_t *dst,
			      const subtilis_type_rec_t *src,
			      subtilis_error_t *err)
{
	size_t i;

	dst->alignment = src->alignment;
	dst->num_fields = 0;
	dst->fields = NULL;
	dst->field_types = NULL;

	dst->name = malloc(strlen(src->name) + 1);
	if (!dst->name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(dst->name, src->name);
	dst->max_fields = src->max_fields;
	dst->field_types = malloc(src->max_fields * sizeof(*dst->field_types));
	if (!dst->field_types) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	dst->fields = malloc(src->max_fields * sizeof(*dst->fields));
	if (!dst->fields) {
		subtilis_error_set_oom(err);
		goto cleanup;
	}
	for (i = 0; i < src->num_fields; i++) {
		dst->fields[dst->num_fields].name =
		    malloc(strlen(src->fields[dst->num_fields].name) + 1);
		if (!dst->fields[dst->num_fields].name) {
			subtilis_error_set_oom(err);
			goto cleanup;
		}
		strcpy(dst->fields[dst->num_fields].name,
		       src->fields[dst->num_fields].name);
		dst->fields[dst->num_fields].size =
		    src->fields[dst->num_fields].size;
		dst->fields[dst->num_fields].alignment =
		    src->fields[dst->num_fields].alignment;
		dst->fields[dst->num_fields].vec_dim =
		    src->fields[dst->num_fields].vec_dim;
		subtilis_type_init_copy(&dst->field_types[dst->num_fields],
					&src->field_types[dst->num_fields],
					err);
		if (err->type != SUBTILIS_ERROR_OK) {
			free(dst->fields[dst->num_fields].name);
			goto cleanup;
		}
		dst->num_fields++;
	}

	return;

cleanup:
	prv_rec_type_free(dst);
}

void subtilis_type_init_copy(subtilis_type_t *dst, const subtilis_type_t *src,
			     subtilis_error_t *err)
{
	if (src->type == SUBTILIS_TYPE_FN) {
		dst->type = src->type;
		prv_init_copy_fn(&dst->params.fn, &src->params.fn, err);
	} else if (src->type == SUBTILIS_TYPE_REC) {
		dst->type = src->type;
		prv_init_copy_rec(&dst->params.rec, &src->params.rec, err);
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

void subtilis_type_to_from_fn(subtilis_type_fn_t *dst,
			      const subtilis_type_t *src, subtilis_error_t *err)
{
	prv_fn_type_free(dst);
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

void subtilis_type_init_rec(subtilis_type_t *dst, const char *name,
			    subtilis_error_t *err)
{
	dst->type = SUBTILIS_TYPE_REC;
	dst->params.rec.alignment = 1;
	dst->params.rec.num_fields = 0;
	dst->params.rec.max_fields = 0;
	dst->params.rec.field_types = NULL;
	dst->params.rec.fields = NULL;
	dst->params.rec.name = malloc(strlen(name) + 1);
	if (!dst->params.rec.name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(dst->params.rec.name, name);
}

size_t subtilis_type_rec_find_field(const subtilis_type_rec_t *rec,
				    const char *fname)
{
	size_t i;

	for (i = 0; i < rec->num_fields; i++) {
		if (!strcmp(rec->fields[i].name, fname))
			return i;
	}

	return SIZE_MAX;
}

void subtilis_type_rec_add_field(subtilis_type_t *dst, const char *name,
				 const subtilis_type_t *field,
				 uint32_t alignment, uint32_t size,
				 int32_t vec_dim, subtilis_error_t *err)
{
	size_t new_max;
	subtilis_type_t *new_field_types;
	subtilis_type_field_t *new_fields;
	subtilis_type_rec_t *rec = &dst->params.rec;

	/*
	 * Alignment must be a power of 2.
	 */

	if ((alignment == 0) || (alignment & (alignment - 1))) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (subtilis_type_rec_find_field(rec, name) != SIZE_MAX) {
		subtilis_error_set_type_already_defined(err);
		return;
	}

	if (rec->max_fields == rec->num_fields) {
		new_max = rec->max_fields + SUBTILIS_FIELD_GRANULARITY;
		new_field_types = realloc(rec->field_types,
					  new_max * sizeof(*new_field_types));
		if (!new_field_types) {
			subtilis_error_set_oom(err);
			return;
		}
		new_fields =
		    realloc(rec->fields, new_max * sizeof(*new_fields));
		if (!new_fields) {
			free(new_fields);
			subtilis_error_set_oom(err);
			return;
		}
		rec->field_types = new_field_types;
		rec->fields = new_fields;
		rec->max_fields = new_max;
	}
	rec->fields[rec->num_fields].name = malloc(strlen(name) + 1);
	if (!rec->fields[rec->num_fields].name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(rec->fields[rec->num_fields].name, name);
	rec->fields[rec->num_fields].alignment = alignment;
	rec->fields[rec->num_fields].size = size;
	rec->fields[rec->num_fields].vec_dim = vec_dim;
	if (rec->alignment < alignment)
		rec->alignment = alignment;

	subtilis_type_init_copy(&rec->field_types[rec->num_fields], field, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		free(rec->fields[rec->num_fields].name);
		return;
	}
	rec->num_fields++;
}

uint32_t subtilis_type_rec_field_offset_id(const subtilis_type_rec_t *rec,
					   size_t id)
{
	size_t i;
	uint32_t align;
	uint32_t adjust;
	uint32_t offset = 0;

	for (i = 0; i < id; i++) {
		align = rec->fields[i].alignment;
		adjust = offset & (align - 1);
		if (adjust != 0)
			offset += align - adjust;
		offset += rec->fields[i].size;
	}
	align = rec->fields[i].alignment;
	adjust = offset & (align - 1);
	if (adjust != 0)
		offset += align - adjust;

	return offset;
}

uint32_t subtilis_type_rec_field_offset(subtilis_type_t *typ, const char *name)
{
	subtilis_type_rec_t *rec = &typ->params.rec;
	size_t id = subtilis_type_rec_find_field(rec, name);

	if (id == SIZE_MAX)
		return UINT32_MAX;

	return subtilis_type_rec_field_offset_id(rec, id);
}

size_t subtilis_type_rec_size(const subtilis_type_t *typ)
{
	size_t id;
	const subtilis_type_rec_t *rec = &typ->params.rec;

	if (rec->num_fields == 0)
		return 0;

	id = rec->num_fields - 1;
	return subtilis_type_rec_field_offset_id(rec, id) +
	       rec->fields[id].size;
}

size_t subtilis_type_rec_align(const subtilis_type_t *typ)
{
	const subtilis_type_rec_t *rec = &typ->params.rec;

	return rec->alignment;
}

bool subtilis_type_rec_is_scalar(const subtilis_type_t *typ)
{
	subtilis_type_t *field;
	size_t i;
	const subtilis_type_rec_t *rec = &typ->params.rec;

	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		switch (field->type) {
		case SUBTILIS_TYPE_REAL:
		case SUBTILIS_TYPE_INTEGER:
		case SUBTILIS_TYPE_BYTE:
		case SUBTILIS_TYPE_FN:
		case SUBTILIS_TYPE_LOCAL_BUFFER:
			continue;
		case SUBTILIS_TYPE_REC:
			if (subtilis_type_rec_is_scalar(field))
				continue;
			break;
		default:
			break;
		}
		return false;
	}

	return true;
}

size_t subtilis_type_rec_zero_fill_size(const subtilis_type_t *typ)
{
	subtilis_type_t *field;
	size_t i;
	size_t size = 0;
	const subtilis_type_rec_t *rec = &typ->params.rec;

	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		switch (field->type) {
		case SUBTILIS_TYPE_REAL:
		case SUBTILIS_TYPE_INTEGER:
		case SUBTILIS_TYPE_BYTE:
		case SUBTILIS_TYPE_STRING:
		case SUBTILIS_TYPE_LOCAL_BUFFER:
			size += rec->fields[i].size;
			break;
		case SUBTILIS_TYPE_REC:
			size += subtilis_type_rec_zero_fill_size(field);
			break;
		default:
			break;
		}
	}

	return size;
}

bool subtilis_type_rec_need_deref(const subtilis_type_t *typ)
{
	subtilis_type_t *field;
	size_t i;
	const subtilis_type_rec_t *rec = &typ->params.rec;

	for (i = 0; i < rec->num_fields; i++) {
		field = &rec->field_types[i];
		switch (field->type) {
		case SUBTILIS_TYPE_REAL:
		case SUBTILIS_TYPE_INTEGER:
		case SUBTILIS_TYPE_BYTE:
		case SUBTILIS_TYPE_LOCAL_BUFFER:
		case SUBTILIS_TYPE_FN:
			continue;
		case SUBTILIS_TYPE_REC:
			if (subtilis_type_rec_need_deref(field))
				return true;
			break;
		default:
			return true;
		}
	}
	return false;
}
