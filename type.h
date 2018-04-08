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

#ifndef __SUBTILIS_TYPE_H
#define __SUBTILIS_TYPE_H

#include "error.h"

typedef enum {
	SUBTILIS_TYPE_REAL,
	SUBTILIS_TYPE_INTEGER,
	SUBTILIS_TYPE_STRING,
	SUBTILIS_TYPE_VOID,
} subtilis_type_t;

struct subtilis_type_section_t_ {
	subtilis_type_t return_type;
};

typedef struct subtilis_type_section_t_ subtilis_type_section_t;

subtilis_type_section_t *subtilis_type_section_new(subtilis_type_t rtype,
						   subtilis_error_t *err);
void subtilis_type_section_delete(subtilis_type_section_t *stype);

#endif
