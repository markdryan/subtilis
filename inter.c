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

#include <locale.h>
#include <stdio.h>

#include "common/error.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

static void prv_run_prog(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_buffer_t b;
	subitlis_vm_t *vm = NULL;

	subtilis_buffer_init(&b, 1024);

	vm = subitlis_vm_new(p->prog, p->st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subitlis_vm_run(vm, &b, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&b, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	printf("\n\n==== OUTPUT====\n\n");
	printf("%s\n", subtilis_buffer_get_string(&b));

cleanup:

	subitlis_vm_delete(vm);
	subtilis_buffer_free(&b);
}

int main(int argc, char *argv[])
{
	subtilis_error_t err;
	subtilis_stream_t s;
	subtilis_lexer_t *l = NULL;
	subtilis_parser_t *p = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: inter file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");

	subtilis_error_init(&err);
	subtilis_stream_from_file(&s, argv[1], &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, SUBTILIS_CONFIG_LEXER_BUF_SIZE, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p = subtilis_parser_new(l, SUBTILIS_BACKEND_INTER_CAPS, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_prog_dump(p->prog);

	prv_run_prog(p, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_delete(p);
	subtilis_lexer_delete(l, &err);

	return 0;

cleanup:
	subtilis_parser_delete(p);
	if (l)
		subtilis_lexer_delete(l, &err);
	else
		s.close(s.handle, &err);

fail:
	subtilis_error_fprintf(stderr, &err, true);

	return 1;
}
