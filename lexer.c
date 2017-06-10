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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

/* TODO token too long
 *
-*  We probably do want to set a limit which we can make configurable
 *  even if we use buffers.
-* or maybe not.  If we do set a limit we will need to skip the rest of the
 * token.
 *
 * - I'm now think the code will be simpler if we don't set any limit
 */

typedef enum {
	SUBTILIS_TOKEN_END_TOKEN,
	SUBTILIS_TOKEN_END_NOT_TOKEN,
	SUBTILIS_TOKEN_END_RETURN,
} subtilis_token_end_t;

typedef subtilis_token_end_t (*token_end_t)(subtilis_lexer_t *,
					    subtilis_token_t *, char ch,
					    subtilis_error_t *);

subtilis_lexer_t *subtilis_lexer_new(subtilis_stream_t *s, size_t buf_size,
				     subtilis_error_t *err)
{
	subtilis_lexer_t *l;

	l = malloc(sizeof(subtilis_lexer_t) + (buf_size - 1));
	if (!l) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	l->buf_size = buf_size;
	l->buf_end = 0;
	l->stream = s;
	l->line = 1;
	l->character = 1;
	l->index = 0;

	return l;
}

void subtilis_lexer_delete(subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (l) {
		l->stream->close(l->stream->handle, err);
		free(l);
	}
}

static void prv_set_first(subtilis_lexer_t *l, subtilis_token_t *t, char ch,
			  subtilis_token_type_t type)
{
	t->type = type;
	t->buf[0] = ch;
	t->buf[1] = 0;
	l->index++;
	t->token_size = 1;
}

static void prv_set_next(subtilis_lexer_t *l, subtilis_token_t *t, char ch,
			 int index)
{
	l->index++;
	t->buf[index] = ch;
	t->token_size++;
	t->buf[index + 1] = 0;
}

// TODO: We can make these two return the errors which should remove a
// line of code for the callers.
static void prv_ensure_buffer(subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (l->index < l->buf_end)
		return;

	l->buf_end =
	    l->stream->read(l->buffer, l->buf_size, l->stream->handle, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// TODO:  This is wrong.  We should set this to 0 before we return

	l->index = 0;
}

static void prv_ensure_token_buffer(subtilis_lexer_t *l, subtilis_token_t *t,
				    subtilis_error_t *err,
				    subtilis_error_type_t type)
{
	if (t->token_size < SUBTILIS_MAX_TOKEN_SIZE)
		return;
	t->buf[SUBTILIS_MAX_TOKEN_SIZE] = 0;
	subtilis_error_set1(err, type, t->buf, l->stream->name, l->line);
}

static bool prv_is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// TODO: This needs to be a bit mask or something
static bool prv_is_simple_operator(char c)
{
	return c == '/' || c == '*' || c == '(' || c == ')' || c == ':' ||
	       c == '=' || c == ',' || c == ';' || c == '^' || c == '[' ||
	       c == ']' || c == '|' || c == '~' || c == '?' || c == '$';
}

static bool prv_is_separator(char c)
{
	return prv_is_whitespace(c) || prv_is_simple_operator(c);
}

static void prv_process_complex_operator(subtilis_lexer_t *l, char ch,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	char ch1;

	prv_set_first(l, t, ch, SUBTILIS_TOKEN_OPERATOR);
	prv_ensure_buffer(l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	if (l->index == l->buf_end)
		return;

	ch1 = l->buffer[l->index];
	switch (ch1) {
	case '=':
		prv_set_next(l, t, ch1, 1);
		break;
	case '<':
		if (ch == '<')
			prv_set_next(l, t, ch1, 1);
		break;
	case '>':
		if (ch == '<') {
			prv_set_next(l, t, ch1, 1);
			break;
		}

		if (ch != '>')
			return;
		prv_set_next(l, t, ch1, 1);

		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			return;

		ch1 = l->buffer[l->index];
		if (ch1 != '>')
			return;

		prv_set_next(l, t, ch1, 2);
		break;
	default:
		break;
	}
}

static bool prv_extract_number(subtilis_lexer_t *l, subtilis_token_t *t,
			       token_end_t end_fn, subtilis_error_t *err)
{
	char ch;
	subtilis_token_end_t end;

	while (t->token_size < SUBTILIS_MAX_TOKEN_SIZE) {
		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
		if (l->index == l->buf_end)
			return true;
		ch = l->buffer[l->index];
		end = end_fn(l, t, ch, err);
		if (end == SUBTILIS_TOKEN_END_RETURN)
			return false;
		else if (end == SUBTILIS_TOKEN_END_NOT_TOKEN)
			return true;
		t->buf[t->token_size++] = ch;
		l->index++;
	}

	if (t->token_size == 255) {
		t->buf[t->token_size] = 0;
		subtilis_error_set_number_too_long(err, t->buf, l->stream->name,
						   l->line);
	}

	return true;
}

static subtilis_token_end_t prv_float_end(subtilis_lexer_t *l,
					  subtilis_token_t *t, char ch,
					  subtilis_error_t *err)
{
	if (!(ch >= '0' && ch <= '9'))
		return SUBTILIS_TOKEN_END_NOT_TOKEN;

	return SUBTILIS_TOKEN_END_TOKEN;
}

static void prv_process_float(subtilis_lexer_t *l, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	t->type = SUBTILIS_TOKEN_REAL;
	t->buf[t->token_size++] = '.';
	l->index++;
	if (!prv_extract_number(l, t, prv_float_end, err))
		return;

	t->buf[t->token_size] = 0;
	t->tok.real = atof(t->buf);
}

/* TODO:  Need to figure out if maximum negative int constant is correct */

static void prv_parse_integer(subtilis_lexer_t *l, subtilis_token_t *t,
			      int base, subtilis_error_t *err)
{
	char *end_ptr = 0;
	unsigned long num;

	t->buf[t->token_size] = 0;
	errno = 0;
	num = strtoul(t->buf, &end_ptr, base);
	if (*end_ptr != 0 || errno != 0 || num > 2147483647) {
		subtilis_error_set_number_too_long(err, t->buf, l->stream->name,
						   l->line);
		return;
	}
	t->tok.integer = (int32_t)num;
}

static subtilis_token_end_t prv_decimal_end(subtilis_lexer_t *l,
					    subtilis_token_t *t, char ch,
					    subtilis_error_t *err)
{
	if (!(ch >= '0' && ch <= '9')) {
		if (ch == '.') {
			prv_process_float(l, t, err);
			return SUBTILIS_TOKEN_END_RETURN;
		}
		return SUBTILIS_TOKEN_END_NOT_TOKEN;
	}
	return SUBTILIS_TOKEN_END_TOKEN;
}

static void prv_process_decimal(subtilis_lexer_t *l, char ch,
				subtilis_token_t *t, subtilis_error_t *err)
{
	prv_set_first(l, t, ch, SUBTILIS_TOKEN_INTEGER);
	if (!prv_extract_number(l, t, prv_decimal_end, err))
		return;

	prv_parse_integer(l, t, 10, err);
}

static subtilis_token_end_t prv_hexadecimal_end(subtilis_lexer_t *l,
						subtilis_token_t *t, char ch,
						subtilis_error_t *err)
{
	if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
	    (ch >= 'A' && ch <= 'F')) {
		return SUBTILIS_TOKEN_END_TOKEN;
	}
	return SUBTILIS_TOKEN_END_NOT_TOKEN;
}

static void prv_process_hexadecimal(subtilis_lexer_t *l, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	t->type = SUBTILIS_TOKEN_INTEGER;
	l->index++;
	if (!prv_extract_number(l, t, prv_hexadecimal_end, err))
		return;

	prv_parse_integer(l, t, 16, err);
}

static subtilis_token_end_t prv_binary_end(subtilis_lexer_t *l,
					   subtilis_token_t *t, char ch,
					   subtilis_error_t *err)
{
	if (ch == '0' || ch == '1')
		return SUBTILIS_TOKEN_END_TOKEN;
	return SUBTILIS_TOKEN_END_NOT_TOKEN;
}

static void prv_process_binary(subtilis_lexer_t *l, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	t->type = SUBTILIS_TOKEN_INTEGER;
	l->index++;
	if (!prv_extract_number(l, t, prv_binary_end, err))
		return;

	prv_parse_integer(l, t, 2, err);
}

static void prv_process_string(subtilis_lexer_t *l, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	l->index++;
	t->type = SUBTILIS_TOKEN_STRING;
	do {
		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end) {
			subtilis_error_set_unterminated_string(
			    err, t->buf, l->stream->name, l->line);
			return;
		}

		while (l->index < l->buf_end) {
			if (l->buffer[l->index] == '"') {
				if ((l->index + 1 == l->buf_end) ||
				    (l->buffer[l->index + 1] != '"'))
					goto done;
				l->index++;
			}
			if (t->token_size == SUBTILIS_MAX_TOKEN_SIZE) {
				subtilis_error_set_string_too_long(
				    err, t->buf, l->stream->name, l->line);
				t->buf[t->token_size] = 0;
				return;
			}
			t->buf[t->token_size++] = l->buffer[l->index++];
		}
	} while (l->index == l->buf_end);

// TODO: Above code is wrong could lead to loop terminating early

done:

	l->index++;
	t->buf[t->token_size] = 0;
}

static int prv_compare_keyword(const void *a, const void *b)
{
	subtilis_keyword_t *kw1 = (subtilis_keyword_t *)a;
	subtilis_keyword_t *kw2 = (subtilis_keyword_t *)b;

	return strcmp(kw1->str, kw2->str);
}

static void prv_validate_identifier(subtilis_lexer_t *l, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	char ch;

	for (;;) {
		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		      (ch >= '0' && ch <= '9') || (ch == '_')))
			break;
		prv_ensure_token_buffer(l, t, err,
					SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		t->buf[t->token_size++] = l->buffer[l->index++];
	}

	if (l->index < l->buf_end) {
		if (ch == '$')
			t->tok.id_type = SUBTILIS_IDENTIFIER_STRING;
		else if (ch == '%')
			t->tok.id_type = SUBTILIS_IDENTIFIER_INTEGER;
		else
			t->tok.id_type = SUBTILIS_IDENTIFIER_REAL;

		if (t->tok.id_type != SUBTILIS_IDENTIFIER_REAL) {
			prv_ensure_token_buffer(
			    l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			t->buf[t->token_size++] = l->buffer[l->index++];
		}
	} else {
		t->tok.id_type = SUBTILIS_IDENTIFIER_REAL;
	}

	t->buf[t->token_size] = 0;
}

static void prv_process_identifier(subtilis_lexer_t *l, char ch,
				   subtilis_token_t *t, subtilis_error_t *err)
{
	prv_set_first(l, t, ch, SUBTILIS_TOKEN_IDENTIFIER);
	prv_validate_identifier(l, t, err);
}

static void prv_process_call(subtilis_lexer_t *l, subtilis_token_t *t,
			     bool possible_proc, subtilis_error_t *err)
{
	t->type = SUBTILIS_TOKEN_KEYWORD;
	prv_validate_identifier(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (possible_proc) {
		if (t->tok.id_type != SUBTILIS_IDENTIFIER_REAL) {
			subtilis_error_set_bad_proc_name(
			    err, t->buf, l->stream->name, l->line);
			return;
		}
		t->tok.keyword.type = SUBTILIS_KEYWORD_PROC;
		t->tok.keyword.supported = true;
	} else {
		if (t->tok.id_type != SUBTILIS_IDENTIFIER_REAL) {
			subtilis_error_set_bad_fn_name(
			    err, t->buf, l->stream->name, l->line);
			return;
		}
		t->tok.keyword.type = SUBTILIS_KEYWORD_FN;
		t->tok.keyword.supported = true;
	}
}

static void prv_process_keyword(subtilis_lexer_t *l, char ch,
				subtilis_token_t *t, subtilis_error_t *err)
{
	subtilis_keyword_t *kw;
	subtilis_keyword_t key;
	bool possible_id = true;
	bool possible_fn = false;
	bool possible_proc = false;

	prv_set_first(l, t, ch, SUBTILIS_TOKEN_UNKNOWN);

	// TODO: We have very similar loops elsewhere
	for (;;) {
		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (!(ch >= 'A' && ch <= 'Z'))
			break;
		prv_ensure_token_buffer(l, t, err,
					SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		t->buf[t->token_size++] = l->buffer[l->index++];
	}

	if (l->index < l->buf_end) {
		if (ch == '$') {
			prv_ensure_token_buffer(
			    l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			t->buf[t->token_size++] = l->buffer[l->index++];
			possible_id = false;
			prv_ensure_buffer(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if ((l->index < l->buf_end) &&
			    (l->buffer[l->index] == '#')) {
				prv_ensure_token_buffer(
				    l, t, err,
				    SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				t->buf[t->token_size++] = l->buffer[l->index++];
			}
		} else if (ch == '#') {
			prv_ensure_token_buffer(
			    l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			t->buf[t->token_size++] = l->buffer[l->index++];
		}
	}

	t->buf[t->token_size] = 0;

	// TODO: bsearch may not be efficient for large files.

	if (possible_id) {
		possible_proc = strncmp(t->buf, "PROC", 4) == 0;
		possible_fn = strncmp(t->buf, "FN", 2) == 0;
		if (possible_proc || possible_fn) {
			prv_process_call(l, t, possible_proc, err);
			return;
		}
	}

	key.str = t->buf;
	kw =
	    bsearch(&key, subtilis_keywords_list,
		    sizeof(subtilis_keywords_list) / sizeof(subtilis_keyword_t),
		    sizeof(subtilis_keyword_t), prv_compare_keyword);

	if (!kw) {
		if (!possible_id) {
			subtilis_error_set_unknown_token(
			    err, t->buf, l->stream->name, l->line);
		} else {
			t->type = SUBTILIS_TOKEN_IDENTIFIER;
			prv_validate_identifier(l, t, err);
		}
		return;
	}
	t->type = SUBTILIS_TOKEN_KEYWORD;
	t->tok.keyword.type = kw->type;
	t->tok.keyword.supported = kw->supported;
}

static void prv_process_unknown(subtilis_lexer_t *l, char ch,
				subtilis_token_t *t, subtilis_error_t *err)
{
	prv_set_first(l, t, ch, SUBTILIS_TOKEN_UNKNOWN);
	for (;;) {
		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (prv_is_separator(ch))
			break;
		prv_ensure_token_buffer(l, t, err,
					SUBTILIS_ERROR_UNKNOWN_TOKEN);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		t->buf[t->token_size++] = l->buffer[l->index++];
	}

	t->buf[t->token_size] = 0;
	subtilis_error_set_unknown_token(err, t->buf, l->stream->name, l->line);
}

static void prv_process_token(subtilis_lexer_t *l, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	char ch = l->buffer[l->index];

	if (prv_is_simple_operator(ch)) {
		prv_set_first(l, t, ch, SUBTILIS_TOKEN_OPERATOR);
		return;
	}

	if (ch == '<' || ch == '>' || ch == '+' || ch == '-') {
		prv_process_complex_operator(l, ch, t, err);
		return;
	}

	if (ch == '"') {
		prv_process_string(l, t, err);
		return;
	}

	if (ch >= '0' && ch <= '9') {
		prv_process_decimal(l, ch, t, err);
		return;
	}

	if (ch == '&') {
		prv_process_hexadecimal(l, t, err);
		return;
	}

	if (ch == '%') {
		prv_process_binary(l, t, err);
		return;
	}

	if (ch == '.') {
		prv_process_float(l, t, err);
		return;
	}

	if (ch >= 'A' && ch <= 'Z') {
		prv_process_keyword(l, ch, t, err);
		return;
	}

	if ((ch >= 'a' && ch <= 'z') || (ch == '_')) {
		prv_process_identifier(l, ch, t, err);
		return;
	}

	prv_process_unknown(l, ch, t, err);
}

static void prv_skip_white(subtilis_lexer_t *l, subtilis_error_t *err)
{
	do {
		while (l->index < l->buf_end &&
		       prv_is_whitespace(l->buffer[l->index])) {
			if (l->buffer[l->index] == '\n')
				l->line++;
			l->index++;
		}

		prv_ensure_buffer(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (l->index == l->buf_end)
			return;
	} while (prv_is_whitespace(l->buffer[l->index]));
}

void subtilis_init_token(subtilis_token_t *t)
{
	t->type = SUBTILIS_TOKEN_EOF;
	t->buf[0] = 0;
	t->token_size = 0;
	memset(&t->tok, 0, sizeof(t->tok));
}

void subtilis_dump_token(subtilis_token_t *t)
{
	if (t->type == SUBTILIS_TOKEN_INTEGER)
		printf("[%d %d %s]\n", t->type, t->tok.integer, t->buf);
	else if (t->type == SUBTILIS_TOKEN_REAL)
		printf("[%d %f %s]\n", t->type, t->tok.real, t->buf);
	else if (t->type == SUBTILIS_TOKEN_KEYWORD)
		printf("[%d %d %s]\n", t->type, t->tok.keyword.type, t->buf);
	else if (t->type == SUBTILIS_TOKEN_IDENTIFIER)
		printf("[%d %d %s]\n", t->type, t->tok.id_type, t->buf);
	else
		printf("[%d %s]\n", t->type, t->buf);
}

void subtilis_lexer_get(subtilis_lexer_t *l, subtilis_token_t *t,
			subtilis_error_t *err)
{
	subtilis_init_token(t);
	prv_skip_white(l, err);
	if ((err->type != SUBTILIS_ERROR_OK) || (l->index == l->buf_end))
		return;
	prv_process_token(l, t, err);
}
