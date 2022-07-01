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

#ifndef __SUBTILIS_TYPE_H
#define __SUBTILIS_TYPE_H

#include "buffer.h"
#include "error.h"

typedef enum {
	SUBTILIS_TYPE_CONST_REAL,
	SUBTILIS_TYPE_CONST_INTEGER,
	SUBTILIS_TYPE_CONST_BYTE,
	SUBTILIS_TYPE_CONST_STRING,
	SUBTILIS_TYPE_REAL,
	SUBTILIS_TYPE_INTEGER,
	SUBTILIS_TYPE_BYTE,
	SUBTILIS_TYPE_STRING,
	SUBTILIS_TYPE_VOID,
	SUBTILIS_TYPE_FN,
	SUBTILIS_TYPE_ARRAY_REAL,
	SUBTILIS_TYPE_ARRAY_INTEGER,
	SUBTILIS_TYPE_ARRAY_BYTE,
	SUBTILIS_TYPE_ARRAY_STRING,
	SUBTILIS_TYPE_ARRAY_FN,
	SUBTILIS_TYPE_ARRAY_REC,
	SUBTILIS_TYPE_VECTOR_REAL,
	SUBTILIS_TYPE_VECTOR_INTEGER,
	SUBTILIS_TYPE_VECTOR_BYTE,
	SUBTILIS_TYPE_VECTOR_STRING,
	SUBTILIS_TYPE_VECTOR_FN,
	SUBTILIS_TYPE_VECTOR_REC,
	SUBTILIS_TYPE_REC,
	SUBTILIS_TYPE_LOCAL_BUFFER,
	SUBTILIS_TYPE_TYPEDEF,
	SUBTILIS_TYPE_MAX,
} subtilis_type_type_t;

#define SUBTILIS_MAX_DIMENSIONS 10
#define SUBTILIS_DYNAMIC_DIMENSION -1
#define SUBTILIS_MAX_ARGS 32

typedef struct subtilis_type_t_ subtilis_type_t;

struct subtilis_type_fn_t_ {
	subtilis_type_t *ret_val;
	size_t num_params;
	subtilis_type_t *params[SUBTILIS_MAX_ARGS];
};

typedef struct subtilis_type_fn_t_ subtilis_type_fn_t;

struct subtilis_type_field_t_ {
	char *name;
	uint32_t alignment;
	uint32_t size;
	int32_t vec_dim;
};

typedef struct subtilis_type_field_t_ subtilis_type_field_t;

struct subtilis_type_rec_t_ {
	uint32_t alignment;
	char *name;
	subtilis_type_t *field_types;
	subtilis_type_field_t *fields;
	size_t num_fields;
	size_t max_fields;
};

typedef struct subtilis_type_rec_t_ subtilis_type_rec_t;

/*
 * TODO: This integer needs to be dependent on the int size of the backend
 */

struct subtilis_type_array_t_ {
	int32_t num_dims;
	int32_t dims[SUBTILIS_MAX_DIMENSIONS];
	union {
		subtilis_type_fn_t fn;
		subtilis_type_rec_t rec;
	} params;
};

typedef struct subtilis_type_array_t_ subtilis_type_array_t;

/*
 * Partly because of legacy code, subtilis_type_t objects are declared
 * on the stack.  Some subtilis_type_t objects will own heap memory,
 * recursive types, and so it is expected that subtilis_type_free is
 * called on these objects when they go out of scope.  Type objects
 * are owned by the function that created them or the structure of
 * which they are a part, and the convention is to always use
 * subtilis_type_free, even if the type is not self referencing.  The
 * one exception here is the lexer which does not understand complex
 * types.
 *
 * To copy a type one must call subtilis_type_copy.  This assumes
 * that the type has already been initialised.  It will free any
 * existing content before making the copy.  If the type has not
 * been initialised called subtilis_type_init_copy instead.
 */

struct subtilis_type_t_ {
	subtilis_type_type_t type;
	union {
		subtilis_type_array_t array;
		subtilis_type_fn_t fn;
		subtilis_type_rec_t rec;
	} params;
};

struct subtilis_check_args_t_ {
	char *arg_name;
	size_t call_site;
};

typedef struct subtilis_check_args_t_ subtilis_check_args_t;

struct subtilis_type_section_t_ {
	subtilis_type_t type;
	subtilis_check_args_t *check_args;
	size_t ref_count;
	size_t int_regs;
	size_t fp_regs;
};

typedef struct subtilis_type_section_t_ subtilis_type_section_t;

/* Takes ownership of parameters */
subtilis_type_section_t *
subtilis_type_section_new(const subtilis_type_t *rtype, size_t num_parameters,
			  const subtilis_type_t *parameters,
			  subtilis_check_args_t *check_args,
			  subtilis_error_t *err);
void subtilis_type_section_delete(subtilis_type_section_t *stype);
bool subtilis_type_eq(const subtilis_type_t *a, const subtilis_type_t *b);
const char *subtilis_type_name(const subtilis_type_t *typ);
void subtilis_full_type_name(const subtilis_type_t *typ, subtilis_buffer_t *buf,
			     subtilis_error_t *err);
subtilis_type_section_t *
subtilis_type_section_dup(subtilis_type_section_t *stype);
void subtilis_type_copy(subtilis_type_t *dst, const subtilis_type_t *src,
			subtilis_error_t *err);
void subtilis_type_init_copy(subtilis_type_t *dst, const subtilis_type_t *src,
			     subtilis_error_t *err);
void subtilis_type_copy_from_fn(subtilis_type_t *dst,
				const subtilis_type_fn_t *src,
				subtilis_error_t *err);
void subtilis_type_init_copy_from_fn(subtilis_type_t *dst,
				     const subtilis_type_fn_t *src,
				     subtilis_error_t *err);
void subtilis_type_to_from_fn(subtilis_type_fn_t *dst,
			      const subtilis_type_t *src,
			      subtilis_error_t *err);
void subtilis_type_init_to_from_fn(subtilis_type_fn_t *dst,
				   const subtilis_type_t *src,
				   subtilis_error_t *err);
void subtilis_type_to_from_rec(subtilis_type_rec_t *dst,
			       const subtilis_type_t *src,
			       subtilis_error_t *err);
void subtilis_type_init_to_from_rec(subtilis_type_rec_t *dst,
				    const subtilis_type_t *src,
				    subtilis_error_t *err);
void subtilis_type_copy_from_rec(subtilis_type_t *dst,
				 const subtilis_type_rec_t *src,
				 subtilis_error_t *err);
void subtilis_type_init_copy_from_rec(subtilis_type_t *dst,
				      const subtilis_type_rec_t *src,
				      subtilis_error_t *err);
void subtilis_type_init_rec(subtilis_type_t *dst, const char *name,
			    subtilis_error_t *err);
size_t subtilis_type_rec_find_field(const subtilis_type_rec_t *rec,
				    const char *fname);
void subtilis_type_rec_add_field(subtilis_type_t *dst, const char *name,
				 const subtilis_type_t *field,
				 uint32_t alignment, uint32_t size,
				 int32_t vec_dim, subtilis_error_t *err);
bool subtilis_type_rec_is_scalar(const subtilis_type_t *typ);
bool subtilis_type_rec_can_zero_fill(const subtilis_type_t *typ);
size_t subtilis_type_rec_zero_fill_size(const subtilis_type_t *typ);
uint32_t subtilis_type_rec_field_offset_id(const subtilis_type_rec_t *rec,
					   size_t id);
uint32_t subtilis_type_rec_field_offset(subtilis_type_t *typ, const char *name);
size_t subtilis_type_rec_size(const subtilis_type_t *typ);
size_t subtilis_type_rec_align(const subtilis_type_t *typ);
bool subtilis_type_rec_need_deref(const subtilis_type_t *typ);
bool subtilis_type_rec_need_zero_alloc(const subtilis_type_t *typ);
void subtilis_type_free(subtilis_type_t *typ);

extern const subtilis_type_t subtilis_type_const_real;
extern const subtilis_type_t subtilis_type_const_integer;
extern const subtilis_type_t subtilis_type_const_string;
extern const subtilis_type_t subtilis_type_real;
extern const subtilis_type_t subtilis_type_integer;
extern const subtilis_type_t subtilis_type_byte;
extern const subtilis_type_t subtilis_type_string;
extern const subtilis_type_t subtilis_type_void;
extern const subtilis_type_t subtilis_type_local_buffer;
extern const subtilis_type_t subtilis_type_typedef;

#endif
