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

#ifndef __SUBTILIS_UTILS_H
#define __SUBTILIS_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

const char *subtilis_utils_basename(const char *path);
/*
 * Returns centi-seconds since a fixed date.  Results are expected to be
 * positive.  Date will need to be updated every year or so.
 */
int32_t subtilis_get_i32_time(void);

bool subtils_get_file_size(FILE *f, int32_t *size);
char *subtilis_make_cmdline(int argc, char **argv, subtilis_error_t *err);
#endif
