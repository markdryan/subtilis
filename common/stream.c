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
#include <stdlib.h>
#include <string.h>

#include "stream.h"

struct _subtilis_text_stream_t {
	size_t len;
	size_t read;
	const char *text;
};

typedef struct _subtilis_text_stream_t subtilis_text_stream_t;

static size_t prv_file_read(void *ptr, size_t n, void *handle,
			    subtilis_error_t *err)
{
	size_t read = fread(ptr, 1, n, (FILE *)handle);

	if (ferror((FILE *)handle) && !feof((FILE *)handle))
		subtilis_error_set_file_read(err);

	return read;
}

static void prv_file_close(void *handle, subtilis_error_t *err)
{
	if (fclose((FILE *)handle) != 0)
		subtilis_error_set_file_close(err);
}

void subtilis_stream_from_file(subtilis_stream_t *s, const char *path,
			       subtilis_error_t *err)
{
	FILE *f;

	f = fopen(path, "r");
	if (!f) {
		subtilis_error_set_file_open(err, path);
		goto on_err;
	}

	strncpy(s->name, path, SUBTILIS_CONFIG_PATH_MAX);
	s->name[SUBTILIS_CONFIG_PATH_MAX] = 0;

	s->handle = f;
	s->read = prv_file_read;
	s->close = prv_file_close;

on_err:
	return;
}

static size_t prv_text_read(void *ptr, size_t n, void *handle,
			    subtilis_error_t *err)
{
	subtilis_text_stream_t *stream = (subtilis_text_stream_t *)handle;
	size_t left = stream->len - stream->read;

	if (left == 0)
		return 0;
	if (left < n)
		n = left;

	memcpy(ptr, &stream->text[stream->read], n);
	stream->read += n;

	return n;
}

static void prv_text_close(void *handle, subtilis_error_t *err)
{
	free(handle);
}

void subtilis_stream_from_text(subtilis_stream_t *s, const char *text,
			       subtilis_error_t *err)
{
	subtilis_text_stream_t *stream = malloc(sizeof(*stream));

	if (!stream) {
		subtilis_error_set_oom(err);
		return;
	}
	stream->len = strlen(text);
	stream->read = 0;
	stream->text = text;
	s->handle = stream;
	s->read = prv_text_read;
	s->close = prv_text_close;
	s->name[0] = 0;
}
