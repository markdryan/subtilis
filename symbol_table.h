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

#ifndef __SUBTILIS_SYMBOL_TABLE_H
#define __SUBTILIS_SYMBOL_TABLE_H

#include "hash_table.h"
#include "lexer.h"

struct subtilis_symbol_t_ {
	size_t loc;
	subtilis_type_t t;
	size_t size;
};

typedef struct subtilis_symbol_t_ subtilis_symbol_t;

struct subtilis_symbol_table_t_ {
	subtilis_hashtable_t *h;
	size_t allocated;
};

typedef struct subtilis_symbol_table_t_ subtilis_symbol_table_t;

subtilis_symbol_table_t *subtilis_symbol_table_new(subtilis_error_t *err);
void subtilis_symbol_table_delete(subtilis_symbol_table_t *st);
const subtilis_symbol_t *
subtilis_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			     subtilis_type_t id_type, subtilis_error_t *err);
bool subtilis_symbol_table_remove(subtilis_symbol_table_t *st, const char *key);
const subtilis_symbol_t *
subtilis_symbol_table_lookup(subtilis_symbol_table_t *st, const char *key);
void subtilis_symbol_table_reset(subtilis_symbol_table_t *st);

#endif
