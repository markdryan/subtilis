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

#ifndef __SUBTILIS_BACKEND_CAPS_H
#define __SUBTILIS_BACKEND_CAPS_H

#include <stdint.h>

#define SUBTILIS_BACKEND_HAVE_DIV 1
#define SUBTILIS_BACKEND_HAVE_REAL_TO_DEC 2
#define SUBTILIS_BACKEND_HAVE_I32_TO_DEC 4
#define SUBTILIS_BACKEND_HAVE_I32_TO_HEX 8
#define SUBTILIS_BACKEND_HAVE_ALLOC 16
#define SUBTILIS_BACKEND_REVERSE_DOUBLES 32
#define SUBTILIS_BACKEND_HAVE_TINT 64

#define SUBTILIS_BACKEND_INTER_CAPS                                            \
	(SUBTILIS_BACKEND_HAVE_DIV | SUBTILIS_BACKEND_HAVE_ALLOC |             \
	 SUBTILIS_BACKEND_HAVE_TINT)
typedef uint32_t subtilis_backend_caps_t;

#endif
