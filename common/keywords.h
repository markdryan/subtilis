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

#ifndef __SUBTILIS_KEYWORD_H__
#define __SUBTILIS_KEYWORD_H__

#include <stdbool.h>

/* clang-format off */

enum {
	SUBTILIS_KEYWORD_FN,
	SUBTILIS_KEYWORD_PROC,
	SUBTILIS_KEYWORD_REM,
	SUBTILIS_KEYWORD_COMMON_MAX
};

/* clang-format on */

struct _subtilis_keyword_t {
	const char *str;
	int type;
	bool supported;
};

typedef struct _subtilis_keyword_t subtilis_keyword_t;

#endif
