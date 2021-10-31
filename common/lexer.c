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
#include <inttypes.h>
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

static void prv_skip_line(subtilis_lexer_t *l, subtilis_error_t *err);

typedef subtilis_token_end_t (*token_end_t)(subtilis_lexer_t *,
					    subtilis_token_t *, char ch,
					    subtilis_error_t *);
static void prv_process_negative_decimal(subtilis_lexer_t *l,
					 subtilis_token_t *t,
					 subtilis_error_t *err);

subtilis_lexer_t *subtilis_lexer_new(subtilis_stream_t *s, size_t buf_size,
				     const subtilis_keyword_t *basic_keywords,
				     size_t num_basic_keywords,
				     const subtilis_keyword_t *ass_keywords,
				     size_t num_ass_keywords,
				     subtilis_error_t *err)
{
	subtilis_lexer_t *l;

	l = malloc(sizeof(*l));
	if (!l) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	l->buffer = malloc(buf_size);
	if (!l->buffer) {
		free(l);
		subtilis_error_set_oom(err);
		return NULL;
	}

	l->keywords = basic_keywords;
	l->num_keywords = num_basic_keywords;
	l->basic_keywords = basic_keywords;
	l->num_basic_keywords = num_basic_keywords;
	l->ass_keywords = ass_keywords;
	l->num_ass_keywords = num_ass_keywords;
	l->buf_size = buf_size;
	l->buf_end = 0;
	l->stream = s;
	l->line = 1;
	l->character = 1;
	l->index = 0;
	l->num_blocks = 0;
	l->next = NULL;

	return l;
}

void subtilis_lexer_delete(subtilis_lexer_t *l, subtilis_error_t *err)
{
	size_t i;

	if (l) {
		subtilis_token_delete(l->next);
		l->stream->close(l->stream->handle, err);
		free(l->buffer);

		for (i = 0; i < l->num_blocks; i++)
			subtilis_token_delete(l->blocks[i].t);

		free(l);
	}
}

void subtilis_lexer_set_ass_keywords(subtilis_lexer_t *l, bool ass,
				     subtilis_error_t *err)
{
	if (!ass) {
		l->keywords = l->basic_keywords;
		l->num_keywords = l->num_basic_keywords;
		return;
	}

	if (!l->ass_keywords) {
		subtilis_error_set_not_supported(
		    err, "Assembly language on this target is", l->stream->name,
		    l->line);
		return;
	}

	l->keywords = l->ass_keywords;
	l->num_keywords = l->num_ass_keywords;
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
	size_t new_buf_size;
	char *new_buffer;

	if (l->index < l->buf_end)
		return;

	if (l->num_blocks == 0) {
		l->buf_end = l->stream->read(l->buffer, l->buf_size,
					     l->stream->handle, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		l->index = 0;
		return;
	}

	/* We can't overwrite the existing data in the buffer because we
	 * have stacked blocks.  So we're just going to have to make our
	 * buffer bigger and append to it.  This feature is currently only
	 * used by the assemblers.
	 */

	new_buf_size = l->buf_size + SUBTILIS_CONFIG_LEXER_BUF_SIZE_INC;
	new_buffer = realloc(l->buffer, new_buf_size);
	if (!new_buffer) {
		subtilis_error_set_oom(err);
		return;
	}

	l->buf_size = new_buf_size;
	l->buffer = new_buffer;
	l->buf_end += l->stream->read(&l->buffer[l->index],
				      SUBTILIS_CONFIG_LEXER_BUF_SIZE_INC,
				      l->stream->handle, err);
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
	return c == '/' || c == '*' || c == '(' || c == ')' || c == '=' ||
	       c == ',' || c == ';' || c == '^' || c == '[' || c == ']' ||
	       c == '|' || c == '~' || c == '?' || c == '$' || c == '!' ||
	       c == '{' || c == '}';
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
	if ((ch == '-') && (ch1 >= '0' && ch1 <= '9')) {
		prv_process_negative_decimal(l, t, err);
		return;
	}

	switch (ch1) {
	case '=':
		prv_set_next(l, t);
		break;
	case '-':
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

static void prv_process_negative_decimal(subtilis_lexer_t *l,
					 subtilis_token_t *t,
					 subtilis_error_t *err)
{
	const char *tbuf;

	if (l->next) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	l->next = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_set_first(l, l->next, SUBTILIS_TOKEN_INTEGER);
	if (!prv_extract_number(l, l->next, prv_decimal_end, err))
		return;

	tbuf = subtilis_token_get_text_with_err(l->next, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (strcmp(tbuf, "2147483648")) {
		prv_parse_integer(l, l->next, 10, err);
	} else {
		/* This is a special case where we have the smallest possible
		 * 32 bit integer.  Here we return 1 token, the integer, rather
		 * than the '-' on this call to the lexer and the integer on the
		 * next.
		 */

		subtilis_token_claim(t, l->next);
		subtilis_token_delete(l->next);
		l->next = NULL;
		t->tok.integer = -2147483648;
	}
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

static void prv_complete_custom_type(subtilis_lexer_t *l, subtilis_token_t *t,
				     subtilis_error_t *err)
{
	char ch;
	char prefix[5];
	size_t i = 0;
	const char *tbuf;

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

		if (i < 4)
			prefix[i] = ch;
		i++;
	}

	tbuf = subtilis_token_get_text(t);
	if (i < 2) {
		subtilis_error_set_bad_type_name(err, tbuf, l->stream->name,
						 l->line);
		return;
	}
	if (i > 4)
		i = 4;
	prefix[i] = 0;
	if (strncmp(prefix, "FN", 2) && strncmp(prefix, "PROC", 4)) {
		subtilis_error_set_bad_type_name(err, tbuf, l->stream->name,
						 l->line);
		return;
	}

	/*
	 * The lexer doesn't know the complete type of the token as this
	 * information is stored in the symbol table to which it doesn't have
	 * access.  It does know however, that we have a custom type and the
	 * type is either a function or a procedure.
	 */

	t->tok.id_type.type = SUBTILIS_TYPE_FN;
	t->tok.id_type.params.fn.num_params = 0;
	t->tok.id_type.params.fn.ret_val =
	    calloc(1, sizeof(*t->tok.id_type.params.fn.ret_val));
	if (!t->tok.id_type.params.fn.ret_val) {
		subtilis_error_set_oom(err);
		return;
	}
	t->tok.id_type.params.fn.ret_val->type = SUBTILIS_TYPE_VOID;
}

static void prv_validate_identifier(subtilis_lexer_t *l, subtilis_token_t *t,
				    char ch, subtilis_error_t *err)
{
	if (ch == 0) {
		for (;;) {
			prv_ensure_input(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (l->index == l->buf_end)
				break;

			ch = l->buffer[l->index];
			if (!((ch >= 'A' && ch <= 'Z') ||
			      (ch >= 'a' && ch <= 'z') ||
			      (ch >= '0' && ch <= '9') || (ch == '_')))
				break;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}

		if (l->index == l->buf_end) {
			t->tok.id_type.type = SUBTILIS_TYPE_REAL;
		} else if (ch == '$') {
			t->tok.id_type.type = SUBTILIS_TYPE_STRING;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (ch == '%') {
			t->tok.id_type.type = SUBTILIS_TYPE_INTEGER;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (ch == '&') {
			t->tok.id_type.type = SUBTILIS_TYPE_BYTE;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if (ch == '@') {
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			prv_complete_custom_type(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else {
			t->tok.id_type.type = SUBTILIS_TYPE_REAL;
		}
	} else {
		if (ch == '$') {
			t->tok.id_type.type = SUBTILIS_TYPE_STRING;
		} else if (ch == '%') {
			t->tok.id_type.type = SUBTILIS_TYPE_INTEGER;
		} else if (ch == '&') {
			t->tok.id_type.type = SUBTILIS_TYPE_BYTE;
		} else if (ch == '@') {
			prv_complete_custom_type(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else {
			t->tok.id_type.type = SUBTILIS_TYPE_REAL;
		}
	}

	prv_check_token_buffer(l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_process_identifier(subtilis_lexer_t *l, subtilis_token_t *t,
				   subtilis_error_t *err)
{
	prv_set_first(l, t, SUBTILIS_TOKEN_IDENTIFIER);
	prv_validate_identifier(l, t, 0, err);
}

static void prv_process_call(subtilis_lexer_t *l, subtilis_token_t *t,
			     bool proc, char ch, subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_type_t id_type;

	t->type = SUBTILIS_TOKEN_IDENTIFIER;
	prv_validate_identifier(l, t, ch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (proc) {
		if (t->tok.id_type.type != SUBTILIS_TYPE_REAL) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_bad_proc_name(
			    err, tbuf, l->stream->name, l->line);
			return;
		}
		t->tok.keyword.type = SUBTILIS_KEYWORD_PROC;
		t->tok.keyword.supported = true;
		t->tok.keyword.id_type.type = SUBTILIS_TYPE_VOID;
	} else {
		subtilis_type_init_copy(&id_type, &t->tok.id_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		subtilis_type_free(&t->tok.id_type);
		t->tok.keyword.id_type = id_type;
		t->tok.keyword.type = SUBTILIS_KEYWORD_FN;
		t->tok.keyword.supported = true;
	}
	t->type = SUBTILIS_TOKEN_KEYWORD;
}

static bool prv_process_keyword(subtilis_lexer_t *l, char ch,
				subtilis_token_t *t, subtilis_error_t *err)
{
	subtilis_keyword_t *kw;
	subtilis_keyword_t key;
	const char *tbuf;
	bool possible_id = true;
	bool possible_fn = false;
	bool possible_proc = false;

	prv_set_first(l, t, SUBTILIS_TOKEN_UNKNOWN);

	/* TODO: We have very similar loops elsewhere */
	for (;;) {
		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
		if (l->index == l->buf_end)
			break;

		ch = l->buffer[l->index];
		if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')))
			break;
		prv_set_next_with_err(l, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return false;
	}

	if (l->index < l->buf_end) {
		if (ch == '$') {
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			prv_ensure_input(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			if ((l->index < l->buf_end) &&
			    (l->buffer[l->index] == '#')) {
				possible_id = false;
				prv_set_next_with_err(l, t, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return false;
			}
		} else if (ch == '#') {
			possible_id = false;
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
		} else if (ch == '%' || ch == '&' || ch == '@') {
			prv_set_next_with_err(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
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
		return false;

	if (possible_id) {
		possible_proc = strncmp(tbuf, "PROC", 4) == 0;
		possible_fn = strncmp(tbuf, "FN", 2) == 0;
		if (possible_proc || possible_fn) {
			subtilis_buffer_remove_terminator(&t->buf);
			if (ch != '$' && ch != '%' && ch != '&' && ch != '@')
				ch = 0;
			prv_process_call(l, t, possible_proc, ch, err);
			return false;
		}
	}

	// TODO: bsearch may not be efficient for large files.

	key.str = tbuf;
	kw = bsearch(&key, l->keywords, l->num_keywords, sizeof(*l->keywords),
		     prv_compare_keyword);

	if (kw) {
		if (kw->type == SUBTILIS_KEYWORD_REM) {
			prv_skip_line(l, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return false;
			return true;
		}

		t->type = SUBTILIS_TOKEN_KEYWORD;
		t->tok.keyword.type = kw->type;
		t->tok.keyword.supported = kw->supported;
		return false;
	}

	subtilis_buffer_remove_terminator(&t->buf);
	if (possible_id) {
		t->type = SUBTILIS_TOKEN_IDENTIFIER;
		if (ch != '$' && ch != '%' && ch != '&' && ch != '@')
			ch = 0;
		prv_validate_identifier(l, t, ch, err);
		return false;
	}

	/* It's an invalid token */

	prv_check_token_buffer(l, t, err, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
	if (err->type != SUBTILIS_ERROR_OK)
		return false;
	tbuf = subtilis_token_get_text(t);
	subtilis_error_set_unknown_token(err, tbuf, l->stream->name, l->line);

	return false;
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

static bool prv_process_token(subtilis_lexer_t *l, subtilis_token_t *t,
			      subtilis_error_t *err)
{
	char ch = l->buffer[l->index];

	if (prv_is_simple_operator(ch)) {
		prv_set_first(l, t, SUBTILIS_TOKEN_OPERATOR);
		return false;
	}

	if (ch == '<' || ch == '>' || ch == '+' || ch == '-' || ch == ':') {
		prv_process_complex_operator(l, t, err);
		return false;
	}

	if (ch == '"') {
		prv_process_string(l, t, err);
		return false;
	}

	if (ch >= '0' && ch <= '9') {
		prv_process_decimal(l, t, err);
		return false;
	}

	if (ch == '&') {
		prv_process_hexadecimal(l, t, err);
		return false;
	}

	if (ch == '%') {
		prv_process_binary(l, t, err);
		return false;
	}

	if (ch == '.') {
		prv_process_float(l, t, err);
		return false;
	}

	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
		if (prv_process_keyword(l, ch, t, err))
			return true;
		return false;
	}

	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
	    (ch == '_')) {
		prv_process_identifier(l, t, err);
		return false;
	}

	prv_process_unknown(l, ch, t, err);

	return false;
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

static void prv_skip_line(subtilis_lexer_t *l, subtilis_error_t *err)
{
	do {
		while (l->index < l->buf_end && l->buffer[l->index] != '\n')
			l->index++;

		prv_ensure_input(l, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		if (l->index == l->buf_end)
			return;
	} while (l->buffer[l->index] != '\n');
	l->index++;
	l->line++;
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

void subtilis_token_claim(subtilis_token_t *dest, subtilis_token_t *src)
{
	/*
	 * This is truly horrid.
	 */
	if (dest->type == SUBTILIS_TOKEN_IDENTIFIER)
		subtilis_type_free(&dest->tok.id_type);
	else if (dest->type == SUBTILIS_TOKEN_KEYWORD)
		subtilis_type_free(&dest->tok.keyword.id_type);
	subtilis_buffer_free(&dest->buf);
	*dest = *src;
	if (src->type == SUBTILIS_TOKEN_IDENTIFIER)
		src->tok.id_type.params.fn.ret_val = NULL;
	else if (dest->type == SUBTILIS_TOKEN_KEYWORD)
		src->tok.keyword.id_type.params.fn.ret_val = NULL;
	subtilis_buffer_init(&src->buf, 1);
}

subtilis_token_t *subtilis_token_dup(const subtilis_token_t *src,
				     subtilis_error_t *err)
{
	size_t buf_size;
	subtilis_token_t *t;

	t = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;
	t->type = src->type;
	t->tok = src->tok;
	buf_size = subtilis_buffer_get_size(&src->buf);
	subtilis_buffer_append(&t->buf, subtilis_buffer_get_string(&src->buf),
			       buf_size, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		subtilis_token_delete(t);
		t = NULL;
	}

	return t;
}

void subtilis_token_delete(subtilis_token_t *t)
{
	if (!t)
		return;

	if (t->type == SUBTILIS_TOKEN_IDENTIFIER)
		subtilis_type_free(&t->tok.id_type);
	else if (t->type == SUBTILIS_TOKEN_KEYWORD)
		subtilis_type_free(&t->tok.keyword.id_type);

	subtilis_buffer_free(&t->buf);
	free(t);
}

static void prv_reinit_token(subtilis_token_t *t)
{
	if (t->type == SUBTILIS_TOKEN_IDENTIFIER)
		subtilis_type_free(&t->tok.id_type);
	else if (t->type == SUBTILIS_TOKEN_KEYWORD)
		subtilis_type_free(&t->tok.keyword.id_type);
	t->type = SUBTILIS_TOKEN_EOF;
	memset(&t->tok, 0, sizeof(t->tok));
	t->tok.id_type.type = SUBTILIS_TYPE_VOID;
	subtilis_buffer_reset(&t->buf);
}

void subtilis_dump_token(subtilis_token_t *t)
{
	const char *tbuf = subtilis_token_get_text(t);

	if (t->type == SUBTILIS_TOKEN_INTEGER)
		printf("[%d %" PRIu32 " %s]\n", t->type, t->tok.integer, tbuf);
	else if (t->type == SUBTILIS_TOKEN_REAL)
		printf("[%d %f %s]\n", t->type, t->tok.real, tbuf);
	else if (t->type == SUBTILIS_TOKEN_KEYWORD)
		printf("[%d %d %s]\n", t->type, t->tok.keyword.type, tbuf);
	else if (t->type == SUBTILIS_TOKEN_IDENTIFIER)
		printf("[%d %d %s]\n", t->type, t->tok.id_type.type, tbuf);
	else
		printf("[%d %s]\n", t->type, tbuf);
}

void subtilis_lexer_push_block(subtilis_lexer_t *l, const subtilis_token_t *t,
			       subtilis_error_t *err)
{
	subtilis_token_t *t_dup;

	if (l->num_blocks == SUBTILIS_LEXER_MAX_BLOCKS) {
		subtilis_error_set_too_many_blocks(err, l->stream->name,
						   l->line);
		return;
	}

	if (l->next) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	t_dup = subtilis_token_dup(t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	l->blocks[l->num_blocks].index = l->index;
	l->blocks[l->num_blocks].t = t_dup;
	l->blocks[l->num_blocks].line = l->line;
	l->num_blocks++;
}

void subtilis_lexer_set_block_start(subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (l->num_blocks == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	if (l->next) {
		subtilis_token_delete(l->next);
		l->next = NULL;
	}

	l->index = l->blocks[l->num_blocks - 1].index;
	l->line = l->blocks[l->num_blocks - 1].line;
	l->next = subtilis_token_dup(l->blocks[l->num_blocks - 1].t, err);
}

void subtilis_lexer_pop_block(subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (l->num_blocks == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	l->num_blocks--;
	subtilis_token_delete(l->blocks[l->num_blocks].t);
}

void subtilis_lexer_get(subtilis_lexer_t *l, subtilis_token_t *t,
			subtilis_error_t *err)
{
	bool rem;

	do {
		if (l->next) {
			/*
			 * We transfer ownership of our buffer.
			 */

			subtilis_token_claim(t, l->next);
			subtilis_token_delete(l->next);
			l->next = NULL;
			rem = false;
		} else {
			prv_reinit_token(t);
			prv_skip_white(l, err);
			if ((err->type != SUBTILIS_ERROR_OK) ||
			    (l->index == l->buf_end))
				return;
			rem = prv_process_token(l, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	} while (rem);
	subtilis_buffer_zero_terminate(&t->buf, err);
}
