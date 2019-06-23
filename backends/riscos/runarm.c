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

#include "../../arch/arm32/arm_disass.h"
#include "../../arch/arm32/arm_vm.h"
#include "../../common/buffer.h"

const size_t block_size = 16 * 1024;

void prv_read_code(FILE *f, subtilis_buffer_t *b, subtilis_error_t *err)
{
	size_t num_read = 0;
	size_t total_read = 0;
	size_t buf_size = block_size;

	do {
		subtilis_buffer_reserve(b, buf_size, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;

		num_read =
		    fread(&b->buffer->data[b->buffer->start] + total_read, 1,
			  block_size, f);
		total_read += num_read;
	} while (num_read == block_size);

	if (ferror(f)) {
		subtilis_error_set_file_read(err);
		return;
	}

	b->buffer->end = total_read;
}

int main(int argc, char *argv[])
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	subtilis_buffer_t out_b;
	int retval = 1;
	uint8_t *code;
	size_t code_len;
	subtilis_arm_vm_t *vm = NULL;
	FILE *f = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: inter file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");
	subtilis_error_init(&err);
	subtilis_buffer_init(&b, block_size);
	subtilis_buffer_init(&out_b, block_size);

	f = fopen(argv[1], "r");
	if (!f) {
		fprintf(stderr, "Usage: failed to open %s\n", argv[1]);
		return 1;
	}

	prv_read_code(f, &b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	(void)fclose(f);
	f = NULL;

	code = &b.buffer->data[b.buffer->start];
	code_len = subtilis_buffer_get_size(&b);

	subtilis_arm_disass_dump(code, code_len);

	vm = subtilis_arm_vm_new((uint32_t *)code, code_len / 4, 16 * 1024,
				 &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_vm_run(vm, &out_b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&out_b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	printf("\n\n==== OUTPUT====\n\n");

	printf("%s\n", subtilis_buffer_get_string(&out_b));

	retval = 0;

cleanup:

	if (err.type != SUBTILIS_ERROR_OK)
		subtilis_error_fprintf(stderr, &err, true);

	subtilis_arm_vm_delete(vm);

	if (f)
		(void)fclose(f);

	subtilis_buffer_free(&out_b);
	subtilis_buffer_free(&b);

	return retval;
}
