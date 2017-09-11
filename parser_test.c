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

#include <string.h>

#include "parser_test.h"

#include "lexer.h"
#include "parser.h"
#include "vm.h"

static int prv_test_wrapper(const char *text,
			    int (*fn)(subtilis_lexer_t *, subtilis_parser_t *,
				      const char *expected),
			    const char *expected)
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

	retval = fn(l, p, expected);

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

static int prv_check_not_keyword(subtilis_lexer_t *l, subtilis_parser_t *p,
				 const char *expected)
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

/*
 * static int prv_check_parse_ok(subtilis_lexer_t *l, subtilis_parser_t *p,
 *			      const char *expected)
 * {
 *	subtilis_error_t err;
 *
 *	subtilis_error_init(&err);
 *	subtilis_parse(p, &err);
 *	if (err.type != SUBTILIS_ERROR_OK) {
 *		subtilis_error_fprintf(stderr, &err, true);
 *		return 1;
 *	}
 *
 *	return 0;
 *}
 */

static int prv_check_eval_res(subtilis_lexer_t *l, subtilis_parser_t *p,
			      const char *expected)
{
	subtilis_buffer_t b;
	subtilis_error_t err;
	const char *tbuf;
	subitlis_vm_t *vm = NULL;
	int retval = 1;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto cleanup;
	}

	vm = subitlis_vm_new(p->p, p->st, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto cleanup;
	}

	subitlis_vm_run(vm, &b, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto cleanup;
	}

	subtilis_buffer_zero_terminate(&b, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto cleanup;
	}

	tbuf = subtilis_buffer_get_string(&b);

	if (strcmp(tbuf, expected)) {
		fprintf(stderr, "Expected result %s got %s\n", expected, tbuf);
		goto cleanup;
	}

	retval = 0;

cleanup:

	subitlis_vm_delete(vm);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_let(void)
{
	const char *let_test = "LET b% = 99\n"
			       "PRINT 10 + 10 + b%\n";

	printf("parser_let");
	return prv_test_wrapper(let_test, prv_check_eval_res, "119\n");
}

struct expression_test_t_ {
	const char *name;
	const char *source;
	const char *result;
};

typedef struct expression_test_t_ expression_test_t;

/* clang-format off */
static const expression_test_t expression_tests[] = {
	{ "parser_subtraction",
	  "LET b% = 100 - 5\n"
	  "LET c% = 10 - b% -10\n"
	  "LET d% = c% - b% - 1\n"
	  "PRINT d%\n",
	  "-191\n"},
	{ "parser_division",
	  "LET b% = 100 / 5\n"
	  "LET c% = 1000 / b% / 10\n"
	  "LET d% = b% / c% / 2\n"
	  "PRINT d%\n",
	  "2\n"},
	{ "parser_multiplication",
	  "LET b% = 1 * 10\n"
	  "LET c% = 10 * b% * 5\n"
	  "LET d% = c% * 2 * b%\n"
	  "PRINT d%\n",
	  "10000\n"},
	{ "parser_addition",
	  "LET b% = &ff + 10\n"
	  "LET c% = &10 + b% + 5\n"
	  "LET d% = c% + 2 + b%\n"
	  "PRINT d%\n",
	  "553\n"},
	{ "nested_expression",
	  "LET a% = (1 + 1)\n"
	  "LET b% = ((((10 - a%) -5) * 10))\n"
	  "PRINT b%",
	  "30\n"},
	{ "parser_unary_minus",
	  "LET b% = -110\n"
	  "LET c% = -b%\n"
	  "LET d% = 10 - -(c% - 0)\n"
	  "PRINT d%\n",
	  "120\n"},
};

/* clang-format on */

static int prv_test_expressions(void)
{
	size_t i;
	int retval = 0;

	for (i = 0; i < sizeof(expression_tests) / sizeof(expression_test_t);
	     i++) {
		printf("%s", expression_tests[i].name);
		retval |= prv_test_wrapper(expression_tests[i].source,
					   prv_check_eval_res,
					   expression_tests[i].result);
	}

	return retval;
}

static int prv_test_print(void)
{
	printf("parser_print");
	return prv_test_wrapper("PRINT (10 * 3 * 3 + 1) / 2",
				prv_check_eval_res, "45\n");
}

static int prv_test_not_keyword(void)
{
	printf("parser_not_keyword");
	return prv_test_wrapper("id", prv_check_not_keyword, NULL);
}

int parser_test(void)
{
	int failure = 0;

	failure |= prv_test_not_keyword();
	failure |= prv_test_let();
	failure |= prv_test_print();
	failure |= prv_test_expressions();

	return failure;
}
