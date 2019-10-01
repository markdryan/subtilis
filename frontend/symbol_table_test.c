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

#include "symbol_table_test.h"

#include "keywords.h"
#include "symbol_table.h"

static int prv_test_type(subtilis_symbol_table_t *st, subtilis_token_t *t,
			 const subtilis_type_t *id_type, size_t exp,
			 subtilis_error_t *err)
{
	size_t i;
	size_t j;
	const subtilis_symbol_t *s;
	size_t loc;
	size_t total = 0;
	const char *tbuf;

	t->tok.id_type = *id_type;

	for (j = 0; j < 2; j++) {
		loc = 0;
		for (i = 0; i < SUBTILIS_KEYWORD_MAX; i++) {
			subtilis_buffer_reset(&t->buf);
			subtilis_buffer_append_string(
			    &t->buf, subtilis_keywords_list[i].str, err);
			if (err->type != SUBTILIS_ERROR_OK) {
				subtilis_error_fprintf(stderr, err, true);
				return 1;
			}
			subtilis_buffer_zero_terminate(&t->buf, err);
			if (err->type != SUBTILIS_ERROR_OK) {
				subtilis_error_fprintf(stderr, err, true);
				return 1;
			}
			tbuf = subtilis_token_get_text(t);
			s = subtilis_symbol_table_insert(st, tbuf,
							 &t->tok.id_type, err);
			if (err->type != SUBTILIS_ERROR_OK) {
				subtilis_error_fprintf(stderr, err, true);
				return 1;
			}
			if ((s->loc != loc) || (s->size != exp)) {
				fprintf(stderr,
					"Symbol incorrect: %zu %zu - %zu %zu\n",
					s->loc, loc, s->size, exp);
				return 1;
			}
			if (!subtilis_symbol_table_lookup(st, tbuf)) {
				fprintf(stderr, "Symbol lookup %s failed\n",
					subtilis_token_get_text(t));
				return 1;
			}
			loc += exp;
			if (total > 0) {
				if (st->allocated != total) {
					fprintf(stderr,
						"Size bad.Found %zu want %zu\n",
						st->allocated, total);
					return 1;
				}
			} else if (loc != st->allocated) {
				fprintf(stderr,
					"Size bad. Found %zu, expected %zu\n",
					st->allocated, loc);
				return 1;
			}
		}
		total = loc;
	}

	return 0;
}

static int prv_test(void)
{
	subtilis_symbol_table_t *st = NULL;
	int retval = 1;
	subtilis_error_t err;
	subtilis_token_t *t;
	subtilis_type_t type;

	printf("symbol_table_test");

	subtilis_error_init(&err);

	t = subtilis_token_new(&err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto on_error;
	}

	t->type = SUBTILIS_TOKEN_IDENTIFIER;

	st = subtilis_symbol_table_new(&err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto on_error;
	}

	type.type = SUBTILIS_TYPE_REAL;
	retval = prv_test_type(st, t, &type, 8, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		goto on_error;
	}

	if (retval != 0)
		goto on_error;

	retval = 0;

on_error:

	printf(": [%s]\n", retval ? "FAIL" : "OK");

	subtilis_symbol_table_delete(st);
	subtilis_token_delete(t);

	return retval;
}

int symbol_table_test(void)
{
	int failure = 0;

	failure |= prv_test();

	return failure;
}
