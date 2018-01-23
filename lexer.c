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

/* subtilis_token_new will reserve SUBTILIS_MAX_TOKEN_SIZE + 1 in bytes for the
 * token's buffer.  As long as token processing code doesn't write more than
 * SUBTILIS_MAX_TOKEN_SIZE into the token buffer, it can use prv_set_first,
 * prv_set_next and subtilis_token_get_text without having to worry about
 * errors.  subtilis_lexer_get always zero terminates the token buffer so it is
 * always safe to use subtilis_token_get_text to retrieve the zero terminated
 * token text after subtilis_lexer_get has returned without error.  When
 * the token text must be included into an error string also use
 * subtilis_token_get_text?  There is of course a chance this can fail but what
 * are we going to do with the error.  A constant error string will be written
 * in place of the token text to the error message.  In all other cases,
 * prv_set_next_with_err and subtilis_token_get_text_with_err should be used.
 */

static void prv_set_next_with_err(subtilis_lexer_t *l, subtilis_token_t *t,
				  subtilis_error_t *err)
{
	char ch = l->buffer[l->index];

	subtilis_buffer_append(&t->buf, &ch, 1, err);
	if (err->type == SUBTILIS_ERROR_OK)
		l->index++;
}

static void prv_set_next(subtilis_lexer_t *l, subtilis_token_t *t)
{
	subtilis_error_t err;

	subtilis_error_init(&err);
	prv_set_next_with_err(l, t, &err);
}

static void prv_set_first(subtilis_lexer_t *l, subtilis_token_t *t,
			  subtilis_token_type_t type)
{
	t->type = type;
	prv_set_next(l, t);
}

const char *subtilis_token_get_text(subtilis_token_t *t)
{
	const char *tbuf;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_buffer_zero_terminate(&t->buf, &err);
	if (err.type == SUBTILIS_ERROR_OK)
		tbuf = subtilis_buffer_get_string(&t->buf);
	else
		tbuf = "<TOKEN TEXT UNAVAILABLE>";
	return tbuf;
}

const char *subtilis_token_get_text_with_err(subtilis_token_t *t,
					     subtilis_error_t *err)
{
	subtilis_buffer_zero_terminate(&t->buf, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	return subtilis_buffer_get_string(&t->buf);
}

static void prv_ensure_input(subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (l->index < l->buf_end)
		return;

	l->buf_end =
	    l->stream->read(l->buffer, l->buf_size, l->stream->handle, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	l->index = 0;
}

static void prv_check_token_buffer(subtilis_lexer_t *l, subtilis_token_t *t,
				   subtilis_error_t *err,
				   subtilis_error_type_t type)
{
	const char *tbuf;

	if (subtilis_buffer_get_size(&t->buf) <= SUBTILIS_MAX_TOKEN_SIZE)
		return;

	tbuf = subtilis_token_get_text(t);
	subtilis_error_set1(err, type, tbuf, l->stream->name, l->line);
}

static bool prv_is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* TODO: This needs to be a bit mask or something */
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

static void prv_process_complex_operator(subtilis_lexer_t *l,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	char ch;
	char ch1;

	ch = l->buffer[l->index];

	prv_set_first(l, t, SUBTILIS_TOKEN_OPERATOR);
	prv_ensure_input(l, err);
	if ((err->type != SUBTILIS_ERROR_OK) || (l->index == l->buf_end))
		return;

	ch1 = l->buffer[l->index];
	switch (ch1) {
	case '=':
		prv_set_next(l, t);
		break;
	case '<':
		if (ch == '<')
			prv_set_next(l, t);
		break;
	case '>':
		if (ch == '<') {
			prv_set_next(l, t);
			break;
		}

		if (ch != '>')
			return;
		prv_set_next(l, t);

		prv_ensure_input(l, err);
		if ((err->type != SUBTILIS_ERROR_OK) ||
		    (l->index == l->buf_end))
			break;

		ch1 = l->buffer[l->index];
		if (ch1 != '>')
			break;

		prv_set_next(l, t);
		break;
	default:
		break;
	}
}

static bool prv_extract_number(subtilis_lexer_t *l, subtilis_token_t *t,
			       token_end_t end_fn, subtilis_error_t *err)
{
	subtilis_token_end_t end;
	bool processed = false;

	for (;;) {
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			break;
		if (l->index == l->buf_end) {
			processed = true;
			break;
		}
		end = end_fn(l, t, l->buffer[l->index], err);
		if (end == SUBTILIS_TOKEN_END_RETURN) {
			break;
		} else if (end == SUBTILIS_TOKEN_END_NOT_TOKEN) {
			processed = true;
			break;
		}

		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			break;
	}

	prv_check_token_buffer(l, t, err, SUBTILIS_ERROR_NUMBER_TOO_LONG);

	return processed;
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
	const char *tbuf;

	prv_set_first(l, t, SUBTILIS_TOKEN_REAL);
	if (!prv_extract_number(l, t, prv_float_end, err))
		return;

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	t->tok.real = atof(tbuf);
}

/* TODO:  The maximum negative int constant is incorrect */

static void prv_parse_integer(subtilis_lexer_t *l, subtilis_token_t *t,
			      int base, subtilis_error_t *err)
{
	char *end_ptr = 0;
	unsigned long num;
	const char *tbuf;

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	errno = 0;
	num = strtoul(tbuf, &end_ptr, base);
	if (*end_ptr != 0 || errno != 0) {
		subtilis_error_set_number_too_long(err, tbuf, l->stream->name,
						   l->line);
		return;
	}
	if ((base == 10 && num > 2147483647) || (num > 0xffffffff)) {
		subtilis_error_set_number_too_long(err, tbuf, l->stream->name,
						   l->line);
		return;
	}

	t->tok.integer = (int32_t)num;
}

/* TODO: This is a bit nasty.  Is there a nicer way? */
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

static void prv_process_decimal(subtilis_lexer_t *l, subtilis_token_t *t,
				subtilis_error_t *err)
{
	prv_set_first(l, t, SUBTILIS_TOKEN_INTEGER);
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
	const char *tbuf;

	l->index++;
	t->type = SUBTILIS_TOKEN_STRING;
	do {
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			goto unterminated;

		while (l->index < l->buf_end) {
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (l->buffer[l->index - 1] != '"')
				continue;

			prv_ensure_input(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				goto done;
			if ((l->index == l->buf_end) ||
			    (l->buffer[l->index] != '"'))
				goto done;
			l->index++;
		}
	} while (l->index == l->buf_end);

done:

	subtilis_buffer_delete(&t->buf, subtilis_buffer_get_size(&t->buf) - 1,
			       1, err);

	return;

unterminated:
	tbuf = subtilis_token_get_text(t);
	subtilis_error_set_unterminated_string(err, tbuf, l->stream->name,
					       l->line);
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
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		      (ch >= '0' && ch <= '9') || (ch == '_')))
			break;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (l->index == l->buf_end) {
		t->tok.id_type = SUBTILIS_TYPE_REAL;
	} else if (ch == '$') {
		t->tok.id_type = SUBTILIS_TYPE_STRING;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else if (ch == '%') {
		t->tok.id_type = SUBTILIS_TYPE_INTEGER;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	} else {
		t->tok.id_type = SUBTILIS_TYPE_REAL;
	}

	prv_check_token_buffer(l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_process_identifier(subtilis_lexer_t *l, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	prv_set_first(l, t, SUBTILIS_TOKEN_IDENTIFIER);
	prv_validate_identifier(l, t, err);
}

static void prv_process_call(subtilis_lexer_t *l, subtilis_token_t *t,
			     bool possible_proc, subtilis_error_t *err)
{
	const char *tbuf;

	t->type = SUBTILIS_TOKEN_KEYWORD;
	prv_validate_identifier(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (possible_proc) {
		if (t->tok.id_type != SUBTILIS_TYPE_REAL) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_bad_proc_name(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
		t->tok.keyword.type = SUBTILIS_KEYWORD_PROC;
		t->tok.keyword.supported = true;
	} else {
		if (t->tok.id_type != SUBTILIS_TYPE_REAL) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_bad_fn_name(
			    err, tbuf, l->stream->name, l->line);
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
	const char *tbuf;

	prv_set_first(l, t, SUBTILIS_TOKEN_UNKNOWN);

	/* TODO: We have very similar loops elsewhere */
	for (;;) {
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (!(ch >= 'A' && ch <= 'Z'))
			break;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	if (l->index < l->buf_end) {
		if (ch == '$') {
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_ensure_input(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if ((l->index < l->buf_end) &&
			    (l->buffer[l->index] == '#')) {
				possible_id = false;
				prv_set_next_with_err(l, t, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
			}
		} else if (ch == '#') {
			possible_id = false;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}

	/* There's no need to check token length here.  If it's a keyword
	 * the length must be less than SUBTILIS_MAX_TOKEN_SIZE.  Otherwise
	 * the token size will be checked by prv_validate_identifier.
	 */

	/* This will zero terminate the token buffer.  This is what we need
	 * for performing the various string comparsions we're going to do
	 * but we'll need to remove the trailing zero if we don't have a
	 * keyword.
	 */

	tbuf = subtilis_token_get_text_with_err(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	// TODO: bsearch may not be efficient for large files.

	if (possible_id) {
		possible_proc = strncmp(tbuf, "PROC", 4) == 0;
		possible_fn = strncmp(tbuf, "FN", 2) == 0;
		if (possible_proc || possible_fn) {
			subtilis_buffer_remove_terminator(&t->buf);
			prv_process_call(l, t, possible_proc, err);
			return;
		}
	}

	key.str = tbuf;
	kw =
	    bsearch(&key, subtilis_keywords_list,
		    sizeof(subtilis_keywords_list) / sizeof(subtilis_keyword_t),
		    sizeof(subtilis_keyword_t), prv_compare_keyword);

	if (kw) {
		t->type = SUBTILIS_TOKEN_KEYWORD;
		t->tok.keyword.type = kw->type;
		t->tok.keyword.supported = kw->supported;
		return;
	}

	subtilis_buffer_remove_terminator(&t->buf);
	if (possible_id) {
		t->type = SUBTILIS_TOKEN_IDENTIFIER;
		prv_validate_identifier(l, t, err);
		return;
	}

	/* It's an invalid token */

	prv_check_token_buffer(l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	tbuf = subtilis_token_get_text(t);
	subtilis_error_set_unknown_token(err, tbuf, l->stream->name, l->line);
}

static void prv_process_unknown(subtilis_lexer_t *l, char ch,
				subtilis_token_t *t, subtilis_error_t *err)
{
	const char *tbuf;

	prv_set_first(l, t, SUBTILIS_TOKEN_UNKNOWN);
	for (;;) {
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (prv_is_separator(ch))
			break;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	tbuf = subtilis_token_get_text(t);
	subtilis_error_set_unknown_token(err, tbuf, l->stream->name, l->line);
}

static void prv_process_token(subtilis_lexer_t *l, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	char ch = l->buffer[l->index];

	if (prv_is_simple_operator(ch)) {
		prv_set_first(l, t, SUBTILIS_TOKEN_OPERATOR);
		return;
	}

	if (ch == '<' || ch == '>' || ch == '+' || ch == '-') {
		prv_process_complex_operator(l, t, err);
		return;
	}

	if (ch == '"') {
		prv_process_string(l, t, err);
		return;
	}

	if (ch >= '0' && ch <= '9') {
		prv_process_decimal(l, t, err);
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
		prv_process_identifier(l, t, err);
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

		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (l->index == l->buf_end)
			return;
	} while (prv_is_whitespace(l->buffer[l->index]));
}

subtilis_token_t *subtilis_token_new(subtilis_error_t *err)
{
	subtilis_token_t *t;

	t = malloc(sizeof(*t));
	if (!t) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	t->type = SUBTILIS_TOKEN_EOF;
	memset(&t->tok, 0, sizeof(t->tok));
	subtilis_buffer_init(&t->buf, SUBTILIS_MAX_TOKEN_SIZE + 1);
	subtilis_buffer_reserve(&t->buf, SUBTILIS_MAX_TOKEN_SIZE + 1, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_buffer_free(&t->buf);
		free(t);
		t = NULL;
	}
	return t;
}

void subtilis_token_delete(subtilis_token_t *t)
{
	if (!t)
		return;

	subtilis_buffer_free(&t->buf);
	free(t);
}

static void prv_reinit_token(subtilis_token_t *t)
{
	t->type = SUBTILIS_TOKEN_EOF;
	memset(&t->tok, 0, sizeof(t->tok));
	subtilis_buffer_reset(&t->buf);
}

void subtilis_dump_token(subtilis_token_t *t)
{
	const char *tbuf = subtilis_token_get_text(t);

	if (t->type == SUBTILIS_TOKEN_INTEGER)
		printf("[%d %d %s]\n", t->type, t->tok.integer, tbuf);
	else if (t->type == SUBTILIS_TOKEN_REAL)
		printf("[%d %f %s]\n", t->type, t->tok.real, tbuf);
	else if (t->type == SUBTILIS_TOKEN_KEYWORD)
		printf("[%d %d %s]\n", t->type, t->tok.keyword.type, tbuf);
	else if (t->type == SUBTILIS_TOKEN_IDENTIFIER)
		printf("[%d %d %s]\n", t->type, t->tok.id_type, tbuf);
	else
		printf("[%d %s]\n", t->type, tbuf);
}

void subtilis_lexer_get(subtilis_lexer_t *l, subtilis_token_t *t,
			subtilis_error_t *err)
{
	prv_reinit_token(t);
	prv_skip_white(l, err);
	if ((err->type != SUBTILIS_ERROR_OK) || (l->index == l->buf_end))
		return;
	prv_process_token(l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	subtilis_buffer_zero_terminate(&t->buf, err);
}
