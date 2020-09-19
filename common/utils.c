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
#include <sys/time.h>
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
	struct timeval tv;

	memset(&ts, 0, sizeof(ts));
	ts.tm_year = 2018 - 1900;
	ts.tm_mon = 11;
	ts.tm_mday = 22;

	gettimeofday(&tv, NULL);
	tv.tv_sec -= mktime(&ts);
	tv.tv_sec *= 100;
	tv.tv_sec += tv.tv_usec / 10000;

	return tv.tv_sec;
}
