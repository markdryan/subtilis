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
#include <time.h>

#include "config.h"
#include "utils.h"

const char *subtilis_utils_basename(const char *path)
{
	size_t len = strlen(path);
	int i;

	if (len == 0)
		return path;
	i = ((int)len) - 1;
	if (path[i] == SUBTILIS_CONFIG_PATH_SEPARATOR)
		i--;
	for (; i >= 0 && path[i] != SUBTILIS_CONFIG_PATH_SEPARATOR; i--)
		;
	if (i >= 0)
		return &path[i + 1];
	else
		return path;
}

int32_t subtilis_get_i32_time(void)
{
	struct tm ts;
	time_t t = time(NULL);

	memset(&ts, 0, sizeof(ts));
	ts.tm_year = 2018 - 1900;
	ts.tm_mon = 11;
	ts.tm_mday = 22;

	t = t - mktime(&ts);

	return (int32_t)(t * 100);
}

bool subtils_get_file_size(FILE *f, int32_t *size)
{
	long cur_pos = ftell(f);
	long fsize;

	if (cur_pos == -1)
		return false;

	if (fseek(f, 0L, SEEK_END))
		return false;

	fsize = (int32_t)ftell(f);
	if (fsize == -1)
		return false;

	if (fseek(f, 0L, cur_pos))
		return false;

	*size = fsize;

	return true;
}

char *subtilis_make_cmdline(int argc, char **argv, subtilis_error_t *err)
{
	size_t size = 0;
	size_t i;
	char *ret_val;

	for (i = 1; i < argc; i++)
		size += strlen(argv[i]) + 1;

	if (size < 2) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	ret_val = malloc(size);
	if (!ret_val) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	ret_val[0] = 0;

	for (i = 1; i < argc - 1; i++) {
		strcat(ret_val, argv[i]);
		strcat(ret_val, " ");
	}
	strcat(ret_val, argv[i]);

	return ret_val;
}
