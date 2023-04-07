/*
 * Copyright (c) 2023 Mark Ryan
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

#ifndef __SUBTILIS_RV_ENCODE_H
#define __SUBTILIS_RV_ENCODE_H

#include "rv32_core.h"

typedef void (*subtilis_rv_encode_plat_t)(FILE *fp, uint8_t *bytes,
					  size_t bytes_written,
					  size_t globals,
					  subtilis_error_t *err);

void subtilis_rv_encode(subtilis_rv_prog_t *rv_p, const char *fname,
			size_t globals,
			subtilis_rv_encode_plat_t plat_header,
			subtilis_rv_encode_plat_t plat_tail,
			subtilis_error_t *err);
uint8_t *subtilis_rv_encode_buf(subtilis_rv_prog_t *arm_p,
				size_t *bytes_written, size_t globals,
				subtilis_error_t *err);

#endif
