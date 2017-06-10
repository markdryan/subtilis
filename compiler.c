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

#include <stdio.h>

#include "error.h"
#include "lexer.h"

int main(int argc, char *argv[])
{
	subtilis_error_t err;
	subtilis_stream_t s;
	subtilis_lexer_t *l = NULL;
	subtilis_token_t t;

	if (argc != 2) {
		fprintf(stderr, "Usage: basicc file\n");
		return 1;
	}

	subtilis_stream_from_file(&s, argv[1], &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, SUBTILIS_CONFIG_LEXER_BUF_SIZE, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	do {
		subtilis_lexer_get(l, &t, &err);
		if (err.type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_dump_token(&t);
	} while (t.type != SUBTILIS_TOKEN_EOF);

	subtilis_lexer_delete(l, &err);

	return 0;

cleanup:
	if (l)
		subtilis_lexer_delete(l, &err);
	else
		s.close(s.handle, &err);

fail:
	subtilis_error_fprintf(stderr, &err, true);

	return 1;
}
