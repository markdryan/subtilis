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
	SUBTILIS_TYPE_VECTOR_REAL,
	SUBTILIS_TYPE_VECTOR_INTEGER,
	SUBTILIS_TYPE_VECTOR_BYTE,
	SUBTILIS_TYPE_VECTOR_STRING,
	SUBTILIS_TYPE_VECTOR_FN,
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

/*
 * TODO: This integer needs to be dependent on the int size of the backend
 */

struct subtilis_type_array_t_ {
	int32_t num_dims;
	int32_t dims[SUBTILIS_MAX_DIMENSIONS];
	union {
		subtilis_type_fn_t fn;
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
	} params;
};

struct subtilis_type_section_t_ {
	subtilis_type_t return_type;
	size_t num_parameters;
	subtilis_type_t *parameters;
	size_t ref_count;
	size_t int_regs;
	size_t fp_regs;
};

typedef struct subtilis_type_section_t_ subtilis_type_section_t;

/* Takes ownership of parameters */
subtilis_type_section_t *subtilis_type_section_new(const subtilis_type_t *rtype,
						   size_t num_parameters,
						   subtilis_type_t *parameters,
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
void subtilis_type_copy_to_fn(subtilis_type_fn_t *dst,
			      const subtilis_type_t *src,
			      subtilis_error_t *err);
void subtilis_type_init_to_from_fn(subtilis_type_fn_t *dst,
				   const subtilis_type_t *src,
				   subtilis_error_t *err);

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
