/*
 * Copyright (c) 2017 Mark Ryan
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

#ifndef __SUBTILIS_LEXER_H
#define __SUBTILIS_LEXER_H

#include <stdint.h>

#include "../common/buffer.h"
#include "../common/keywords.h"
#include "../common/stream.h"
#include "../common/type.h"

/* Does not apply to strings which are limitless in size */

#define SUBTILIS_MAX_TOKEN_SIZE 255
#define SUBTILIS_LEXER_MAX_BLOCKS 32

typedef enum {
	SUBTILIS_TOKEN_EOF,
	SUBTILIS_TOKEN_KEYWORD,
	SUBTILIS_TOKEN_OPERATOR,
	SUBTILIS_TOKEN_STRING,
	SUBTILIS_TOKEN_INTEGER,
	SUBTILIS_TOKEN_REAL,
	SUBTILIS_TOKEN_IDENTIFIER,
	SUBTILIS_TOKEN_UNKNOWN,
} subtilis_token_type_t;

struct _subtilis_token_keyword_t {
	int type;
	bool supported;

	/*
	 * As the lexer does not understand or return complex types
	 * we're just going to treat this as a scalar type, e.g.,
	 * no subtilis_type_copy or subtilis_type_free needed here.
	 */

	subtilis_type_t id_type;
};

typedef struct _subtilis_token_t subtilis_token_t;

typedef struct _subtilis_token_keyword_t subtilis_token_keyword_t;

struct _subtilis_token_t {
	subtilis_token_type_t type;
	union {
		subtilis_token_keyword_t keyword;
		int32_t integer;
		double real;
		subtilis_type_t id_type;
	} tok;
	subtilis_buffer_t buf;
};

struct subtilis_lexer_block_t_ {
	size_t index;
	subtilis_token_t *t;
	unsigned int line;
};

typedef struct subtilis_lexer_block_t_ subtilis_lexer_block_t;

struct _subtilis_lexer_t {
	const subtilis_keyword_t *keywords;
	size_t num_keywords;
	const subtilis_keyword_t *basic_keywords;
	size_t num_basic_keywords;
	const subtilis_keyword_t *ass_keywords;
	size_t num_ass_keywords;
	subtilis_stream_t *stream;
	unsigned int line;
	unsigned int character;
	size_t index;
	size_t num_blocks;
	subtilis_lexer_block_t blocks[SUBTILIS_LEXER_MAX_BLOCKS];
	subtilis_token_t *next;
	size_t buf_size;
	size_t buf_end;
	char *buffer;
};

typedef struct _subtilis_lexer_t subtilis_lexer_t;

/* Takes ownership of s*/
subtilis_lexer_t *subtilis_lexer_new(subtilis_stream_t *s, size_t buf_size,
				     const subtilis_keyword_t *basic_keywords,
				     size_t num_basic_keywords,
				     const subtilis_keyword_t *ass_keywords,
				     size_t num_ass_keywords,
				     subtilis_error_t *err);
void subtilis_lexer_delete(subtilis_lexer_t *l, subtilis_error_t *err);
void subtilis_lexer_set_ass_keywords(subtilis_lexer_t *l, bool ass,
				     subtilis_error_t *err);
void subtilis_lexer_get(subtilis_lexer_t *l, subtilis_token_t *t,
			subtilis_error_t *err);
subtilis_token_t *subtilis_token_new(subtilis_error_t *err);
void subtilis_token_claim(subtilis_token_t *dest, subtilis_token_t *src);
void subtilis_lexer_push_block(subtilis_lexer_t *l, const subtilis_token_t *t,
			       subtilis_error_t *err);
void subtilis_lexer_set_block_start(subtilis_lexer_t *l, subtilis_error_t *err);
void subtilis_lexer_pop_block(subtilis_lexer_t *l, subtilis_error_t *err);
const char *subtilis_token_get_text(subtilis_token_t *t);
const char *subtilis_token_get_text_with_err(subtilis_token_t *t,
					     subtilis_error_t *err);
subtilis_token_t *subtilis_token_dup(const subtilis_token_t *src,
				     subtilis_error_t *err);
void subtilis_token_delete(subtilis_token_t *t);
void subtilis_dump_token(subtilis_token_t *t);

#endif
