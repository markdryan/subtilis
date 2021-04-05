/*
 * Copyright (c) 2021 Mark Ryan
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

#ifndef __SUBTILIS_COLLECTION_H
#define __SUBTILIS_COLLECTION_H

#include "expression.h"
#include "parser.h"

void subtilis_collection_copy_scalar(subtilis_parser_t *p, subtilis_exp_t *obj1,
				     subtilis_exp_t *obj2, bool cow,
				     subtilis_error_t *err);
void subtilis_collection_copy_ref(subtilis_parser_t *p, subtilis_exp_t *obj1,
				  subtilis_exp_t *obj2, subtilis_error_t *err);

#endif
