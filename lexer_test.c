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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "lexer.h"

static int prv_test_wrapper(const char *text, size_t buf_size,
			    int (*fn)(subtilis_lexer_t *))
{
	subtilis_stream_t s;
	subtilis_error_t err;
	subtilis_lexer_t *l;
	int retval;

	subtilis_error_init(&err);
	subtilis_stream_from_text(&s, text, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, buf_size, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		s.close(s.handle, &err);
		goto fail;
	}
	retval = fn(l);
	subtilis_lexer_delete(l, &err);
	printf(": [%s]\n", retval ? "FAIL" : "OK");

	return retval;

fail:

	printf(": [FAIL]\n");
	subtilis_error_fprintf(stderr, &err, true);

	return 1;
}

static int prv_test_too_long(subtilis_lexer_t *l,
			     subtilis_error_type_t err_type)
{
	subtilis_token_t t;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_lexer_get(l, &t, &err);
	if (err.type != err_type) {
		fprintf(stderr, "Expected error %d got %d\n", err_type,
			err.type);
		return 1;
	}

	if (t.token_size > SUBTILIS_MAX_TOKEN_SIZE) {
		fprintf(stderr, "Token overflow.  l->token_size = %zu\n",
			t.token_size);
		return 1;
	}

	return 0;
}

static int prv_check_string_too_long(subtilis_lexer_t *l)
{
	return prv_test_too_long(l, SUBTILIS_ERROR_STRING_TOO_LONG);
}

static int prv_check_id_too_long(subtilis_lexer_t *l)
{
	return prv_test_too_long(l, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG);
}

static int prv_test_max_id(subtilis_lexer_t *l,
			   subtilis_identifier_type_t id_type)
{
	subtilis_token_t t;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	if ((t.type != SUBTILIS_TOKEN_IDENTIFIER) ||
	    (t.tok.id_type != id_type)) {
		fprintf(stderr,
			"Unexpected token.  Expected identifier of type %d\n",
			id_type);
		return 1;
	}

	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	if ((t.type != SUBTILIS_TOKEN_OPERATOR) || (strcmp(t.buf, "+"))) {
		fprintf(stderr, "Unexpected token.  Expected +\n");
		return 1;
	}

	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	if ((t.type != SUBTILIS_TOKEN_IDENTIFIER) ||
	    (t.tok.id_type != id_type)) {
		fprintf(stderr,
			"Unexpected token.  Expected identifier of type %d\n",
			id_type);
		return 1;
	}

	return 0;
}

static int prv_check_max_int_var(subtilis_lexer_t *l)
{
	return prv_test_max_id(l, SUBTILIS_IDENTIFIER_INTEGER);
}

static int prv_check_max_str_var(subtilis_lexer_t *l)
{
	return prv_test_max_id(l, SUBTILIS_IDENTIFIER_STRING);
}

static int prv_check_max_real_var(subtilis_lexer_t *l)
{
	return prv_test_max_id(l, SUBTILIS_IDENTIFIER_REAL);
}

static int prv_test_string_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 4];

	str[0] = '"';
	str[SUBTILIS_MAX_TOKEN_SIZE + 3] = 0;
	str[SUBTILIS_MAX_TOKEN_SIZE + 2] = '"';
	memset(&str[1], '1', SUBTILIS_MAX_TOKEN_SIZE + 1);

	printf("lexer_string_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_string_too_long);
}

static int prv_test_real_var_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE + 1);
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_real_var_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_real_var_upper_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'A', SUBTILIS_MAX_TOKEN_SIZE + 1);
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_real_var_upper_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_string_var_upper_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'A', SUBTILIS_MAX_TOKEN_SIZE);
	str[SUBTILIS_MAX_TOKEN_SIZE] = '$';
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_string_var_upper_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_id_hash_var_upper_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'A', SUBTILIS_MAX_TOKEN_SIZE);
	str[SUBTILIS_MAX_TOKEN_SIZE] = '#';
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_id_hash_var_upper_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_string_hash_var_upper_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'A', SUBTILIS_MAX_TOKEN_SIZE - 1);
	str[SUBTILIS_MAX_TOKEN_SIZE - 1] = '$';
	str[SUBTILIS_MAX_TOKEN_SIZE] = '#';
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_string_hash_var_upper_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_string_var_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE);
	str[SUBTILIS_MAX_TOKEN_SIZE] = '$';
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_string_var_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_int_var_too_long(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE + 2];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE);
	str[SUBTILIS_MAX_TOKEN_SIZE] = '%';
	str[SUBTILIS_MAX_TOKEN_SIZE + 1] = 0;

	printf("lexer_int_var_too_long");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_id_too_long);
}

static int prv_test_max_int_var(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE * 2 + 1 + 1];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE - 1);
	str[SUBTILIS_MAX_TOKEN_SIZE - 1] = '%';
	str[SUBTILIS_MAX_TOKEN_SIZE] = '+';
	memset(&str[SUBTILIS_MAX_TOKEN_SIZE + 1], 'b',
	       SUBTILIS_MAX_TOKEN_SIZE - 1);
	str[SUBTILIS_MAX_TOKEN_SIZE * 2] = '%';
	str[(SUBTILIS_MAX_TOKEN_SIZE * 2) + 1] = 0;

	printf("lexer_max_int_var");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_max_int_var);
}

static int prv_test_max_str_var(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE * 2 + 1 + 1];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE - 1);
	str[SUBTILIS_MAX_TOKEN_SIZE - 1] = '$';
	str[SUBTILIS_MAX_TOKEN_SIZE] = '+';
	memset(&str[SUBTILIS_MAX_TOKEN_SIZE + 1], 'b',
	       SUBTILIS_MAX_TOKEN_SIZE - 1);
	str[SUBTILIS_MAX_TOKEN_SIZE * 2] = '$';
	str[(SUBTILIS_MAX_TOKEN_SIZE * 2) + 1] = 0;

	printf("lexer_max_str_var");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_max_str_var);
}

static int prv_test_max_real_var(void)
{
	char str[SUBTILIS_MAX_TOKEN_SIZE * 2 + 1 + 1];

	memset(str, 'a', SUBTILIS_MAX_TOKEN_SIZE);
	str[SUBTILIS_MAX_TOKEN_SIZE] = '+';
	memset(&str[SUBTILIS_MAX_TOKEN_SIZE + 1], 'b', SUBTILIS_MAX_TOKEN_SIZE);
	str[(SUBTILIS_MAX_TOKEN_SIZE * 2) + 1] = 0;

	printf("lexer_max_real_var");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_max_real_var);
}

static int prv_check_keywords(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	subtilis_error_init(&err);

	for (i = 0; i < SUBTILIS_KEYWORD_MAX; i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_KEYWORD) {
			fprintf(stderr, "Expected token type %d got %d\n",
				t.type, SUBTILIS_TOKEN_KEYWORD);
			return 1;
		}

		if (t.tok.keyword.type != subtilis_keywords_list[i].type) {
			fprintf(stderr, "Expected keyword type %d got %d\n",
				subtilis_keywords_list[i].type,
				t.tok.keyword.type);
			return 1;
		}

		if (t.tok.keyword.supported !=
		    subtilis_keywords_list[i].supported) {
			fprintf(stderr, "Expected supported %d got %d\n",
				subtilis_keywords_list[i].supported,
				t.tok.keyword.supported);
			return 1;
		}
	}

	return 0;
}

static int prv_test_keywords(void)
{
	int i;
	subtilis_buffer buf;
	subtilis_error_t err;
	const char sep[] = "\n \r \t";

	printf("lexer_keywords");

	subtilis_error_init(&err);
	subtilis_buffer_init(&buf, 4096);
	for (i = 0; i < SUBTILIS_KEYWORD_MAX; i++) {
		subtilis_buffer_append(&buf, subtilis_keywords_list[i].str,
				       strlen(subtilis_keywords_list[i].str),
				       &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			goto on_error;
		}

		subtilis_buffer_append(&buf, sep, strlen(sep), &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			goto on_error;
		}
	}

	subtilis_buffer_zero_terminate(&buf, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto on_error;
	}

	return prv_test_wrapper(subtilis_buffer_get_string(&buf),
				SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_keywords);

on_error:
	printf(": [FAIL]\n");

	subtilis_buffer_free(&buf);
	return 1;
}

static int prv_check_procedure_call(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;
	const char *const names[] = {"PROCINVOKEME", "PROCinvokeme"};

	subtilis_error_init(&err);

	for (i = 0; i < 2; i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_KEYWORD) {
			fprintf(stderr,
				"SUBTILIS_TOKEN_KEYWORD expected.  Found %d\n",
				t.type);
			return 1;
		}

		if (t.tok.keyword.type != SUBTILIS_KEYWORD_PROC) {
			fprintf(stderr,
				"SUBTILIS_TOKEN_KEYWORD expected.  Found %d\n",
				t.tok.keyword.type);
			return 1;
		}

		if (strcmp(names[i], t.buf)) {
			fprintf(stderr,
				"Unexpected procedure name %s, expected %s\n",
				t.buf, names[i]);
			return 1;
		}
	}

	return 0;
}

static int prv_check_empty(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;

	subtilis_error_init(&err);

	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	if (t.type != SUBTILIS_TOKEN_EOF) {
		fprintf(stderr, "EOF expected, found %d\n", t.type);
		return 1;
	}

	return 0;
}

static int prv_test_procedure_call(void)
{
	const char *str = "PROCINVOKEME PROCinvokeme";

	printf("lexer_procedure_call");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_procedure_call);
}

static int prv_test_empty(void)
{
	const char *str = "";

	printf("lexer_empty");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_empty);
}

static int prv_check_bad_proc_name(subtilis_lexer_t *l,
				   subtilis_error_type_t err_type)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	for (i = 0; i < 2; i++) {
		subtilis_error_init(&err);
		subtilis_lexer_get(l, &t, &err);
		if (err.type != err_type) {
			fprintf(stderr, "Expected error %d, got %d\n", err_type,
				err.type);
			return 1;
		}
	}

	return 0;
}

static int prv_check_proc_typed(subtilis_lexer_t *l)
{
	return prv_check_bad_proc_name(l, SUBTILIS_ERROR_BAD_PROC_NAME);
}

static int prv_test_proc_typed(void)
{
	const char *str = "PROCcalculate% PROCcalculate$";

	printf("lexer_proc_typed");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_proc_typed);
}

static int prv_check_fn_typed(subtilis_lexer_t *l)
{
	return prv_check_bad_proc_name(l, SUBTILIS_ERROR_BAD_FN_NAME);
}

static int prv_test_fn_typed(void)
{
	const char *str = "FNcalculate% FNcalculate$";

	printf("lexer_fn_typed");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_fn_typed);
}

static int prv_check_int(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	const int expected[] = {
	    12345, 67890, 0x12345, 0x67890, 0xabcef, 2147483647, 170,
	};

	const int expected_signed[] = {255, 0xff, 255};

	subtilis_error_init(&err);
	for (i = 0; i < sizeof(expected) / sizeof(int); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_INTEGER) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_INTEGER, t.type);
			return 1;
		}

		if (t.tok.integer != expected[i]) {
			fprintf(stderr, "Expected integer value %d, got %d\n",
				expected[i], t.tok.integer);
			return 1;
		}
	}

	for (i = 0; i < sizeof(expected_signed) / sizeof(int); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_OPERATOR) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_OPERATOR, t.type);
			return 1;
		}

		if (strcmp(t.buf, "-")) {
			fprintf(stderr, "Expected operator '-', got '%s'\n",
				t.buf);
			return 1;
		}

		subtilis_lexer_get(l, &t, &err);
		if (t.type != SUBTILIS_TOKEN_INTEGER) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_INTEGER, t.type);
			return 1;
		}

		if (t.tok.integer != expected_signed[i]) {
			fprintf(stderr, "Expected integer value %d, got %d\n",
				expected_signed[i], t.tok.integer);
			return 1;
		}
	}

	return 0;
}

static int prv_test_int(void)
{
	const char *str = "12345 67890 &12345 &67890 &abcef 2147483647"
			  "%10101010 -255 -&ff -%11111111";

	printf("lexer_int");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_int);
}

static int prv_check_real(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	const double expected[] = {
	    12345.67890, 0.314, .314,
	};

	const double expected_signed[] = {0.314, .314};

	subtilis_error_init(&err);
	for (i = 0; i < sizeof(expected) / sizeof(double); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_REAL) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_REAL, t.type);
			return 1;
		}

		if (fabs(t.tok.real - expected[i]) > 0.001) {
			fprintf(stderr, "Expected real value %lf, got %lf\n",
				expected[i], t.tok.real);
			return 1;
		}
	}

	for (i = 0; i < sizeof(expected_signed) / sizeof(double); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_OPERATOR) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_OPERATOR, t.type);
			return 1;
		}

		if (strcmp(t.buf, "-")) {
			fprintf(stderr, "Expected operator '-', got '%s'\n",
				t.buf);
			return 1;
		}

		subtilis_lexer_get(l, &t, &err);
		if (t.type != SUBTILIS_TOKEN_REAL) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_REAL, t.type);
			return 1;
		}

		if (fabs(t.tok.real - expected_signed[i]) > 0.001) {
			fprintf(stderr, "Expected real value %lf, got %lf\n",
				expected_signed[i], t.tok.real);
			return 1;
		}
	}

	return 0;
}

static int prv_test_real(void)
{
	const char *str = "12345.67890 0.314 .314 -0.314 -.314";

	printf("lexer_real");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_real);
}

static int prv_check_vars(subtilis_lexer_t *l, const char *const expected[],
			  size_t len, subtilis_identifier_type_t id_type)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	subtilis_error_init(&err);
	for (i = 0; i < len; i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_IDENTIFIER) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_IDENTIFIER, t.type);
			return 1;
		}

		if (t.tok.id_type != id_type) {
			fprintf(stderr,
				"Expected variable of type %d, got %d\n",
				id_type, t.tok.id_type);
			return 1;
		}

		if (strcmp(t.buf, expected[i])) {
			fprintf(stderr, "Expected real variable %s, got %s\n",
				expected[i], t.buf);
			return 1;
		}
	}

	return 0;
}

static int prv_check_real_vars(subtilis_lexer_t *l)
{
	const char *const expected[] = {
	    "The",   "quick",	  "brown",     "fox",
	    "jumps", "over",	   "the",       "lazy",
	    "dog",   "floating_point", "PROcedure", "index001of2"};
	return prv_check_vars(l, expected,
			      sizeof(expected) / sizeof(const char *const),
			      SUBTILIS_IDENTIFIER_REAL);
}

static int prv_test_real_vars(void)
{
	const char *str = "The quick brown fox jumps over the lazy dog"
			  " floating_point PROcedure index001of2";

	printf("lexer_real_vars");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_real_vars);
}

static int prv_check_int_vars(subtilis_lexer_t *l)
{
	const char *const expected[] = {
	    "The%",   "quick%",		 "brown%",     "fox%",
	    "jumps%", "over%",		 "the%",       "lazy%",
	    "dog%",   "floating_point%", "PROcedure%", "index001of2%"};
	return prv_check_vars(l, expected,
			      sizeof(expected) / sizeof(const char *const),
			      SUBTILIS_IDENTIFIER_INTEGER);
}

static int prv_test_int_vars(void)
{
	const char *str = "The% quick% brown% fox% jumps% over% the% lazy% dog%"
			  " floating_point% PROcedure% index001of2%";

	printf("lexer_int_vars");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_int_vars);
}

static int prv_check_str_vars(subtilis_lexer_t *l)
{
	const char *const expected[] = {
	    "The$",   "quick$",		 "brown$",     "fox$",
	    "jumps$", "over$",		 "the$",       "lazy$",
	    "dog$",   "floating_point$", "PROcedure$", "index001of2$"};
	return prv_check_vars(l, expected,
			      sizeof(expected) / sizeof(const char *const),
			      SUBTILIS_IDENTIFIER_STRING);
}

static int prv_test_str_vars(void)
{
	const char *str = "The$ quick$ brown$ fox$ jumps$ over$ the$ lazy$ dog$"
			  " floating_point$ PROcedure$ index001of2$";

	printf("lexer_str_vars");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_str_vars);
}

static int prv_check_operators(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;

	const char *const expected[] = {
	    "<", "<<", ">",  ">>", ">>>", ">=", "<=", "+=", "-=",
	    "+", "-",  "<>", "^",  "(",   ")",  "/",  "|",  "?",
	    ":", ";",  ",",  "[",  "]",   "~",  "*",  "$",
	};

	subtilis_error_init(&err);
	for (i = 0; i < sizeof(expected) / sizeof(const char *); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_OPERATOR) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_OPERATOR, t.type);
			return 1;
		}

		if (strcmp(t.buf, expected[i])) {
			fprintf(stderr,
				"Expected operator variable %s, got %s\n",
				expected[i], t.buf);
			return 1;
		}
	}

	return 0;
}

static int prv_test_operators(void)
{
	const char *str =
	    "< << > >> >>> >= <= += -= + - <> ^ ( ) / | ? : ; , [ ] ~"
	    " * $";

	printf("lexer_operators");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_operators);
}

static int prv_check_number_too_large(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_NUMBER_TOO_LONG) {
		fprintf(stderr, "Expected error %d, got %d\n",
			SUBTILIS_ERROR_NUMBER_TOO_LONG, err.type);
		return 1;
	}

	return 0;
}

static int prv_test_number_too_long(void)
{
	subtilis_buffer buf;
	subtilis_error_t err;
	int i;

	printf("lexer_number_too_long");
	subtilis_buffer_init(&buf, 512);
	subtilis_error_init(&err);
	subtilis_buffer_append_reserve(&buf, SUBTILIS_MAX_TOKEN_SIZE + 1, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}
	for (i = 0; i < SUBTILIS_MAX_TOKEN_SIZE + 1; i++)
		buf.buffer->data[i] = '0' + i % 10;

	subtilis_buffer_zero_terminate(&buf, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	return prv_test_wrapper(subtilis_buffer_get_string(&buf),
				SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_number_too_large);
}

static int prv_test_number_too_large(void)
{
	printf("lexer_number_too_large");

	return prv_test_wrapper("2147483648", SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_number_too_large);
}

static int prv_check_strings(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;
	const char *const expected[] = {
	    "one two three four five size 0 1 2", "\"No way\", he said",
	};

	subtilis_error_init(&err);
	for (i = 0; i < sizeof(expected) / sizeof(const char *); i++) {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK) {
			subtilis_error_fprintf(stderr, &err, true);
			return 1;
		}
		if (t.type != SUBTILIS_TOKEN_STRING) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_STRING, t.type);
			return 1;
		}
		if (strcmp(t.buf, expected[i])) {
			fprintf(stderr,
				"Unexpected string constant %s, expected %s\n",
				t.buf, expected[i]);
			return 1;
		}
	}

	return 0;
}

static int prv_test_string(void)
{
	const char *str = "\"one two three four five size 0 1 2\""
			  " \"\"\"No way\"\", he said\"";

	printf("lexer_strings");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_strings);
}

static int prv_check_string_unterminated(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_lexer_get(l, &t, &err);
	if (err.type != SUBTILIS_ERROR_UNTERMINATED_STRING) {
		fprintf(stderr, "Expected err %d, got %d\n",
			SUBTILIS_ERROR_UNTERMINATED_STRING, err.type);
		return 1;
	}

	return 0;
}

static int prv_test_string_unterminated(void)
{
	const char *str = "\"Unterminated";

	printf("lexer_string_unterminated");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_string_unterminated);
}

static int prv_check_unknown(subtilis_lexer_t *l)
{
	subtilis_token_t t;
	subtilis_error_t err;
	int i;
	const char *const expected[] = {
	    "#UNKNOWN", "#1234567890",
	};

	for (i = 0; i < sizeof(expected) / sizeof(const char *); i++) {
		subtilis_error_init(&err);
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_UNKNOWN_TOKEN) {
			fprintf(stderr, "Expected err %d, got %d\n",
				SUBTILIS_ERROR_UNKNOWN_TOKEN, err.type);
			return 1;
		}

		if (t.type != SUBTILIS_TOKEN_UNKNOWN) {
			fprintf(stderr, "Expected token type %d, got %d\n",
				SUBTILIS_TOKEN_UNKNOWN, t.type);
			return 1;
		}
		if (strcmp(t.buf, expected[i])) {
			fprintf(stderr,
				"Unexpected string constant %s, expected %s\n",
				t.buf, expected[i]);
			return 1;
		}
	}

	return 0;
}

static int prv_test_unknown(void)
{
	const char *str = "#UNKNOWN #1234567890";

	printf("lexer_unknown");
	return prv_test_wrapper(str, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
				prv_check_unknown);
}

/* TODO
 *   expressions with and without white space
 * array indexes
 */

int main(int argc, char *argv[])
{
	int failure = 0;

	failure |= prv_test_string_too_long();
	failure |= prv_test_real_var_too_long();
	failure |= prv_test_string_var_too_long();
	failure |= prv_test_int_var_too_long();
	failure |= prv_test_max_int_var();
	failure |= prv_test_max_str_var();
	failure |= prv_test_max_real_var();
	failure |= prv_test_real_var_upper_too_long();
	failure |= prv_test_string_var_upper_too_long();
	failure |= prv_test_id_hash_var_upper_too_long();
	failure |= prv_test_string_hash_var_upper_too_long();
	failure |= prv_test_keywords();
	failure |= prv_test_procedure_call();
	failure |= prv_test_empty();
	failure |= prv_test_proc_typed();
	failure |= prv_test_fn_typed();
	failure |= prv_test_int();
	failure |= prv_test_real();
	failure |= prv_test_real_vars();
	failure |= prv_test_int_vars();
	failure |= prv_test_str_vars();
	failure |= prv_test_operators();
	failure |= prv_test_number_too_long();
	failure |= prv_test_number_too_large();
	failure |= prv_test_string();
	failure |= prv_test_string_unterminated();
	failure |= prv_test_unknown();

	return failure;
}
