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

#include "parser_test.h"

#include "lexer.h"
#include "parser.h"

static int prv_test_wrapper(const char *text,
			    int (*fn)(subtilis_lexer_t *,
				      subtilis_parser_t *))
{
	subtilis_stream_t s;
	subtilis_error_t err;
	subtilis_lexer_t *l = NULL;
	subtilis_parser_t *p = NULL;
	int retval;

	subtilis_error_init(&err);
	subtilis_stream_from_text(&s, text, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, SUBTILIS_CONFIG_LEXER_BUF_SIZE, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		s.close(s.handle, &err);
		goto fail;
	}

	p = subtilis_parser_new(l, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	retval = fn(l, p);

	printf(": [%s]\n", retval ? "FAIL" : "OK");

	subtilis_parser_delete(p);
	subtilis_lexer_delete(l, &err);

	return retval;

fail:
	printf(": [FAIL]\n");

	subtilis_error_fprintf(stderr, &err, true);
	subtilis_parser_delete(p);
	subtilis_lexer_delete(l, &err);

	return 1;
}

static int prv_check_not_keyword(subtilis_lexer_t *l, subtilis_parser_t *p)
{
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_KEYWORD_EXPECTED) {
		fprintf(stderr, "Expected err %d, got %d\n",
			SUBTILIS_ERROR_KEYWORD_EXPECTED, err.type);
		return 1;
	}

	return 0;
}

static int prv_check_parse_ok(subtilis_lexer_t *l, subtilis_parser_t *p)
{
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	return 0;
}

static int prv_test_let(void)
{
	printf("parser_let");
	return prv_test_wrapper("LET x% = (10 * 3 * 3 + 1) /2",
				prv_check_parse_ok);
}

static int prv_test_print(void)
{
	printf("parser_print");
	return prv_test_wrapper("PRINT (10 * 3 * 3 + 1) /2",
				prv_check_parse_ok);
}

static int prv_test_not_keyword(void)
{
	printf("parser_not_keyword");
	return prv_test_wrapper("id", prv_check_not_keyword);
}

int parser_test(void)
{
	int failure = 0;

	failure |= prv_test_not_keyword();
	failure |= prv_test_let();
	failure |= prv_test_print();

	return failure;
}
