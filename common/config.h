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

#ifndef __SUBTILIS_CONFIG_H
#define __SUBTILIS_CONFIG_H

/* Global constants that can be overridden at build time. */

#ifndef SUBTILIS_CONFIG_LEXER_BUF_SIZE
#define SUBTILIS_CONFIG_LEXER_BUF_SIZE (128 * 1024)
#endif

#ifndef SUBTILIS_CONFIG_PATH_MAX
#define SUBTILIS_CONFIG_PATH_MAX 512
#endif

#ifndef SUBTILIS_CONFIG_ERROR_LEN
#define SUBTILIS_CONFIG_ERROR_LEN 1024
#endif

#ifndef SUBTILIS_CONFIG_PATH_SEPARATOR
#define SUBTILIS_CONFIG_PATH_SEPARATOR '.'
#endif

#ifndef SUBTILIS_CONFIG_PROGRAM_GRAN
#define SUBTILIS_CONFIG_PROGRAM_GRAN 4096
#endif

#ifndef SUBTILIS_CONFIG_LABEL_GRAN
#define SUBTILIS_CONFIG_LABEL_GRAN (SUBTILIS_CONFIG_PROGRAM_GRAN / 4)
#endif

#ifndef SUBTILIS_CONFIG_PROC_GRAN
#define SUBTILIS_CONFIG_PROC_GRAN 64
#endif

#ifndef SUBTILIS_CONFIG_ST_SIZE
#define SUBTILIS_CONFIG_ST_SIZE 1024
#endif

#ifndef SUBTILIS_CONFIG_SSS_GRAN
#define SUBTILIS_CONFIG_SSS_GRAN 16
#endif

#endif
